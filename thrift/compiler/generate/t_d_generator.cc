/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include <thrift/compiler/generate/t_oop_generator.h>

using namespace std;

namespace apache {
namespace thrift {
namespace compiler {

/*
 * If the name of an identifier in the object model
 * is a keword in D language, replacing
 *   obj->get_name()
 * with
 *   get_name(obj)
 * will append an underscore to the name thus solving the problem.
 */
template <class T>
string get_name(const T& obj) {
  static const set<string> reserved = {"version"};
  string name = obj->get_name();
  while (reserved.find(name) != reserved.end()) {
    name += "_";
  }
  return name;
}

/**
 * D code generator.
 *
 * generate_*() functions are called by the base class to emit code for the
 * given entity, print_*() functions write a piece of code to the passed
 * stream, and render_*() return a string containing the D representation of
 * the passed entity.
 */
class t_d_generator : public t_oop_generator {
 public:
  t_d_generator(
      t_program* program,
      t_generation_context context,
      const std::map<string, string>& parsed_options,
      const string& option_string)
      : t_oop_generator(program, std::move(context)) {
    (void)parsed_options;
    (void)option_string;
    out_dir_base_ = "gen-d";
  }

 protected:
  void init_generator() override {
    // Make output directory
    boost::filesystem::create_directory(get_out_dir());

    string dir = program_->get_namespace("d");
    string subdir = get_out_dir();
    string::size_type loc;
    while ((loc = dir.find('.')) != string::npos) {
      subdir = subdir + "/" + dir.substr(0, loc);
      boost::filesystem::create_directory(subdir);
      dir = dir.substr(loc + 1);
    }
    if (!dir.empty()) {
      subdir = subdir + "/" + dir;
      boost::filesystem::create_directory(subdir);
    }

    package_dir_ = subdir + "/";

    // Make output file
    string f_types_name = package_dir_ + program_name_ + "_types.d";
    f_types_.open(f_types_name.c_str());

    // Print header
    f_types_ << autogen_comment() << "module " << render_package(*program_)
             << program_name_ << "_types;" << endl
             << endl;

    print_default_imports(f_types_);

    // Include type modules from other imported programs.
    const vector<t_program*>& includes = program_->get_included_programs();
    for (size_t i = 0; i < includes.size(); ++i) {
      f_types_ << "import " << render_package(*(includes[i]))
               << includes[i]->get_name() << "_types;" << endl;
    }
    if (!includes.empty())
      f_types_ << endl;
  }

  void close_generator() override {
    // Close output file
    f_types_.close();
  }

  void generate_consts(std::vector<t_const*> consts) override {
    string f_consts_name = package_dir_ + program_name_ + "_constants.d";
    ofstream f_consts;
    f_consts.open(f_consts_name.c_str());

    f_consts << autogen_comment() << "module " << render_package(*program_)
             << program_name_ << "_constants;" << endl
             << endl;

    print_default_imports(f_consts);

    f_consts << "import " << render_package(*get_program()) << program_name_
             << "_types;" << endl
             << endl;

    vector<t_const*>::iterator c_iter;
    for (c_iter = consts.begin(); c_iter != consts.end(); ++c_iter) {
      string name = (*c_iter)->get_name();
      const t_type* type = (*c_iter)->get_type();
      indent(f_consts) << "immutable(" << render_type_name(type) << ") " << name
                       << ";" << endl;
    }

    f_consts << endl << "static this() {" << endl;
    indent_up();

    bool first = true;
    for (c_iter = consts.begin(); c_iter != consts.end(); ++c_iter) {
      if (first) {
        first = false;
      } else {
        f_consts << endl;
      }
      const t_type* type = (*c_iter)->get_type();
      indent(f_consts) << (*c_iter)->get_name() << " = ";
      if (!is_immutable_type(type)) {
        f_consts << "cast(immutable(" << render_type_name(type) << ")) ";
      }
      f_consts << render_const_value(type, (*c_iter)->get_value()) << ";"
               << endl;
    }
    indent_down();
    indent(f_consts) << "}" << endl;
  }

  void generate_typedef(const t_typedef* ttypedef) override {
    f_types_ << indent() << "alias " << render_type_name(ttypedef->get_type())
             << " " << ttypedef->get_symbolic() << ";" << endl
             << endl;
  }

  void generate_enum(const t_enum* tenum) override {
    vector<t_enum_value*> constants = tenum->get_enum_values();

    const string& enum_name = tenum->get_name();
    f_types_ << indent() << "enum " << enum_name << " {" << endl;

    indent_up();

    vector<t_enum_value*>::const_iterator c_iter;
    bool first = true;
    for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
      if (first) {
        first = false;
      } else {
        f_types_ << "," << endl;
      }
      indent(f_types_) << (*c_iter)->get_name();
      if ((*c_iter)->has_value()) {
        f_types_ << " = " << (*c_iter)->get_value();
      }
    }

    f_types_ << endl;
    indent_down();
    indent(f_types_) << "}" << endl;

    f_types_ << endl;
  }

  void generate_struct(const t_struct* tstruct) override {
    print_struct_definition(f_types_, tstruct, false);
  }

  void generate_xception(const t_struct* txception) override {
    print_struct_definition(f_types_, txception, true);
  }

  void generate_service(const t_service* tservice) override {
    string svc_name = tservice->get_name();

    // Service implementation file includes
    string f_servicename = package_dir_ + svc_name + ".d";
    std::ofstream f_service;
    f_service.open(f_servicename.c_str());
    f_service << autogen_comment() << "module " << render_package(*program_)
              << svc_name << ";" << endl
              << endl;

    print_default_imports(f_service);

    f_service << "import " << render_package(*get_program()) << program_name_
              << "_types;" << endl;

    // Include type modules from other imported programs.
    const vector<t_program*>& includes = program_->get_included_programs();

    for (size_t i = 0; i < includes.size(); ++i) {
      f_service << "import " << render_package(*(includes[i]))
                << includes[i]->get_name() << "_types;" << endl;
    }

    const t_service* extends_service = tservice->get_extends();
    if (extends_service != nullptr) {
      f_service << "import "
                << render_package(*(extends_service->get_program()))
                << extends_service->get_name() << ";" << endl;
    }

    f_service << endl;

    string extends = "";
    if (tservice->get_extends() != nullptr) {
      extends = " : " + render_type_name(tservice->get_extends());
    }

    f_service << indent() << "interface " << svc_name << extends << " {"
              << endl;
    indent_up();

    // Collect all the exception types service methods can throw so we can
    // emit the necessary aliases later.
    set<const t_type*> exception_types;

    // Print the method signatures.
    for (const auto* function : tservice->get_functions()) {
      f_service << indent();
      print_function_signature(f_service, function);
      f_service << ";" << endl;

      for (const auto* ex : function->get_xceptions()->fields()) {
        exception_types.insert(ex->get_type());
      }
    }

    // Alias the exception types into the current scope.
    if (!exception_types.empty())
      f_service << endl;
    for (const auto* et : exception_types) {
      indent(f_service) << "alias " << render_package(*et->get_program())
                        << et->get_program()->get_name() << "_types"
                        << "." << et->get_name() << " " << et->get_name() << ";"
                        << endl;
    }

    // Write the method metadata.
    ostringstream meta;
    indent_up();
    auto func_delim = "";
    for (const auto* function : tservice->get_functions()) {
      if (function->get_paramlist()->fields().empty() &&
          function->get_xceptions()->fields().empty() &&
          !function->is_oneway()) {
        continue;
      }
      meta << func_delim << endl
           << indent() << "TMethodMeta(`" << function->get_name() << "`, "
           << endl;
      func_delim = ",";
      indent_up();
      indent(meta) << "[";

      auto param_delim = "";
      for (const auto* param : function->get_paramlist()->fields()) {
        meta << param_delim << "TParamMeta(`" << param->get_name() << "`, "
             << param->get_key();
        param_delim = ", ";
        if (const t_const_value* cv = param->get_value()) {
          meta << ", q{" << render_const_value(param->get_type(), cv) << "}";
        }
        meta << ")";
      }

      meta << "]";

      if (!function->get_xceptions()->fields().empty() ||
          function->is_oneway()) {
        meta << "," << endl << indent() << "[";
        auto ex_delim = "";
        for (const auto* ex : function->get_xceptions()->fields()) {
          meta << ex_delim << "TExceptionMeta(`" << ex->get_name() << "`, "
               << ex->get_key() << ", `" << ex->get_type()->get_name() << "`)";
          ex_delim = ", ";
        }

        meta << "]";
      }

      if (function->is_oneway()) {
        meta << "," << endl << indent() << "TMethodType.ONEWAY";
      }

      indent_down();
      meta << endl << indent() << ")";
    }
    indent_down();

    string meta_str(meta.str());
    if (!meta_str.empty()) {
      f_service << endl
                << indent() << "enum methodMeta = [" << meta_str << endl
                << indent() << "];" << endl;
    }

    indent_down();
    indent(f_service) << "}" << endl;

    // Server skeleton generation.
    string f_skeletonname = package_dir_ + svc_name + "_server.skeleton.d";
    std::ofstream f_skeleton;
    f_skeleton.open(f_skeletonname.c_str());
    print_server_skeleton(f_skeleton, tservice);
    f_skeleton.close();
  }

 private:
  /**
   * Writes a server skeleton for the passed service to out.
   */
  void print_server_skeleton(ostream& out, const t_service* tservice) {
    const string& svc_name = tservice->get_name();

    out << "/*" << endl
        << " * This auto-generated skeleton file illustrates how to build a server. If you"
        << endl
        << " * intend to customize it, you should edit a copy with another file name to "
        << endl
        << " * avoid overwriting it when running the generator again." << endl
        << " */" << endl
        << "module " << render_package(*tservice->get_program()) << svc_name
        << "_server;" << endl
        << endl
        << "import std.stdio;" << endl
        << "import thrift.codegen.processor;" << endl
        << "import thrift.protocol.binary;" << endl
        << "import thrift.server.simple;" << endl
        << "import thrift.server.transport.socket;" << endl
        << "import thrift.transport.buffered;" << endl
        << "import thrift.util.hashset;" << endl
        << endl
        << "import " << render_package(*tservice->get_program()) << svc_name
        << ";" << endl
        << "import " << render_package(*get_program()) << program_name_
        << "_types;" << endl
        << endl
        << endl
        << "class " << svc_name << "Handler : " << svc_name << " {" << endl;

    indent_up();
    out << indent() << "this() {" << endl
        << indent() << "  // Your initialization goes here." << endl
        << indent() << "}" << endl
        << endl;

    vector<t_function*> functions = tservice->get_functions();
    vector<t_function*>::iterator f_iter;
    for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
      out << indent();
      print_function_signature(out, *f_iter);
      out << " {" << endl;

      indent_up();

      out << indent() << "// Your implementation goes here." << endl
          << indent() << "writeln(\"" << (*f_iter)->get_name() << " called\");"
          << endl;

      if (!(*f_iter)->get_returntype()->is_void()) {
        indent(out) << "return typeof(return).init;" << endl;
      }

      indent_down();

      out << indent() << "}" << endl << endl;
    }

    indent_down();
    out << "}" << endl << endl;

    out << indent() << "void main() {" << endl;
    indent_up();
    out << indent() << "auto protocolFactory = new TBinaryProtocolFactory!();"
        << endl
        << indent() << "auto processor = new TServiceProcessor!" << svc_name
        << "(new " << svc_name << "Handler);" << endl
        << indent() << "auto serverTransport = new TServerSocket(9090);" << endl
        << indent() << "auto transportFactory = new TBufferedTransportFactory;"
        << endl
        <<

        indent() << "auto server = new TSimpleServer(" << endl
        << indent()
        << "  processor, serverTransport, transportFactory, protocolFactory);"
        << endl
        << indent() << "server.serve();" << endl;
    indent_down();
    out << "}" << endl;
  }

  /**
   * Writes the definition of a struct or an exception type to out.
   */
  void print_struct_definition(
      ostream& out,
      const t_struct* tstruct,
      bool is_exception) {
    if (is_exception) {
      indent(out) << "class " << tstruct->get_name() << " : TException {"
                  << endl;
    } else {
      indent(out) << "struct " << tstruct->get_name() << " {" << endl;
    }
    indent_up();

    { // separate scope to avoid shadowing "m_iter" below.
      // Declare all fields.
      for (const auto* field : tstruct->fields()) {
        indent(out) << render_type_name(field->get_type()) << " "
                    << get_name(field) << ";" << endl;
      }
    }

    if (tstruct->has_fields()) {
      indent(out) << endl;
    }
    indent(out) << "mixin TStructHelpers!(";

    if (tstruct->has_fields()) {
      // If there are any fields, construct the TFieldMeta array to pass to
      // TStructHelpers. We can't just pass an empty array if not because []
      // doesn't pass the TFieldMeta[] constraint.
      out << "[";
      indent_up();
      auto delim = "";
      for (const auto* field : tstruct->fields()) {
        out << delim << endl;
        delim = ",";
        indent(out) << "TFieldMeta(`" << get_name(field) << "`, "
                    << field->get_key();

        const t_const_value* cv = field->get_value();
        t_field::e_req req = field->get_req();
        out << ", " << render_req(req);
        if (cv != nullptr) {
          out << ", q{" << render_const_value(field->get_type(), cv) << "}";
        }
        out << ")";
      }

      indent_down();
      out << endl << indent() << "]";
    }

    out << ");" << endl;

    indent_down();
    indent(out) << "}" << endl << endl;
  }

  /**
   * Prints the D function signature (including return type) for the given
   * method.
   */
  void print_function_signature(ostream& out, const t_function* fn) {
    out << render_type_name(fn->get_returntype()) << " " << fn->get_name()
        << "(";
    auto delim = "";
    for (const auto* param : fn->get_paramlist()->fields()) {
      out << delim << render_type_name(param->get_type(), true) << " "
          << get_name(param);
      delim = ", ";
    }
    out << ")";
  }

  /**
   * Returns the D representation of value. The result is guaranteed to be a
   * single expression; for complex types, immediately called delegate
   * literals are used to achieve this.
   */
  string render_const_value(const t_type* type, const t_const_value* value) {
    // Resolve any typedefs.
    type = type->get_true_type();

    ostringstream out;
    if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
      switch (tbase) {
        case t_base_type::TYPE_STRING:
        case t_base_type::TYPE_BINARY:
          out << '"' << get_escaped_string(value) << '"';
          break;
        case t_base_type::TYPE_BOOL:
          out << ((value->get_integer() > 0) ? "true" : "false");
          break;
        case t_base_type::TYPE_BYTE:
        case t_base_type::TYPE_I16:
          out << "cast(" << render_type_name(type) << ")"
              << value->get_integer();
          break;
        case t_base_type::TYPE_I32:
          out << value->get_integer();
          break;
        case t_base_type::TYPE_I64:
          out << value->get_integer() << "L";
          break;
        case t_base_type::TYPE_DOUBLE:
          if (value->get_type() == t_const_value::CV_INTEGER) {
            out << value->get_integer();
          } else {
            out << value->get_double();
          }
          break;
        default:
          throw std::runtime_error(
              "Compiler error: No const of base type " +
              t_base_type::t_base_name(tbase));
      }
    } else if (type->is_enum()) {
      out << "cast(" << render_type_name(type) << ")" << value->get_integer();
    } else {
      out << "{" << endl;
      indent_up();

      indent(out) << render_type_name(type) << " v;" << endl;
      if (type->is_struct() || type->is_xception()) {
        indent(out) << "v = " << (type->is_xception() ? "new " : "")
                    << render_type_name(type) << "();" << endl;
        const auto* as_struct = static_cast<const t_struct*>(type);
        for (const auto& entry : value->get_map()) {
          const auto* field =
              as_struct->get_field_by_name(entry.first->get_string());
          if (field == nullptr) {
            throw std::runtime_error(
                "Type error: " + type->get_name() + " has no field " +
                entry.first->get_string());
          }
          string val = render_const_value(field->get_type(), entry.second);
          indent(out) << "v.set!`" << entry.first->get_string() << "`(" << val
                      << ");" << endl;
        }
      } else if (type->is_map()) {
        const t_type* ktype = ((t_map*)type)->get_key_type();
        const t_type* vtype = ((t_map*)type)->get_val_type();
        const vector<pair<t_const_value*, t_const_value*>>& val =
            value->get_map();
        vector<pair<t_const_value*, t_const_value*>>::const_iterator v_iter;
        for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
          string key = render_const_value(ktype, v_iter->first);
          string val = render_const_value(vtype, v_iter->second);
          indent(out) << "v[";
          if (!is_immutable_type(ktype)) {
            out << "cast(immutable(" << render_type_name(ktype) << "))";
          }
          out << key << "] = " << val << ";" << endl;
        }
      } else if (type->is_list()) {
        const auto* as_list = static_cast<const t_list*>(type)->get_elem_type();
        for (const auto* val : value->get_list()) {
          indent(out) << "v ~= " << render_const_value(as_list, val) << ";"
                      << endl;
        }
      } else if (type->is_set()) {
        const t_type* etype = ((t_set*)type)->get_elem_type();
        const vector<t_const_value*>& val = value->get_list();
        vector<t_const_value*>::const_iterator v_iter;
        for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
          string val = render_const_value(etype, *v_iter);
          indent(out) << "v ~= " << val << ";" << endl;
        }
      } else {
        throw std::runtime_error(
            "Compiler error: Invalid type in render_const_value: " +
            type->get_name());
      }
      indent(out) << "return v;" << endl;

      indent_down();
      indent(out) << "}()";
    }

    return out.str();
  }

  /**
   * Returns the D package to which modules for program are written (with a
   * trailing dot, if not empty).
   */
  string render_package(const t_program& program) const {
    string package = program.get_namespace("d");
    if (package.size() == 0)
      return "";
    return package + ".";
  }

  /**
   * Returns the name of the D repesentation of ttype.
   *
   * If isArg is true, a const reference to the type will be returned for
   * structs.
   */
  string render_type_name(const t_type* ttype, bool isArg = false) const {
    if (ttype->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type*)ttype)->get_base();
      switch (tbase) {
        case t_base_type::TYPE_VOID:
          return "void";
        case t_base_type::TYPE_STRING:
        case t_base_type::TYPE_BINARY:
          return "string";
        case t_base_type::TYPE_BOOL:
          return "bool";
        case t_base_type::TYPE_BYTE:
          return "byte";
        case t_base_type::TYPE_I16:
          return "short";
        case t_base_type::TYPE_I32:
          return "int";
        case t_base_type::TYPE_I64:
          return "long";
        case t_base_type::TYPE_DOUBLE:
          return "double";
        default:
          throw std::runtime_error(
              "Compiler error: No D type name for base type " +
              t_base_type::t_base_name(tbase));
      }
    }

    if (ttype->is_container()) {
      const t_container* tcontainer = (t_container*)ttype;
      if (tcontainer->has_cpp_name()) {
        return tcontainer->get_cpp_name();
      } else if (ttype->is_map()) {
        const t_map* tmap = (t_map*)ttype;
        const t_type* ktype = tmap->get_key_type();

        string name = render_type_name(tmap->get_val_type()) + "[";
        if (!is_immutable_type(ktype)) {
          name += "immutable(";
        }
        name += render_type_name(ktype);
        if (!is_immutable_type(ktype)) {
          name += ")";
        }
        name += "]";
        return name;
      } else if (ttype->is_set()) {
        const t_set* tset = (t_set*)ttype;
        return "HashSet!(" + render_type_name(tset->get_elem_type()) + ")";
      } else if (ttype->is_list()) {
        const t_list* tlist = (t_list*)ttype;
        return render_type_name(tlist->get_elem_type()) + "[]";
      }
    }

    string type_name;
    const t_program* type_program = ttype->get_program();
    if (type_program != get_program()) {
      if (ttype->is_service()) {
        type_name = render_package(*type_program) + ttype->get_name() + "." +
            ttype->get_name();
      } else {
        type_name = render_package(*type_program) + type_program->get_name() +
            "_types." + ttype->get_name();
      }
    } else {
      type_name = ttype->get_name();
    }

    if (ttype->is_struct() && isArg) {
      return "ref const(" + type_name + ")";
    } else {
      return type_name;
    }
  }

  /**
   * Returns the D TReq enum member corresponding to req.
   */
  string render_req(t_field::e_req req) const {
    switch (req) {
      case t_field::e_req::opt_in_req_out:
        return "TReq.OPT_IN_REQ_OUT";
      case t_field::e_req::optional:
        return "TReq.OPTIONAL";
      case t_field::e_req::required:
        return "TReq.REQUIRED";
      default:
        throw std::runtime_error(
            "Compiler error: Invalid requirement level: " +
            std::to_string(static_cast<int>(req)));
    }
  }

  /**
   * Writes the default list of imports (which are written to every generated
   * module) to f.
   */
  void print_default_imports(ostream& out) {
    indent(out) << "import thrift.base;" << endl
                << "import thrift.codegen.base;" << endl
                << "import thrift.util.hashset;" << endl
                << endl;
  }

  /**
   * Returns whether type is "intrinsically immutable", in the sense that
   * a value of that type is implicitly castable to immutable(type), and it is
   * allowed for AA keys without an immutable() qualifier.
   */
  bool is_immutable_type(const t_type* type) const {
    const t_type* ttype = type->get_true_type();
    return ttype->is_base_type() || ttype->is_enum();
  }

  /*
   * File streams, stored here to avoid passing them as parameters to every
   * function.
   */
  ofstream f_types_;
  ofstream f_header_;

  string package_dir_;
};

THRIFT_REGISTER_GENERATOR(d, "D", "");

} // namespace compiler
} // namespace thrift
} // namespace apache
