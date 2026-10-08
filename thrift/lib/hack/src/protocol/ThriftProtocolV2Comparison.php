<?hh
// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

case type ThriftProtocolComparisonBytesOrException = string | Throwable;

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

  // Called only when at least one side threw.
  private static function compareFailures(mixed $base, mixed $v2): ?string {
    $describe = (Throwable $e) ==> Classnames::getx($e).': '.$e->getMessage();
    if ($base is Throwable && $v2 is Throwable) {
      return Classnames::getx($base) === Classnames::getx($v2) &&
        $base->getMessage() === $v2->getMessage()
        ? null
        : Str\format(
            'Base and V2 failed differently. Base: %s, V2: %s',
            $describe($base),
            $describe($v2),
          );
    }
    if ($base is Throwable) {
      return 'Base failed but V2 succeeded. '.$describe($base);
    }
    if ($v2 is Throwable) {
      return 'V2 failed but Base succeeded. '.$describe($v2);
    }
    return null;
  }

  <<__TestsBypassVisibility>>
  private static function compareWrite(
    string $bytes_written_by_base,
    ThriftStructTypes::TGenericSpec $spec,
    bool $compact,
    ?int $version,
    int $options,
  ): ?string {
    $base_roundtrip = self::decode(
      $bytes_written_by_base,
      $spec,
      $compact,
      false,
      $version,
      $options,
    );
    if ($base_roundtrip is Throwable) {
      // The base protocol can't read bytes it wrote, so there is no data for
      // V2 to write; V2 should at least fail the same way.
      return self::compareFailures(
        $base_roundtrip,
        self::decode(
          $bytes_written_by_base,
          $spec,
          $compact,
          true,
          $version,
          $options,
        ),
      );
    }
    $v2_bytes = self::encode($base_roundtrip, $spec, $compact, true, $version);
    if ($v2_bytes is Throwable) {
      // If the base protocol fails too, the value is at fault rather than V2.
      return self::compareFailures(
        self::encode($base_roundtrip, $spec, $compact, false, $version),
        $v2_bytes,
      );
    }
    return $v2_bytes === $bytes_written_by_base
      ? null
      : 'V2 differs from the base protocol';
  }

  <<__TestsBypassVisibility>>
  private static function compareRead(
    string $input_bytes,
    ThriftStructTypes::TGenericSpec $spec,
    bool $compact,
    ?int $version,
    int $options,
  ): ?string {
    $base_read =
      self::decode($input_bytes, $spec, $compact, false, $version, $options);
    $v2_read =
      self::decode($input_bytes, $spec, $compact, true, $version, $options);
    if ($base_read is Throwable || $v2_read is Throwable) {
      return self::compareFailures($base_read, $v2_read);
    }

    // Decoded values can't be compared directly, so the base protocol
    // re-encodes both.
    $base_bytes = self::encode($base_read, $spec, $compact, false, $version);
    $v2_bytes = self::encode($v2_read, $spec, $compact, false, $version);
    if ($base_bytes is Throwable || $v2_bytes is Throwable) {
      return self::compareFailures($base_bytes, $v2_bytes);
    }

    // Inputs from other writers can carry unknown fields or a different field
    // order that no Hack round trip reproduces, so the re-encodings are
    // compared with each other rather than with the input.
    return
      $base_bytes === $v2_bytes ? null : 'V2 differs from the base protocol';
  }

  // decode and encode return failures instead of throwing them, so a protocol
  // that throws can be compared against one that doesn't.
  <<__TestsBypassVisibility>>
  private static function decode(
    string $bytes,
    ThriftStructTypes::TGenericSpec $spec,
    bool $compact,
    bool $v2,
    ?int $version,
    int $options,
  ): mixed {
    try {
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
    } catch (
      /* This is intentional to catch TypeErrors in serializer */
      Throwable $e
    ) {
      return $e;
    }
  }

  <<__TestsBypassVisibility>>
  private static function encode(
    mixed $value,
    ThriftStructTypes::TGenericSpec $spec,
    bool $compact,
    bool $v2,
    ?int $version,
  ): ThriftProtocolComparisonBytesOrException {
    try {
      $transport = new TMemoryBuffer();
      // decode() returns the adapted value, but writeStructHelper expects the
      // thrift one.
      ThriftSerializationHelper::writeStructHelper(
        self::protocol($transport, $compact, $v2, $version),
        $spec['type'],
        ThriftSerializationHelper::unwrapApplyAdapter($value, $spec),
        $spec,
      );
      return $transport->getBuffer();
    } catch (
      /* This is intentional to catch TypeErrors in serializer */
      Throwable $e
    ) {
      return $e;
    }
  }

  public static function schedule(
    string $bytes,
    ThriftStructTypes::TGenericSpec $spec,
    bool $compact,
    ?int $version,
    int $options,
    bool $compare_read,
  ): void {
    if ($bytes === '') {
      // The base serializer wrote nothing or read an empty input, so there is
      // nothing to compare.
      return;
    }
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
          $class = Shapes::idx($spec, 'class');
          $type = $class is nonnull
            ? HH\class_to_classname($class)
            : TType::getNames()[$spec['type']];
          $protocol = $compact ? 'compact' : 'binary';

          $details = $compare_read
            ? self::compareRead($bytes, $spec, $compact, $version, $options)
            : self::compareWrite($bytes, $spec, $compact, $version, $options);
          if ($details === null) {
            return;
          }
          // Reports at most once per type per request, so a systematic V2 bug
          // doesn't turn into an error spike.
          $key = $protocol.':'.$type;
          if (C\contains_key(self::$reported, $key)) {
            return;
          }
          // V2 failures are expected findings of this comparison, not
          // production errors, so they go to FBLogger rather than Opes.
          $metadata = dict[
            'type' => $type,
            'protocol' => $protocol,
            'direction' => $compare_read ? 'read' : 'write',
          ];
          self::$reported[] = $key;
          FBLogger('thrift_protocol_v2', 'mismatch')
            ->oncall(OncallShortName\thrift_hack)
            ->addLoggableMetadata($metadata)
            ->info("%s", $details);
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
