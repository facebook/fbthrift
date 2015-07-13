<?php

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift
*/

require_once ($GLOBALS['HACKLIB_ROOT']);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__;
}
require_once $GLOBALS['THRIFT_ROOT'].'/TType.php';
abstract class TBase {
  static
    $tmethod = array(
      TType::BOOL => 'Bool',
      TType::BYTE => 'Byte',
      TType::I16 => 'I16',
      TType::I32 => 'I32',
      TType::I64 => 'I64',
      TType::DOUBLE => 'Double',
      TType::STRING => 'String',
      TType::FLOAT => 'Float'
    );
  public abstract function read($input);
  public abstract function write($output);
  public function __construct($spec = null, $vals = null) {
    if (\hacklib_cast_as_boolean(is_array($spec)) &&
        \hacklib_cast_as_boolean(is_array($vals))) {
      foreach ($spec as $fid => $fspec) {
        $var = $fspec[\hacklib_id('var')];
        if (\hacklib_cast_as_boolean(isset($vals[$var]))) {
          $this->$var = $vals[$var];
        }
      }
    }
  }
  private function _readMap(&$var, $spec, $input) {
    $xfer = 0;
    $ktype = $spec[\hacklib_id('ktype')];
    $vtype = $spec[\hacklib_id('vtype')];
    $kread = $vread = null;
    $kspec = array();
    $vspec = array();
    if (\hacklib_cast_as_boolean(isset(TBase::$tmethod[$ktype]))) {
      $kread = 'read'.TBase::$tmethod[$ktype];
    } else {
      $kspec = $spec[\hacklib_id('key')];
    }
    if (\hacklib_cast_as_boolean(isset(TBase::$tmethod[$vtype]))) {
      $vread = 'read'.TBase::$tmethod[$vtype];
    } else {
      $vspec = $spec[\hacklib_id('val')];
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
            $class = $kspec[\hacklib_id('class')];
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
            $class = $vspec[\hacklib_id('class')];
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
  private function _readList(&$var, $spec, $input, $set = false) {
    $xfer = 0;
    $etype = $spec[\hacklib_id('etype')];
    $eread = $vread = null;
    if (\hacklib_cast_as_boolean(isset(TBase::$tmethod[$etype]))) {
      $eread = 'read'.TBase::$tmethod[$etype];
    } else {
      $espec = $spec[\hacklib_id('elem')];
    }
    $var = array();
    $_etype = $size = 0;
    if (\hacklib_cast_as_boolean($set)) {
      $xfer += $input->readSetBegin($_etype, $size);
    } else {
      $xfer += $input->readListBegin($_etype, $size);
    }
    for ($i = 0; $i < $size; ++$i) {
      $elem = null;
      if ($eread !== null) {
        $xfer += $input->$eread($elem);
      } else {
        $espec = $spec[\hacklib_id('elem')];
        switch ($etype) {
          case TType::STRUCT:
            $class = $espec[\hacklib_id('class')];
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
      if (\hacklib_cast_as_boolean($set)) {
        $var[$elem] = true;
      } else {
        $var[] = $elem;
      }
    }
    if (\hacklib_cast_as_boolean($set)) {
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
      if (\hacklib_equals($ftype, TType::STOP)) {
        break;
      }
      if (\hacklib_cast_as_boolean(isset($spec[$fid]))) {
        $fspec = $spec[$fid];
        $var = $fspec[\hacklib_id('var')];
        if (\hacklib_equals($ftype, $fspec[\hacklib_id('type')])) {
          $xfer = 0;
          if (\hacklib_cast_as_boolean(isset(TBase::$tmethod[$ftype]))) {
            $func = 'read'.TBase::$tmethod[$ftype];
            $xfer += $input->$func($this->$var);
          } else {
            switch ($ftype) {
              case TType::STRUCT:
                $class = $fspec[\hacklib_id('class')];
                $this->$var = new $class();
                $xfer += $this->$var->read($input);
                break;
              case TType::MAP:
                $xfer += $this->_readMap($this->$var, $fspec, $input);
                break;
              case TType::LST:
                $xfer +=
                  $this->_readList($this->$var, $fspec, $input, false);
                break;
              case TType::SET:
                $xfer +=
                  $this->_readList($this->$var, $fspec, $input, true);
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
    $ktype = $spec[\hacklib_id('ktype')];
    $vtype = $spec[\hacklib_id('vtype')];
    $kwrite = $vwrite = null;
    $kspec = null;
    $vspec = null;
    if (\hacklib_cast_as_boolean(isset(TBase::$tmethod[$ktype]))) {
      $kwrite = 'write'.TBase::$tmethod[$ktype];
    } else {
      $kspec = $spec[\hacklib_id('key')];
    }
    if (\hacklib_cast_as_boolean(isset(TBase::$tmethod[$vtype]))) {
      $vwrite = 'write'.TBase::$tmethod[$vtype];
    } else {
      $vspec = $spec[\hacklib_id('val')];
    }
    $xfer += $output->writeMapBegin($ktype, $vtype, count($var));
    foreach ($var as $key => $val) {
      if (\hacklib_cast_as_boolean(isset($kwrite))) {
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
      if (\hacklib_cast_as_boolean(isset($vwrite))) {
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
  private function _writeList($var, $spec, $output, $set = false) {
    $xfer = 0;
    $etype = $spec[\hacklib_id('etype')];
    $ewrite = null;
    $espec = null;
    if (\hacklib_cast_as_boolean(isset(TBase::$tmethod[$etype]))) {
      $ewrite = 'write'.TBase::$tmethod[$etype];
    } else {
      $espec = $spec[\hacklib_id('elem')];
    }
    if (\hacklib_cast_as_boolean($set)) {
      $xfer += $output->writeSetBegin($etype, count($var));
    } else {
      $xfer += $output->writeListBegin($etype, count($var));
    }
    foreach ($var as $key => $val) {
      $elem = \hacklib_cast_as_boolean($set) ? $key : $val;
      if (\hacklib_cast_as_boolean(isset($ewrite))) {
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
    if (\hacklib_cast_as_boolean($set)) {
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
      $var = $fspec[\hacklib_id('var')];
      if ($this->$var !== null) {
        $ftype = $fspec[\hacklib_id('type')];
        $xfer += $output->writeFieldBegin($var, $ftype, $fid);
        if (\hacklib_cast_as_boolean(isset(TBase::$tmethod[$ftype]))) {
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
              $xfer +=
                $this->_writeList($this->$var, $fspec, $output, false);
              break;
            case TType::SET:
              $xfer +=
                $this->_writeList($this->$var, $fspec, $output, true);
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
