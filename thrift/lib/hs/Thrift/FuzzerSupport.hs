{-# LANGUAGE ScopedTypeVariables, DeriveDataTypeable #-}
module Thrift.FuzzerSupport
where

import Control.Exception (catches, Handler(..), throw, IOException, Exception)
import Data.Maybe (isNothing)
import Data.Typeable (Typeable)
import Network (PortNumber, PortID(PortNumber))
import System.Console.GetOpt (getOpt, usageInfo, ArgOrder(..), OptDescr(..), ArgDescr(..))
import System.IO (Handle, hFlush, stdout)
import System.Random (split)
import System.Timeout (timeout)
import Test.QuickCheck.Gen (Gen(..))
import Test.QuickCheck.Random (newQCGen)
import Thrift (AppExn)
import Thrift.Protocol.Binary (BinaryProtocol(..))
import Thrift.Transport (Transport, TransportExn, tClose)
import Thrift.Transport.Handle (hOpen)
import Thrift.Transport.Framed (openFramedTransport, FramedTransport)

-- Configuration via command-line parsing
data Options = Options {
    opt_host :: String,
    opt_port :: PortNumber,
    opt_service :: String,
    opt_timeout :: Int,
    opt_framed :: Bool,
    opt_verbose :: Bool
}

defaultOptions :: Options
defaultOptions = Options {
     opt_host = "localhost"
   , opt_port = 9090
   , opt_service = "ERROR"
   , opt_timeout = 1
   , opt_framed = False
   , opt_verbose = False
}
optionsDescriptions :: [OptDescr (Options -> Options)]
optionsDescriptions = [
      Option ['h'] ["host"] (ReqArg getHost "HOST") "hostname of service server"
    , Option ['p'] ["port"] (ReqArg getPort "PORT") "port of service"
    , Option ['t'] ["timeout"] (ReqArg getTimeout "TIMEOUT") "timeout (s) to determine if a service died"
    , Option ['f'] ["framed"] (NoArg getFramed) "use a framed transport"
    , Option ['v'] ["verbose"] (NoArg getVerbose) "print information for application exceptions"
    ]
getHost, getPort, getTimeout :: String -> Options -> Options
getHost newHost oldOpts = oldOpts { opt_host = newHost }
getPort newPort oldOpts = oldOpts { opt_port = fromIntegral $ (read newPort :: Integer) }
getTimeout newTimeout oldOpts = oldOpts { opt_timeout = fromIntegral $ (read newTimeout :: Int) }

getFramed, getVerbose :: Options -> Options
getFramed oldOpts = oldOpts { opt_framed = True }
getVerbose oldOpts = oldOpts { opt_verbose = True }

getOptions :: [String] -> ([Options -> Options], [String], [String])
getOptions = getOpt RequireOrder optionsDescriptions

usage :: String
usage = usageInfo header optionsDescriptions
    where
        header = "[OPTIONS ...] serviceName"

-- timeout
data Timeout = Timeout deriving (Show, Typeable)
instance Exception Timeout

-- Generic random data generation
infexamples :: Gen a -> IO [a]
infexamples (MkGen m) =
  do rand <- newQCGen
     let rnds rnd = rnd1 : rnds rnd2 where (rnd1, rnd2) = split rnd
     return [(m r n) | (r, n) <- rnds rand `zip` [0,2..] ]

-- Thrift setup
withHandle :: Options -> (Handle -> IO a) -> IO a
withHandle opts action = do
    transport <- getHandle opts
    result <- action transport
    tClose transport
    return result
  where
      getHandle (Options host port _service _timeout _framed _verbose) =
          hOpen (host, PortNumber port)

withFramedTransport :: Options -> (FramedTransport Handle -> IO a) -> IO a
withFramedTransport opts action = withHandle opts $ \h -> do
    transport <- openFramedTransport h
    result <- action transport
    tClose transport
    return result

getClient :: Transport a => a -> (BinaryProtocol a, BinaryProtocol a)
getClient transport = (BinaryProtocol transport, BinaryProtocol transport)

secsToMicrosecs :: Int
secsToMicrosecs = 10 ^ (6 :: Int)

withThriftDo :: Transport t => Options -> ((t -> IO ()) -> IO ()) -> ((BinaryProtocol t, BinaryProtocol t) -> IO ()) -> IO () -> IO ()
withThriftDo opts withTransport action onException = do
    hFlush stdout
    timed <- timeout (opt_timeout opts * secsToMicrosecs) (withTransport (action . getClient))
    if isNothing timed then throw Timeout else return ()
    putStr "."
    `catches`
    [ Handler (\ (ex :: TransportExn) -> do { putStrLn "Crashed it. I win. "; onException; throw ex } )
    , Handler (\ (_ :: AppExn) -> if opt_verbose opts then putStr "\n" >> onException else putStr "*" )
    , Handler (\ (ex :: IOException) -> do { putStrLn "Service is down. I win."; onException; throw ex } )
    , Handler (\ (ex :: Timeout) -> do { putStrLn "Timeout. I win."; onException; throw ex } )
    ]
