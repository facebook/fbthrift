// Autogenerated by Thrift for thrift/annotation/thrift.thrift
//
// DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
//  @generated

package thrift

import (
    "maps"
    "sync"

    thrift "github.com/facebook/fbthrift/thrift/lib/go/thrift/types"
    metadata "github.com/facebook/fbthrift/thrift/lib/thrift/metadata"
)

// (needed to ensure safety because of naive import list construction)
var _ = thrift.ZERO
var _ = maps.Copy[map[int]int, map[int]int]
var _ = metadata.GoUnusedProtection__

// Premade Thrift types
var (
    premadeThriftType_thrift_RpcPriority *metadata.ThriftType = nil
    premadeThriftType_thrift_Experimental *metadata.ThriftType = nil
    premadeThriftType_i32 *metadata.ThriftType = nil
    premadeThriftType_list_i32 *metadata.ThriftType = nil
    premadeThriftType_map_i32_i32 *metadata.ThriftType = nil
    premadeThriftType_thrift_ReserveIds *metadata.ThriftType = nil
    premadeThriftType_bool *metadata.ThriftType = nil
    premadeThriftType_thrift_RequiresBackwardCompatibility *metadata.ThriftType = nil
    premadeThriftType_thrift_TerseWrite *metadata.ThriftType = nil
    premadeThriftType_thrift_Box *metadata.ThriftType = nil
    premadeThriftType_thrift_Mixin *metadata.ThriftType = nil
    premadeThriftType_thrift_SerializeInFieldIdOrder *metadata.ThriftType = nil
    premadeThriftType_thrift_BitmaskEnum *metadata.ThriftType = nil
    premadeThriftType_thrift_ExceptionMessage *metadata.ThriftType = nil
    premadeThriftType_thrift_InternBox *metadata.ThriftType = nil
    premadeThriftType_thrift_Serial *metadata.ThriftType = nil
    premadeThriftType_string *metadata.ThriftType = nil
    premadeThriftType_thrift_Uri *metadata.ThriftType = nil
    premadeThriftType_thrift_Priority *metadata.ThriftType = nil
    premadeThriftType_map_string_string *metadata.ThriftType = nil
    premadeThriftType_thrift_DeprecatedUnvalidatedAnnotations *metadata.ThriftType = nil
    premadeThriftType_thrift_AllowReservedIdentifier *metadata.ThriftType = nil
    premadeThriftType_thrift_AllowReservedFilename *metadata.ThriftType = nil
)

// Premade Thrift type initializer
var premadeThriftTypesInitOnce = sync.OnceFunc(func() {
    premadeThriftType_thrift_RpcPriority = metadata.NewThriftType().SetTEnum(
        metadata.NewThriftEnumType().
            SetName("thrift.RpcPriority"),
    )
    premadeThriftType_thrift_Experimental = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("thrift.Experimental"),
    )
    premadeThriftType_i32 = metadata.NewThriftType().SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
    )
    premadeThriftType_list_i32 = metadata.NewThriftType().SetTList(
        metadata.NewThriftListType().
            SetValueType(premadeThriftType_i32),
    )
    premadeThriftType_map_i32_i32 = metadata.NewThriftType().SetTMap(
        metadata.NewThriftMapType().
            SetKeyType(premadeThriftType_i32).
            SetValueType(premadeThriftType_i32),
    )
    premadeThriftType_thrift_ReserveIds = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("thrift.ReserveIds"),
    )
    premadeThriftType_bool = metadata.NewThriftType().SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_BOOL_TYPE.Ptr(),
    )
    premadeThriftType_thrift_RequiresBackwardCompatibility = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("thrift.RequiresBackwardCompatibility"),
    )
    premadeThriftType_thrift_TerseWrite = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("thrift.TerseWrite"),
    )
    premadeThriftType_thrift_Box = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("thrift.Box"),
    )
    premadeThriftType_thrift_Mixin = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("thrift.Mixin"),
    )
    premadeThriftType_thrift_SerializeInFieldIdOrder = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("thrift.SerializeInFieldIdOrder"),
    )
    premadeThriftType_thrift_BitmaskEnum = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("thrift.BitmaskEnum"),
    )
    premadeThriftType_thrift_ExceptionMessage = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("thrift.ExceptionMessage"),
    )
    premadeThriftType_thrift_InternBox = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("thrift.InternBox"),
    )
    premadeThriftType_thrift_Serial = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("thrift.Serial"),
    )
    premadeThriftType_string = metadata.NewThriftType().SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_STRING_TYPE.Ptr(),
    )
    premadeThriftType_thrift_Uri = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("thrift.Uri"),
    )
    premadeThriftType_thrift_Priority = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("thrift.Priority"),
    )
    premadeThriftType_map_string_string = metadata.NewThriftType().SetTMap(
        metadata.NewThriftMapType().
            SetKeyType(premadeThriftType_string).
            SetValueType(premadeThriftType_string),
    )
    premadeThriftType_thrift_DeprecatedUnvalidatedAnnotations = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("thrift.DeprecatedUnvalidatedAnnotations"),
    )
    premadeThriftType_thrift_AllowReservedIdentifier = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("thrift.AllowReservedIdentifier"),
    )
    premadeThriftType_thrift_AllowReservedFilename = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("thrift.AllowReservedFilename"),
    )
})

// Helper type to allow us to store Thrift types in a slice at compile time,
// and put them in a map at runtime. See comment at the top of template
// about a compilation limitation that affects map literals.
type thriftTypeWithFullName struct {
    fullName   string
    thriftType *metadata.ThriftType
}

var premadeThriftTypesMapOnce = sync.OnceValue(
    func() map[string]*metadata.ThriftType {
        // Relies on premade Thrift types initialization
        premadeThriftTypesInitOnce()

        thriftTypesWithFullName := make([]thriftTypeWithFullName, 0)
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "thrift.RpcPriority", premadeThriftType_thrift_RpcPriority })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "thrift.Experimental", premadeThriftType_thrift_Experimental })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "i32", premadeThriftType_i32 })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "thrift.ReserveIds", premadeThriftType_thrift_ReserveIds })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "bool", premadeThriftType_bool })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "thrift.RequiresBackwardCompatibility", premadeThriftType_thrift_RequiresBackwardCompatibility })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "thrift.TerseWrite", premadeThriftType_thrift_TerseWrite })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "thrift.Box", premadeThriftType_thrift_Box })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "thrift.Mixin", premadeThriftType_thrift_Mixin })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "thrift.SerializeInFieldIdOrder", premadeThriftType_thrift_SerializeInFieldIdOrder })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "thrift.BitmaskEnum", premadeThriftType_thrift_BitmaskEnum })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "thrift.ExceptionMessage", premadeThriftType_thrift_ExceptionMessage })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "thrift.InternBox", premadeThriftType_thrift_InternBox })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "thrift.Serial", premadeThriftType_thrift_Serial })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "string", premadeThriftType_string })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "thrift.Uri", premadeThriftType_thrift_Uri })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "thrift.Priority", premadeThriftType_thrift_Priority })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "thrift.DeprecatedUnvalidatedAnnotations", premadeThriftType_thrift_DeprecatedUnvalidatedAnnotations })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "thrift.AllowReservedIdentifier", premadeThriftType_thrift_AllowReservedIdentifier })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "thrift.AllowReservedFilename", premadeThriftType_thrift_AllowReservedFilename })

        fbthriftThriftTypesMap := make(map[string]*metadata.ThriftType, len(thriftTypesWithFullName))
        for _, value := range thriftTypesWithFullName {
            fbthriftThriftTypesMap[value.fullName] = value.thriftType
        }
        return fbthriftThriftTypesMap
    },
)

var structMetadatasOnce = sync.OnceValue(
    func() []*metadata.ThriftStruct {
        fbthriftResults := make([]*metadata.ThriftStruct, 0)
        for _, fbthriftStructSpec := range premadeStructSpecsOnce() {
            if !fbthriftStructSpec.IsException {
                fbthriftResults = append(fbthriftResults, getMetadataThriftStruct(fbthriftStructSpec))
            }
        }
        return fbthriftResults
    },
)

var exceptionMetadatasOnce = sync.OnceValue(
    func() []*metadata.ThriftException {
        fbthriftResults := make([]*metadata.ThriftException, 0)
        for _, fbthriftStructSpec := range premadeStructSpecsOnce() {
            if fbthriftStructSpec.IsException {
                fbthriftResults = append(fbthriftResults, getMetadataThriftException(fbthriftStructSpec))
            }
        }
        return fbthriftResults
    },
)

var enumMetadatasOnce = sync.OnceValue(
    func() []*metadata.ThriftEnum {
        // Relies on premade Thrift types initialization
        premadeThriftTypesInitOnce()

        fbthriftResults := make([]*metadata.ThriftEnum, 0)
        fbthriftResults = append(fbthriftResults, metadata.NewThriftEnum().
    SetName("thrift.RpcPriority").
    SetElements(
        map[int32]string{
            0: "HIGH_IMPORTANT",
            1: "HIGH",
            2: "IMPORTANT",
            3: "NORMAL",
            4: "BEST_EFFORT",
        },
    ))
        return fbthriftResults
    },
)

var serviceMetadatasOnce = sync.OnceValue(
    func() []*metadata.ThriftService {
        // Relies on premade Thrift types initialization
        premadeThriftTypesInitOnce()

        fbthriftResults := make([]*metadata.ThriftService, 0)
        return fbthriftResults
    },
)

// GetMetadataThriftType (INTERNAL USE ONLY).
// Returns metadata ThriftType for a given full type name.
func GetMetadataThriftType(fullName string) *metadata.ThriftType {
    return premadeThriftTypesMapOnce()[fullName]
}

// GetThriftMetadata returns complete Thrift metadata for current and imported packages.
func GetThriftMetadata() *metadata.ThriftMetadata {
    allEnumsMap := make(map[string]*metadata.ThriftEnum)
    allStructsMap := make(map[string]*metadata.ThriftStruct)
    allExceptionsMap := make(map[string]*metadata.ThriftException)
    allServicesMap := make(map[string]*metadata.ThriftService)

    // Add enum metadatas from the current program...
    for _, enumMetadata := range enumMetadatasOnce() {
        allEnumsMap[enumMetadata.GetName()] = enumMetadata
    }
    // Add struct metadatas from the current program...
    for _, structMetadata := range structMetadatasOnce() {
        allStructsMap[structMetadata.GetName()] = structMetadata
    }
    // Add exception metadatas from the current program...
    for _, exceptionMetadata := range exceptionMetadatasOnce() {
        allExceptionsMap[exceptionMetadata.GetName()] = exceptionMetadata
    }
    // Add service metadatas from the current program...
    for _, serviceMetadata := range serviceMetadatasOnce() {
        allServicesMap[serviceMetadata.GetName()] = serviceMetadata
    }

    // Obtain Thrift metadatas from recursively included programs...
    var recursiveThriftMetadatas []*metadata.ThriftMetadata

    // ...now merge metadatas from recursively included programs.
    for _, thriftMetadata := range recursiveThriftMetadatas {
        maps.Copy(allEnumsMap, thriftMetadata.GetEnums())
        maps.Copy(allStructsMap, thriftMetadata.GetStructs())
        maps.Copy(allExceptionsMap, thriftMetadata.GetExceptions())
        maps.Copy(allServicesMap, thriftMetadata.GetServices())
    }

    return metadata.NewThriftMetadata().
        SetEnums(allEnumsMap).
        SetStructs(allStructsMap).
        SetExceptions(allExceptionsMap).
        SetServices(allServicesMap)
}

// GetThriftMetadataForService returns Thrift metadata for the given service.
func GetThriftMetadataForService(scopedServiceName string) *metadata.ThriftMetadata {
    thriftMetadata := GetThriftMetadata()

    allServicesMap := thriftMetadata.GetServices()
    relevantServicesMap := make(map[string]*metadata.ThriftService)

    serviceMetadata := allServicesMap[scopedServiceName]
    // Visit and record all recursive parents of the target service.
    for serviceMetadata != nil {
        relevantServicesMap[serviceMetadata.GetName()] = serviceMetadata
        if serviceMetadata.IsSetParent() {
            serviceMetadata = allServicesMap[serviceMetadata.GetParent()]
        } else {
            serviceMetadata = nil
        }
    }

    thriftMetadata.SetServices(relevantServicesMap)

    return thriftMetadata
}

func getMetadataThriftPrimitiveType(s *thrift.CodecPrimitiveSpec) *metadata.ThriftPrimitiveType {
	var value metadata.ThriftPrimitiveType

	switch s.PrimitiveType {
	case thrift.CODEC_PRIMITIVE_TYPE_BYTE:
		value = metadata.ThriftPrimitiveType_THRIFT_BYTE_TYPE
	case thrift.CODEC_PRIMITIVE_TYPE_BOOL:
		value = metadata.ThriftPrimitiveType_THRIFT_BOOL_TYPE
	case thrift.CODEC_PRIMITIVE_TYPE_I16:
		value = metadata.ThriftPrimitiveType_THRIFT_I16_TYPE
	case thrift.CODEC_PRIMITIVE_TYPE_I32:
		value = metadata.ThriftPrimitiveType_THRIFT_I32_TYPE
	case thrift.CODEC_PRIMITIVE_TYPE_I64:
		value = metadata.ThriftPrimitiveType_THRIFT_I64_TYPE
	case thrift.CODEC_PRIMITIVE_TYPE_FLOAT:
		value = metadata.ThriftPrimitiveType_THRIFT_FLOAT_TYPE
	case thrift.CODEC_PRIMITIVE_TYPE_DOUBLE:
		value = metadata.ThriftPrimitiveType_THRIFT_DOUBLE_TYPE
	case thrift.CODEC_PRIMITIVE_TYPE_BINARY:
		value = metadata.ThriftPrimitiveType_THRIFT_BINARY_TYPE
	case thrift.CODEC_PRIMITIVE_TYPE_STRING:
		value = metadata.ThriftPrimitiveType_THRIFT_STRING_TYPE
	}

	return value.Ptr()
}

func getMetadataThriftEnumType(s *thrift.CodecEnumSpec) *metadata.ThriftEnumType {
	return metadata.NewThriftEnumType().
		SetName(s.ScopedName)
}

func getMetadataThriftSetType(s *thrift.CodecSetSpec) *metadata.ThriftSetType {
	return metadata.NewThriftSetType().
		SetValueType(getMetadataThriftType(s.ElementTypeSpec))
}

func getMetadataThriftListType(s *thrift.CodecListSpec) *metadata.ThriftListType {
	return metadata.NewThriftListType().
		SetValueType(getMetadataThriftType(s.ElementTypeSpec))
}

func getMetadataThriftMapType(s *thrift.CodecMapSpec) *metadata.ThriftMapType {
	return metadata.NewThriftMapType().
		SetKeyType(getMetadataThriftType(s.KeyTypeSpec)).
		SetValueType(getMetadataThriftType(s.ValueTypeSpec))
}

func getMetadataThriftTypedefType(s *thrift.CodecTypedefSpec) *metadata.ThriftTypedefType {
	return metadata.NewThriftTypedefType().
		SetName(s.ScopedName).
		SetUnderlyingType(getMetadataThriftType(s.UnderlyingTypeSpec))
}

func getMetadataThriftStructType(s *thrift.CodecStructSpec) *metadata.ThriftStructType {
	return metadata.NewThriftStructType().
		SetName(s.ScopedName)
}

func getMetadataThriftUnionType(s *thrift.CodecStructSpec) *metadata.ThriftUnionType {
	return metadata.NewThriftUnionType().
		SetName(s.ScopedName)
}

func getMetadataThriftType(s *thrift.TypeSpec) *metadata.ThriftType {
	thriftType := metadata.NewThriftType()
	switch {
	case s.CodecPrimitiveSpec != nil:
		thriftType.SetTPrimitive(getMetadataThriftPrimitiveType(s.CodecPrimitiveSpec))
	case s.CodecEnumSpec != nil:
		thriftType.SetTEnum(getMetadataThriftEnumType(s.CodecEnumSpec))
	case s.CodecSetSpec != nil:
		thriftType.SetTSet(getMetadataThriftSetType(s.CodecSetSpec))
	case s.CodecListSpec != nil:
		thriftType.SetTList(getMetadataThriftListType(s.CodecListSpec))
	case s.CodecMapSpec != nil:
		thriftType.SetTMap(getMetadataThriftMapType(s.CodecMapSpec))
	case s.CodecTypedefSpec != nil:
		thriftType.SetTTypedef(getMetadataThriftTypedefType(s.CodecTypedefSpec))
	case s.CodecStructSpec != nil:
		if s.CodecStructSpec.IsUnion {
			thriftType.SetTUnion(getMetadataThriftUnionType(s.CodecStructSpec))
		} else {
			thriftType.SetTStruct(getMetadataThriftStructType(s.CodecStructSpec))
		}
	}
	return thriftType
}

func getMetadataThriftField(s *thrift.FieldSpec) *metadata.ThriftField {
	return metadata.NewThriftField().
		SetId(int32(s.ID)).
		SetName(s.Name).
		SetIsOptional(s.IsOptional).
		SetType(getMetadataThriftType(s.ValueTypeSpec))
}

func getMetadataThriftStruct(s *thrift.StructSpec) *metadata.ThriftStruct {
	metadataThriftFields := make([]*metadata.ThriftField, len(s.FieldSpecs), len(s.FieldSpecs))
	for i, fieldSpec := range s.FieldSpecs {
		metadataThriftFields[i] = getMetadataThriftField(&fieldSpec)
	}

	return metadata.NewThriftStruct().
		SetName(s.ScopedName).
		SetIsUnion(s.IsUnion).
		SetFields(metadataThriftFields)
}

func getMetadataThriftException(s *thrift.StructSpec) *metadata.ThriftException {
	metadataThriftFields := make([]*metadata.ThriftField, len(s.FieldSpecs), len(s.FieldSpecs))
	for i, fieldSpec := range s.FieldSpecs {
		metadataThriftFields[i] = getMetadataThriftField(&fieldSpec)
	}

	return metadata.NewThriftException().
		SetName(s.ScopedName).
		SetFields(metadataThriftFields)
}
