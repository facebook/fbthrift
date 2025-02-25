<?hh
// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

final class UpdateUniverseContextHandler implements IContextHandler {
  use TThriftPoliciedOptOutList;
  use PZ2UniversePropagationTrait;

  public function onIncomingDownstream(
    ThriftContextPropState $mutable_ctx,
    ClientInstrumentationParams $params,
    ImmutableThriftFrameworkMetadataOnResponse $immutable_tfmr,
  ): void {}

  public function onIncomingUpstream(
    ThriftContextPropState $mutable_ctx,
    ServerInstrumentationParams $params,
    ImmutableThriftFrameworkMetadata $immutable_tfm,
  ): void {}

  public function onOutgoingUpstream(
    ServerInstrumentationParams $params,
    ThriftFrameworkMetadataOnResponse $mutable_tfmr,
    ImmutableThriftContextPropState $immutable_ctx,
  ): void {}

  public function onOutgoingDownstream(
    ClientInstrumentationParams $params,
    ThriftFrameworkMetadata $mutable_tfm,
    ImmutableThriftContextPropState $immutable_ctx,
  ): void {
    $service_interface = Shapes::idx($params, 'service_interface');
    $function_name = Shapes::idx($params, 'fn_name');
    if ($service_interface is null || $function_name is null) {
      return;
    }
    $thrift_name = ThriftServiceHelper::extractServiceName($service_interface);

    if (self::isServiceNameOptedOut($thrift_name)) {
      return;
    }

    self::updateContextPropUniverseInThriftFrameworkMetadata(
      $thrift_name,
      $function_name,
      $mutable_tfm,
    );
  }

  private static function updateContextPropUniverseInThriftFrameworkMetadata(
    string $thrift_name,
    string $function_name,
    ThriftFrameworkMetadata $mutable_tfm,
  )[zoned_local]: void {
    if (PrivacyLibKS::isKilled(PLKS::XSU_UNIVERSE_CONTEXT_PROP)) {
      return;
    }

    try {
      $current_universe =
        self::getCurrentUniverse($thrift_name, $function_name);
      // set current universe in TFM
      $current_universe_int = $current_universe?->getValue();
      if (
        $current_universe_int is nonnull &&
        $mutable_tfm->privacyUniverse !== $current_universe_int
      ) {
        $mutable_tfm->privacyUniverse = $current_universe_int;
      }
    } catch (Exception $e) {
      FBLogger('privacylib', 'thrift_propagation_exception')
        ->handle(
          $e,
          causes_the('Universe')->to('not update')
            ->document('fail to update thrift context prop universe'),
        );
    }
  }

  private static function getCurrentUniverse(
    string $thrift_name,
    string $function_name,
  ): ?UniverseDesignator {
    try {
      $xid = ThriftServiceMethodNameAssetXID::unsafeGet(
        $thrift_name,
        $function_name,
      );
      $privacy_lib = ThriftServiceMethodNamePrivacyLib::get($xid);
      $asset_universe = self::getPLArtifactUniverse($privacy_lib);
      if ($asset_universe is nonnull) {
        if ($asset_universe->shouldDynamicallyPropagate()) {
          $current_universe =
            self::getUniverseForPropagation(); // dynamic propagation

          self::logAsyncPropagation(
            $xid,
            Str\format(
              'async_submitter_propagation_nonnull_dynamic_%d',
              $current_universe?->getValue() ?? 0,
            ),
          );
        } else {
          $current_universe =
            $asset_universe->getUniverseDesignator(); // static propagation

          self::logAsyncPropagation(
            $xid,
            Str\format(
              'async_submitter_propagation_nonnull_static_%d',
              $current_universe?->getValue() ?? 0,
            ),
          );
        }
      } else {
        // TODO: all assets should have universe, for now, default to dynamic propagation
        $current_universe =
          self::getUniverseForPropagation(); // dynamic propagation

        self::logAsyncPropagation(
          $xid,
          Str\format(
            'async_submitter_propagation_null_dynamic_%d',
            $current_universe?->getValue() ?? 0,
          ),
        );
      }
      return $current_universe;
    } catch (Exception $e) {
      FBLogger('privacylib', 'thrift_propagation_exception')
        ->handle(
          $e,
          causes_the('Universe')->to('not update')
            ->document('fail to update thrift context prop universe'),
        );
      return null;
    }
  }

  // Temporary function to log information about async XSU propagation from client
  private static function logAsyncPropagation(
    ThriftServiceMethodNameAssetXID $asset_xid,
    string $key,
  ): void {
    if (self::isSignalDynamicLoggerKilled()) {
      return;
    }
    if ($asset_xid->equals(self::getAsyncSubmitterThriftService())) {
      signal_log_in_psp_no_stack(
        SignalDynamicLoggerDataset::PRIVACY_INFRASTRUCTURE,
        SignalDynamicLoggerProject::PRIVACYLIB_WWW,
        $key,
      );
    }
  }

  <<__Memoize>>
  private static function isSignalDynamicLoggerKilled(): bool {
    return PrivacyLibKS::isKilled(PLKS::SIGNAL_DYNAMIC_LOGGER);
  }

  // Temporary hardcoded function to get the Async Thrift service that we want to filter to
  <<__Memoize>>
  private static function getAsyncSubmitterThriftService(
  ): ThriftServiceMethodNameAssetXID {
    return ThriftServiceMethodNameAssetXID::unsafeGet(
      'AsyncTier_AsyncTierSubmitter',
      'scheduleJobs',
    );
  }
}
