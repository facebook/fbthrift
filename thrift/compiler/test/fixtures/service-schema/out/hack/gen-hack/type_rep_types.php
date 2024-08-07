<?hh
/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

enum apache_thrift_type_rep_ProtocolUnionEnum: int {
  _EMPTY_ = 0;
  standard = 1;
  custom = 2;
  id = 3;
}

/**
 * A union representation of a protocol.
 *
 * Original thrift union:-
 * ProtocolUnion
 */
<<\ThriftTypeInfo(shape('uri' => 'facebook.com/thrift/type/Protocol'))>>
class apache_thrift_type_rep_ProtocolUnion implements \IThriftSyncStruct, \IThriftStructMetadata, \IThriftUnion<apache_thrift_type_rep_ProtocolUnionEnum> {
  use \ThriftUnionSerializationTrait;

  const \ThriftStructTypes::TSpec SPEC = dict[
    1 => shape(
      'var' => 'standard',
      'union' => true,
      'type' => \TType::I32,
      'enum' => apache_thrift_type_standard_StandardProtocol::class,
    ),
    2 => shape(
      'var' => 'custom',
      'union' => true,
      'type' => \TType::STRING,
    ),
    3 => shape(
      'var' => 'id',
      'union' => true,
      'type' => \TType::I64,
    ),
  ];
  const dict<string, int> FIELDMAP = dict[
    'standard' => 1,
    'custom' => 2,
    'id' => 3,
  ];

  const type TConstructorShape = shape(
    ?'standard' => ?apache_thrift_type_standard_StandardProtocol,
    ?'custom' => ?string,
    ?'id' => ?int,
  );

  const int STRUCTURAL_ID = 9023300838152729930;
  /**
   * A standard protocol, known by all Thrift implementations.
   * 
   * Original thrift field:-
   * 1: standard.StandardProtocol standard
   */
  public ?apache_thrift_type_standard_StandardProtocol $standard;
  /**
   * A custom protocol.
   * 
   * Original thrift field:-
   * 2: string custom
   */
  public ?string $custom;
  /**
   * An externally stored protocol.
   * 
   * Original thrift field:-
   * 3: id.ProtocolId id
   */
  public ?int $id;
  protected apache_thrift_type_rep_ProtocolUnionEnum $_type = apache_thrift_type_rep_ProtocolUnionEnum::_EMPTY_;

  public function __construct(?apache_thrift_type_standard_StandardProtocol $standard = null, ?string $custom = null, ?int $id = null)[] {
    $this->_type = apache_thrift_type_rep_ProtocolUnionEnum::_EMPTY_;
    if ($standard !== null) {
      $this->standard = $standard;
      $this->_type = apache_thrift_type_rep_ProtocolUnionEnum::standard;
    }
    if ($custom !== null) {
      $this->custom = $custom;
      $this->_type = apache_thrift_type_rep_ProtocolUnionEnum::custom;
    }
    if ($id !== null) {
      $this->id = $id;
      $this->_type = apache_thrift_type_rep_ProtocolUnionEnum::id;
    }
  }

  public static function withDefaultValues()[]: this {
    return new static();
  }

  public static function fromShape(self::TConstructorShape $shape)[]: this {
    return new static(
      Shapes::idx($shape, 'standard'),
      Shapes::idx($shape, 'custom'),
      Shapes::idx($shape, 'id'),
    );
  }

  public function getName()[]: string {
    return 'ProtocolUnion';
  }

  public function getType()[]: apache_thrift_type_rep_ProtocolUnionEnum {
    return $this->_type;
  }

  public function reset()[write_props]: void {
    switch ($this->_type) {
      case apache_thrift_type_rep_ProtocolUnionEnum::standard:
        $this->standard = null;
        break;
      case apache_thrift_type_rep_ProtocolUnionEnum::custom:
        $this->custom = null;
        break;
      case apache_thrift_type_rep_ProtocolUnionEnum::id:
        $this->id = null;
        break;
      case apache_thrift_type_rep_ProtocolUnionEnum::_EMPTY_:
        break;
    }
    $this->_type = apache_thrift_type_rep_ProtocolUnionEnum::_EMPTY_;
  }

  public function set_standard(apache_thrift_type_standard_StandardProtocol $standard)[write_props]: this {
    $this->reset();
    $this->_type = apache_thrift_type_rep_ProtocolUnionEnum::standard;
    $this->standard = $standard;
    return $this;
  }

  public function get_standard()[]: ?apache_thrift_type_standard_StandardProtocol {
    return $this->standard;
  }

  public function getx_standard()[]: apache_thrift_type_standard_StandardProtocol {
    invariant(
      $this->_type === apache_thrift_type_rep_ProtocolUnionEnum::standard,
      'get_standard called on an instance of ProtocolUnion whose current type is %s',
      (string)$this->_type,
    );
    return $this->standard as nonnull;
  }

  public function set_custom(string $custom)[write_props]: this {
    $this->reset();
    $this->_type = apache_thrift_type_rep_ProtocolUnionEnum::custom;
    $this->custom = $custom;
    return $this;
  }

  public function get_custom()[]: ?string {
    return $this->custom;
  }

  public function getx_custom()[]: string {
    invariant(
      $this->_type === apache_thrift_type_rep_ProtocolUnionEnum::custom,
      'get_custom called on an instance of ProtocolUnion whose current type is %s',
      (string)$this->_type,
    );
    return $this->custom as nonnull;
  }

  public function set_id(int $id)[write_props]: this {
    $this->reset();
    $this->_type = apache_thrift_type_rep_ProtocolUnionEnum::id;
    $this->id = $id;
    return $this;
  }

  public function get_id()[]: ?int {
    return $this->id;
  }

  public function getx_id()[]: int {
    invariant(
      $this->_type === apache_thrift_type_rep_ProtocolUnionEnum::id,
      'get_id called on an instance of ProtocolUnion whose current type is %s',
      (string)$this->_type,
    );
    return $this->id as nonnull;
  }

  public static function getStructMetadata()[]: \tmeta_ThriftStruct {
    return tmeta_ThriftStruct::fromShape(
      shape(
        "name" => "type_rep.ProtocolUnion",
        "fields" => vec[
          tmeta_ThriftField::fromShape(
            shape(
              "id" => 1,
              "type" => tmeta_ThriftType::fromShape(
                shape(
                  "t_enum" => tmeta_ThriftEnumType::fromShape(
                    shape(
                      "name" => "standard.StandardProtocol",
                    )
                  ),
                )
              ),
              "name" => "standard",
            )
          ),
          tmeta_ThriftField::fromShape(
            shape(
              "id" => 2,
              "type" => tmeta_ThriftType::fromShape(
                shape(
                  "t_primitive" => tmeta_ThriftPrimitiveType::THRIFT_STRING_TYPE,
                )
              ),
              "name" => "custom",
            )
          ),
          tmeta_ThriftField::fromShape(
            shape(
              "id" => 3,
              "type" => tmeta_ThriftType::fromShape(
                shape(
                  "t_typedef" => tmeta_ThriftTypedefType::fromShape(
                    shape(
                      "name" => "id.ProtocolId",
                      "underlyingType" => tmeta_ThriftType::fromShape(
                        shape(
                          "t_typedef" => tmeta_ThriftTypedefType::fromShape(
                            shape(
                              "name" => "id.ExternId",
                              "underlyingType" => tmeta_ThriftType::fromShape(
                                shape(
                                  "t_primitive" => tmeta_ThriftPrimitiveType::THRIFT_I64_TYPE,
                                )
                              ),
                            )
                          ),
                        )
                      ),
                    )
                  ),
                )
              ),
              "name" => "id",
            )
          ),
        ],
        "is_union" => true,
      )
    );
  }

  public static function getAllStructuredAnnotations()[write_props]: \TStructAnnotations {
    return shape(
      'struct' => dict[],
      'fields' => dict[
        'id' => shape(
          'field' => dict[
            '\facebook\thrift\annotation\python\Py3Hidden' => \facebook\thrift\annotation\python\Py3Hidden::fromShape(
              shape(
              )
            ),
          ],
          'type' => dict[
            '\facebook\thrift\annotation\cpp\Adapter' => \facebook\thrift\annotation\cpp\Adapter::fromShape(
              shape(
                "name" => "::apache::thrift::type::detail::StrongIntegerAdapter<::apache::thrift::type::ProtocolId>",
              )
            ),
          ],
        ),
      ],
    );
  }

  public function getInstanceKey()[write_props]: string {
    return \TCompactSerializer::serialize($this);
  }

}

/**
 * A concrete Thrift type.
 *
 * Original thrift struct:-
 * TypeStruct
 */
<<\ThriftTypeInfo(shape('uri' => 'facebook.com/thrift/type/Type'))>>
class apache_thrift_type_rep_TypeStruct implements \IThriftSyncStruct, \IThriftStructMetadata {
  use \ThriftSerializationTrait;

  const \ThriftStructTypes::TSpec SPEC = dict[
    1 => shape(
      'var' => 'name',
      'type' => \TType::STRUCT,
      'class' => apache_thrift_type_standard_TypeName::class,
    ),
    2 => shape(
      'var' => 'params',
      'type' => \TType::LST,
      'etype' => \TType::STRUCT,
      'elem' => shape(
        'type' => \TType::STRUCT,
        'class' => apache_thrift_type_rep_TypeStruct::class,
      ),
      'format' => 'collection',
    ),
  ];
  const dict<string, int> FIELDMAP = dict[
    'name' => 1,
    'params' => 2,
  ];

  const type TConstructorShape = shape(
    ?'name' => ?apache_thrift_type_standard_TypeName,
    ?'params' => ?Vector<apache_thrift_type_rep_TypeStruct>,
  );

  const int STRUCTURAL_ID = 5995936969521999575;
  /**
   * The type name.
   * 
   * Original thrift field:-
   * 1: standard.TypeName name
   */
  public ?apache_thrift_type_standard_TypeName $name;
  /**
   * The type params, if appropriate.
   * 
   * Original thrift field:-
   * 2: list<type_rep.TypeStruct> params
   */
  public Vector<apache_thrift_type_rep_TypeStruct> $params;

  public function __construct(?apache_thrift_type_standard_TypeName $name = null, ?Vector<apache_thrift_type_rep_TypeStruct> $params = null)[] {
    $this->name = $name;
    $this->params = $params ?? Vector {};
  }

  public static function withDefaultValues()[]: this {
    return new static();
  }

  public static function fromShape(self::TConstructorShape $shape)[]: this {
    return new static(
      Shapes::idx($shape, 'name'),
      Shapes::idx($shape, 'params'),
    );
  }

  public function getName()[]: string {
    return 'TypeStruct';
  }

  public static function getStructMetadata()[]: \tmeta_ThriftStruct {
    return tmeta_ThriftStruct::fromShape(
      shape(
        "name" => "type_rep.TypeStruct",
        "fields" => vec[
          tmeta_ThriftField::fromShape(
            shape(
              "id" => 1,
              "type" => tmeta_ThriftType::fromShape(
                shape(
                  "t_struct" => tmeta_ThriftStructType::fromShape(
                    shape(
                      "name" => "standard.TypeName",
                    )
                  ),
                )
              ),
              "name" => "name",
            )
          ),
          tmeta_ThriftField::fromShape(
            shape(
              "id" => 2,
              "type" => tmeta_ThriftType::fromShape(
                shape(
                  "t_list" => tmeta_ThriftListType::fromShape(
                    shape(
                      "valueType" => tmeta_ThriftType::fromShape(
                        shape(
                          "t_struct" => tmeta_ThriftStructType::fromShape(
                            shape(
                              "name" => "type_rep.TypeStruct",
                            )
                          ),
                        )
                      ),
                    )
                  ),
                )
              ),
              "name" => "params",
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

}

