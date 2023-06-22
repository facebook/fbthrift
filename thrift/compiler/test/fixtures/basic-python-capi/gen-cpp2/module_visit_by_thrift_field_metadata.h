/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/basic-python-capi/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#pragma once

#include <thrift/lib/cpp2/visitation/visit_by_thrift_field_metadata.h>
#include "thrift/compiler/test/fixtures/basic-python-capi/gen-cpp2/module_metadata.h"

namespace apache {
namespace thrift {
namespace detail {

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::MyStruct> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).inty_ref());
    case 2:
      return f(1, static_cast<T&&>(t).stringy_ref());
    case 3:
      return f(2, static_cast<T&&>(t).myItemy_ref());
    case 4:
      return f(3, static_cast<T&&>(t).myEnumy_ref());
    case 5:
      return f(4, static_cast<T&&>(t).boulet_ref());
    case 6:
      return f(5, static_cast<T&&>(t).floatListy_ref());
    case 7:
      return f(6, static_cast<T&&>(t).strMappy_ref());
    case 8:
      return f(7, static_cast<T&&>(t).intSetty_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::MyStruct");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::MyDataItem> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).s_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::MyDataItem");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::TransitiveDoubler> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::TransitiveDoubler");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::detail::DoubledPair> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).s_ref());
    case 2:
      return f(1, static_cast<T&&>(t).x_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::detail::DoubledPair");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::StringPair> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).normal_ref());
    case 2:
      return f(1, static_cast<T&&>(t).doubled_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::StringPair");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::VapidStruct> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::VapidStruct");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::PrimitiveStruct> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).booly_ref());
    case 2:
      return f(1, static_cast<T&&>(t).charry_ref());
    case 3:
      return f(2, static_cast<T&&>(t).shortay_ref());
    case 5:
      return f(3, static_cast<T&&>(t).inty_ref());
    case 7:
      return f(4, static_cast<T&&>(t).longy_ref());
    case 8:
      return f(5, static_cast<T&&>(t).floaty_ref());
    case 9:
      return f(6, static_cast<T&&>(t).dubby_ref());
    case 12:
      return f(7, static_cast<T&&>(t).stringy_ref());
    case 13:
      return f(8, static_cast<T&&>(t).bytey_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::PrimitiveStruct");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::ListStruct> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).boolz_ref());
    case 2:
      return f(1, static_cast<T&&>(t).intz_ref());
    case 3:
      return f(2, static_cast<T&&>(t).stringz_ref());
    case 4:
      return f(3, static_cast<T&&>(t).encoded_ref());
    case 5:
      return f(4, static_cast<T&&>(t).uidz_ref());
    case 6:
      return f(5, static_cast<T&&>(t).matrix_ref());
    case 7:
      return f(6, static_cast<T&&>(t).ucharz_ref());
    case 8:
      return f(7, static_cast<T&&>(t).voxels_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::ListStruct");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::ComposeStruct> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).enum__ref());
    case 2:
      return f(1, static_cast<T&&>(t).renamed__ref());
    case 3:
      return f(2, static_cast<T&&>(t).primitive_ref());
    case 4:
      return f(3, static_cast<T&&>(t).aliased_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::ComposeStruct");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::OurUnion> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).myEnum_ref());
    case 2:
      return f(1, static_cast<T&&>(t).myStruct_ref());
    case 3:
      return f(2, static_cast<T&&>(t).myDataItem_ref());
    case 4:
      return f(3, static_cast<T&&>(t).intSet_ref());
    case 5:
      return f(4, static_cast<T&&>(t).doubleList_ref());
    case 6:
      return f(5, static_cast<T&&>(t).strMap_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::OurUnion");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::MyStructPatchStruct> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).assign_ref());
    case 2:
      return f(1, static_cast<T&&>(t).clear_ref());
    case 3:
      return f(2, static_cast<T&&>(t).patchPrior_ref());
    case 5:
      return f(3, static_cast<T&&>(t).ensure_ref());
    case 6:
      return f(4, static_cast<T&&>(t).patch_ref());
    case 7:
      return f(5, static_cast<T&&>(t).remove_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::MyStructPatchStruct");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::MyStructField4PatchStruct> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).assign_ref());
    case 2:
      return f(1, static_cast<T&&>(t).clear_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::MyStructField4PatchStruct");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::MyStructField6PatchStruct> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).assign_ref());
    case 2:
      return f(1, static_cast<T&&>(t).clear_ref());
    case 8:
      return f(2, static_cast<T&&>(t).prepend_ref());
    case 9:
      return f(3, static_cast<T&&>(t).append_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::MyStructField6PatchStruct");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::MyStructField7PatchStruct> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).assign_ref());
    case 2:
      return f(1, static_cast<T&&>(t).clear_ref());
    case 3:
      return f(2, static_cast<T&&>(t).patchPrior_ref());
    case 5:
      return f(3, static_cast<T&&>(t).add_ref());
    case 6:
      return f(4, static_cast<T&&>(t).patch_ref());
    case 7:
      return f(5, static_cast<T&&>(t).remove_ref());
    case 9:
      return f(6, static_cast<T&&>(t).put_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::MyStructField7PatchStruct");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::MyStructField8PatchStruct> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).assign_ref());
    case 2:
      return f(1, static_cast<T&&>(t).clear_ref());
    case 7:
      return f(2, static_cast<T&&>(t).remove_ref());
    case 8:
      return f(3, static_cast<T&&>(t).add_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::MyStructField8PatchStruct");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::MyStructFieldPatchStruct> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).inty_ref());
    case 2:
      return f(1, static_cast<T&&>(t).stringy_ref());
    case 3:
      return f(2, static_cast<T&&>(t).myItemy_ref());
    case 4:
      return f(3, static_cast<T&&>(t).myEnumy_ref());
    case 5:
      return f(4, static_cast<T&&>(t).booly_ref());
    case 6:
      return f(5, static_cast<T&&>(t).floatListy_ref());
    case 7:
      return f(6, static_cast<T&&>(t).strMappy_ref());
    case 8:
      return f(7, static_cast<T&&>(t).intSetty_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::MyStructFieldPatchStruct");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::MyStructEnsureStruct> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).inty_ref());
    case 2:
      return f(1, static_cast<T&&>(t).stringy_ref());
    case 3:
      return f(2, static_cast<T&&>(t).myItemy_ref());
    case 4:
      return f(3, static_cast<T&&>(t).myEnumy_ref());
    case 5:
      return f(4, static_cast<T&&>(t).booly_ref());
    case 6:
      return f(5, static_cast<T&&>(t).floatListy_ref());
    case 7:
      return f(6, static_cast<T&&>(t).strMappy_ref());
    case 8:
      return f(7, static_cast<T&&>(t).intSetty_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::MyStructEnsureStruct");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::MyDataItemPatchStruct> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).assign_ref());
    case 2:
      return f(1, static_cast<T&&>(t).clear_ref());
    case 3:
      return f(2, static_cast<T&&>(t).patchPrior_ref());
    case 5:
      return f(3, static_cast<T&&>(t).ensure_ref());
    case 6:
      return f(4, static_cast<T&&>(t).patch_ref());
    case 7:
      return f(5, static_cast<T&&>(t).remove_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::MyDataItemPatchStruct");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::MyDataItemFieldPatchStruct> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).s_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::MyDataItemFieldPatchStruct");
    }
  }
};

template <>
struct VisitByFieldId<::test::fixtures::basic-python-capi::MyDataItemEnsureStruct> {
  template <typename F, typename T>
  void operator()(FOLLY_MAYBE_UNUSED F&& f, int32_t fieldId, FOLLY_MAYBE_UNUSED T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).s_ref());
    default:
      throwInvalidThriftId(fieldId, "::test::fixtures::basic-python-capi::MyDataItemEnsureStruct");
    }
  }
};
} // namespace detail
} // namespace thrift
} // namespace apache
