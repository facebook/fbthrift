--
-- Licensed to the Apache Software Foundation (ASF) under one
-- or more contributor license agreements. See the NOTICE file
-- distributed with this work for additional information
-- regarding copyright ownership. The ASF licenses this file
-- to you under the Apache License, Version 2.0 (the
-- "License"); you may not use this file except in compliance
-- with the License. You may obtain a copy of the License at
--
--   http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing,
-- software distributed under the License is distributed on an
-- "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
-- KIND, either express or implied. See the License for the
-- specific language governing permissions and limitations
-- under the License.
--

{-# LANGUAGE CPP #-}
{-# LANGUAGE ExistentialQuantification #-}
{-# LANGUAGE OverloadedStrings #-}
{-# LANGUAGE ScopedTypeVariables #-}
{-# LANGUAGE TupleSections #-}

module Thrift.Protocol.SimpleJSON
    ( module Thrift.Protocol
    , SimpleJSONProtocol(..)
    ) where

import Control.Applicative
import Control.Exception
import Data.Attoparsec.ByteString as P
import Data.Attoparsec.ByteString.Char8 as PC
import Data.Attoparsec.ByteString.Lazy as LP
import Data.ByteString.Builder as B
import Data.ByteString.Internal (c2w, w2c)
import Data.Functor
import Data.Int
import Data.List
import Data.Maybe (catMaybes)
import Data.Monoid
import Data.Text.Lazy.Encoding
import Data.Word
import qualified Data.HashMap.Strict as Map

import Thrift.Protocol
import Thrift.Transport
import Thrift.Types

import qualified Data.ByteString.Lazy as LBS
import qualified Data.Text.Lazy as LT

-- | The Simple JSON Protocol data uses the standard 'TSimpleJSONProtocol'.
-- Data is encoded as a JSON 'ByteString'
data SimpleJSONProtocol t = SimpleJSONProtocol t
                      -- ^ Construct a 'JSONProtocol' with a 'Transport'

version :: Int32
version = 1

instance Protocol SimpleJSONProtocol where
    getTransport (SimpleJSONProtocol t) = t

    writeMessage (SimpleJSONProtocol t) (s, ty, sq) =
      bracket writeMessageBegin writeMessageEnd . const
      where
        writeMessageBegin = tWrite t $ toLazyByteString $
          "[" <> int32Dec version <>
          ",\"" <> escape (encodeUtf8 s) <> "\"" <>
          "," <> intDec (fromEnum ty) <>
          "," <> int32Dec sq <>
          ","
        writeMessageEnd _ = tWrite t "]"
    readMessage p = bracket readMessageBegin readMessageEnd
      where
        readMessageBegin = runParser p $ skipSpace *> do
          _ver :: Int32 <- lexeme (PC.char8 '[') *> lexeme (signed decimal)
          bs <- lexeme (PC.char8 ',') *> lexeme escapedString
          case decodeUtf8' bs of
            Left _ -> fail "readMessage: invalid text encoding"
            Right str -> do
              ty <- toEnum <$> (lexeme (PC.char8 ',') *>
                                lexeme (signed decimal))
              seqNum <- lexeme (PC.char8 ',') *> signed decimal
              return (str, ty, seqNum)
        readMessageEnd _ = runParser p $ skipSpace *> PC.char8 ']'

    serializeVal _ = toLazyByteString . buildJSONValue
    deserializeVal _ ty bs =
      case LP.eitherResult $ LP.parse (parseJSONValue ty) bs of
        Left s -> error s
        Right val -> val

    readVal p ty = runParser p $ skipSpace *> parseJSONValue ty


-- Writing Functions

buildJSONValue :: ThriftVal -> Builder
buildJSONValue (TStruct fields) = "{" <> buildJSONStruct fields <> "}"
buildJSONValue (TMap _ _ entries) = "{" <> buildJSONMap entries <> "}"
buildJSONValue (TList _ entries) = "[" <> buildJSONList entries <> "]"
buildJSONValue (TSet _ entries) = "[" <> buildJSONList entries <> "]"
buildJSONValue (TBool b) = if b then "true" else "false"
buildJSONValue (TByte b) = int8Dec b
buildJSONValue (TI16 i) = int16Dec i
buildJSONValue (TI32 i) = int32Dec i
buildJSONValue (TI64 i) = int64Dec i
buildJSONValue (TFloat f) = floatDec f
buildJSONValue (TDouble d) = doubleDec d
buildJSONValue (TString s) = "\"" <> escape s <> "\""

buildJSONStruct :: Map.HashMap Int16 (LT.Text, ThriftVal) -> Builder
buildJSONStruct = mconcat . intersperse "," . Map.elems . Map.map (\(str,val) ->
  "\"" <> B.lazyByteString (encodeUtf8 str) <> "\":" <> buildJSONValue val)

buildJSONMap :: [(ThriftVal, ThriftVal)] -> Builder
buildJSONMap = mconcat . intersperse "," . map buildKV
  where
    buildKV (key@(TString _), val) =
      buildJSONValue key <> ":" <> buildJSONValue val
    buildKV (key, val) =
      "\"" <> buildJSONValue key <> "\":" <> buildJSONValue val
buildJSONList :: [ThriftVal] -> Builder
buildJSONList = mconcat . intersperse "," . map buildJSONValue


-- Reading Functions

parseJSONValue :: ThriftType -> Parser ThriftVal
parseJSONValue (T_STRUCT tmap) =
  TStruct <$> (lexeme (PC.char8 '{') *> parseJSONStruct tmap <* PC.char8 '}')
parseJSONValue (T_MAP kt vt) =
  TMap kt vt <$> between '{' '}' (parseJSONMap kt vt)
parseJSONValue (T_LIST ty) =
  TList ty <$> between '[' ']' (parseJSONList ty)
parseJSONValue (T_SET ty) =
  TSet ty <$> between '[' ']' (parseJSONList ty)
parseJSONValue T_BOOL =
  (TBool True <$ string "true") <|> (TBool False <$ string "false")
parseJSONValue T_BYTE = TByte <$> signed decimal
parseJSONValue T_I16 = TI16 <$> signed decimal
parseJSONValue T_I32 = TI32 <$> signed decimal
parseJSONValue T_I64 = TI64 <$> signed decimal
parseJSONValue T_FLOAT = TFloat . realToFrac <$> double
parseJSONValue T_DOUBLE = TDouble <$> double
parseJSONValue T_STRING = TString <$> escapedString
parseJSONValue T_STOP = fail "parseJSONValue: cannot parse type T_STOP"
parseJSONValue T_VOID = fail "parseJSONValue: cannot parse type T_VOID"

parseAnyValue :: Parser ()
parseAnyValue = choice $
                skipBetween '{' '}' :
                skipBetween '[' ']' :
                map (void . parseJSONValue)
                  [ T_BOOL
                  , T_I16
                  , T_I32
                  , T_I64
                  , T_FLOAT
                  , T_DOUBLE
                  , T_STRING
                  ]
  where
    skipBetween :: Char -> Char -> Parser ()
    skipBetween a b = between a b $ void (PC.satisfy (\c -> c /= a && c /= b))
                                          <|> skipBetween a b

parseJSONStruct :: TypeMap -> Parser (Map.HashMap Int16 (LT.Text, ThriftVal))
parseJSONStruct tmap = Map.fromList . catMaybes <$> parseField
                       `sepBy` lexeme (PC.char8 ',')
  where
    parseField = do
      bs <- lexeme escapedString <* lexeme (PC.char8 ':')
      case decodeUtf8' bs of
        Left _ -> fail "parseJSONStruct: invalid key encoding"
        Right str -> case Map.lookup str tmap of
          Just (fid, ftype) -> do
            val <- lexeme (parseJSONValue ftype)
            return $ Just (fid, (str, val))
          Nothing -> lexeme parseAnyValue *> return Nothing

parseJSONMap :: ThriftType -> ThriftType -> Parser [(ThriftVal, ThriftVal)]
parseJSONMap kt vt =
  ((,) <$> lexeme (PC.char8 '"' *> parseJSONValue kt <* PC.char8 '"') <*>
   (lexeme (PC.char8 ':') *> lexeme (parseJSONValue vt))) `sepBy`
  lexeme (PC.char8 ',')

parseJSONList :: ThriftType -> Parser [ThriftVal]
parseJSONList ty = lexeme (parseJSONValue ty) `sepBy` lexeme (PC.char8 ',')

escapedString :: Parser LBS.ByteString
escapedString = PC.char8 '"' *>
                (LBS.pack <$> P.many' (escapedChar <|> notChar8 '"')) <*
                PC.char8 '"'

escapedChar :: Parser Word8
escapedChar = PC.char8 '\\' *> (c2w <$> choice
                                [ '\SOH' <$ P.string "u0001"
                                , '\STX' <$ P.string "u0002"
                                , '\ETX' <$ P.string "u0003"
                                , '\EOT' <$ P.string "u0004"
                                , '\ENQ' <$ P.string "u0005"
                                , '\ACK' <$ P.string "u0006"
                                , '\BEL' <$ P.string "u0007"
                                , '\BS'  <$ P.string "u0008"
                                , '\VT'  <$ P.string "u000b"
                                , '\FF'  <$ P.string "u000c"
                                , '\CR'  <$ P.string "u000d"
                                , '\SO'  <$ P.string "u000e"
                                , '\SI'  <$ P.string "u000f"
                                , '\DLE' <$ P.string "u0010"
                                , '\DC1' <$ P.string "u0011"
                                , '\DC2' <$ P.string "u0012"
                                , '\DC3' <$ P.string "u0013"
                                , '\DC4' <$ P.string "u0014"
                                , '\NAK' <$ P.string "u0015"
                                , '\SYN' <$ P.string "u0016"
                                , '\ETB' <$ P.string "u0017"
                                , '\CAN' <$ P.string "u0018"
                                , '\EM'  <$ P.string "u0019"
                                , '\SUB' <$ P.string "u001a"
                                , '\ESC' <$ P.string "u001b"
                                , '\FS'  <$ P.string "u001c"
                                , '\GS'  <$ P.string "u001d"
                                , '\RS'  <$ P.string "u001e"
                                , '\US'  <$ P.string "u001f"
                                , '\DEL' <$ P.string "u007f"
                                , '\0' <$ PC.char '0'
                                , '\a' <$ PC.char 'a'
                                , '\b' <$ PC.char 'b'
                                , '\f' <$ PC.char 'f'
                                , '\n' <$ PC.char 'n'
                                , '\r' <$ PC.char 'r'
                                , '\t' <$ PC.char 't'
                                , '\v' <$ PC.char 'v'
                                , '\"' <$ PC.char '"'
                                , '\'' <$ PC.char '\''
                                , '\\' <$ PC.char '\\'
                                , '/'  <$ PC.char '/'
                                ])

escape :: LBS.ByteString -> Builder
escape = LBS.foldl' escapeChar mempty
  where
    escapeChar b w = b <> case w2c w of
      '\0' -> "\\0"
      '\b' -> "\\b"
      '\f' -> "\\f"
      '\n' -> "\\n"
      '\r' -> "\\r"
      '\t' -> "\\t"
      '\"' -> "\\\""
      '\\' -> "\\\\"
      '\SOH' -> "\\u0001"
      '\STX' -> "\\u0002"
      '\ETX' -> "\\u0003"
      '\EOT' -> "\\u0004"
      '\ENQ' -> "\\u0005"
      '\ACK' -> "\\u0006"
      '\BEL' -> "\\u0007"
      '\VT'  -> "\\u000b"
      '\SO'  -> "\\u000e"
      '\SI'  -> "\\u000f"
      '\DLE' -> "\\u0010"
      '\DC1' -> "\\u0011"
      '\DC2' -> "\\u0012"
      '\DC3' -> "\\u0013"
      '\DC4' -> "\\u0014"
      '\NAK' -> "\\u0015"
      '\SYN' -> "\\u0016"
      '\ETB' -> "\\u0017"
      '\CAN' -> "\\u0018"
      '\EM'  -> "\\u0019"
      '\SUB' -> "\\u001a"
      '\ESC' -> "\\u001b"
      '\FS'  -> "\\u001c"
      '\GS'  -> "\\u001d"
      '\RS'  -> "\\u001e"
      '\US'  -> "\\u001f"
      '\DEL' -> "\\u007f"
      _ -> B.word8 w

lexeme :: Parser a -> Parser a
lexeme = (<* skipSpace)

notChar8 :: Char -> Parser Word8
notChar8 c = P.satisfy (/= c2w c)

between :: Char -> Char -> Parser a -> Parser a
between a b p = lexeme (PC.char8 a) *> lexeme p <* lexeme (PC.char8 b)
