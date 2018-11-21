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
{-# LANGUAGE OverloadedStrings #-}

module Thrift.Protocol.JSONUtils
       ( escapedString
       , escapedChar
       , escape
       , lexeme
       , notChar8
       , between
       ) where

import Control.Applicative
import Data.Attoparsec.ByteString as P
import Data.Attoparsec.ByteString.Char8 as PC
import Data.Bits
import Data.ByteString.Builder as B
import Data.ByteString.Internal (c2w, w2c)
import Data.Char as C
#if __GLASGOW_HASKELL__ < 804
import Data.Monoid
#endif
import Data.Word
import qualified Data.ByteString.Lazy as LBS

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
                                , '/'  <$ PC.char '/'
                                , PC.string "u00" *> hexdigits
                                ])
    where
      -- The cpp implementation only accepts escaped characters of the form
      -- "\u00XX" where the X's are hex digits, so we will do the same here
      hexdigits :: Parser Char
      hexdigits = do
        msB <- hexdigit =<< anyWord8
        lsB <- hexdigit =<< anyWord8
        return $ C.chr $ (msB `shiftL` 4) .|. lsB

      hexdigit n | n >= w_0 && n <= w_9 = return $ fromIntegral $ n - w_0
                 | n >= w_a && n <= w_f = return $ fromIntegral $ n - w_a + 10
                 | n >= w_A && n <= w_F = return $ fromIntegral $ n - w_A + 10
                 | otherwise            = fail $ "not a hex digit: " ++ show n

      w_0 = fromIntegral $ ord '0'
      w_9 = fromIntegral $ ord '9'
      w_a = fromIntegral $ ord 'a'
      w_f = fromIntegral $ ord 'f'
      w_A = fromIntegral $ ord 'A'
      w_F = fromIntegral $ ord 'F'

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
