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

import Control.Applicative
import Data.Attoparsec.ByteString as P
import Data.Attoparsec.ByteString.Char8 as PC
import Data.ByteString.Builder as B
import Data.ByteString.Internal (c2w, w2c)
import Data.Int
import Data.List
import Data.Monoid
import Data.Text.Lazy.Encoding
import Data.Word
import qualified Data.HashMap.Strict as Map

import Thrift.Protocol
import Thrift.Transport
import Thrift.Types

import qualified Data.ByteString.Lazy as LBS
import qualified Data.Text.Lazy as LT

data JSONProtocol t = JSONProtocol t

instance Protocol JSONProtocol where
    getTransport (JSONProtocol t) = t

    writeMessage (JSONProtocol t) (s, ty, sq) = tWrite t $ toLazyByteString $
      "[" <> int32Dec version1 <>
      ",\"" <> escape (encodeUtf8 s) <> "\"" <>
      "," <> intDec (fromEnum ty) <>
      "," <> int32Dec sq <>
      "]"
    readMessage p = runParser p $ skipSpace *> do
      _ver :: Int32 <- lexeme (PC.char8 '[') *> lexeme (signed decimal)
      bs <- lexeme (PC.char8 ',') *> lexeme escapedString
      case decodeUtf8' bs of
        Left _ -> fail "readMessage: invalid text encoding"
        Right str -> do
          ty <- toEnum <$> (lexeme (PC.char8 ',') *> lexeme (signed decimal))
          seqNum <- between ',' ']' $ lexeme (signed decimal)
          return (str, ty, seqNum)

    writeVal (JSONProtocol t) = tWrite t . toLazyByteString . buildJSONValue
    readVal p ty = runParser p $ skipSpace *> parseJSONValue ty


-- | Writing Functions

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

buildJSONStruct :: [(Int16, LT.Text, ThriftVal)] -> Builder
buildJSONStruct = mconcat . intersperse "," . map (\(_,str,val) ->
  "\"" <> B.lazyByteString (encodeUtf8 str) <> "\":" <> buildJSONValue val)

buildJSONMap :: [(ThriftVal, ThriftVal)] -> Builder
buildJSONMap = mconcat . intersperse "," . map (\(key, val) ->
  buildJSONValue key <> ":" <> buildJSONValue val)

buildJSONList :: [ThriftVal] -> Builder
buildJSONList = mconcat . intersperse "," . map buildJSONValue


-- | Reading Functions

parseJSONValue :: ThriftType -> Parser ThriftVal
parseJSONValue (T_STRUCT tmap) =
  TStruct <$> between '{' '}' (parseJSONStruct tmap)
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

parseJSONStruct :: TypeMap -> Parser [(Int16, LT.Text, ThriftVal)]
parseJSONStruct tmap = flip sepBy (lexeme $ PC.char8 ',') $ do
  bs <- lexeme escapedString <* lexeme (PC.char8 ':')
  case decodeUtf8' bs of
    Left _ -> fail "parseJSONStruct: invalid key encoding"
    Right str -> case Map.lookup str tmap of
      Just (fid, ftype) -> do
        val <- lexeme (parseJSONValue ftype)
        return (fid, str, val)
      Nothing -> fail "parseJSONStruct: invalid key"

parseJSONMap :: ThriftType -> ThriftType -> Parser [(ThriftVal, ThriftVal)]
parseJSONMap kt vt = liftA2 (,)
                     (lexeme (parseJSONValue kt) <* lexeme (PC.char8 ':'))
                     (lexeme $ parseJSONValue vt) `sepBy`
                     lexeme (PC.char8 ',')

parseJSONList :: ThriftType -> Parser [ThriftVal]
parseJSONList ty = lexeme (parseJSONValue ty) `sepBy` lexeme (PC.char8 ',')

escapedString :: Parser LBS.ByteString
escapedString = PC.char8 '"' *>
                (LBS.pack <$> P.many' (escapedChar <|> notChar8 '"')) <*
                PC.char8 '"'

escapedChar :: Parser Word8
escapedChar = PC.char8 '\\' *> (c2w <$> choice
                                [ '\0' <$ PC.char '0'
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
                                ])

escape :: LBS.ByteString -> Builder
escape = LBS.foldl' escapeChar mempty
  where
    escapeChar b w = b <> case w2c w of
      '\0' -> "\\0"
      '\a' -> "\\a"
      '\b' -> "\\b"
      '\f' -> "\\f"
      '\n' -> "\\n"
      '\r' -> "\\r"
      '\t' -> "\\t"
      '\v' -> "\\v"
      '\"' -> "\\\""
      '\'' -> "\\'"
      '\\' -> "\\\\"
      _ -> B.word8 w

lexeme :: Parser a -> Parser a
lexeme = (<* skipSpace)

notChar8 :: Char -> Parser Word8
notChar8 c = P.satisfy (/= c2w c)

between :: Char -> Char -> Parser a -> Parser a
between a b p = lexeme (PC.char8 a) *> lexeme p <* lexeme (PC.char8 b)
