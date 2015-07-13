<?hh

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.protocol.compact
*/

/**
 * Accelerated compact protocol: used in conjunction with a Thrift HPHP
 * extension for faster serialization and deserialization. The generated Thrift
 * code uses instanceof to look for this class and call into the extension.
 */
class TCompactProtocolAccelerated extends TCompactProtocolBase {
  // The generated Thrift code calls this as a final check. If it returns true,
  // the HPHP extension will be used; if it returns false, the above PHP code is
  // used as a fallback.
  public static function checkVersion($v) {
    return $v == 1;
  }

  public function __construct($trans) {
    // If the transport doesn't implement putBack, wrap it in a
    // TBufferedTransport (which does)
    if (!($trans instanceof IThriftBufferedTransport)) {
      $trans = new TBufferedTransport($trans);
    }

    parent::__construct($trans);
  }
}
