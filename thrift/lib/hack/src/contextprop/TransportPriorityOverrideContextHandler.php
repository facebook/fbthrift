<?hh
// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * Writes the RequestPriorityTransportScope priority onto the per-call TFM
 * copy, overwriting whatever was copied from ThriftContextPropState.
 */
<<Oncalls('flow_control')>>
final class TransportPriorityOverrideContextHandler implements IContextHandler {

  public function onIncomingDownstream(
    ThriftContextPropState $mutable_ctx,
    ClientInstrumentationParams $params,
    ImmutableThriftFrameworkMetadataOnResponse $immutable_tfmr,
  ): void {}

  public function onOutgoingDownstream(
    ClientInstrumentationParams $params,
    ThriftFrameworkMetadata $mutable_tfm,
    ImmutableThriftContextPropState $immutable_ctx,
  ): void {
    $priority = MCPContext::getTransportPriorityOverride();
    if ($priority is nonnull) {
      $mutable_tfm->request_priority = $priority;
    }
  }

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
}
