<?hh
// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

<<Oncalls('thrift_hack')>>
final class ThriftEventHandlerMetadataTest extends WWWTest {
  public async function testMetadataBackwardCompatibility(): Awaitable<void> {
    $handler = mock(ExampleRootServiceAsyncIf::class);
    $proc = new ExampleRootServiceAsyncProcessor($handler);

    // The event handler will assert that the received metadata matches
    // the expected metadata provided here
    $event_handler =
      new VerifyNameEventHandler('example_ExampleRootService', 'doNothing');
    $proc->setEventHandler($event_handler);

    // Make the RPC call, which will invoke the event handler
    $input = new TCompactProtocol(new TMemoryBuffer());
    $input->writeRPCMessage(
      'doNothing',
      TMessageType::CALL,
      example_ExampleRootService_doNothing_args::withDefaultValues(),
      0,
    );
    $_ = await $proc->processAsync(
      $input,
      new TCompactProtocol(new TMemoryBuffer()),
      'doNothing',
    );
  }
}

final class VerifyNameEventHandler extends TProcessorEventHandler {
  public function __construct(
    private string $expectedServiceName,
    private string $expectedMethodName,
  ) {
  }

  <<__Override>>
  public function preExec(
    mixed $handler_context,
    string $service_name,
    string $method_name,
    mixed $args,
  ): void {
    expect($service_name)->toEqual($this->expectedServiceName);
    expect($method_name)->toEqual($this->expectedMethodName);
  }
}
