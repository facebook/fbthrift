<?hh
/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

namespace test\namespace_from_package_without_module_name;

/**
 * Original thrift service:-
 * TestService
 */
<<\ThriftTypeInfo(shape('uri' => 'test.dev/namespace_from_package_without_module_name/TestService'))>>
interface TestServiceAsyncIf extends \IThriftAsyncIf {
  /**
   * Original thrift definition:-
   * i64
   *   init(1: i64 int1);
   */
  public function init(int $int1): Awaitable<int>;
}

/**
 * Original thrift service:-
 * TestService
 */
<<\ThriftTypeInfo(shape('uri' => 'test.dev/namespace_from_package_without_module_name/TestService'))>>
interface TestServiceIf extends \IThriftSyncIf {
  /**
   * Original thrift definition:-
   * i64
   *   init(1: i64 int1);
   */
  public function init(int $int1): int;
}

/**
 * Original thrift service:-
 * TestService
 */
<<\ThriftTypeInfo(shape('uri' => 'test.dev/namespace_from_package_without_module_name/TestService'))>>
interface TestServiceAsyncClientIf extends TestServiceAsyncIf {
}

/**
 * Original thrift service:-
 * TestService
 */
<<\ThriftTypeInfo(shape('uri' => 'test.dev/namespace_from_package_without_module_name/TestService'))>>
interface TestServiceClientIf extends \IThriftSyncIf {
  /**
   * Original thrift definition:-
   * i64
   *   init(1: i64 int1);
   */
  public function init(int $int1): Awaitable<int>;
}

/**
 * Original thrift service:-
 * TestService
 */
trait TestServiceClientBase {
  require extends \ThriftClientBase;

}

class TestServiceAsyncClient extends \ThriftClientBase implements TestServiceAsyncClientIf {
  use TestServiceClientBase;

  /**
   * Original thrift definition:-
   * i64
   *   init(1: i64 int1);
   */
  public async function init(int $int1): Awaitable<int> {
    $hh_frame_metadata = $this->getHHFrameMetadata();
    if ($hh_frame_metadata !== null) {
      \HH\set_frame_metadata($hh_frame_metadata);
    }
    $rpc_options = $this->getAndResetOptions() ?? \ThriftClientBase::defaultOptions();
    $args = \test\namespace_from_package_without_module_name\TestService_init_args::fromShape(shape(
      'int1' => $int1,
    ));
    await $this->asyncHandler_->genBefore("TestService", "init", $args);
    $currentseqid = $this->sendImplHelper($args, "init", false, "TestService" );
    return await $this->genAwaitResponse(\test\namespace_from_package_without_module_name\TestService_init_result::class, "init", false, $currentseqid, $rpc_options);
  }

}

class TestServiceClient extends \ThriftClientBase implements TestServiceClientIf {
  use TestServiceClientBase;

  /**
   * Original thrift definition:-
   * i64
   *   init(1: i64 int1);
   */
  public async function init(int $int1): Awaitable<int> {
    $hh_frame_metadata = $this->getHHFrameMetadata();
    if ($hh_frame_metadata !== null) {
      \HH\set_frame_metadata($hh_frame_metadata);
    }
    $rpc_options = $this->getAndResetOptions() ?? \ThriftClientBase::defaultOptions();
    $args = \test\namespace_from_package_without_module_name\TestService_init_args::fromShape(shape(
      'int1' => $int1,
    ));
    await $this->asyncHandler_->genBefore("TestService", "init", $args);
    $currentseqid = $this->sendImplHelper($args, "init", false, "TestService" );
    return await $this->genAwaitResponse(\test\namespace_from_package_without_module_name\TestService_init_result::class, "init", false, $currentseqid, $rpc_options);
  }

  /* send and recv functions */
  public function send_init(int $int1): int {
    $args = \test\namespace_from_package_without_module_name\TestService_init_args::fromShape(shape(
      'int1' => $int1,
    ));
    return $this->sendImplHelper($args, "init", false, "TestService" );
  }
  public function recv_init(?int $expectedsequenceid = null): int {
    return $this->recvImplHelper(\test\namespace_from_package_without_module_name\TestService_init_result::class, "init", false, $expectedsequenceid);
  }
}

abstract class TestServiceAsyncProcessorBase extends \ThriftAsyncProcessor {
  use \GetThriftServiceMetadata;
  abstract const type TThriftIf as TestServiceAsyncIf;
  const classname<\IThriftServiceStaticMetadata> SERVICE_METADATA_CLASS = TestServiceStaticMetadata::class;
  const string THRIFT_SVC_NAME = 'TestService';

  protected async function process_init(int $seqid, \TProtocol $input, \TProtocol $output): Awaitable<void> {
    $handler_ctx = $this->eventHandler_->getHandlerContext('init');
    $reply_type = \TMessageType::REPLY;
    $args = $this->readHelper(\test\namespace_from_package_without_module_name\TestService_init_args::class, $input, 'init', $handler_ctx);
    $result = \test\namespace_from_package_without_module_name\TestService_init_result::withDefaultValues();
    try {
      $this->eventHandler_->preExec($handler_ctx, '\test\namespace_from_package_without_module_name\TestService', 'init', $args);
      $result->success = await $this->handler->init($args->int1);
      $this->eventHandler_->postExec($handler_ctx, 'init', $result);
    } catch (\Exception $ex) {
      $reply_type = \TMessageType::EXCEPTION;
      $this->eventHandler_->handlerError($handler_ctx, 'init', $ex);
      $result = new \TApplicationException($ex->getMessage()."\n".$ex->getTraceAsString());
    }
    $this->writeHelper($result, 'init', $seqid, $handler_ctx, $output, $reply_type);
  }
  protected async function process_getThriftServiceMetadata(int $seqid, \TProtocol $input, \TProtocol $output): Awaitable<void> {
    $this->process_getThriftServiceMetadataHelper($seqid, $input, $output, TestServiceStaticMetadata::class);
  }
}
class TestServiceAsyncProcessor extends TestServiceAsyncProcessorBase {
  const type TThriftIf = TestServiceAsyncIf;
}

abstract class TestServiceSyncProcessorBase extends \ThriftSyncProcessor {
  use \GetThriftServiceMetadata;
  abstract const type TThriftIf as TestServiceIf;
  const classname<\IThriftServiceStaticMetadata> SERVICE_METADATA_CLASS = TestServiceStaticMetadata::class;
  const string THRIFT_SVC_NAME = 'TestService';

  protected function process_init(int $seqid, \TProtocol $input, \TProtocol $output): void {
    $handler_ctx = $this->eventHandler_->getHandlerContext('init');
    $reply_type = \TMessageType::REPLY;
    $args = $this->readHelper(\test\namespace_from_package_without_module_name\TestService_init_args::class, $input, 'init', $handler_ctx);
    $result = \test\namespace_from_package_without_module_name\TestService_init_result::withDefaultValues();
    try {
      $this->eventHandler_->preExec($handler_ctx, '\test\namespace_from_package_without_module_name\TestService', 'init', $args);
      $result->success = $this->handler->init($args->int1);
      $this->eventHandler_->postExec($handler_ctx, 'init', $result);
    } catch (\Exception $ex) {
      $reply_type = \TMessageType::EXCEPTION;
      $this->eventHandler_->handlerError($handler_ctx, 'init', $ex);
      $result = new \TApplicationException($ex->getMessage()."\n".$ex->getTraceAsString());
    }
    $this->writeHelper($result, 'init', $seqid, $handler_ctx, $output, $reply_type);
  }
  protected function process_getThriftServiceMetadata(int $seqid, \TProtocol $input, \TProtocol $output): void {
    $this->process_getThriftServiceMetadataHelper($seqid, $input, $output, TestServiceStaticMetadata::class);
  }
}
class TestServiceSyncProcessor extends TestServiceSyncProcessorBase {
  const type TThriftIf = TestServiceIf;
}
// For backwards compatibility
class TestServiceProcessor extends TestServiceSyncProcessor {}

// HELPER FUNCTIONS AND STRUCTURES

class TestService_init_args implements \IThriftSyncStruct, \IThriftStructMetadata, \IThriftShapishSyncStruct {
  use \ThriftSerializationTrait;

  const \ThriftStructTypes::TSpec SPEC = dict[
    1 => shape(
      'var' => 'int1',
      'type' => \TType::I64,
    ),
  ];
  const dict<string, int> FIELDMAP = dict[
    'int1' => 1,
  ];

  const type TConstructorShape = shape(
    ?'int1' => ?int,
  );

  const type TShape = shape(
    'int1' => int,
  );
  const int STRUCTURAL_ID = 975124300794717332;
  public int $int1;

  public function __construct(?int $int1 = null)[] {
    $this->int1 = $int1 ?? 0;
  }

  public static function withDefaultValues()[]: this {
    return new static();
  }

  public static function fromShape(self::TConstructorShape $shape)[]: this {
    return new static(
      Shapes::idx($shape, 'int1'),
    );
  }

  public function getName()[]: string {
    return 'TestService_init_args';
  }

  public static function getStructMetadata()[]: \tmeta_ThriftStruct {
    return \tmeta_ThriftStruct::fromShape(
      shape(
        "name" => "module.init_args",
        "fields" => vec[
          \tmeta_ThriftField::fromShape(
            shape(
              "id" => 1,
              "type" => \tmeta_ThriftType::fromShape(
                shape(
                  "t_primitive" => \tmeta_ThriftPrimitiveType::THRIFT_I64_TYPE,
                )
              ),
              "name" => "int1",
            )
          ),
        ],
        "is_union" => false,
      )
    );
  }

  public static function getAllStructuredAnnotations()[write_props]: \TStructAnnotations {
    return shape(
      'struct' => dict[],
      'fields' => dict[
      ],
    );
  }

  public static function __fromShape(self::TShape $shape)[]: this {
    return new static(
      $shape['int1'],
    );
  }

  public function __toShape()[]: self::TShape {
    return shape(
      'int1' => $this->int1,
    );
  }
  public function getInstanceKey()[write_props]: string {
    return \TCompactSerializer::serialize($this);
  }

  public function readFromJson(string $jsonText): void {
    $parsed = json_decode($jsonText, true);

    if ($parsed === null || !($parsed is KeyedContainer<_, _>)) {
      throw new \TProtocolException("Cannot parse the given json string.");
    }

    if (idx($parsed, 'int1') !== null) {
      $this->int1 = HH\FIXME\UNSAFE_CAST<mixed, int>($parsed['int1']);
    }
  }

}

class TestService_init_result extends \ThriftSyncStructWithResult implements \IThriftStructMetadata {
  use \ThriftSerializationTrait;

  const type TResult = int;

  const \ThriftStructTypes::TSpec SPEC = dict[
    0 => shape(
      'var' => 'success',
      'type' => \TType::I64,
    ),
  ];
  const dict<string, int> FIELDMAP = dict[
    'success' => 0,
  ];

  const type TConstructorShape = shape(
    ?'success' => ?this::TResult,
  );

  const int STRUCTURAL_ID = 5548670328188446575;
  public ?this::TResult $success;

  public function __construct(?this::TResult $success = null)[] {
    $this->success = $success;
  }

  public static function withDefaultValues()[]: this {
    return new static();
  }

  public static function fromShape(self::TConstructorShape $shape)[]: this {
    return new static(
      Shapes::idx($shape, 'success'),
    );
  }

  public function getName()[]: string {
    return 'TestService_init_result';
  }

  public static function getStructMetadata()[]: \tmeta_ThriftStruct {
    return \tmeta_ThriftStruct::fromShape(
      shape(
        "name" => "module.TestService_init_result",
        "fields" => vec[
          \tmeta_ThriftField::fromShape(
            shape(
              "id" => 0,
              "type" => \tmeta_ThriftType::fromShape(
                shape(
                  "t_primitive" => \tmeta_ThriftPrimitiveType::THRIFT_I64_TYPE,
                )
              ),
              "name" => "success",
            )
          ),
        ],
        "is_union" => false,
      )
    );
  }

  public static function getAllStructuredAnnotations()[write_props]: \TStructAnnotations {
    return shape(
      'struct' => dict[],
      'fields' => dict[
      ],
    );
  }

  public function getInstanceKey()[write_props]: string {
    return \TCompactSerializer::serialize($this);
  }

  public function readFromJson(string $jsonText): void {
    $parsed = json_decode($jsonText, true);

    if ($parsed === null || !($parsed is KeyedContainer<_, _>)) {
      throw new \TProtocolException("Cannot parse the given json string.");
    }

    if (idx($parsed, 'success') !== null) {
      $this->success = HH\FIXME\UNSAFE_CAST<mixed, int>($parsed['success']);
    }
  }

}

class TestServiceStaticMetadata implements \IThriftServiceStaticMetadata {
  public static function getServiceMetadata()[]: \tmeta_ThriftService {
    return \tmeta_ThriftService::fromShape(
      shape(
        "name" => "module.TestService",
        "functions" => vec[
          \tmeta_ThriftFunction::fromShape(
            shape(
              "name" => "init",
              "return_type" => \tmeta_ThriftType::fromShape(
                shape(
                  "t_primitive" => \tmeta_ThriftPrimitiveType::THRIFT_I64_TYPE,
                )
              ),
              "arguments" => vec[
                \tmeta_ThriftField::fromShape(
                  shape(
                    "id" => 1,
                    "type" => \tmeta_ThriftType::fromShape(
                      shape(
                        "t_primitive" => \tmeta_ThriftPrimitiveType::THRIFT_I64_TYPE,
                      )
                    ),
                    "name" => "int1",
                  )
                ),
              ],
            )
          ),
        ],
      )
    );
  }

  public static function getServiceMetadataResponse()[]: \tmeta_ThriftServiceMetadataResponse {
    return \tmeta_ThriftServiceMetadataResponse::fromShape(
      shape(
        'context' => \tmeta_ThriftServiceContext::fromShape(
          shape(
            'service_info' => self::getServiceMetadata(),
            'module' => \tmeta_ThriftModuleContext::fromShape(
              shape(
                'name' => 'module',
              )
            ),
          )
        ),
        'metadata' => \tmeta_ThriftMetadata::fromShape(
          shape(
            'enums' => dict[
            ],
            'structs' => dict[
            ],
            'exceptions' => dict[
            ],
            'services' => dict[
            ],
          )
        ),
      )
    );
  }

  public static function getAllStructuredAnnotations()[write_props]: \TServiceAnnotations {
    return shape(
      'service' => dict[],
      'functions' => dict[
      ],
    );
  }
}

