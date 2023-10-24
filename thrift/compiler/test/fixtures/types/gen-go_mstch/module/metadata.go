// @generated by Thrift for [[[ program path ]]]
// This file is probably not the place you want to edit!

package module // [[[ program thrift source path ]]]

import (
    included "included"
    thrift "github.com/facebook/fbthrift/thrift/lib/go/thrift"
    metadata "github.com/facebook/fbthrift/thrift/lib/thrift/metadata"
)

var _ = included.GoUnusedProtection__
// (needed to ensure safety because of naive import list construction)
var _ = thrift.ZERO
var _ = metadata.GoUnusedProtection__

var structMetadatas = []*metadata.ThriftStruct{
    metadata.NewThriftStruct().
    SetName("module.empty_struct").
    SetIsUnion(false),
    metadata.NewThriftStruct().
    SetName("module.decorated_struct").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("field").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_STRING_TYPE.Ptr(),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.ContainerStruct").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(2).
    SetName("fieldB").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTList(
        metadata.NewThriftListType().
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )),
            ),
    ),
            metadata.NewThriftField().
    SetId(3).
    SetName("fieldC").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTList(
        metadata.NewThriftListType().
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )),
            ),
    ),
            metadata.NewThriftField().
    SetId(4).
    SetName("fieldD").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTList(
        metadata.NewThriftListType().
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )),
            ),
    ),
            metadata.NewThriftField().
    SetId(5).
    SetName("fieldE").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTList(
        metadata.NewThriftListType().
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )),
            ),
    ),
            metadata.NewThriftField().
    SetId(6).
    SetName("fieldF").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTSet(
        metadata.NewThriftSetType().
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )),
            ),
    ),
            metadata.NewThriftField().
    SetId(7).
    SetName("fieldG").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTMap(
        metadata.NewThriftMapType().
            SetKeyType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )).
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_STRING_TYPE.Ptr(),
            )),
            ),
    ),
            metadata.NewThriftField().
    SetId(8).
    SetName("fieldH").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTTypedef(
        metadata.NewThriftTypedefType().
            SetName("included.SomeMap").
            SetUnderlyingType(
                metadata.NewThriftType().
            SetTMap(
                metadata.NewThriftMapType().
            SetKeyType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )).
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_STRING_TYPE.Ptr(),
            )),
            )    ),
            ),
    ),
            metadata.NewThriftField().
    SetId(12).
    SetName("fieldA").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTList(
        metadata.NewThriftListType().
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.CppTypeStruct").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("fieldA").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTList(
        metadata.NewThriftListType().
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.VirtualStruct").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("MyIntField").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I64_TYPE.Ptr(),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.MyStructWithForwardRefEnum").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("a").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTEnum(
        metadata.NewThriftEnumType().
    SetName("module.MyForwardRefEnum"),
    ),
    ),
            metadata.NewThriftField().
    SetId(2).
    SetName("b").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTEnum(
        metadata.NewThriftEnumType().
    SetName("module.MyForwardRefEnum"),
    ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.TrivialNumeric").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("a").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(2).
    SetName("b").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_BOOL_TYPE.Ptr(),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.TrivialNestedWithDefault").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("z").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(2).
    SetName("n").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.TrivialNumeric"),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.ComplexString").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("a").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_STRING_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(2).
    SetName("b").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTMap(
        metadata.NewThriftMapType().
            SetKeyType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_STRING_TYPE.Ptr(),
            )).
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.ComplexNestedWithDefault").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("z").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_STRING_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(2).
    SetName("n").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.ComplexString"),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.MinPadding").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("small").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_BYTE_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(2).
    SetName("big").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I64_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(3).
    SetName("medium").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I16_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(4).
    SetName("biggish").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(5).
    SetName("tiny").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_BYTE_TYPE.Ptr(),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.MinPaddingWithCustomType").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("small").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_BYTE_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(2).
    SetName("big").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I64_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(3).
    SetName("medium").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I16_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(4).
    SetName("biggish").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(5).
    SetName("tiny").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_BYTE_TYPE.Ptr(),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.MyStruct").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("MyIntField").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I64_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(2).
    SetName("MyStringField").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_STRING_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(3).
    SetName("majorVer").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I64_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(4).
    SetName("data").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTStruct(
        metadata.NewThriftStructType().
    SetName("module.MyDataItem"),
    ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.MyDataItem").
    SetIsUnion(false),
    metadata.NewThriftStruct().
    SetName("module.Renaming").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("foo").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I64_TYPE.Ptr(),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.AnnotatedTypes").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("binary_field").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTTypedef(
        metadata.NewThriftTypedefType().
    SetName("module.TBinary").
    SetUnderlyingType(
        metadata.NewThriftType().
            SetTPrimitive(
                metadata.ThriftPrimitiveType_THRIFT_BINARY_TYPE.Ptr(),
            )    ),
    ),
    ),
            metadata.NewThriftField().
    SetId(2).
    SetName("list_field").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTTypedef(
        metadata.NewThriftTypedefType().
    SetName("included.SomeListOfTypeMap").
    SetUnderlyingType(
        metadata.NewThriftType().
            SetTList(
                metadata.NewThriftListType().
            SetValueType(metadata.NewThriftType().
    SetTTypedef(
        metadata.NewThriftTypedefType().
            SetName("included.SomeMap").
            SetUnderlyingType(
                metadata.NewThriftType().
            SetTMap(
                metadata.NewThriftMapType().
            SetKeyType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )).
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_STRING_TYPE.Ptr(),
            )),
            )    ),
            )),
            )    ),
    ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.ForwardUsageRoot").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("ForwardUsageStruct").
    SetIsOptional(true).
    SetType(
        metadata.NewThriftType().
    SetTStruct(
        metadata.NewThriftStructType().
    SetName("module.ForwardUsageStruct"),
    ),
    ),
            metadata.NewThriftField().
    SetId(2).
    SetName("ForwardUsageByRef").
    SetIsOptional(true).
    SetType(
        metadata.NewThriftType().
    SetTStruct(
        metadata.NewThriftStructType().
    SetName("module.ForwardUsageByRef"),
    ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.ForwardUsageStruct").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("foo").
    SetIsOptional(true).
    SetType(
        metadata.NewThriftType().
    SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.ForwardUsageRoot"),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.ForwardUsageByRef").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("foo").
    SetIsOptional(true).
    SetType(
        metadata.NewThriftType().
    SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.ForwardUsageRoot"),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.IncompleteMap").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("field").
    SetIsOptional(true).
    SetType(
        metadata.NewThriftType().
    SetTMap(
        metadata.NewThriftMapType().
            SetKeyType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )).
            SetValueType(metadata.NewThriftType().
    SetTStruct(
        metadata.NewThriftStructType().
    SetName("module.IncompleteMapDep"),
    )),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.IncompleteMapDep").
    SetIsUnion(false),
    metadata.NewThriftStruct().
    SetName("module.CompleteMap").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("field").
    SetIsOptional(true).
    SetType(
        metadata.NewThriftType().
    SetTMap(
        metadata.NewThriftMapType().
            SetKeyType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )).
            SetValueType(metadata.NewThriftType().
    SetTStruct(
        metadata.NewThriftStructType().
    SetName("module.CompleteMapDep"),
    )),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.CompleteMapDep").
    SetIsUnion(false),
    metadata.NewThriftStruct().
    SetName("module.IncompleteList").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("field").
    SetIsOptional(true).
    SetType(
        metadata.NewThriftType().
    SetTList(
        metadata.NewThriftListType().
            SetValueType(metadata.NewThriftType().
    SetTStruct(
        metadata.NewThriftStructType().
    SetName("module.IncompleteListDep"),
    )),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.IncompleteListDep").
    SetIsUnion(false),
    metadata.NewThriftStruct().
    SetName("module.CompleteList").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("field").
    SetIsOptional(true).
    SetType(
        metadata.NewThriftType().
    SetTList(
        metadata.NewThriftListType().
            SetValueType(metadata.NewThriftType().
    SetTStruct(
        metadata.NewThriftStructType().
    SetName("module.CompleteListDep"),
    )),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.CompleteListDep").
    SetIsUnion(false),
    metadata.NewThriftStruct().
    SetName("module.AdaptedList").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("field").
    SetIsOptional(true).
    SetType(
        metadata.NewThriftType().
    SetTList(
        metadata.NewThriftListType().
            SetValueType(metadata.NewThriftType().
    SetTStruct(
        metadata.NewThriftStructType().
    SetName("module.AdaptedListDep"),
    )),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.AdaptedListDep").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("field").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.AdaptedList"),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.DependentAdaptedList").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("field").
    SetIsOptional(true).
    SetType(
        metadata.NewThriftType().
    SetTList(
        metadata.NewThriftListType().
            SetValueType(metadata.NewThriftType().
    SetTStruct(
        metadata.NewThriftStructType().
    SetName("module.DependentAdaptedListDep"),
    )),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.DependentAdaptedListDep").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("field").
    SetIsOptional(true).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I16_TYPE.Ptr(),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.AllocatorAware").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("aa_list").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTList(
        metadata.NewThriftListType().
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )),
            ),
    ),
            metadata.NewThriftField().
    SetId(2).
    SetName("aa_set").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTSet(
        metadata.NewThriftSetType().
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )),
            ),
    ),
            metadata.NewThriftField().
    SetId(3).
    SetName("aa_map").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTMap(
        metadata.NewThriftMapType().
            SetKeyType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )).
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )),
            ),
    ),
            metadata.NewThriftField().
    SetId(4).
    SetName("aa_string").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_STRING_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(5).
    SetName("not_a_container").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(6).
    SetName("aa_unique").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(7).
    SetName("aa_shared").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.AllocatorAware2").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("not_a_container").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(2).
    SetName("box_field").
    SetIsOptional(true).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.TypedefStruct").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("i32_field").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            ),
    ),
            metadata.NewThriftField().
    SetId(2).
    SetName("IntTypedef_field").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTTypedef(
        metadata.NewThriftTypedefType().
            SetName("module.IntTypedef").
            SetUnderlyingType(
                metadata.NewThriftType().
            SetTPrimitive(
                metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )    ),
            ),
    ),
            metadata.NewThriftField().
    SetId(3).
    SetName("UintTypedef_field").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTTypedef(
        metadata.NewThriftTypedefType().
            SetName("module.UintTypedef").
            SetUnderlyingType(
                metadata.NewThriftType().
            SetTTypedef(
                metadata.NewThriftTypedefType().
    SetName("module.IntTypedef").
    SetUnderlyingType(
        metadata.NewThriftType().
            SetTPrimitive(
                metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )    ),
    )    ),
            ),
    ),
        },
    ),
    metadata.NewThriftStruct().
    SetName("module.StructWithDoubleUnderscores").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("__field").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            ),
    ),
        },
    ),
}

var exceptionMetadatas = []*metadata.ThriftException{
}

var enumMetadatas = []*metadata.ThriftEnum{
    metadata.NewThriftEnum().
    SetName("module.has_bitwise_ops").
    SetElements(
        map[int32]string{
            0: "none",
            1: "zero",
            2: "one",
            4: "two",
            8: "three",
        },
    ),
    metadata.NewThriftEnum().
    SetName("module.is_unscoped").
    SetElements(
        map[int32]string{
            0: "hello",
            1: "world",
        },
    ),
    metadata.NewThriftEnum().
    SetName("module.MyForwardRefEnum").
    SetElements(
        map[int32]string{
            0: "ZERO",
            12: "NONZERO",
        },
    ),
}

var serviceMetadatas = []*metadata.ThriftService{
    metadata.NewThriftService().
    SetName("module.SomeService").
    SetFunctions(
        []*metadata.ThriftFunction{
            metadata.NewThriftFunction().
    SetName("bounce_map").
    SetIsOneway(false).
    SetReturnType(
        metadata.NewThriftType().
    SetTTypedef(
        metadata.NewThriftTypedefType().
            SetName("included.SomeMap").
            SetUnderlyingType(
                metadata.NewThriftType().
            SetTMap(
                metadata.NewThriftMapType().
            SetKeyType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )).
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_STRING_TYPE.Ptr(),
            )),
            )    ),
            ),
    ).
    SetArguments(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("m").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTTypedef(
        metadata.NewThriftTypedefType().
            SetName("included.SomeMap").
            SetUnderlyingType(
                metadata.NewThriftType().
            SetTMap(
                metadata.NewThriftMapType().
            SetKeyType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )).
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_STRING_TYPE.Ptr(),
            )),
            )    ),
            ),
    ),
        },
    ),
            metadata.NewThriftFunction().
    SetName("binary_keyed_map").
    SetIsOneway(false).
    SetReturnType(
        metadata.NewThriftType().
    SetTMap(
        metadata.NewThriftMapType().
            SetKeyType(metadata.NewThriftType().
    SetTTypedef(
        metadata.NewThriftTypedefType().
            SetName("module.TBinary").
            SetUnderlyingType(
                metadata.NewThriftType().
            SetTPrimitive(
                metadata.ThriftPrimitiveType_THRIFT_BINARY_TYPE.Ptr(),
            )    ),
            )).
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I64_TYPE.Ptr(),
            )),
            ),
    ).
    SetArguments(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("r").
    SetIsOptional(false).
    SetType(
        metadata.NewThriftType().
    SetTList(
        metadata.NewThriftListType().
            SetValueType(metadata.NewThriftType().
    SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I64_TYPE.Ptr(),
            )),
            ),
    ),
        },
    ),
        },
    ),
}

// GetThriftMetadata returns complete Thrift metadata for current and imported packages.
func GetThriftMetadata() *metadata.ThriftMetadata {
    includedEnumsMetadatas := []map[string]*metadata.ThriftEnum{
        GetEnumsMetadata(),
        included.GetEnumsMetadata(),
    }
    includedStructsMetadatas := []map[string]*metadata.ThriftStruct{
        GetStructsMetadata(),
        included.GetStructsMetadata(),
    }
    includedExceptionsMetadatas := []map[string]*metadata.ThriftException{
        GetExceptionsMetadata(),
        included.GetExceptionsMetadata(),
    }
    includedServicesMetadatas := []map[string]*metadata.ThriftService{
        GetServicesMetadata(),
        included.GetServicesMetadata(),
    }

	allEnums := make(map[string]*metadata.ThriftEnum)
	allStructs := make(map[string]*metadata.ThriftStruct)
	allExceptions := make(map[string]*metadata.ThriftException)
    allServices := make(map[string]*metadata.ThriftService)

    for _, includedEnumsMetadata := range includedEnumsMetadatas {
        for enumName, thriftEnum := range includedEnumsMetadata {
            allEnums[enumName] = thriftEnum
        }
    }
    for _, includedStructsMetadata := range includedStructsMetadatas {
        for structName, thriftStruct := range includedStructsMetadata {
            allStructs[structName] = thriftStruct
        }
    }
    for _, includedExceptionsMetadata := range includedExceptionsMetadatas {
        for exceptionName, thriftException := range includedExceptionsMetadata {
            allExceptions[exceptionName] = thriftException
        }
    }
    for _, includedServicesMetadata := range includedServicesMetadatas {
        for serviceName, thriftService := range includedServicesMetadata {
            allServices[serviceName] = thriftService
        }
    }

    return metadata.NewThriftMetadata().
        SetEnums(allEnums).
        SetStructs(allStructs).
        SetExceptions(allExceptions).
        SetServices(allServices)
}

// GetStructsMetadata returns Thrift metadata for enums in the current package.
func GetEnumsMetadata() map[string]*metadata.ThriftEnum {
    result := make(map[string]*metadata.ThriftEnum)
    for _, enumMetadata := range enumMetadatas {
        result[enumMetadata.GetName()] = enumMetadata
    }
    return result
}

// GetStructsMetadata returns Thrift metadata for structs in the current package.
func GetStructsMetadata() map[string]*metadata.ThriftStruct {
    result := make(map[string]*metadata.ThriftStruct)
    for _, structMetadata := range structMetadatas {
        result[structMetadata.GetName()] = structMetadata
    }
    return result
}

// GetStructsMetadata returns Thrift metadata for exceptions in the current package.
func GetExceptionsMetadata() map[string]*metadata.ThriftException {
    result := make(map[string]*metadata.ThriftException)
    for _, exceptionMetadata := range exceptionMetadatas {
        result[exceptionMetadata.GetName()] = exceptionMetadata
    }
    return result
}

// GetStructsMetadata returns Thrift metadata for services in the current package.
func GetServicesMetadata() map[string]*metadata.ThriftService {
    result := make(map[string]*metadata.ThriftService)
    for _, serviceMetadata := range serviceMetadatas {
        result[serviceMetadata.GetName()] = serviceMetadata
    }
    return result
}