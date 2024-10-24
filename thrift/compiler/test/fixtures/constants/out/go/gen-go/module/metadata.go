// Autogenerated by Thrift for thrift/compiler/test/fixtures/constants/src/module.thrift
//
// DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
//  @generated

package module

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
    premadeThriftType_module_EmptyEnum *metadata.ThriftType = nil
    premadeThriftType_module_City *metadata.ThriftType = nil
    premadeThriftType_module_Company *metadata.ThriftType = nil
    premadeThriftType_i32 *metadata.ThriftType = nil
    premadeThriftType_string *metadata.ThriftType = nil
    premadeThriftType_double *metadata.ThriftType = nil
    premadeThriftType_module_Internship *metadata.ThriftType = nil
    premadeThriftType_module_Range *metadata.ThriftType = nil
    premadeThriftType_module_struct1 *metadata.ThriftType = nil
    premadeThriftType_list_i32 *metadata.ThriftType = nil
    premadeThriftType_module_struct2 *metadata.ThriftType = nil
    premadeThriftType_module_struct3 *metadata.ThriftType = nil
    premadeThriftType_byte *metadata.ThriftType = nil
    premadeThriftType_module_struct4 *metadata.ThriftType = nil
    premadeThriftType_module_union1 *metadata.ThriftType = nil
    premadeThriftType_module_union2 *metadata.ThriftType = nil
    premadeThriftType_module_MyCompany *metadata.ThriftType = nil
    premadeThriftType_module_MyStringIdentifier *metadata.ThriftType = nil
    premadeThriftType_module_MyIntIdentifier *metadata.ThriftType = nil
    premadeThriftType_map_string_string *metadata.ThriftType = nil
    premadeThriftType_module_MyMapIdentifier *metadata.ThriftType = nil
)

// Premade Thrift type initializer
var premadeThriftTypesInitOnce = sync.OnceFunc(func() {
    premadeThriftType_module_EmptyEnum = metadata.NewThriftType().SetTEnum(
        metadata.NewThriftEnumType().
            SetName("module.EmptyEnum"),
            )
    premadeThriftType_module_City = metadata.NewThriftType().SetTEnum(
        metadata.NewThriftEnumType().
            SetName("module.City"),
            )
    premadeThriftType_module_Company = metadata.NewThriftType().SetTEnum(
        metadata.NewThriftEnumType().
            SetName("module.Company"),
            )
    premadeThriftType_i32 = metadata.NewThriftType().SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
            )
    premadeThriftType_string = metadata.NewThriftType().SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_STRING_TYPE.Ptr(),
            )
    premadeThriftType_double = metadata.NewThriftType().SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_DOUBLE_TYPE.Ptr(),
            )
    premadeThriftType_module_Internship = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.Internship"),
            )
    premadeThriftType_module_Range = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.Range"),
            )
    premadeThriftType_module_struct1 = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.struct1"),
            )
    premadeThriftType_list_i32 = metadata.NewThriftType().SetTList(
        metadata.NewThriftListType().
            SetValueType(premadeThriftType_i32),
            )
    premadeThriftType_module_struct2 = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.struct2"),
            )
    premadeThriftType_module_struct3 = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.struct3"),
            )
    premadeThriftType_byte = metadata.NewThriftType().SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_BYTE_TYPE.Ptr(),
            )
    premadeThriftType_module_struct4 = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.struct4"),
            )
    premadeThriftType_module_union1 = metadata.NewThriftType().SetTUnion(
        metadata.NewThriftUnionType().
            SetName("module.union1"),
            )
    premadeThriftType_module_union2 = metadata.NewThriftType().SetTUnion(
        metadata.NewThriftUnionType().
            SetName("module.union2"),
            )
    premadeThriftType_module_MyCompany = metadata.NewThriftType().SetTTypedef(
        metadata.NewThriftTypedefType().
            SetName("module.MyCompany").
            SetUnderlyingType(premadeThriftType_module_Company),
            )
    premadeThriftType_module_MyStringIdentifier = metadata.NewThriftType().SetTTypedef(
        metadata.NewThriftTypedefType().
            SetName("module.MyStringIdentifier").
            SetUnderlyingType(premadeThriftType_string),
            )
    premadeThriftType_module_MyIntIdentifier = metadata.NewThriftType().SetTTypedef(
        metadata.NewThriftTypedefType().
            SetName("module.MyIntIdentifier").
            SetUnderlyingType(premadeThriftType_i32),
            )
    premadeThriftType_map_string_string = metadata.NewThriftType().SetTMap(
        metadata.NewThriftMapType().
            SetKeyType(premadeThriftType_string).
            SetValueType(premadeThriftType_string),
            )
    premadeThriftType_module_MyMapIdentifier = metadata.NewThriftType().SetTTypedef(
        metadata.NewThriftTypedefType().
            SetName("module.MyMapIdentifier").
            SetUnderlyingType(premadeThriftType_map_string_string),
            )
})

var premadeThriftTypesMapOnce = sync.OnceValue(
    func() map[string]*metadata.ThriftType {
        // Relies on premade Thrift types initialization
        premadeThriftTypesInitOnce()
        return map[string]*metadata.ThriftType{
            "module.EmptyEnum": premadeThriftType_module_EmptyEnum,
            "module.City": premadeThriftType_module_City,
            "module.Company": premadeThriftType_module_Company,
            "i32": premadeThriftType_i32,
            "string": premadeThriftType_string,
            "double": premadeThriftType_double,
            "module.Internship": premadeThriftType_module_Internship,
            "module.Range": premadeThriftType_module_Range,
            "module.struct1": premadeThriftType_module_struct1,
            "module.struct2": premadeThriftType_module_struct2,
            "module.struct3": premadeThriftType_module_struct3,
            "byte": premadeThriftType_byte,
            "module.struct4": premadeThriftType_module_struct4,
            "module.union1": premadeThriftType_module_union1,
            "module.union2": premadeThriftType_module_union2,
            "module.MyCompany": premadeThriftType_module_MyCompany,
            "module.MyStringIdentifier": premadeThriftType_module_MyStringIdentifier,
            "module.MyIntIdentifier": premadeThriftType_module_MyIntIdentifier,
            "module.MyMapIdentifier": premadeThriftType_module_MyMapIdentifier,
        }
    },
)

var structMetadatasOnce = sync.OnceValue(
    func() []*metadata.ThriftStruct {
        // Relies on premade Thrift types initialization
        premadeThriftTypesInitOnce()
        return []*metadata.ThriftStruct{
            metadata.NewThriftStruct().
    SetName("module.Internship").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("weeks").
    SetIsOptional(false).
    SetType(premadeThriftType_i32),
            metadata.NewThriftField().
    SetId(2).
    SetName("title").
    SetIsOptional(false).
    SetType(premadeThriftType_string),
            metadata.NewThriftField().
    SetId(3).
    SetName("employer").
    SetIsOptional(true).
    SetType(premadeThriftType_module_Company),
            metadata.NewThriftField().
    SetId(4).
    SetName("compensation").
    SetIsOptional(true).
    SetType(premadeThriftType_double),
            metadata.NewThriftField().
    SetId(5).
    SetName("school").
    SetIsOptional(true).
    SetType(premadeThriftType_string),
        },
    ),
            metadata.NewThriftStruct().
    SetName("module.Range").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("min").
    SetIsOptional(false).
    SetType(premadeThriftType_i32),
            metadata.NewThriftField().
    SetId(2).
    SetName("max").
    SetIsOptional(false).
    SetType(premadeThriftType_i32),
        },
    ),
            metadata.NewThriftStruct().
    SetName("module.struct1").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("a").
    SetIsOptional(false).
    SetType(premadeThriftType_i32),
            metadata.NewThriftField().
    SetId(2).
    SetName("b").
    SetIsOptional(false).
    SetType(premadeThriftType_string),
        },
    ),
            metadata.NewThriftStruct().
    SetName("module.struct2").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("a").
    SetIsOptional(false).
    SetType(premadeThriftType_i32),
            metadata.NewThriftField().
    SetId(2).
    SetName("b").
    SetIsOptional(false).
    SetType(premadeThriftType_string),
            metadata.NewThriftField().
    SetId(3).
    SetName("c").
    SetIsOptional(false).
    SetType(premadeThriftType_module_struct1),
            metadata.NewThriftField().
    SetId(4).
    SetName("d").
    SetIsOptional(false).
    SetType(premadeThriftType_list_i32),
        },
    ),
            metadata.NewThriftStruct().
    SetName("module.struct3").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("a").
    SetIsOptional(false).
    SetType(premadeThriftType_string),
            metadata.NewThriftField().
    SetId(2).
    SetName("b").
    SetIsOptional(false).
    SetType(premadeThriftType_i32),
            metadata.NewThriftField().
    SetId(3).
    SetName("c").
    SetIsOptional(false).
    SetType(premadeThriftType_module_struct2),
        },
    ),
            metadata.NewThriftStruct().
    SetName("module.struct4").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("a").
    SetIsOptional(false).
    SetType(premadeThriftType_i32),
            metadata.NewThriftField().
    SetId(2).
    SetName("b").
    SetIsOptional(true).
    SetType(premadeThriftType_double),
            metadata.NewThriftField().
    SetId(3).
    SetName("c").
    SetIsOptional(true).
    SetType(premadeThriftType_byte),
        },
    ),
            metadata.NewThriftStruct().
    SetName("module.union1").
    SetIsUnion(true).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("i").
    SetIsOptional(false).
    SetType(premadeThriftType_i32),
            metadata.NewThriftField().
    SetId(2).
    SetName("d").
    SetIsOptional(false).
    SetType(premadeThriftType_double),
        },
    ),
            metadata.NewThriftStruct().
    SetName("module.union2").
    SetIsUnion(true).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("i").
    SetIsOptional(false).
    SetType(premadeThriftType_i32),
            metadata.NewThriftField().
    SetId(2).
    SetName("d").
    SetIsOptional(false).
    SetType(premadeThriftType_double),
            metadata.NewThriftField().
    SetId(3).
    SetName("s").
    SetIsOptional(false).
    SetType(premadeThriftType_module_struct1),
            metadata.NewThriftField().
    SetId(4).
    SetName("u").
    SetIsOptional(false).
    SetType(premadeThriftType_module_union1),
        },
    ),
        }
    },
)

var exceptionMetadatasOnce = sync.OnceValue(
    func() []*metadata.ThriftException {
        // Relies on premade Thrift types initialization
        premadeThriftTypesInitOnce()
        return []*metadata.ThriftException{
        }
    },
)

var enumMetadatasOnce = sync.OnceValue(
    func() []*metadata.ThriftEnum {
        // Relies on premade Thrift types initialization
        premadeThriftTypesInitOnce()
        return []*metadata.ThriftEnum{
            metadata.NewThriftEnum().
    SetName("module.EmptyEnum").
    SetElements(
        map[int32]string{
        },
    ),
            metadata.NewThriftEnum().
    SetName("module.City").
    SetElements(
        map[int32]string{
            0: "NYC",
            1: "MPK",
            2: "SEA",
            3: "LON",
        },
    ),
            metadata.NewThriftEnum().
    SetName("module.Company").
    SetElements(
        map[int32]string{
            0: "FACEBOOK",
            1: "WHATSAPP",
            2: "OCULUS",
            3: "INSTAGRAM",
        },
    ),
        }
    },
)

var serviceMetadatasOnce = sync.OnceValue(
    func() []*metadata.ThriftService {
        // Relies on premade Thrift types initialization
        premadeThriftTypesInitOnce()
        return []*metadata.ThriftService{
        }
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
