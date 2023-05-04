/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/compiler/parse/parse_ast.h>

#include <stdlib.h>
#include <cmath>
#include <cstddef>
#include <limits>
#include <set>

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <fmt/format.h>

#include <thrift/compiler/ast/node_list.h>
#include <thrift/compiler/ast/t_const_value.h>
#include <thrift/compiler/ast/t_field.h>
#include <thrift/compiler/ast/t_named.h>
#include <thrift/compiler/ast/t_node.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_program_bundle.h>
#include <thrift/compiler/ast/t_scope.h>
#include <thrift/compiler/ast/t_union.h>
#include <thrift/compiler/diagnostic.h>
#include <thrift/compiler/parse/lexer.h>
#include <thrift/compiler/parse/parser.h>
#include <thrift/compiler/source_location.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace {

// Cleans up text commonly found in doxygen-like comments.
//
// Warning: mixing tabs and spaces may mess up formatting.
std::string clean_up_doctext(std::string docstring) {
  // Convert to C++ string, and remove Windows's carriage returns.
  docstring.erase(
      remove(docstring.begin(), docstring.end(), '\r'), docstring.end());

  // Separate into lines.
  std::vector<std::string> lines;
  std::string::size_type pos = std::string::npos;
  std::string::size_type last;
  while (true) {
    last = (pos == std::string::npos) ? 0 : pos + 1;
    pos = docstring.find('\n', last);
    if (pos == std::string::npos) {
      // First bit of cleaning. If the last line is only whitespace, drop it.
      std::string::size_type nonwhite =
          docstring.find_first_not_of(" \t", last);
      if (nonwhite != std::string::npos) {
        lines.push_back(docstring.substr(last));
      }
      break;
    }
    lines.push_back(docstring.substr(last, pos - last));
  }

  // A very profound docstring.
  if (lines.empty()) {
    return {};
  }

  // Clear leading whitespace from the first line.
  pos = lines.front().find_first_not_of(" \t");
  lines.front().erase(0, pos);

  // If every nonblank line after the first has the same number of spaces/tabs,
  // then a comment prefix, remove them.
  enum class prefix {
    none = 0,
    star = 1, // length of '*'
    slashes = 3, // length of '///'
    inline_slash = 4, // length of '///<'
  };
  prefix found_prefix = prefix::none;
  std::string::size_type prefix_len = 0;
  std::vector<std::string>::iterator l_iter;

  // Start searching for prefixes from second line, since lexer already removed
  // initial prefix/suffix.
  for (l_iter = lines.begin() + 1; l_iter != lines.end(); ++l_iter) {
    if (l_iter->empty()) {
      continue;
    }

    pos = l_iter->find_first_not_of(" \t");
    if (found_prefix == prefix::none) {
      if (pos != std::string::npos) {
        if (l_iter->at(pos) == '*') {
          found_prefix = prefix::star;
          prefix_len = pos;
        } else if (l_iter->compare(pos, 4, "///<") == 0) {
          found_prefix = prefix::inline_slash;
          prefix_len = pos;
        } else if (l_iter->compare(pos, 3, "///") == 0) {
          found_prefix = prefix::slashes;
          prefix_len = pos;
        } else {
          found_prefix = prefix::none;
          break;
        }
      } else {
        // Whitespace-only line.  Truncate it.
        l_iter->clear();
      }
    } else if (
        l_iter->size() > pos && pos == prefix_len &&
        ((found_prefix == prefix::star && l_iter->at(pos) == '*') ||
         (found_prefix == prefix::inline_slash &&
          l_iter->compare(pos, 4, "///<") == 0) ||
         (found_prefix == prefix::slashes &&
          l_iter->compare(pos, 3, "///") == 0))) {
      // Business as usual
    } else if (pos == std::string::npos) {
      // Whitespace-only line.  Let's truncate it for them.
      l_iter->clear();
    } else {
      // The pattern has been broken.
      found_prefix = prefix::none;
      break;
    }
  }

  // If our prefix survived, delete it from every line.
  if (found_prefix != prefix::none) {
    // Get the prefix too.
    prefix_len += static_cast<int>(found_prefix);
    for (l_iter = lines.begin() + 1; l_iter != lines.end(); ++l_iter) {
      l_iter->erase(0, prefix_len);
    }
  }

  // Now delete the minimum amount of leading whitespace from each line.
  prefix_len = std::string::npos;
  for (l_iter = lines.begin() + 1; l_iter != lines.end(); ++l_iter) {
    if (l_iter->empty()) {
      continue;
    }
    pos = l_iter->find_first_not_of(" \t");
    if (pos != std::string::npos &&
        (prefix_len == std::string::npos || pos < prefix_len)) {
      prefix_len = pos;
    }
  }

  // If our whitespace prefix survived, delete it from every line.
  if (prefix_len != std::string::npos) {
    for (l_iter = lines.begin() + 1; l_iter != lines.end(); ++l_iter) {
      l_iter->erase(0, prefix_len);
    }
  }

  // Remove trailing whitespace from every line.
  for (l_iter = lines.begin(); l_iter != lines.end(); ++l_iter) {
    pos = l_iter->find_last_not_of(" \t");
    if (pos != std::string::npos && pos != l_iter->length() - 1) {
      l_iter->erase(pos + 1);
    }
  }

  // If the first line is empty, remove it.
  // Don't do this earlier because a lot of steps skip the first line.
  if (lines.front().empty()) {
    lines.erase(lines.begin());
  }

  // Now rejoin the lines and copy them back into doctext.
  docstring.clear();
  for (l_iter = lines.begin(); l_iter != lines.end(); ++l_iter) {
    docstring += *l_iter;
    docstring += '\n';
  }

  return docstring;
}

class ast_parser : public parser_actions {
 private:
  source_manager& source_mgr_;
  std::set<std::string> already_parsed_paths_;
  std::set<std::string> circular_deps_;

  diagnostics_engine& diags_;
  parsing_params params_;

  std::unordered_set<std::string> programs_that_parsed_definition_;

  // The last parsed doctext comment.
  boost::optional<comment> doctext_;

  enum class parsing_mode {
    INCLUDES = 1,
    PROGRAM = 2,
  };

  // The parsing pass that we are on. We do different things on each pass.
  parsing_mode mode_ = parsing_mode::INCLUDES;

  // The master program AST. This is accessed from within the parser code to
  // build up the program elements.
  t_program* program_;

  std::unique_ptr<t_program_bundle> program_bundle_;

  // Global scope cache for faster compilation.
  t_scope* scope_cache_;

  // A global map that holds a pointer to all programs already cached.
  std::map<std::string, t_program*> program_cache_;

  class lex_handler_impl : public lex_handler {
   private:
    ast_parser& parser_;

   public:
    explicit lex_handler_impl(ast_parser& p) : parser_(p) {}

    // Consume doctext and store it in `driver_.doctext`.
    //
    // It is non-trivial for a yacc-style LR(1) parser to accept doctext
    // as an optional prefix before either a definition or standalone at
    // the header. Hence this method of "pushing" it into the driver and
    // "pop-ing" it on the node as needed.
    void on_doc_comment(fmt::string_view text, source_location loc) override {
      parser_.clear_doctext();
      parser_.doctext_ = comment{parser_.strip_doctext(text), loc};
    }
  };

  // Parses a single .thrift file and populates program_ with the parsed AST.
  void parse_file() {
    // Skip already parsed files.
    const std::string& path = program_->path();
    if (!already_parsed_paths_.insert(path).second) {
      return;
    }

    auto src = source();
    try {
      src = source_mgr_.get_file(path);
    } catch (const std::runtime_error& e) {
      diags_.error(source_location(), "{}", e.what());
      end_parsing();
    }
    auto lex_handler = lex_handler_impl(*this);
    auto lexer = compiler::lexer(src, lex_handler, diags_);
    program_->set_src_range({src.start, src.start});

    // Create a new scope and scan for includes.
    diags_.report(
        src.start, diagnostic_level::info, "Scanning {} for includes\n", path);
    mode_ = parsing_mode::INCLUDES;
    if (!compiler::parse(lexer, *this, diags_)) {
      diags_.error(*program_, "Parser error during include pass.");
      end_parsing();
    }

    // Recursively parse all the included programs.
    const std::vector<t_include*>& includes = program_->includes();
    // Always enable allow_neg_field_keys when parsing included files.
    // This way if a thrift file has negative keys, --allow-neg-keys doesn't
    // have to be used by everyone that includes it.
    auto old_params = params_;
    auto old_program = program_;
    for (auto include : includes) {
      t_program* included_program = include->get_program();
      circular_deps_.insert(path);

      // Fail on circular dependencies.
      if (circular_deps_.count(included_program->path()) != 0) {
        diags_.error(
            *include,
            "Circular dependency found: file `{}` is already parsed.",
            included_program->path());
        end_parsing();
      }

      // This must be after the previous circular include check, since the
      // emitted error message above is supposed to reference the parent file
      // name.
      params_.allow_neg_field_keys = true;
      program_ = included_program;
      try {
        parse_file();
      } catch (...) {
        if (!params_.allow_missing_includes) {
          throw;
        }
      }

      size_t num_removed = circular_deps_.erase(path);
      (void)num_removed;
      assert(num_removed == 1);
    }
    params_ = old_params;
    program_ = old_program;

    // Parse the program file.
    try {
      src = source_mgr_.get_file(path);
    } catch (const std::runtime_error& e) {
      diags_.error(source_location(), "{}", e.what());
      end_parsing();
    }
    lexer = compiler::lexer(src, lex_handler, diags_);

    mode_ = parsing_mode::PROGRAM;
    diags_.report(
        src.start, diagnostic_level::info, "Parsing {} for types\n", path);
    if (!compiler::parse(lexer, *this, diags_)) {
      diags_.error(*program_, "Parser error during types pass.");
      end_parsing();
    }
  }

  class parsing_terminator : public std::runtime_error {
   public:
    parsing_terminator()
        : std::runtime_error(
              "internal exception for terminating the parsing process") {}
  };

  [[noreturn]] void end_parsing() { throw parsing_terminator(); }

  // Finds the path for the given include filename.
  std::string find_include_file(
      source_location loc, const std::string& filename) {
    // Absolute path? Just try that.
    boost::filesystem::path path(filename);
    if (path.has_root_directory()) {
      try {
        return boost::filesystem::canonical(path).string();
      } catch (const boost::filesystem::filesystem_error& e) {
        diags_.error(
            loc, "Could not find file: {}. Error: {}", filename, e.what());
        end_parsing();
      }
    }

    // Relative path, start searching
    // new search path with current dir global
    std::vector<std::string> sp = params_.incl_searchpath;
    auto dir = boost::filesystem::path(program_->path()).parent_path().string();
    dir = dir.empty() ? "." : dir;
    sp.insert(sp.begin(), std::move(dir));
    // Iterate through paths.
    std::vector<std::string>::iterator it;
    for (it = sp.begin(); it != sp.end(); it++) {
      boost::filesystem::path sfilename = filename;
      if ((*it) != "." && (*it) != "") {
        sfilename = boost::filesystem::path(*(it)) / filename;
      }
      if (boost::filesystem::exists(sfilename)) {
        return sfilename.string();
      }
#ifdef _WIN32
      // On Windows, handle files found at potentially long paths.
      sfilename = R"(\\?\)" +
          boost::filesystem::absolute(sfilename)
              .make_preferred()
              .lexically_normal()
              .string();
      if (boost::filesystem::exists(sfilename)) {
        return sfilename.string();
      }
#endif
      diags_.report(
          loc, diagnostic_level::debug, "Could not find: {}.", filename);
    }
    // File was not found.
    diags_.error(loc, "Could not find include file {}", filename);
    end_parsing();
  }

  void validate_header_location(source_location loc) {
    if (programs_that_parsed_definition_.find(program_->path()) !=
        programs_that_parsed_definition_.end()) {
      diags_.error(loc, "Headers must be specified before definitions.");
    }
  }

  // Checks that the constant name does not refer to an ambiguous enum.
  // An ambiguous enum is one that is redefined but not referred to by
  // ENUM_NAME.ENUM_VALUE.
  void validate_not_ambiguous_enum(
      source_location loc, const std::string& name) {
    if (scope_cache_->is_ambiguous_enum_value(name)) {
      std::string possible_enums =
          scope_cache_->get_fully_qualified_enum_value_names(name).c_str();
      diags_.warning(
          loc,
          "The ambiguous enum `{}` is defined in more than one place. "
          "Please refer to this enum using ENUM_NAME.ENUM_VALUE.{}",
          name,
          possible_enums.empty() ? "" : " Possible options: " + possible_enums);
    }
  }

  // Clears any previously stored doctext string and prints a warning if
  // information is discarded.
  void clear_doctext() {
    if (doctext_ && mode_ == parsing_mode::PROGRAM) {
      diags_.warning_legacy_strict(doctext_->loc, "uncaptured doctext");
    }
    doctext_ = boost::none;
  }

  // Returns a previously pushed doctext.
  boost::optional<comment> pop_doctext() {
    return mode_ == parsing_mode::PROGRAM ? std::exchange(doctext_, boost::none)
                                          : boost::none;
  }

  // Strips comment text and aligns leading whitespace on multiline doctext.
  std::string strip_doctext(fmt::string_view text) {
    if (mode_ != parsing_mode::PROGRAM) {
      return {};
    }

    std::string str(text.data(), text.size());
    if (str.compare(0, 3, "/**") == 0) {
      str = str.substr(3, str.length() - 3 - 2);
    } else if (str.compare(0, 3, "///") == 0) {
      str = str.substr(3, str.length() - 3);
    }
    if ((str.size() >= 1) && str[0] == '<') {
      str = str.substr(1, str.length() - 1);
    }

    return clean_up_doctext(str);
  }

  // Updates doctext of the given node.
  void set_doctext(t_node& node, boost::optional<comment> doc) const {
    if (!doc) {
      return;
    }
    // Concatenate prefix doctext with inline doctext via a newline
    // (discouraged).
    node.set_doc(
        node.has_doc() ? node.doc() + "\n" + std::move(doc->text)
                       : std::move(doc->text));
  }

  // Sets the annotations on the given node.
  static void set_annotations(
      t_node* node, std::unique_ptr<deprecated_annotations> annotations) {
    if (annotations) {
      node->reset_annotations(annotations->strings);
    }
  }

  // Sets the attributes on the given node.
  void set_attributes(
      t_named& node,
      std::unique_ptr<attributes> attrs,
      std::unique_ptr<deprecated_annotations> annots,
      const source_range& range) const {
    if (mode_ != parsing_mode::PROGRAM) {
      return;
    }
    node.set_src_range(range);
    if (attrs) {
      if (attrs->doc) {
        node.set_doc(std::move(attrs->doc->text));
      }
      for (auto& annotation : attrs->annotations) {
        node.add_structured_annotation(std::move(annotation));
      }
    }
    set_annotations(&node, std::move(annots));
  }

  // DEPRECATED! Adds an unnamed typedef to the program.
  const t_type* add_unnamed_typedef(
      std::unique_ptr<t_typedef> node,
      std::unique_ptr<deprecated_annotations> annotations,
      const source_range& range) {
    const t_type* result(node.get());
    node->set_src_range(range);
    set_annotations(node.get(), std::move(annotations));
    program_->add_unnamed_typedef(std::move(node));
    return result;
  }

  // DEPRECATED! Allocates a new field id using automatic numbering.
  //
  // Field ids are assigned starting from -1 and working their way down.
  void allocate_field_id(t_field_id& next_id, t_field& field) {
    if (params_.strict >= 192) {
      diags_.error(
          field,
          "Implicit field ids are deprecated and not allowed with -strict");
    }
    if (next_id < t_field::min_id) {
      diags_.error(
          field,
          "Cannot allocate an id for `{}`. Automatic field ids are exhausted.",
          field.name());
    }
    field.set_implicit_id(next_id--);
  }

  void maybe_allocate_field_id(t_field_id& next_id, t_field& field) {
    if (!field.explicit_id()) {
      // Auto assign an id.
      allocate_field_id(next_id, field);
      return;
    }

    // Check the explicitly provided id.
    if (field.id() > 0) {
      return;
    }
    if (params_.allow_neg_field_keys) {
      // allow_neg_field_keys exists to allow users to add explicitly specified
      // id values to old .thrift files without breaking protocol compatibility.
      if (field.id() != next_id) {
        diags_.warning(
            field,
            "Nonpositive field id ({}) differs from what would be "
            "auto-assigned by thrift ({}).",
            field.id(),
            next_id);
      }
    } else if (field.id() == next_id) {
      diags_.warning(
          field,
          "Nonpositive value ({}) not allowed as a field id.",
          field.id());
    } else {
      // TODO: Make ignoring the user provided value a failure.
      diags_.warning(
          field,
          "Nonpositive field id ({}) differs from what is auto-assigned by "
          "thrift. The id must be positive or {}.",
          field.id(),
          next_id);
      // Ignore user provided value and auto assign an id.
      allocate_field_id(next_id, field);
    }
    // Skip past any negative, manually assigned ids.
    if (field.id() < 0) {
      // Update the next field id to be one less than the value.
      // The field_list parsing will catch any duplicates.
      next_id = field.id() - 1;
    }
  }

  std::unique_ptr<t_const> new_struct_annotation(
      std::unique_ptr<t_const_value> const_struct, const source_range& range) {
    auto ttype = const_struct->ttype(); // Copy the t_type_ref.
    auto result = std::make_unique<t_const>(
        program_, std::move(ttype), "", std::move(const_struct));
    result->set_src_range(range);
    return result;
  }

  // Creates a reference to a known type, potentally with additional
  // annotations.
  t_type_ref new_type_ref(
      const t_type& type,
      std::unique_ptr<deprecated_annotations> annotations,
      const source_range& range = {}) {
    if (!annotations || mode_ != parsing_mode::PROGRAM) {
      return type;
    }

    // Make a copy of the node to hold the annotations.
    if (const auto* tbase_type = dynamic_cast<const t_base_type*>(&type)) {
      // Base types can be copy constructed.
      auto node = std::make_unique<t_base_type>(*tbase_type);
      set_annotations(node.get(), std::move(annotations));
      t_type_ref result(*node);
      program_->add_unnamed_type(std::move(node));
      return result;
    }

    // Containers always use a new type, so should never show up here.
    assert(!type.is_container());
    // For all other types, we can just create a dummy typedef node with
    // the same name. Note that this is not a safe assumption as it breaks all
    // dynamic casts and t_type::is_* calls.
    return *add_unnamed_typedef(
        t_typedef::make_unnamed(
            const_cast<t_program*>(type.program()), type.get_name(), type),
        std::move(annotations),
        range);
  }

  t_type_ref new_type_ref(
      t_type&& type, std::unique_ptr<deprecated_annotations>) = delete;

  // Creates a reference to a newly instantiated templated type.
  t_type_ref new_type_ref(
      source_range range,
      std::unique_ptr<t_templated_type> node,
      std::unique_ptr<deprecated_annotations> annotations) {
    if (mode_ != parsing_mode::PROGRAM) {
      return {};
    }

    assert(node != nullptr);
    const t_type* type = node.get();
    set_annotations(node.get(), std::move(annotations));
    node->set_src_range(range);
    program_->add_type_instantiation(std::move(node));
    return *type;
  }

  // Creates a reference to a named type.
  t_type_ref new_type_ref(
      std::string name,
      std::unique_ptr<deprecated_annotations> annotations,
      const source_range& range,
      bool is_const = false) {
    if (mode_ != parsing_mode::PROGRAM) {
      return {};
    }

    t_type_ref result = scope_cache_->ref_type(*program_, name, range);

    // TODO: Consider removing this special case for const, which requires a
    // specific declaration order.
    if (!result.resolved() && is_const) {
      diags_.error(
          range.begin,
          "The type '{}' is not defined yet. Types must be "
          "defined before the usage in constant values.",
          name);
    }

    if (auto* node = result.get_unresolved_type()) { // A newly created ph.
      set_annotations(node, std::move(annotations));
    } else if (annotations) {
      // TODO: Remove support for annotations on type references.
      return new_type_ref(result.deref(), std::move(annotations), range);
    }

    return result;
  }

  // Tries to set the given fields, reporting an error on a collision.
  void set_fields(t_structured& s, t_field_list&& fields) {
    if (mode_ != parsing_mode::PROGRAM) {
      return;
    }
    assert(s.fields().empty());
    t_field_id next_id = -1;
    for (auto& field : fields) {
      maybe_allocate_field_id(next_id, *field);
      if (!s.try_append_field(field)) {
        diags_.error(
            *field,
            "Field id {} for `{}` has already been used.",
            field->id(),
            field->name());
      }
    }
  }

  void set_functions(
      t_interface& node, std::unique_ptr<t_function_list> functions) {
    if (functions) {
      node.set_functions(std::move(*functions));
    }
  }

  void add_include(std::string name, const source_range& range) {
    if (mode_ != parsing_mode::INCLUDES) {
      return;
    }

    std::string path;
    try {
      path = find_include_file(range.begin, name);
    } catch (...) {
      if (!params_.allow_missing_includes) {
        throw;
      }
      path = name;
    }
    assert(!path.empty()); // Should have throw an exception if not found.

    if (program_cache_.find(path) == program_cache_.end()) {
      auto included_program = program_->add_include(path, name, range);
      program_cache_[path] = included_program.get();
      program_bundle_->add_program(std::move(included_program));
    } else {
      auto include =
          std::make_unique<t_include>(program_cache_[path], std::move(name));
      include->set_src_range(range);
      program_->add_include(std::move(include));
    }
  }

  template <typename T>
  T narrow_int(source_location loc, int64_t value, const char* name) {
    using limits = std::numeric_limits<T>;
    if (mode_ == parsing_mode::PROGRAM &&
        (value < limits::min() || value > limits::max())) {
      diags_.error(
          loc,
          "Integer constant {} outside the range of {} ([{}, {}]).",
          value,
          name,
          limits::min(),
          limits::max());
    }
    return value;
  }

 public:
  ast_parser(
      source_manager& sm,
      diagnostics_engine& diags,
      std::string path,
      parsing_params parse_params)
      : source_mgr_(sm), diags_(diags), params_(std::move(parse_params)) {
    program_bundle_ = std::make_unique<t_program_bundle>(
        std::make_unique<t_program>(std::move(path)));
    program_ = program_bundle_->root_program();
    scope_cache_ = program_->scope();
  }

  void on_program() override { clear_doctext(); }

  void on_standard_header(
      source_location loc, std::unique_ptr<attributes> attrs) override {
    validate_header_location(loc);
    if (attrs && !attrs->annotations.empty()) {
      diags_.error(
          *attrs->annotations.front(),
          "Structured annotations are not supported for a given entity.");
    }
  }

  void on_package(
      source_range range,
      std::unique_ptr<attributes> attrs,
      fmt::string_view name) override {
    validate_header_location(range.begin);
    if (mode_ != parsing_mode::PROGRAM) {
      return;
    }
    set_attributes(*program_, std::move(attrs), {}, range);
    if (!program_->package().empty()) {
      diags_.error(range.begin, "Package already specified.");
    }
    try {
      program_->set_package(t_package(fmt::to_string(name)));
    } catch (const std::exception& e) {
      diags_.error(range.begin, "{}", e.what());
    }
  }

  void on_include(source_range range, fmt::string_view str) override {
    add_include(fmt::to_string(str), range);
  }

  void on_cpp_include(source_range, fmt::string_view str) override {
    if (mode_ == parsing_mode::PROGRAM) {
      program_->add_language_include("cpp", fmt::to_string(str));
    }
  }

  void on_hs_include(source_range, fmt::string_view str) override {
    if (mode_ == parsing_mode::PROGRAM) {
      program_->add_language_include("hs", fmt::to_string(str));
    }
  }

  void on_namespace(const identifier& language, fmt::string_view ns) override {
    if (mode_ == parsing_mode::PROGRAM) {
      program_->set_namespace(fmt::to_string(language.str), fmt::to_string(ns));
    }
  }

  void on_definition(
      source_range range,
      std::unique_ptr<t_named> defn,
      std::unique_ptr<attributes> attrs,
      std::unique_ptr<deprecated_annotations> annotations) override {
    if (mode_ == parsing_mode::PROGRAM) {
      programs_that_parsed_definition_.insert(program_->path());
    }
    set_attributes(*defn, std::move(attrs), std::move(annotations), range);

    if (mode_ != parsing_mode::PROGRAM) {
      return;
    }

    // Add to scope.
    // TODO: Consider moving program-level scope management to t_program.
    if (auto* tnode = dynamic_cast<t_interaction*>(defn.get())) {
      scope_cache_->add_interaction(program_->scope_name(*defn), tnode);
    } else if (auto* tnode = dynamic_cast<t_service*>(defn.get())) {
      scope_cache_->add_service(program_->scope_name(*defn), tnode);
    } else if (auto* tnode = dynamic_cast<t_const*>(defn.get())) {
      scope_cache_->add_constant(program_->scope_name(*defn), tnode);
    } else if (auto* tnode = dynamic_cast<t_enum*>(defn.get())) {
      scope_cache_->add_type(program_->scope_name(*defn), tnode);
      // Register enum value names in scope.
      for (const auto& value : tnode->consts()) {
        // TODO: Remove the ability to access unscoped enum values.
        scope_cache_->add_constant(program_->scope_name(value), &value);
        scope_cache_->add_constant(program_->scope_name(*defn, value), &value);
      }
    } else if (auto* tnode = dynamic_cast<t_type*>(defn.get())) {
      auto* tnode_true_type = tnode->get_true_type();
      if (tnode_true_type && tnode_true_type->is_enum()) {
        for (const auto& value :
             static_cast<const t_enum*>(tnode_true_type)->consts()) {
          scope_cache_->add_constant(
              program_->scope_name(*defn, value), &value);
        }
      }
      scope_cache_->add_type(program_->scope_name(*defn), tnode);
    } else {
      throw std::logic_error("Unsupported declaration.");
    }
    // Add to program.
    program_->add_definition(std::move(defn));
  }

  boost::optional<comment> on_doctext() override { return pop_doctext(); }

  void on_program_doctext() override {
    // When there is any doctext, assign it to the top-level program.
    set_doctext(*program_, pop_doctext());
  }

  comment on_inline_doc(source_location loc, fmt::string_view text) override {
    return {strip_doctext(text), loc};
  }

  std::unique_ptr<t_const> on_structured_annotation(
      source_range range, fmt::string_view name) override {
    auto value = std::make_unique<t_const_value>();
    value->set_map();
    value->set_ttype(
        new_type_ref(fmt::to_string(name), nullptr, range, /*is_const=*/true));
    return new_struct_annotation(std::move(value), range);
  }

  std::unique_ptr<t_const> on_structured_annotation(
      source_range range, std::unique_ptr<t_const_value> value) override {
    return new_struct_annotation(std::move(value), range);
  }

  std::unique_ptr<t_service> on_service(
      source_range range,
      const identifier& name,
      const identifier& base,
      std::unique_ptr<t_function_list> functions) override {
    auto find_base_service = [&]() -> const t_service* {
      if (mode_ == parsing_mode::PROGRAM && base.str.size() != 0) {
        auto base_name = fmt::to_string(base.str);
        if (auto* result = scope_cache_->find_service(base_name)) {
          return result;
        }
        if (auto* result =
                scope_cache_->find_service(program_->scope_name(base_name))) {
          return result;
        }
        diags_.error(
            range.begin, "Service \"{}\" has not been defined.", base_name);
      }
      return nullptr;
    };
    auto service = std::make_unique<t_service>(
        program_, fmt::to_string(name.str), find_base_service());
    service->set_src_range(range);
    set_functions(*service, std::move(functions));
    return service;
  }

  std::unique_ptr<t_interaction> on_interaction(
      source_range range,
      const identifier& name,
      std::unique_ptr<t_function_list> functions) override {
    auto interaction =
        std::make_unique<t_interaction>(program_, fmt::to_string(name.str));
    interaction->set_src_range(range);
    set_functions(*interaction, std::move(functions));
    return interaction;
  }

  std::unique_ptr<t_function> on_function(
      source_range range,
      std::unique_ptr<attributes> attrs,
      t_function_qualifier qual,
      std::vector<t_type_ref> return_type,
      const identifier& name,
      t_field_list params,
      std::unique_ptr<t_throws> throws,
      std::unique_ptr<deprecated_annotations> annotations) override {
    auto function = std::make_unique<t_function>(
        program_, std::move(return_type), fmt::to_string(name.str));
    function->set_qualifier(qual);
    set_fields(function->params(), std::move(params));
    function->set_exceptions(std::move(throws));
    function->set_src_range(range);
    // TODO: Leave the param list unnamed.
    function->params().set_name(function->name() + "_args");
    set_attributes(*function, std::move(attrs), std::move(annotations), range);
    return function;
  }

  t_type_ref on_stream_return_type(
      source_range range, type_throws_spec spec) override {
    auto stream_response =
        std::make_unique<t_stream_response>(std::move(spec.type));
    stream_response->set_exceptions(std::move(spec.throws));
    return new_type_ref(range, std::move(stream_response), {});
  }

  t_type_ref on_sink_return_type(
      source_range range,
      type_throws_spec sink_spec,
      type_throws_spec final_response_spec) override {
    auto sink = std::make_unique<t_sink>(
        std::move(sink_spec.type), std::move(final_response_spec.type));
    sink->set_sink_exceptions(std::move(sink_spec.throws));
    sink->set_final_response_exceptions(std::move(final_response_spec.throws));
    return new_type_ref(range, std::move(sink), {});
  }

  t_type_ref on_list_type(
      source_range range,
      t_type_ref element_type,
      std::unique_ptr<deprecated_annotations> annotations) override {
    return new_type_ref(
        range,
        std::make_unique<t_list>(std::move(element_type)),
        std::move(annotations));
  }

  t_type_ref on_set_type(
      source_range range,
      t_type_ref key_type,
      std::unique_ptr<deprecated_annotations> annotations) override {
    return new_type_ref(
        range,
        std::make_unique<t_set>(std::move(key_type)),
        std::move(annotations));
  }

  t_type_ref on_map_type(
      source_range range,
      t_type_ref key_type,
      t_type_ref value_type,
      std::unique_ptr<deprecated_annotations> annotations) override {
    return new_type_ref(
        range,
        std::make_unique<t_map>(std::move(key_type), std::move(value_type)),
        std::move(annotations));
  }

  std::unique_ptr<t_function> on_performs(
      source_range range, t_type_ref type) override {
    std::string name = type.get_type() ? "create" + type.get_type()->get_name()
                                       : "<interaction placeholder>";
    auto function = std::make_unique<t_function>(
        program_, std::move(type), std::move(name));
    function->set_src_range(range);
    function->set_is_interaction_constructor();
    return function;
  }

  std::unique_ptr<t_throws> on_throws(t_field_list exceptions) override {
    auto result = std::make_unique<t_throws>();
    set_fields(*result, std::move(exceptions));
    return result;
  }

  std::unique_ptr<t_typedef> on_typedef(
      source_range range, t_type_ref type, const identifier& name) override {
    auto typedef_node = std::make_unique<t_typedef>(
        program_, fmt::to_string(name.str), std::move(type));
    typedef_node->set_src_range(range);
    return typedef_node;
  }

  std::unique_ptr<t_struct> on_struct(
      source_range range,
      const identifier& name,
      t_field_list fields) override {
    auto struct_node =
        std::make_unique<t_struct>(program_, fmt::to_string(name.str));
    struct_node->set_src_range(range);
    set_fields(*struct_node, std::move(fields));
    return struct_node;
  }

  std::unique_ptr<t_union> on_union(
      source_range range,
      const identifier& name,
      t_field_list fields) override {
    auto union_node =
        std::make_unique<t_union>(program_, fmt::to_string(name.str));
    union_node->set_src_range(range);
    set_fields(*union_node, std::move(fields));
    return union_node;
  }

  std::unique_ptr<t_exception> on_exception(
      source_range range,
      t_error_safety safety,
      t_error_kind kind,
      t_error_blame blame,
      const identifier& name,
      t_field_list fields) override {
    auto exception =
        std::make_unique<t_exception>(program_, fmt::to_string(name.str));
    exception->set_src_range(range);
    exception->set_safety(safety);
    exception->set_kind(kind);
    exception->set_blame(blame);
    set_fields(*exception, std::move(fields));
    return exception;
  }

  std::unique_ptr<t_field> on_field(
      source_range range,
      std::unique_ptr<attributes> attrs,
      boost::optional<int64_t> id,
      t_field_qualifier qual,
      t_type_ref type,
      const identifier& name,
      std::unique_ptr<t_const_value> value,
      std::unique_ptr<deprecated_annotations> annotations,
      boost::optional<comment> doc) override {
    auto valid_id = id ? narrow_int<t_field_id>(range.begin, *id, "field ids")
                       : boost::optional<t_field_id>();
    auto field = std::make_unique<t_field>(
        std::move(type), fmt::to_string(name.str), valid_id);
    field->set_qualifier(qual);
    if (mode_ == parsing_mode::PROGRAM) {
      field->set_default_value(std::move(value));
    }
    field->set_src_range(range);
    set_attributes(*field, std::move(attrs), std::move(annotations), range);
    if (doc) {
      set_doctext(*field, doc);
    }
    return field;
  }

  t_type_ref on_type(
      const t_base_type& type,
      std::unique_ptr<deprecated_annotations> annotations) override {
    return new_type_ref(type, std::move(annotations));
  }

  t_type_ref on_type(
      source_range range,
      fmt::string_view name,
      std::unique_ptr<deprecated_annotations> annotations) override {
    return new_type_ref(fmt::to_string(name), std::move(annotations), range);
  }

  std::unique_ptr<t_enum> on_enum(
      source_range range,
      const identifier& name,
      t_enum_value_list values) override {
    auto enum_node =
        std::make_unique<t_enum>(program_, fmt::to_string(name.str));
    enum_node->set_src_range(range);
    enum_node->set_values(std::move(values));
    return enum_node;
  }

  std::unique_ptr<t_enum_value> on_enum_value(
      source_range range,
      std::unique_ptr<attributes> attrs,
      const identifier& name,
      boost::optional<int64_t> value,
      std::unique_ptr<deprecated_annotations> annotations,
      boost::optional<comment> doc) override {
    auto enum_value = std::make_unique<t_enum_value>(fmt::to_string(name.str));
    enum_value->set_src_range(range);
    set_attributes(
        *enum_value, std::move(attrs), std::move(annotations), range);
    if (value) {
      enum_value->set_value(
          narrow_int<int32_t>(range.begin, *value, "enum values"));
    }
    if (doc) {
      set_doctext(*enum_value, std::move(doc));
    }
    return enum_value;
  }

  std::unique_ptr<t_const> on_const(
      source_range range,
      t_type_ref type,
      const identifier& name,
      std::unique_ptr<t_const_value> value) override {
    auto constant = std::make_unique<t_const>(
        program_, std::move(type), fmt::to_string(name.str), std::move(value));
    constant->set_src_range(range);
    return constant;
  }

  std::unique_ptr<t_const_value> on_const_ref(const identifier& name) override {
    auto find_const =
        [this](source_location loc, const std::string& name) -> const t_const* {
      validate_not_ambiguous_enum(loc, name);
      if (const t_const* constant = scope_cache_->find_constant(name)) {
        return constant;
      }
      if (const t_const* constant =
              scope_cache_->find_constant(program_->scope_name(name))) {
        validate_not_ambiguous_enum(loc, program_->scope_name(name));
        return constant;
      }
      return nullptr;
    };

    auto name_str = fmt::to_string(name.str);
    if (const t_const* constant = find_const(name.loc, name_str)) {
      // Copy const_value to perform isolated mutations.
      auto result = constant->get_value()->clone();
      // We only want to clone the value, while discarding all real type
      // information.
      result->set_ttype({});
      result->set_is_enum(false);
      result->set_enum(nullptr);
      result->set_enum_value(nullptr);
      return result;
    }

    // TODO: Make this an error.
    if (mode_ == parsing_mode::PROGRAM) {
      diags_.warning(
          name.loc,
          "The identifier '{}' is not defined yet. Constants and enums should "
          "be defined before using them as default values.",
          name.str);
    }
    return std::make_unique<t_const_value>(std::move(name_str));
  }

  std::unique_ptr<t_const_value> on_integer(
      source_location loc, int64_t value) override {
    if (mode_ == parsing_mode::PROGRAM && !params_.allow_64bit_consts &&
        (value < INT32_MIN || value > INT32_MAX)) {
      diags_.warning(
          loc, "64-bit constant {} may not work in all languages", value);
    }
    auto node = std::make_unique<t_const_value>();
    node->set_integer(value);
    return node;
  }

  std::unique_ptr<t_const_value> on_float(double value) override {
    auto const_value = std::make_unique<t_const_value>();
    const_value->set_double(value);
    return const_value;
  }

  std::unique_ptr<t_const_value> on_string_literal(std::string value) override {
    return std::make_unique<t_const_value>(std::move(value));
  }

  std::unique_ptr<t_const_value> on_bool_literal(bool value) override {
    auto const_value = std::make_unique<t_const_value>();
    const_value->set_bool(value);
    return const_value;
  }

  std::unique_ptr<t_const_value> on_list_literal() override {
    auto const_value = std::make_unique<t_const_value>();
    const_value->set_list();
    return const_value;
  }

  std::unique_ptr<t_const_value> on_map_literal() override {
    auto const_value = std::make_unique<t_const_value>();
    const_value->set_map();
    return const_value;
  }

  std::unique_ptr<t_const_value> on_struct_literal(
      source_range range, fmt::string_view name) override {
    auto const_value = std::make_unique<t_const_value>();
    const_value->set_map();
    const_value->set_ttype(
        new_type_ref(fmt::to_string(name), nullptr, range, /*is_const=*/true));
    return const_value;
  }

  int64_t on_integer(source_range range, sign s, uint64_t value) override {
    constexpr uint64_t max = std::numeric_limits<int64_t>::max();
    if (s == sign::minus) {
      if (mode_ == parsing_mode::PROGRAM && value > max + 1) {
        diags_.error(range.begin, "integer constant -{} is too small", value);
      }
      return -value;
    }
    if (mode_ == parsing_mode::PROGRAM && value > max) {
      diags_.error(range.begin, "integer constant {} is too large", value);
    }
    return value;
  }

  [[noreturn]] void on_error() override { end_parsing(); }

  std::unique_ptr<t_program_bundle> parse() {
    std::unique_ptr<t_program_bundle> result;
    try {
      parse_file();
      result = std::move(program_bundle_);
    } catch (const parsing_terminator&) {
      // No need to do anything here. The purpose of the exception is simply to
      // end the parsing process by unwinding to here.
    }
    return result;
  }
};

} // namespace

std::unique_ptr<t_program_bundle> parse_ast(
    source_manager& sm,
    diagnostics_engine& diags,
    std::string path,
    parsing_params parse_params) {
  return ast_parser(sm, diags, std::move(path), std::move(parse_params))
      .parse();
}

} // namespace compiler
} // namespace thrift
} // namespace apache