<?hh
// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * Propagates an explicitly tagged Bronze (SHEDDABLE_PLUS) or Sheddable
 * (SHEDDABLE) priority from implicit context onto the outgoing TFM for
 * allowlisted destinations. Other priorities are never written, so untagged
 * traffic keeps sending a null priority.
 */
final class RequestPriorityContextHandler implements IContextHandler {

  const keyset<RequestPriority> PROPAGATED_PRIORITIES = keyset[
    RequestPriority::SHEDDABLE_PLUS,
    RequestPriority::SHEDDABLE,
  ];

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
    // Deliberately no fallback to PrivacyLibRequestPriorityState: it is
    // request-global and last-writer-wins across concurrent branches, so code
    // outside any implicit-context scope could pick up another branch's Bronze.
    $priority = MCPContext::getRequestPriority();
    if (
      $priority is null ||
      !C\contains_key(self::PROPAGATED_PRIORITIES, $priority)
    ) {
      return;
    }
    $service_name = Shapes::idx($params, 'service_name');
    if (
      $service_name is null ||
      !JustKnobs::evalString(
        'www/web_efficiency_infra:pl_set_priority_to_tcp',
        $immutable_ctx->getRequestId(),
        $service_name,
      )
    ) {
      return;
    }
    // A higher value is more sheddable; never upgrade an upstream priority.
    $upstream = $mutable_tfm->request_priority;
    if ($upstream is null || ($priority as int) > ($upstream as int)) {
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
