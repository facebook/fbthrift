/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

/**
 * Macros to be used by codegen'd X_types.h and X_types.cpp files, intended to
 * reduce the coupling between code generation and library code
 */
#define FROZEN_FIELD(NAME, ID, /*TYPE*/...) Field<__VA_ARGS__> NAME##Field;
#define FROZEN_FIELD_OPT(NAME, ID, /*TYPE*/...) \
  Field<folly::Optional<__VA_ARGS__>> NAME##Field;
#define FROZEN_FIELD_REQ FROZEN_FIELD

#define FROZEN_VIEW_FIELD(NAME, /*TYPE*/...)              \
  typename Layout<__VA_ARGS__>::View NAME() const {       \
    return this->layout_->NAME##Field.layout.view(        \
        this->position_(this->layout_->NAME##Field.pos)); \
  }
#define FROZEN_VIEW_FIELD_OPT(NAME, /*TYPE*/...)                     \
  typename Layout<folly::Optional<__VA_ARGS__>>::View NAME() const { \
    return this->layout_->NAME##Field.layout.view(                   \
        this->position_(this->layout_->NAME##Field.pos));            \
  }
#define FROZEN_VIEW_FIELD_REQ FROZEN_VIEW_FIELD
#define FROZEN_VIEW(...)                                     \
  struct View : public ViewBase<View, LayoutSelf, T> {       \
    View() {}                                                \
    View(const LayoutSelf* layout, ViewPosition position)    \
        : ViewBase<View, LayoutSelf, T>(layout, position) {} \
    __VA_ARGS__                                              \
  };                                                         \
  View view(ViewPosition self) const { return View(this, self); }

#define FROZEN_TYPE(TYPE, ...)                                               \
  template <>                                                                \
  struct Layout<TYPE, void> : public LayoutBase {                            \
    typedef Layout LayoutSelf;                                               \
    Layout();                                                                \
    typedef TYPE T;                                                          \
    FieldPosition layout(LayoutRoot& root, const T& x, LayoutPosition self); \
    void freeze(FreezeRoot& root, const T& x, FreezePosition self) const;    \
    void thaw(ViewPosition self, T& out) const;                              \
    void print(std::ostream& os, int level) const final;                  \
    void clear() final;                                                   \
    void save(schema::Schema&, schema::Layout&) const final;              \
    void load(const schema::Schema&, const schema::Layout&) final;        \
    __VA_ARGS__                                                              \
  };

#define FROZEN_CTOR_FIELD(NAME, ID) , NAME##Field(ID, #NAME)
#define FROZEN_CTOR_FIELD_OPT(NAME, ID) , NAME##Field(ID, #NAME)
#define FROZEN_CTOR_FIELD_REQ FROZEN_CTOR_FIELD
#define FROZEN_CTOR(TYPE, ...) \
  Layout<TYPE>::Layout() : LayoutBase(typeid(TYPE)) __VA_ARGS__ {}

#define FROZEN_LAYOUT_FIELD(NAME) \
  pos = root.layoutField(self, pos, this->NAME##Field, x.NAME);
#define FROZEN_LAYOUT_FIELD_OPT(NAME) \
  pos = root.layoutOptionalField(     \
      self, pos, this->NAME##Field, x.__isset.NAME, x.NAME);
#define FROZEN_LAYOUT_FIELD_REQ FROZEN_LAYOUT_FIELD

#define FROZEN_LAYOUT(TYPE, ...)                           \
  FieldPosition Layout<TYPE>::layout(                      \
      LayoutRoot& root, const T& x, LayoutPosition self) { \
    FieldPosition pos = startFieldPosition();              \
    __VA_ARGS__;                                           \
    return pos;                                            \
  }

#define FROZEN_FREEZE_FIELD(NAME) \
  root.freezeField(self, this->NAME##Field, x.NAME);
#define FROZEN_FREEZE_FIELD_OPT(NAME) \
  root.freezeOptionalField(self, this->NAME##Field, x.__isset.NAME, x.NAME);
#define FROZEN_FREEZE_FIELD_REQ FROZEN_FREEZE_FIELD

#define FROZEN_FREEZE(TYPE, ...)                                 \
  void Layout<TYPE>::freeze(                                     \
      FreezeRoot& root, const T& x, FreezePosition self) const { \
    __VA_ARGS__;                                                 \
  }

#define FROZEN_THAW_FIELD(NAME)                 \
  thawField(self, this->NAME##Field, out.NAME); \
  out.__isset.NAME = true;
#define FROZEN_THAW_FIELD_OPT(NAME) \
  thawField(self, this->NAME##Field, out.NAME, out.__isset.NAME);
#define FROZEN_THAW_FIELD_REQ(NAME) \
  thawField(self, this->NAME##Field, out.NAME);
#define FROZEN_THAW(TYPE, ...) \
  void Layout<TYPE>::thaw(ViewPosition self, T& out) const { __VA_ARGS__; }
#define FROZEN_DEBUG_FIELD(NAME) this->NAME##Field.print(os, level + 1);
#define FROZEN_DEBUG(TYPE, ...)                                 \
  void Layout<TYPE>::print(std::ostream& os, int level) const { \
    LayoutBase::print(os, level);                               \
    os << #TYPE;                                                \
    __VA_ARGS__                                                 \
  }
#define FROZEN_CLEAR_FIELD(NAME) this->NAME##Field.clear();
#define FROZEN_CLEAR(TYPE, ...) \
  void Layout<TYPE>::clear() {  \
    LayoutBase::clear();        \
    __VA_ARGS__                 \
  }
#define FROZEN_SAVE_FIELD(NAME) this->NAME##Field.save(schema, layout);
#define FROZEN_SAVE(TYPE, ...)                            \
  void Layout<TYPE>::save(schema::Schema& schema,         \
                          schema::Layout& layout) const { \
    LayoutBase::save(schema, layout);                     \
    __VA_ARGS__                                           \
  }
#define FROZEN_LOAD_FIELD(NAME) this->NAME##Field.load(schema, layout);
#define FROZEN_LOAD(TYPE, ...)                            \
  void Layout<TYPE>::load(const schema::Schema& schema,   \
                          const schema::Layout& layout) { \
    LayoutBase::load(schema, layout);                     \
    __VA_ARGS__                                           \
  }
