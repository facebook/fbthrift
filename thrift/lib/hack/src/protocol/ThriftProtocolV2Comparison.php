<?hh
// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * Checks TBinaryProtocolV2 and TCompactProtocolV2 against the base protocols
 * on production data. TBinarySerializer and TCompactSerializer call it from
 * the paths that serialize in Hack, in requests sampled by their
 * thrift/hack:*_protocol_v2 knobs. The comparison runs in PSP on a copy of
 * the bytes and never changes what the serializer returns.
 *
 * Overflowing compact i64s mismatch; if common, accept V2 matching extension.
 */
// @oss-disable: <<Oncalls('thrift')>>
abstract final class ThriftProtocolV2Comparison {

  private static keyset<string> $reported = keyset[];

  public static function compareStruct(
    string $bytes,
    IThriftStruct $object,
    bool $compact,
    ?int $version = null,
    int $options = 0,
  ): void {
    $class = Classnames::getx($object);
    self::schedule(
      $bytes,
      shape('type' => TType::STRUCT, 'class' => HH\classname_to_class($class)),
      $class,
      $compact,
      $version,
      $options,
    );
  }

  public static function compareData(
    string $bytes,
    ThriftStructTypes::TGenericSpec $spec,
    bool $compact,
  ): void {
    self::schedule($bytes, $spec, null, $compact, null, 0);
  }

  // V2's decode is re-encoded by the base protocol and by V2. Once the base
  // encoding confirms the decoded value, a V2 encode bug can't hide behind a
  // matching V2 decode bug.
  <<__TestsBypassVisibility>>
  private static function decode(
    string $bytes,
    ThriftStructTypes::TGenericSpec $spec,
    bool $compact,
    bool $v2,
    ?int $version,
    int $options,
  ): mixed {
    $value = null;
    $has_wrapper = false;
    ThriftSerializationHelper::readStructHelper(
      self::protocol(new TMemoryBuffer($bytes), $compact, $v2, $version)
        ->setOptions($options),
      $spec['type'],
      inout $value,
      $spec,
      inout $has_wrapper,
    );
    return $value;
  }

  <<__TestsBypassVisibility>>
  private static function encode(
    mixed $value,
    ThriftStructTypes::TGenericSpec $spec,
    bool $compact,
    bool $v2,
    ?int $version,
  ): string {
    $transport = new TMemoryBuffer();
    ThriftSerializationHelper::writeStructHelper(
      self::protocol($transport, $compact, $v2, $version),
      $spec['type'],
      $value,
      $spec,
    );
    return $transport->getBuffer();
  }

  private static function schedule(
    string $bytes,
    ThriftStructTypes::TGenericSpec $spec,
    ?classname<IThriftStruct> $class,
    bool $compact,
    ?int $version,
    int $options,
  ): void {
    // This runs inside a serializer call that already succeeded, so nothing
    // here may throw into it.
    try {
      $psp = PSP();
      // Scheduling after PSP has finished throws; skip that expected case
      // without reporting it.
      if ($psp->isDisallowNewRegisters()) {
        return;
      }
      $psp->startLaterAndWaitFor(
        async () ==> {
          $type = $class ?? TType::getNames()[$spec['type']];
          $protocol = $compact ? 'compact' : 'binary';
          // Reports at most once per type per request, so a systematic V2 bug
          // doesn't turn into an error spike.
          $key = $protocol.':'.$type;
          if (C\contains_key(self::$reported, $key)) {
            return;
          }
          // V2 failures are expected findings of this comparison, not
          // production errors, so they go to FBLogger rather than Opes.
          $metadata = dict['type' => $type, 'protocol' => $protocol];
          $direction = 'read';
          try {
            $value =
              self::decode($bytes, $spec, $compact, true, $version, $options);
            $read = self::encode($value, $spec, $compact, false, $version);
            $direction = 'write';
            $write = self::encode($value, $spec, $compact, true, $version);
          } catch (Throwable $e) {
            self::$reported[] = $key;
            $logger = FBLogger('thrift_protocol_v2', 'exception')
              ->oncall(OncallShortName\thrift_hack)
              ->addLoggableMetadata($metadata);
            if ($e is Exception) {
              $logger->catching($e);
            }
            $logger->info('V2 %s threw %s', $direction, Classnames::getx($e));
            return;
          }
          // Inputs from other writers can carry unknown fields or a different
          // field order that no Hack round trip reproduces, so a result that
          // differs from the input is checked against the base round trip
          // before it counts as a mismatch.
          if ($read === $bytes && $write === $bytes) {
            return;
          }
          // Without a base decode there is nothing to compare V2 against, so
          // this skips the comparison rather than blaming V2.
          try {
            $expected = self::encode(
              self::decode($bytes, $spec, $compact, false, $version, $options),
              $spec,
              $compact,
              false,
              $version,
            );
          } catch (Exception $e) {
            self::$reported[] = $key;
            ope(
              $e,
              causes_the('thrift protocol V2 comparison')->to('be skipped'),
              extras()->add('type', $type)->add('protocol', $protocol),
            );
            return;
          }
          if ($read !== $expected) {
            $direction = 'read';
          } else if ($write !== $expected) {
            $direction = 'write';
          } else {
            return;
          }
          self::$reported[] = $key;
          FBLogger('thrift_protocol_v2', 'mismatch')
            ->oncall(OncallShortName\thrift_hack)
            ->addLoggableMetadata($metadata)
            ->info('V2 %s differs from the base protocol', $direction);
        },
        OncallShortName\thrift_hack,
      );
    } catch (Exception $e) {
      ope(
        $e,
        causes_the('thrift protocol V2 comparison')->to('be skipped'),
        extras()->add('protocol', $compact ? 'compact' : 'binary'),
      );
    }
  }

  private static function protocol(
    TMemoryBuffer $transport,
    bool $compact,
    bool $v2,
    ?int $version,
  ): TProtocol {
    if (!$compact) {
      return $v2
        ? new TBinaryProtocolV2($transport)
        : new TBinaryProtocolAccelerated($transport);
    }
    $protocol = $v2
      ? new TCompactProtocolV2($transport)
      : new TCompactProtocolAccelerated($transport);
    if ($version !== null) {
      $protocol->setWriteVersion($version);
    }
    return $protocol;
  }
}
