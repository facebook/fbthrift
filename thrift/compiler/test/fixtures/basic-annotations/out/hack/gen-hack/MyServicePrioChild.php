<?hh
/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

/**
 * Original thrift service:-
 * MyServicePrioChild
 */
interface MyServicePrioChildAsyncIf extends MyServicePrioParentAsyncIf {
  /**
   * Original thrift definition:-
   * void
   *   pang();
   */
  public function pang(): Awaitable<void>;
}

/**
 * Original thrift service:-
 * MyServicePrioChild
 */
interface MyServicePrioChildIf extends MyServicePrioParentIf {
  /**
   * Original thrift definition:-
   * void
   *   pang();
   */
  public function pang(): void;
}

/**
 * Original thrift service:-
 * MyServicePrioChild
 */
interface MyServicePrioChildAsyncClientIf extends MyServicePrioChildAsyncIf, MyServicePrioParentAsyncClientIf {
}

/**
 * Original thrift service:-
 * MyServicePrioChild
 */
interface MyServicePrioChildClientIf extends MyServicePrioParentClientIf {
  /**
   * Original thrift definition:-
   * void
   *   pang();
   */
  public function pang(): Awaitable<void>;
}

/**
 * Original thrift service:-
 * MyServicePrioChild
 */
trait MyServicePrioChildClientBase {
  require extends \ThriftClientBase;

}

class MyServicePrioChildAsyncClient extends MyServicePrioParentAsyncClient implements MyServicePrioChildAsyncClientIf {
  use MyServicePrioChildClientBase;

  /**
   * Original thrift definition:-
   * void
   *   pang();
   */
  public async function pang(): Awaitable<void> {
    $hh_frame_metadata = $this->getHHFrameMetadata();
    if ($hh_frame_metadata !== null) {
      \HH\set_frame_metadata($hh_frame_metadata);
    }
    $rpc_options = $this->getAndResetOptions() ?? \ThriftClientBase::defaultOptions();
    $args = MyServicePrioChild_pang_args::withDefaultValues();
    await $this->asyncHandler_->genBefore("MyServicePrioChild", "pang", $args);
    $currentseqid = $this->sendImplHelper($args, "pang", false);
    await $this->genAwaitResponse(MyServicePrioChild_pang_result::class, "pang", true, $currentseqid, $rpc_options);
  }

}

class MyServicePrioChildClient extends MyServicePrioParentClient implements MyServicePrioChildClientIf {
  use MyServicePrioChildClientBase;

  /**
   * Original thrift definition:-
   * void
   *   pang();
   */
  public async function pang(): Awaitable<void> {
    $hh_frame_metadata = $this->getHHFrameMetadata();
    if ($hh_frame_metadata !== null) {
      \HH\set_frame_metadata($hh_frame_metadata);
    }
    $rpc_options = $this->getAndResetOptions() ?? \ThriftClientBase::defaultOptions();
    $args = MyServicePrioChild_pang_args::withDefaultValues();
    await $this->asyncHandler_->genBefore("MyServicePrioChild", "pang", $args);
    $currentseqid = $this->sendImplHelper($args, "pang", false);
    await $this->genAwaitResponse(MyServicePrioChild_pang_result::class, "pang", true, $currentseqid, $rpc_options);
  }

  /* send and recv functions */
  public function send_pang(): int {
    $args = MyServicePrioChild_pang_args::withDefaultValues();
    return $this->sendImplHelper($args, "pang", false);
  }
  public function recv_pang(?int $expectedsequenceid = null): void {
    $this->recvImplHelper(MyServicePrioChild_pang_result::class, "pang", true, $expectedsequenceid);
  }
}

abstract class MyServicePrioChildAsyncProcessorBase extends MyServicePrioParentAsyncProcessorBase {
  use \GetThriftServiceMetadata;
  abstract const type TThriftIf as MyServicePrioChildAsyncIf;
  const classname<\IThriftServiceStaticMetadata> SERVICE_METADATA_CLASS = MyServicePrioChildStaticMetadata::class;
  const string THRIFT_SVC_NAME = 'MyServicePrioChild';

  protected async function process_pang(int $seqid, \TProtocol $input, \TProtocol $output): Awaitable<void> {
    $handler_ctx = $this->eventHandler_->getHandlerContext('pang');
    $reply_type = \TMessageType::REPLY;

    $this->eventHandler_->preRead($handler_ctx, 'pang', dict[]);

    if ($input is \TBinaryProtocolAccelerated) {
      $args = \thrift_protocol_read_binary_struct($input, 'MyServicePrioChild_pang_args');
    } else if ($input is \TCompactProtocolAccelerated) {
      $args = \thrift_protocol_read_compact_struct($input, 'MyServicePrioChild_pang_args');
    } else {
      $args = MyServicePrioChild_pang_args::withDefaultValues();
      $args->read($input);
    }
    $input->readMessageEnd();
    $this->eventHandler_->postRead($handler_ctx, 'pang', $args);
    $result = MyServicePrioChild_pang_result::withDefaultValues();
    try {
      $this->eventHandler_->preExec($handler_ctx, 'MyServicePrioChild', 'pang', $args);
      await $this->handler->pang();
      $this->eventHandler_->postExec($handler_ctx, 'pang', $result);
    } catch (\Exception $ex) {
      $reply_type = \TMessageType::EXCEPTION;
      $this->eventHandler_->handlerError($handler_ctx, 'pang', $ex);
      $result = new \TApplicationException($ex->getMessage()."\n".$ex->getTraceAsString());
    }
    $this->eventHandler_->preWrite($handler_ctx, 'pang', $result);
    if ($output is \TBinaryProtocolAccelerated)
    {
      \thrift_protocol_write_binary($output, 'pang', $reply_type, $result, $seqid, $output->isStrictWrite());
    }
    else if ($output is \TCompactProtocolAccelerated)
    {
      \thrift_protocol_write_compact2($output, 'pang', $reply_type, $result, $seqid, false, \TCompactProtocolBase::VERSION);
    }
    else
    {
      $output->writeMessageBegin("pang", $reply_type, $seqid);
      $result->write($output);
      $output->writeMessageEnd();
      $output->getTransport()->flush();
    }
    $this->eventHandler_->postWrite($handler_ctx, 'pang', $result);
  }
  protected async function process_getThriftServiceMetadata(int $seqid, \TProtocol $input, \TProtocol $output): Awaitable<void> {
    $this->process_getThriftServiceMetadataHelper($seqid, $input, $output, MyServicePrioChildStaticMetadata::class);
  }
}
class MyServicePrioChildAsyncProcessor extends MyServicePrioChildAsyncProcessorBase {
  const type TThriftIf = MyServicePrioChildAsyncIf;
}

abstract class MyServicePrioChildSyncProcessorBase extends MyServicePrioParentSyncProcessorBase {
  use \GetThriftServiceMetadata;
  abstract const type TThriftIf as MyServicePrioChildIf;
  const classname<\IThriftServiceStaticMetadata> SERVICE_METADATA_CLASS = MyServicePrioChildStaticMetadata::class;
  const string THRIFT_SVC_NAME = 'MyServicePrioChild';

  protected function process_pang(int $seqid, \TProtocol $input, \TProtocol $output): void {
    $handler_ctx = $this->eventHandler_->getHandlerContext('pang');
    $reply_type = \TMessageType::REPLY;

    $this->eventHandler_->preRead($handler_ctx, 'pang', dict[]);

    if ($input is \TBinaryProtocolAccelerated) {
      $args = \thrift_protocol_read_binary_struct($input, 'MyServicePrioChild_pang_args');
    } else if ($input is \TCompactProtocolAccelerated) {
      $args = \thrift_protocol_read_compact_struct($input, 'MyServicePrioChild_pang_args');
    } else {
      $args = MyServicePrioChild_pang_args::withDefaultValues();
      $args->read($input);
    }
    $input->readMessageEnd();
    $this->eventHandler_->postRead($handler_ctx, 'pang', $args);
    $result = MyServicePrioChild_pang_result::withDefaultValues();
    try {
      $this->eventHandler_->preExec($handler_ctx, 'MyServicePrioChild', 'pang', $args);
      $this->handler->pang();
      $this->eventHandler_->postExec($handler_ctx, 'pang', $result);
    } catch (\Exception $ex) {
      $reply_type = \TMessageType::EXCEPTION;
      $this->eventHandler_->handlerError($handler_ctx, 'pang', $ex);
      $result = new \TApplicationException($ex->getMessage()."\n".$ex->getTraceAsString());
    }
    $this->eventHandler_->preWrite($handler_ctx, 'pang', $result);
    if ($output is \TBinaryProtocolAccelerated)
    {
      \thrift_protocol_write_binary($output, 'pang', $reply_type, $result, $seqid, $output->isStrictWrite());
    }
    else if ($output is \TCompactProtocolAccelerated)
    {
      \thrift_protocol_write_compact2($output, 'pang', $reply_type, $result, $seqid, false, \TCompactProtocolBase::VERSION);
    }
    else
    {
      $output->writeMessageBegin("pang", $reply_type, $seqid);
      $result->write($output);
      $output->writeMessageEnd();
      $output->getTransport()->flush();
    }
    $this->eventHandler_->postWrite($handler_ctx, 'pang', $result);
  }
  protected function process_getThriftServiceMetadata(int $seqid, \TProtocol $input, \TProtocol $output): void {
    $this->process_getThriftServiceMetadataHelper($seqid, $input, $output, MyServicePrioChildStaticMetadata::class);
  }
}
class MyServicePrioChildSyncProcessor extends MyServicePrioChildSyncProcessorBase {
  const type TThriftIf = MyServicePrioChildIf;
}
// For backwards compatibility
class MyServicePrioChildProcessor extends MyServicePrioChildSyncProcessor {}

// HELPER FUNCTIONS AND STRUCTURES

class MyServicePrioChild_pang_args implements \IThriftSyncStruct, \IThriftStructMetadata, \IThriftShapishSyncStruct {
  use \ThriftSerializationTrait;

  const \ThriftStructTypes::TSpec SPEC = dict[
  ];
  const dict<string, int> FIELDMAP = dict[
  ];

  const type TConstructorShape = shape(
  );

  const type TShape = shape(
    ...
  );
  const int STRUCTURAL_ID = 957977401221134810;

  public function __construct()[] {
  }

  public static function withDefaultValues()[]: this {
    return new static();
  }

  public static function fromShape(self::TConstructorShape $shape)[]: this {
    return new static(
    );
  }

  public function getName()[]: string {
    return 'MyServicePrioChild_pang_args';
  }

  public static function getStructMetadata()[]: \tmeta_ThriftStruct {
    return tmeta_ThriftStruct::fromShape(
      shape(
        "name" => "module.pang_args",
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
    );
  }

  public function __toShape()[]: self::TShape {
    return shape(
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

  }

}

class MyServicePrioChild_pang_result extends \ThriftSyncStructWithoutResult implements \IThriftStructMetadata {
  use \ThriftSerializationTrait;

  const \ThriftStructTypes::TSpec SPEC = dict[
  ];
  const dict<string, int> FIELDMAP = dict[
  ];

  const type TConstructorShape = shape(
  );

  const int STRUCTURAL_ID = 957977401221134810;

  public function __construct()[] {
  }

  public static function withDefaultValues()[]: this {
    return new static();
  }

  public static function fromShape(self::TConstructorShape $shape)[]: this {
    return new static(
    );
  }

  public function getName()[]: string {
    return 'MyServicePrioChild_pang_result';
  }

  public static function getStructMetadata()[]: \tmeta_ThriftStruct {
    return tmeta_ThriftStruct::fromShape(
      shape(
        "name" => "module.MyServicePrioChild_pang_result",
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

  }

}

class MyServicePrioChildStaticMetadata implements \IThriftServiceStaticMetadata {
  public static function getServiceMetadata()[]: \tmeta_ThriftService {
    return tmeta_ThriftService::fromShape(
      shape(
        "name" => "module.MyServicePrioChild",
        "functions" => vec[
          tmeta_ThriftFunction::fromShape(
            shape(
              "name" => "pang",
              "return_type" => tmeta_ThriftType::fromShape(
                shape(
                  "t_primitive" => tmeta_ThriftPrimitiveType::THRIFT_VOID_TYPE,
                )
              ),
            )
          ),
        ],
        "parent" => "module.MyServicePrioParent",
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
              'module.MyServicePrioParent' => MyServicePrioParentStaticMetadata::getServiceMetadata(),
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
        'pang' => dict[
          '\facebook\thrift\annotation\Priority' => \facebook\thrift\annotation\Priority::fromShape(
            shape(
              "level" => \facebook\thrift\annotation\RpcPriority::BEST_EFFORT,
            )
          ),
        ],
      ],
    );
  }
}

