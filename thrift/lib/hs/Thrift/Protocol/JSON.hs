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

module Thrift.Protocol.JSON
    ( module Thrift.Protocol
    , JSONProtocol(..)
    ) where

#if __GLASGOW_HASKELL__ < 710
import Control.Applicative
#endif
import Control.Exception
import Data.Attoparsec.ByteString as P
import Data.Attoparsec.ByteString.Char8 as PC
import Data.Attoparsec.ByteString.Lazy as LP
import Data.ByteString.Builder as B
import Data.Int
import Data.List
import Data.Monoid
import Data.Text.Lazy.Encoding
import qualified Data.HashMap.Strict as Map

import Thrift.Protocol
import Thrift.Protocol.JSONUtils
import Thrift.Transport
import Thrift.Types

import qualified Data.ByteString.Lazy as LBS
import qualified Data.Text.Lazy as LT

-- | The JSON Protocol data uses the standard 'TJSONProtocol'.  Data is
-- encoded as a JSON 'ByteString'
data JSONProtocol t = JSONProtocol t
                      -- ^ Construct a 'JSONProtocol' with a 'Transport'

instance Protocol JSONProtocol where
    getTransport (JSONProtocol t) = t

    writeMessage (JSONProtocol t) (s, ty, sq) =
      bracket writeMessageBegin writeMessageEnd . const
      where
        writeMessageBegin = tWrite t $ toLazyByteString $
          "[" <> int32Dec 1 <>
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
              seqNum <- lexeme (PC.char8 ',') *> lexeme (signed decimal)
              _ <- PC.char8 ','
              return (str, ty, seqNum)
        readMessageEnd _ = runParser p (PC.char8 ']')

    serializeVal _ = toLazyByteString . buildJSONValue
    deserializeVal _ ty bs =
      case LP.eitherResult $ LP.parse (parseJSONValue ty) bs of
        Left s -> error s
        Right val -> val

    readVal p ty = runParser p $ skipSpace *> parseJSONValue ty


-- Writing Functions

buildJSONValue :: ThriftVal -> Builder
buildJSONValue (TStruct fields) = "{" <> buildJSONStruct fields <> "}"
buildJSONValue (TMap kt vt entries) =
  "[\"" <> getTypeName kt <> "\"" <>
  ",\"" <> getTypeName vt <> "\"" <>
  "," <> intDec (length entries) <>
  ",{" <> buildJSONMap entries <> "}" <>
  "]"
buildJSONValue (TList ty entries) =
  "[\"" <> getTypeName ty <> "\"" <>
  "," <> intDec (length entries) <>
  (if null entries
   then mempty
   else "," <> buildJSONList entries) <>
  B.char8 ']'
buildJSONValue (TSet ty entries) = buildJSONValue (TList ty entries)
buildJSONValue (TBool b) = intDec $ fromEnum b
buildJSONValue (TByte b) = int8Dec b
buildJSONValue (TI16 i) = int16Dec i
buildJSONValue (TI32 i) = int32Dec i
buildJSONValue (TI64 i) = int64Dec i
buildJSONValue (TFloat f) = floatDec f
buildJSONValue (TDouble d) = doubleDec d
buildJSONValue (TString s) = "\"" <> escape s <> "\""

buildJSONStruct :: Map.HashMap Int16 (LT.Text, ThriftVal) -> Builder
buildJSONStruct = mconcat . intersperse "," . Map.foldrWithKey buildField []
  where
    buildField fid (_,val) = (:) $
      "\"" <> int16Dec fid <> "\":" <>
      "{\"" <> getTypeName (getTypeOf val) <> "\":" <>
      buildJSONValue val <>
      "}"

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
parseJSONValue (T_MAP _ _) = between '[' ']' $ do
  kt <- fromTypeName <$> lexeme escapedString <* lexeme (PC.char8 ',')
  vt <- fromTypeName <$> lexeme escapedString <* lexeme (PC.char8 ',')
  TMap kt vt <$> ((lexeme decimal :: Parser Int) *> lexeme (PC.char8 ',') *>
    between '{' '}' (parseJSONMap kt vt))
parseJSONValue (T_LIST _) = between '[' ']' $ do
  ty <- fromTypeName <$> lexeme escapedString <* lexeme (PC.char8 ',')
  len :: Int <- lexeme decimal
  TList ty <$> if len > 0
               then lexeme (PC.char8 ',') *> parseJSONList ty
               else return []
parseJSONValue (T_SET _) = between '[' ']' $ do
  ty <- fromTypeName <$> lexeme escapedString <* lexeme (PC.char8 ',')
  len :: Int <- lexeme decimal
  TSet ty <$> if len > 0
              then lexeme (PC.char8 ',') *> parseJSONList ty
              else return []
parseJSONValue T_BOOL = TBool . toEnum <$> decimal
parseJSONValue T_BYTE = TByte <$> signed decimal
parseJSONValue T_I16 = TI16 <$> signed decimal
parseJSONValue T_I32 = TI32 <$> signed decimal
parseJSONValue T_I64 = TI64 <$> signed decimal
parseJSONValue T_FLOAT = TFloat . realToFrac <$> double
parseJSONValue T_DOUBLE = TDouble <$> double
parseJSONValue T_STRING = TString <$> escapedString
parseJSONValue T_STOP = fail "parseJSONValue: cannot parse type T_STOP"
parseJSONValue T_VOID = fail "parseJSONValue: cannot parse type T_VOID"

parseJSONStruct :: TypeMap -> Parser (Map.HashMap Int16 (LT.Text, ThriftVal))
parseJSONStruct _ = Map.fromList <$> parseField `sepBy` lexeme (PC.char8 ',')
  where
    parseField = do
      fid <- lexeme (between '"' '"' decimal) <* lexeme (PC.char8 ':')
      between '{' '}' $ do
        ty <- fromTypeName <$> lexeme escapedString <* lexeme (PC.char8 ':')
        val <- lexeme (parseJSONValue ty)
        return (fid, ("", val))

parseJSONMap :: ThriftType -> ThriftType -> Parser [(ThriftVal, ThriftVal)]
parseJSONMap kt vt =
  ((,) <$> lexeme (PC.char8 '"' *> parseJSONValue kt <* PC.char8 '"') <*>
   (lexeme (PC.char8 ':') *> lexeme (parseJSONValue vt))) `sepBy`
  lexeme (PC.char8 ',')

parseJSONList :: ThriftType -> Parser [ThriftVal]
parseJSONList ty = lexeme (parseJSONValue ty) `sepBy` lexeme (PC.char8 ',')

getTypeName :: ThriftType -> Builder
getTypeName ty = case ty of
  T_STRUCT _ -> "rec"
  T_MAP _ _  -> "map"
  T_LIST _   -> "lst"
  T_SET _    -> "set"
  T_BOOL     -> "tf"
  T_BYTE     -> "i8"
  T_I16      -> "i16"
  T_I32      -> "i32"
  T_I64      -> "i64"
  T_FLOAT    -> "flt"
  T_DOUBLE   -> "dbl"
  T_STRING   -> "str"
  _ -> throw $ ProtocolExn PE_INVALID_DATA "Bad Type"

fromTypeName :: LBS.ByteString -> ThriftType
fromTypeName ty = case ty of
  "rec" -> T_STRUCT Map.empty
  "map" -> T_MAP T_VOID T_VOID
  "lst" -> T_LIST T_VOID
  "set" -> T_SET T_VOID
  "tf"  -> T_BOOL
  "i8"  -> T_BYTE
  "i16" -> T_I16
  "i32" -> T_I32
  "i64" -> T_I64
  "flt" -> T_FLOAT
  "dbl" -> T_DOUBLE
  "str" -> T_STRING
  t -> throw $ ProtocolExn PE_INVALID_DATA ("Bad Type: " ++ show t)
