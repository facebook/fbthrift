<?php
// Copyright 2004-present Facebook. All Rights Reserved.

/**
 * Copyright (c) 2006- Facebook
 * Distributed under the Thrift Software License
 *
 * See accompanying file LICENSE or visit the Thrift site at:
 * http://developers.facebook.com/thrift/
 *
 * @package thrift
 */

/**
 * Set global THRIFT ROOT automatically via inclusion here
 */
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__;
}


class TType {
  const STOP   = 0;
  const VOID   = 1;
  const BOOL   = 2;
  const BYTE   = 3;
  const I08    = 3;
  const DOUBLE = 4;
  const I16    = 6;
  const I32    = 8;
  const I64    = 10;
  const STRING = 11;
  const UTF7   = 11;
  const STRUCT = 12;
  const MAP    = 13;
  const SET    = 14;
  const LST    = 15;    // N.B. cannot use LIST keyword in PHP!
  const UTF8   = 16;
  const UTF16  = 17;
  const FLOAT  = 19;
}

/**
 * Message types for RPC
 */
class TMessageType {
  const CALL  = 1;
  const REPLY = 2;
  const EXCEPTION = 3;
  const ONEWAY = 4;
}

include_once $GLOBALS['THRIFT_ROOT'].'/TException.php';


/**
 * Base class from which other Thrift structs extend. This is so that we can
 * cut back on the size of the generated code which is turning out to have a
 * nontrivial cost just to load thanks to the wondrously abysmal implementation
 * of PHP. Note that code is intentionally duplicated in here to avoid making
 * function calls for every field or member of a container.
 */
abstract class TBase {

  static $tmethod = array(TType::BOOL   => 'Bool',
                          TType::BYTE   => 'Byte',
                          TType::I16    => 'I16',
                          TType::I32    => 'I32',
                          TType::I64    => 'I64',
                          TType::DOUBLE => 'Double',
                          TType::STRING => 'String',
                          TType::FLOAT  => 'Float');

  public abstract function read(TProtocol $input);

  public abstract function write(TProtocol $output);

  public function __construct($spec=null, $vals=null) {
    if (is_array($spec) && is_array($vals)) {
      foreach ($spec as $fid => $fspec) {
        $var = $fspec['var'];
        if (isset($vals[$var])) {
          $this->$var = $vals[$var];
        }
      }
    }
  }

  private function _readMap(&$var, $spec, $input) {
    $xfer = 0;
    $ktype = $spec['ktype'];
    $vtype = $spec['vtype'];
    $kread = $vread = null;
    $kspec = array();
    $vspec = array();
    if (isset(TBase::$tmethod[$ktype])) {
      $kread = 'read'.TBase::$tmethod[$ktype];
    } else {
      $kspec = $spec['key'];
    }
    if (isset(TBase::$tmethod[$vtype])) {
      $vread = 'read'.TBase::$tmethod[$vtype];
    } else {
      $vspec = $spec['val'];
    }
    $var = array();
    $_ktype = $_vtype = $size = 0;
    $xfer += $input->readMapBegin($_ktype, $_vtype, $size);
    for ($i = 0; $i < $size; ++$i) {
      $key = $val = null;
      if ($kread !== null) {
        $xfer += $input->$kread($key);
      } else {
        switch ($ktype) {
        case TType::STRUCT:
          $class = $kspec['class'];
          $key = new $class();
          $xfer += $key->read($input);
          break;
        case TType::MAP:
          $xfer += $this->_readMap($key, $kspec, $input);
          break;
        case TType::LST:
          $xfer += $this->_readList($key, $kspec, $input, false);
          break;
        case TType::SET:
          $xfer += $this->_readList($key, $kspec, $input, true);
          break;
        }
      }
      if ($vread !== null) {
        $xfer += $input->$vread($val);
      } else {
        switch ($vtype) {
        case TType::STRUCT:
          $class = $vspec['class'];
          $val = new $class();
          $xfer += $val->read($input);
          break;
        case TType::MAP:
          $xfer += $this->_readMap($val, $vspec, $input);
          break;
        case TType::LST:
          $xfer += $this->_readList($val, $vspec, $input, false);
          break;
        case TType::SET:
          $xfer += $this->_readList($val, $vspec, $input, true);
          break;
        }
      }
      $var[$key] = $val;
    }
    $xfer += $input->readMapEnd();
    return $xfer;
  }

  private function _readList(&$var, $spec, $input, $set=false) {
    $xfer = 0;
    $etype = $spec['etype'];
    $eread = $vread = null;
    if (isset(TBase::$tmethod[$etype])) {
      $eread = 'read'.TBase::$tmethod[$etype];
    } else {
      $espec = $spec['elem'];
    }
    $var = array();
    $_etype = $size = 0;
    if ($set) {
      $xfer += $input->readSetBegin($_etype, $size);
    } else {
      $xfer += $input->readListBegin($_etype, $size);
    }
    for ($i = 0; $i < $size; ++$i) {
      $elem = null;
      if ($eread !== null) {
        $xfer += $input->$eread($elem);
      } else {
        $espec = $spec['elem'];
        switch ($etype) {
        case TType::STRUCT:
          $class = $espec['class'];
          $elem = new $class();
          $xfer += $elem->read($input);
          break;
        case TType::MAP:
          $xfer += $this->_readMap($elem, $espec, $input);
          break;
        case TType::LST:
          $xfer += $this->_readList($elem, $espec, $input, false);
          break;
        case TType::SET:
          $xfer += $this->_readList($elem, $espec, $input, true);
          break;
        }
      }
      if ($set) {
        $var[$elem] = true;
      } else {
        $var[] = $elem;
      }
    }
    if ($set) {
      $xfer += $input->readSetEnd();
    } else {
      $xfer += $input->readListEnd();
    }
    return $xfer;
  }

  protected function _read($class, $spec, $input) {
    $xfer = 0;
    $fname = null;
    $ftype = 0;
    $fid = 0;
    $xfer += $input->readStructBegin($fname);
    while (true) {
      $xfer += $input->readFieldBegin($fname, $ftype, $fid);
      if ($ftype == TType::STOP) {
        break;
      }
      if (isset($spec[$fid])) {
        $fspec = $spec[$fid];
        $var = $fspec['var'];
        if ($ftype == $fspec['type']) {
          $xfer = 0;
          if (isset(TBase::$tmethod[$ftype])) {
            $func = 'read'.TBase::$tmethod[$ftype];
            $xfer += $input->$func($this->$var);
          } else {
            switch ($ftype) {
            case TType::STRUCT:
              $class = $fspec['class'];
              $this->$var = new $class();
              $xfer += $this->$var->read($input);
              break;
            case TType::MAP:
              $xfer += $this->_readMap($this->$var, $fspec, $input);
              break;
            case TType::LST:
              $xfer += $this->_readList($this->$var, $fspec, $input, false);
              break;
            case TType::SET:
              $xfer += $this->_readList($this->$var, $fspec, $input, true);
              break;
            }
          }
        } else {
          $xfer += $input->skip($ftype);
        }
      } else {
        $xfer += $input->skip($ftype);
      }
      $xfer += $input->readFieldEnd();
    }
    $xfer += $input->readStructEnd();
    return $xfer;
  }

  private function _writeMap($var, $spec, $output) {
    $xfer = 0;
    $ktype = $spec['ktype'];
    $vtype = $spec['vtype'];
    $kwrite = $vwrite = null;
    $kspec = null;
    $vspec = null;
    if (isset(TBase::$tmethod[$ktype])) {
      $kwrite = 'write'.TBase::$tmethod[$ktype];
    } else {
      $kspec = $spec['key'];
    }
    if (isset(TBase::$tmethod[$vtype])) {
      $vwrite = 'write'.TBase::$tmethod[$vtype];
    } else {
      $vspec = $spec['val'];
    }
    $xfer += $output->writeMapBegin($ktype, $vtype, count($var));
    foreach ($var as $key => $val) {
      if (isset($kwrite)) {
        $xfer += $output->$kwrite($key);
      } else {
        switch ($ktype) {
        case TType::STRUCT:
          $xfer += $key->write($output);
          break;
        case TType::MAP:
          $xfer += $this->_writeMap($key, $kspec, $output);
          break;
        case TType::LST:
          $xfer += $this->_writeList($key, $kspec, $output, false);
          break;
        case TType::SET:
          $xfer += $this->_writeList($key, $kspec, $output, true);
          break;
        }
      }
      if (isset($vwrite)) {
        $xfer += $output->$vwrite($val);
      } else {
        switch ($vtype) {
        case TType::STRUCT:
          $xfer += $val->write($output);
          break;
        case TType::MAP:
          $xfer += $this->_writeMap($val, $vspec, $output);
          break;
        case TType::LST:
          $xfer += $this->_writeList($val, $vspec, $output, false);
          break;
        case TType::SET:
          $xfer += $this->_writeList($val, $vspec, $output, true);
          break;
        }
      }
    }
    $xfer += $output->writeMapEnd();
    return $xfer;
  }

  private function _writeList($var, $spec, $output, $set=false) {
    $xfer = 0;
    $etype = $spec['etype'];
    $ewrite = null;
    $espec = null;
    if (isset(TBase::$tmethod[$etype])) {
      $ewrite = 'write'.TBase::$tmethod[$etype];
    } else {
      $espec = $spec['elem'];
    }
    if ($set) {
      $xfer += $output->writeSetBegin($etype, count($var));
    } else {
      $xfer += $output->writeListBegin($etype, count($var));
    }
    foreach ($var as $key => $val) {
      $elem = $set ? $key : $val;
      if (isset($ewrite)) {
        $xfer += $output->$ewrite($elem);
      } else {
        switch ($etype) {
        case TType::STRUCT:
          $xfer += $elem->write($output);
          break;
        case TType::MAP:
          $xfer += $this->_writeMap($elem, $espec, $output);
          break;
        case TType::LST:
          $xfer += $this->_writeList($elem, $espec, $output, false);
          break;
        case TType::SET:
          $xfer += $this->_writeList($elem, $espec, $output, true);
          break;
        }
      }
    }
    if ($set) {
      $xfer += $output->writeSetEnd();
    } else {
      $xfer += $output->writeListEnd();
    }
    return $xfer;
  }

  protected function _write($class, $spec, $output) {
    $xfer = 0;
    $xfer += $output->writeStructBegin($class);
    foreach ($spec as $fid => $fspec) {
      $var = $fspec['var'];
      if ($this->$var !== null) {
        $ftype = $fspec['type'];
        $xfer += $output->writeFieldBegin($var, $ftype, $fid);
        if (isset(TBase::$tmethod[$ftype])) {
          $func = 'write'.TBase::$tmethod[$ftype];
          $xfer += $output->$func($this->$var);
        } else {
          switch ($ftype) {
          case TType::STRUCT:
            $xfer += $this->$var->write($output);
            break;
          case TType::MAP:
            $xfer += $this->_writeMap($this->$var, $fspec, $output);
            break;
          case TType::LST:
            $xfer += $this->_writeList($this->$var, $fspec, $output, false);
            break;
          case TType::SET:
            $xfer += $this->_writeList($this->$var, $fspec, $output, true);
            break;
          }
        }
        $xfer += $output->writeFieldEnd();
      }
    }
    $xfer += $output->writeFieldStop();
    $xfer += $output->writeStructEnd();
    return $xfer;
  }
}

/**
 * Base interface for Thrift clients
 */
interface IThriftClient {
  public function getEventHandler(): TClientEventHandler;
  public function setEventHandler(TClientEventHandler $handler): this;
  public function getAsyncHandler(): TClientAsyncHandler;
  public function setAsyncHandler(TClientAsyncHandler $handler): this;
}

/**
 * Base interface for Thrift processors
 */
interface IThriftProcessor {
  public function getEventHandler(): TProcessorEventHandler;
  public function setEventHandler(TProcessorEventHandler $handler): this;
  public function process(TProtocol $input, TProtocol $output): bool;
}

/**
 * Base interface for Thrift structs
 */
interface IThriftStruct {
  public function getName(): string;
  public function read(TProtocol $input): int;
  public function write(TProtocol $input): int;
}

/**
 * Event handler for thrift processors
 */
class TProcessorEventHandler {

  // Called at the start of processing a handler method
  public function getHandlerContext($fn_name) {
    return null;
  }

  // Called before the handler method's argument are read
  public function preRead($handler_context, $fn_name, $args) {}

  // Called after the handler method's argument are read
  public function postRead($handler_context, $fn_name, $args) {}

  // Called right before the handler is executed.
  // Exceptions thrown here are caught by handlerException and handlerError.
  public function preExec($handler_context, $fn_name, $args) {}

  // Called right after the handler is executed.
  // Exceptions thrown here are caught by handlerException and handlerError.
  public function postExec($handler_context, $fn_name, $result) {}

  // Called before the handler method's $results are written
  public function preWrite($handler_context, $fn_name, $result) {}

  // Called after the handler method's $results are written
  public function postWrite($handler_context, $fn_name, $result) {}

  // Called if (and only if) the handler threw an expected $exception.
  public function handlerException($handler_context, $fn_name, $exception) {}

  /**
   * Called if (and only if) the handler threw an unexpected $exception.
   *
   * Note that this method is NOT called if the handler threw an
   * exception that is declared in the thrift service specification
   */
  public function handlerError($handler_context, $fn_name, $exception) {
    FBLogger('thrift')
      ->event('unexpected_exception')
      ->exception(
        $exception,
        'Thrift handler threw an exception while handling %s',
        $fn_name);
  }
}

/**
 * Event handler for thrift clients
 */
class TClientEventHandler {
  private $client;

  final public function setClient($client): this {
    $this->client = $client;
    return $this;
  }

  final public function getClient() {
    return nullthrows($this->client);
  }

  // Called before the request is sent to the server
  public function preSend(string $fn_name, $args, int $sequence_id): void {}

  // Called after the request is sent to the server
  public function postSend(string $fn_name, $args, int $sequence_id): void {}

  // Called if there is an exception writing the data
  public function sendError(string $fn_name, $args, int $sequence_id, Exception $ex): void {}

  // Called before the response is read from the server
  // Exceptions thrown here are caught by clientException and clientError.
  public function preRecv(string $fn_name, ?int $ex_sequence_id): void {}

  // Called after the response is read from the server
  public function postRecv(string $fn_name, ?int $ex_sequence_id, $result): void {}

  // Called if (and only if) the client threw an expected $exception.
  public function recvException(string $fn_name, ?int $ex_sequence_id, TException $exception): void {}

  // Called if (and only if) the client threw an unexpected $exception.
  public function recvError(string $fn_name, ?int $ex_sequence_id, Exception $exception): void {}
}

final class TClientMultiEventHandler extends TClientEventHandler {
  private Map<string, TClientEventHandler> $handlers;

  public function __construct($client) {
    $this->handlers = Map {};
    $this->setClient($client);
  }

  public function addHandler(string $key, TClientEventHandler $handler): this {
    $handler->setClient($this->getClient());
    $this->handlers[$key] = $handler;
    return $this;
  }

  public function getHandler(string $key): TClientEventHandler {
    return $this->handlers[$key];
  }

  public function removeHandler(string $key): TClientEventHandler {
    $handler = $this->getHandler($key);
    $this->handlers->remove($key);
    return $handler;
  }

  public function preSend(string $fn_name, $args, int $sequence_id): void {
    foreach ($this->handlers as $handler) {
      $handler->preSend($fn_name, $args, $sequence_id);
    }
  }

  public function postSend(string $fn_name, $args, int $sequence_id): void {
    foreach ($this->handlers as $handler) {
      $handler->postSend($fn_name, $args, $sequence_id);
    }
  }

  public function sendError(string $fn_name, $args, int $sequence_id, Exception $ex): void {
    foreach ($this->handlers as $handler) {
      $handler->sendError($fn_name, $args, $sequence_id, $ex);
    }
  }

  public function preRecv(string $fn_name, ?int $ex_sequence_id): void {
    foreach ($this->handlers as $handler) {
      $handler->preRecv($fn_name, $ex_sequence_id);
    }
  }

  public function postRecv(string $fn_name, ?int $ex_sequence_id, $result): void {
    foreach ($this->handlers as $handler) {
      $handler->postRecv($fn_name, $ex_sequence_id, $result);
    }
  }

  public function recvException(string $fn_name, ?int $ex_sequence_id, TException $exception): void {
    foreach ($this->handlers as $handler) {
      $handler->recvException($fn_name, $ex_sequence_id, $exception);
    }
  }

  public function recvError(string $fn_name, ?int $ex_sequence_id, Exception $exception): void {
    foreach ($this->handlers as $handler) {
      $handler->recvError($fn_name, $ex_sequence_id, $exception);
    }
  }
}

/**
 * This allows clients to perform calls as generators
 */
class TClientAsyncHandler {
  // This is called between the send_methodName() and recv_methodName() for
  //   gen_methodName() calls
  public async function genWait(int $sequence_id): Awaitable<void> {
    // Do nothing
  }
}

class TApplicationException extends TException {
  static $_TSPEC =
    array(1 => array('var' => 'message',
                     'type' => TType::STRING),
          2 => array('var' => 'code',
                     'type' => TType::I32));

  const UNKNOWN = 0;
  const UNKNOWN_METHOD = 1;
  const INVALID_MESSAGE_TYPE = 2;
  const WRONG_METHOD_NAME = 3;
  const BAD_SEQUENCE_ID = 4;
  const MISSING_RESULT = 5;
  const INVALID_TRANSFORM = 6;

  public function __construct($message=null, $code=0) {
    parent::__construct($message, $code);
  }

  public function read(TProtocol $output) {
    return $this->_read('TApplicationException', self::$_TSPEC, $output);
  }

  public function write(TProtocol $output) {
    $xfer = 0;
    $xfer += $output->writeStructBegin('TApplicationException');
    if ($message = $this->getMessage()) {
      $xfer += $output->writeFieldBegin('message', TType::STRING, 1);
      $xfer += $output->writeString($message);
      $xfer += $output->writeFieldEnd();
    }
    if ($code = $this->getCode()) {
      $xfer += $output->writeFieldBegin('type', TType::I32, 2);
      $xfer += $output->writeI32($code);
      $xfer += $output->writeFieldEnd();
    }
    $xfer += $output->writeFieldStop();
    $xfer += $output->writeStructEnd();
    return $xfer;
  }
}
include_once $GLOBALS['THRIFT_ROOT'].'/protocol/TProtocol.php';
include_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransport.php';
