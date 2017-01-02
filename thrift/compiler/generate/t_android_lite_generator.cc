/*
 * Copyright 2016 Facebook, Inc.
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

#include <sstream>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#include <thrift/compiler/platform.h>
#include <thrift/compiler/generate/t_java_generator.h>

using namespace std;

/**
 * Android code generator. Legacy versions of Android have a strict limit on
 * the number of methods allowed per executable (65,536, or 2^16). The standard
 * Java thrift compiler isn't optimized for Android, so it creates tons of
 * methods for each Thrift object -- this makes it essentially unusable for
 * already large Android applications.
 * The Android-lite implementation here avoids the method cap by cramming most
 * of the logic into one very long method.
 *
 */
class t_android_lite_generator : public t_java_generator {
  public:
    t_android_lite_generator(
        t_program* program,
        const std::map<std::string, std::string>& parsed_options,
        const std::string& option_string)
      : t_java_generator(program, parsed_options, option_string)
    {
      // parse arguments
      package_name_ = program_->get_namespace("android_lite");
      program_name_ = capitalize(get_program()->get_name());
      out_dir_base_ = "gen-android";
      annotate_ = option_is_specified(parsed_options, "annotate");
    }

    void generate_consts(vector<t_const*> tconsts) override;
    void generate_enum(t_enum* tenum) override;
    void generate_service(t_service* tservice) override;
    void generate_struct(t_struct* tstruct) override;
    void generate_typedef(t_typedef* ttypedef) override;
    void generate_xception(t_struct* txception) override;

    void init_generator() override;
    void close_generator() override;

    string java_type_imports() override;
    const string& get_package_dir() override;
    string type_name(t_type* ttype,
                     bool in_container=false,
                     bool in_init=false,
                     bool skip_generic=false) override;

    virtual void print_const_value(
        std::ostream& out,
        std::string name,
        t_type* type,
        const t_const_value* value,
        bool in_static,
        bool defval=false) override;
    string render_const_value(
        ostream& out,
        string name,
        t_type* type,
        const t_const_value* value) override;

    void output_case_statement(t_struct *tstruct);
    void output_property(t_field* tfield, const string parent_name);
    void output_case_body_union(t_struct *tunion);
    void output_case_body_struct(t_struct *tstruct);

    void output_write(t_list* tlist,
                      const string value,
                      int depth,
                      bool needs_cast,
                      stringstream& stream);

    void output_write(t_map* tmap,
                      const string value,
                      int depth,
                      bool needs_cast,
                      stringstream& stream);


    void output_write(t_set* tset,
                      const string value,
                      int depth,
                      bool needs_cast,
                      stringstream& stream);

    void output_write(t_struct* tstruct,
                      const string value,
                      int depth,
                      bool needs_cast,
                      stringstream& stream);

    void output_write(t_type* type,
                      const string value,
                      int depth,
                      bool needs_cast,
                      stringstream& stream);

    void output_write(t_enum* tenum,
                      const string value,
                      int depth,
                      bool needsCast,
                      stringstream& stream);

    void write_class_file();
    void write_logger_file();
    void write_enum_file();

    string android_thrift_imports();
    string package_header();
    string temp_variable(const string& prefix, int postfix);
    string full_property_name(t_struct* tstruct, t_field* tfield);
    string full_enum_name(t_enum* tenum, int offset);

    const string logger_name();
    const string enum_name();
    void record_type_use(t_type* ttype);


  private:
    string package_name_;
    string program_name_;
    string package_dir_;
    bool annotate_;
    set<string> used_types_;

    // We build up the text of the 3 files in these streams before
    // outputting them into their actual files all in one go.
    vector<string> enum_defns_;
    stringstream class_defns_;
    stringstream switch_stmts_;
};

const string& t_android_lite_generator::get_package_dir() {
  return package_dir_;
}

string t_android_lite_generator::temp_variable(const string& prefix,
    int postfix) {
  ostringstream stream;
  stream << prefix << postfix;
  return stream.str();
}

void t_android_lite_generator::init_generator() {
  make_dir(get_out_dir().c_str());
  string dir = package_name_;
  string subdir = get_out_dir();
  string::size_type loc;

  while((loc = dir.find(".")) != string::npos) {
    subdir = subdir + "/" + dir.substr(0, loc);
    make_dir(subdir.c_str());
    dir = dir.substr(loc + 1);
  }
  if (dir.size() > 0) {
    subdir = subdir + "/" + dir;
    make_dir(subdir.c_str());
  }

  package_dir_ = subdir;
};

void t_android_lite_generator::write_logger_file() {
  string file_name;
  ofstream out_logger;
  file_name = package_dir_ + "/" + logger_name() + ".java";
  out_logger.open(file_name.c_str());

  out_logger << autogen_comment() << package_header() << endl;

  // These 3 types are always used in the logger file. We don't want to import
  // them in the class file, so we remove them from the set after printing.
  // `Map` might be needed in the class file, so we shouldn't always remove it.
  bool should_remove = used_types_.insert("java.util.Map").second;
  used_types_.insert("java.io.IOException");
  used_types_.insert("java.util.HashMap");

  out_logger << java_type_imports() << endl
             << android_thrift_imports() << endl
             << endl; // empty line at end

  if (should_remove) {
    used_types_.erase("java.util.Map");
  }
  used_types_.erase("java.io.IOException");
  used_types_.erase("java.util.HashMap");

  out_logger << "public class " <<  logger_name() << " {" << endl
             << endl;

  indent_up();
  indent(out_logger) << "public final " << program_name_ <<
                        ".EventType mEventType;" << endl << endl;

  indent(out_logger) << "private final Map<ThriftProperty<?>, Object> mMap" <<
      " = new HashMap<ThriftProperty<?>, Object>();" << endl << endl;

  indent(out_logger) << "public " << logger_name() << "(" <<
      program_name_ << ".EventType type) {" << endl;
  indent_up();
  indent(out_logger) << "mEventType = type;" << endl;
  indent_down();
  indent(out_logger) << "}" << endl << endl;

  indent(out_logger) << "public <T> " << logger_name() <<
      " addProperty(ThriftProperty<T> property, T value) {" << endl;
  indent_up();
  indent(out_logger) << "mMap.put(property, value);" << endl;
  indent(out_logger) << "return this;" << endl;
  indent_down();
  indent(out_logger) << "}" << endl << endl;

  indent(out_logger) << "public static <T> void writeFieldBegin("
      "TBinaryProtocol oprot, ThriftProperty<T> field) throws IOException {" <<
      endl;
  indent_up();
  indent(out_logger) << "TField tField = new TField(field.key, field.type, "
      "field.id);" << endl;
  indent(out_logger) << "oprot.writeFieldBegin(tField);" << endl;
  indent_down();
  indent(out_logger) << "}" << endl << endl;

  indent(out_logger) << "public void write(TBinaryProtocol oprot) throws "
      "IOException {" << endl;
  indent_up();
  indent(out_logger) << "switch (mEventType) {" << endl;

  indent_up();
  string line;
  while(!switch_stmts_.eof()) {
    getline(switch_stmts_, line);
    indent(out_logger) << line << endl;
  }
  indent_down();

  indent(out_logger) << "}" << endl; // close switch
  indent_down();
  indent(out_logger) << "}" << endl; // close method
  indent_down();
  indent(out_logger) << "}" << endl; // close class

  out_logger.close();
}

void t_android_lite_generator::write_class_file() {
  string class_name;
  ofstream out_class;
  class_name = package_dir_ + "/" + program_name_ + ".java";
  out_class.open(class_name.c_str());

  out_class << autogen_comment() << package_header() << endl;

  out_class << java_type_imports() << endl
             << android_thrift_imports() << endl
             << endl; // empty line at end

  out_class << "public class " << program_name_ << " {" << endl
            << endl;

  indent_up();
  // Break the abstraction a little so that we can output all the enums in one
  // list at the top of the file.
  vector<t_struct*> structs = get_program()->get_structs();
  indent(out_class) << "public enum EventType {" << endl;
  indent_up();
  if (!structs.empty()) {
    vector<t_struct*>::iterator st_iter = structs.begin();
    indent(out_class) << (*st_iter)->get_name();
    for(++st_iter; st_iter != structs.end(); ++st_iter) {
      out_class << ", " << (*st_iter)->get_name();
    }
    out_class << ";" << endl;
  }
  indent_down();
  indent(out_class) << "}" << endl << endl;

  string line;
  while(!class_defns_.eof()) {
    getline(class_defns_, line);
    indent(out_class) << line << endl;
  }

  indent_down();
  out_class << "}" << endl;

  out_class.close();
}

void t_android_lite_generator::write_enum_file() {
  if (enum_defns_.empty()) {
    return;
  }
  ofstream out_enum;
  string file_name = package_dir_ + "/" + enum_name() + ".java";
  out_enum.open(file_name.c_str());

  out_enum << autogen_comment() << package_header() << endl << endl;

  out_enum << "public enum " << enum_name() << " {" << endl;
  indent_up();
  string line;
  vector<string>::const_iterator iter = enum_defns_.cbegin();
  string first = *iter;
  indent(out_enum) << first;
  ++iter;
  for( ; iter != enum_defns_.cend(); ++iter) {
    out_enum << "," << endl << indent() << *iter;
  }
  out_enum << ";" << endl << endl;

  // Definitions to make this class work.
  indent(out_enum) << "private final int mValue;" << endl;
  indent(out_enum) << "private " << enum_name() << "(int value) {" <<
    endl;
  indent_up();
  indent(out_enum) << "mValue = value;" << endl;
  indent_down();
  indent(out_enum) << "}" << endl;
  indent(out_enum) << "public int getValue() {" << endl;
  indent_up();
  indent(out_enum) << "return mValue;" << endl;
  indent_down();
  indent(out_enum) << "}" << endl;

  indent_down();
  out_enum << "}" << endl;
  out_enum.close();
}

void t_android_lite_generator::record_type_use(t_type* ttype) {
  if (ttype->is_set()) {
   used_types_.insert("java.util.Set");
   record_type_use(((t_set*) ttype)->get_elem_type());
  } else if (ttype->is_list()) {
   used_types_.insert("java.util.List");
   record_type_use(((t_list*) ttype)->get_elem_type());
  } else if (ttype->is_map()) {
   used_types_.insert("java.util.Map");
   record_type_use(((t_map*) ttype)->get_key_type());
   record_type_use(((t_map*) ttype)->get_val_type());
  }
}

string t_android_lite_generator::java_type_imports() {
  std::string type_imports;
  for (const string& used_type : used_types_) {
    type_imports += "import " + used_type + ";\n";
  }
  return type_imports;
}

// When we open-source the android compiler, we'll need to also release
// the accompanying thrift library for android imported here.
string t_android_lite_generator::android_thrift_imports() {
  string imports =
    "import com.facebook.thrift.lite.*;\n"
    "import com.facebook.thrift.lite.protocol.*;\n";
  if (annotate_) {
    imports +=
      "import com.facebook.thrift.lite.annotations.*;\n";
  }
  return imports;
}

string t_android_lite_generator::package_header() {
  if (package_name_.length() == 0) {
    return "\n";
  } else {
    return "package " + package_name_ + ";\n";
  }
}

string t_android_lite_generator::type_name(t_type* ttype, bool in_container,
    bool in_init, bool skip_generic) {
  ttype = get_true_type(ttype);
  if (ttype->is_struct() || ttype->is_enum()) {
    string suffix = ttype->is_struct() ? "Logger" : "Enum";
    string name = capitalize(ttype->get_program()->get_name()) + suffix;
    string package = ttype->get_program()->get_namespace("android_lite");
    return package.empty() || package == package_name_
      ? name : package + "." + name;
  } else {
    return t_java_generator::type_name(ttype, in_container, in_init,
        skip_generic);
  }
}

string t_android_lite_generator::full_property_name(t_struct* tstruct,
    t_field* tfield) {
  return capitalize(tstruct->get_program()->get_name()) + "." +
    tstruct->get_name() + "_" + tfield->get_name();
}

string t_android_lite_generator::full_enum_name(t_enum* tenum, int offset) {
  return capitalize(tenum->get_program()->get_name()) + "Enum." +
    tenum->get_name() + "_" + tenum->find_value(offset)->get_name();
}

const string t_android_lite_generator::logger_name() {
  return program_name_ + "Logger";
}

const string t_android_lite_generator::enum_name() {
  return program_name_ + "Enum";
}

void t_android_lite_generator::close_generator() {
  write_class_file();
  write_logger_file();
  write_enum_file();
}

/* Just like Java, we don't do anything for typedefs. We still override the
 * method so that a change to the Java compiler doesn't suprise us. */
void t_android_lite_generator::generate_typedef(t_typedef* /*ttypedef*/) {
  // Empty.
}

void t_android_lite_generator::output_write(t_list* tlist, const string value,
    int depth, bool needs_cast, stringstream& stream) {
  t_type* inner_type = tlist->get_elem_type();
  string inner_type_name = type_name(inner_type);
  string java_name = type_name(tlist);
  string tmp_var = temp_variable("var", depth);

  if (needs_cast) {
    indent(stream) << java_name << " " << tmp_var << " = " <<
        "(" << java_name << ") " << value << ";" << endl;
  } else {
    indent(stream) << java_name << " " << tmp_var << " = " <<
        value << ";" << endl;
  }
  indent(stream) << "oprot.writeListBegin(new TList(" <<
      get_java_type_string(inner_type) << ", " <<
      "var" << depth << ".size()));" << endl;

  string tmp_iter = temp_variable("iter", depth);
  indent(stream) << "for (" << inner_type_name <<
    " " << tmp_iter << " : " << tmp_var << ") {" << endl;
  indent_up();

  output_write(inner_type, tmp_iter, depth + 1, false, stream);
  indent_down();
  indent(stream) << "}" << endl;
  indent(stream) << "oprot.writeListEnd();" << endl;
}

void t_android_lite_generator::output_write(t_map* tmap, const string value,
    int depth, bool needs_cast, stringstream& stream) {
  t_type* key_type = ((t_map*) tmap)->get_key_type();
  t_type* val_type = ((t_map*) tmap)->get_val_type();
  string java_name = type_name(tmap);
  string tmp_var = temp_variable("var", depth);

  if (needs_cast) {
    indent(stream) << java_name << " " << tmp_var << " = " <<
      "(" << java_name << ") " << value << ";" << endl;
  } else {
    indent(stream) << java_name << " " << tmp_var << " = " <<
      value << ";" << endl;
  }

  string tmp_iter = temp_variable("iter", depth);

  indent(stream) << "oprot.writeMapBegin(new TMap(" <<
    get_java_type_string(key_type) << ", " <<
    get_java_type_string(val_type) << ", " <<
    tmp_var << ".size()));" << endl;
  indent(stream) << "for (Map.Entry<" <<
    type_name(key_type, true) << ", " << type_name(val_type, true) << "> " <<
    tmp_iter << " : " <<
    "var" << depth << ".entrySet()) {" << endl;
  indent_up();

  output_write(key_type, tmp_iter + ".getKey()", depth + 1, false, stream);
  output_write(val_type, tmp_iter + ".getValue()", depth + 1, false, stream);

  indent_down();
  indent(stream) << "}" << endl;
  indent(stream) << "oprot.writeMapEnd();" << endl;
}


void t_android_lite_generator::output_write(t_struct* tstruct,
    const string value, int /*depth*/, bool needs_cast, stringstream& stream) {
  if (needs_cast) {
    indent(stream) << "((" << type_name(tstruct) << ") " << value << ")";
  } else {
    indent(stream) << value;
  }
  stream << ".write(oprot);" << endl;
}

void t_android_lite_generator::output_write(t_set* tset, const string value,
    int depth, bool needs_cast, stringstream& stream) {
  t_type* inner_type =((t_set*) tset)->get_elem_type();
  string inner_type_name = type_name(inner_type);
  string java_name = type_name(tset);
  string tmp_var = temp_variable("var", depth);

  if (needs_cast) {
    indent(stream) << java_name << " " << tmp_var << " = " <<
      "(" << java_name << ") " << value << ";" << endl;
  } else {
    indent(stream) << java_name << " " << tmp_var << " = " <<
      value << ";" << endl;
  }
  indent(stream) << "oprot.writeSetBegin(new TSet(" <<
    get_java_type_string(inner_type) << ", " <<
    tmp_var << ".size()));" << endl;

  string tmp_iter = temp_variable("iter", depth);
  indent(stream) << "for(" << inner_type_name <<
    " " << tmp_iter << " : var" << depth << ") {" << endl;
  indent_up();

  output_write(inner_type, tmp_iter, depth + 1, false, stream);
  indent_down();
  indent(stream) << "}" << endl;
  indent(stream) << "oprot.writeSetEnd();" << endl;
}


void t_android_lite_generator::output_write(t_enum* tenum, const string value,
    int /*depth*/, bool needsCast, stringstream& stream) {
  indent(stream) << "oprot.writeI32(";
  if (needsCast) {
    stream << "((" << type_name(tenum) << ") " << value << ")";
  } else {
    stream << value;
  }
  stream << ".getValue());" << endl;
}

void t_android_lite_generator::output_write(t_type* ttype, const string value,
    int depth, bool needs_cast, stringstream& stream) {
  ttype = get_true_type(ttype);
  if (ttype->is_base_type()) {
    string thrift_name = capitalize(ttype->get_name());
    string java_name = base_type_name((t_base_type *) ttype);
    // Edge case: all the methods are named `writeFoo` for primitive type `foo`
    // except `byte[]`, which pairs with `writeBinary`.
    if (((t_base_type*)ttype)->is_binary()) {
      java_name = "Binary";
    }
    if (needs_cast) {
      indent(stream) << "oprot.write" << thrift_name <<
          "((" << java_name << ") " << value << ");" << endl;
    } else {
      indent(stream) << "oprot.write" << thrift_name <<
          "("  << value << ");" << endl;
    }
    // Since C++ dispatch is handled statically at compile-time,
    // we need to cast to each of these methods individually.
  } else if (ttype->is_list()) {
    output_write((t_list *)ttype, value, depth, needs_cast, stream);

  } else if (ttype->is_struct()) {
    output_write((t_struct *)ttype, value, depth, needs_cast, stream);

  } else if (ttype->is_map()) {
    output_write((t_map *)ttype, value, depth, needs_cast, stream);

  } else if (ttype->is_set()) {
    output_write((t_set *)ttype, value, depth, needs_cast, stream);

  } else if (ttype->is_enum()) {
    output_write((t_enum *)ttype, value, depth, needs_cast, stream);

  } else {
    throw "Compiler error: unhandled type.";

  }
}

// OUTPUTS:
// @TOptional/@TRequired
// public static final ThriftProperty<TypeName> ParentName_MyName =
//    new ThriftProperty<TypeName>("Name", (short) idx);
void t_android_lite_generator::output_property(t_field* tfield,
    const string parent_name) {
  if (annotate_) {
    indent(class_defns_) << ((tfield->get_req() == t_field::T_REQUIRED) ?
      "@TRequired(\"" : "@TOptional(\"") << parent_name << "\")" << endl;
  }

  record_type_use(tfield->get_type());

  indent(class_defns_) <<
    "public static final ThriftProperty<" <<
    type_name(tfield->get_type(), true) << "> " <<
    parent_name << "_" <<
    tfield->get_name() <<
    " =" << endl;

  indent_up(); indent_up();
  indent(class_defns_) << "new ThriftProperty<" <<
      type_name(tfield->get_type(), true) << ">(\"" <<
      tfield->get_name() << "\", " <<
      get_java_type_string(tfield->get_type()) << ", " <<
      "(short) " << tfield->get_key() << ");" << endl;
  indent_down(); indent_down();
}

void t_android_lite_generator::output_case_body_struct(t_struct *tstruct) {
  const vector<t_field*> members = tstruct->get_members();
  vector<t_field *>::const_iterator m_iter;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    t_field *tfield = *m_iter;
    string key = full_property_name(tstruct, tfield);

    indent(switch_stmts_) << "if (mMap.containsKey(" + key + ") && " <<
      "mMap.get(" + key + ") != null) {" << endl;
    indent_up();

    indent(switch_stmts_) << "writeFieldBegin(oprot, " + key + ");" << endl;

    string value ="mMap.get(" + key + ")";
    output_write(tfield->get_type(), value, 0, true, switch_stmts_);
    indent(switch_stmts_) << "oprot.writeFieldEnd();" << endl;
    indent_down();

    if (tfield->get_req() == t_field::T_REQUIRED &&
        tfield->get_value() == nullptr) {
      indent(switch_stmts_) << "} else {" << endl;
      indent_up();
      indent(switch_stmts_) << "throw new TProtocolException("
        "TProtocolException.MISSING_REQUIRED_FIELD, \"Required field '" <<
        tstruct->get_name() << "." << tfield->get_name() <<
        "' was not present!\");" << endl;
      indent_down();
    }
    indent(switch_stmts_) << "}" << endl;
    switch_stmts_ << endl; // blank line between each 'if'
  }

}

void t_android_lite_generator::output_case_body_union(t_struct *tunion) {
  // We're guaranteed that there will be only one element in the keySet
  indent(switch_stmts_) << "switch (mMap.keySet().iterator().next().id) {" <<
      endl;

  const vector<t_field*> members = tunion->get_members();
  vector<t_field *>::const_iterator m_iter;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    t_field *tfield = *m_iter;
    // We use a non-standard Java scoping rule here. You can add braces to case
    // statements so that each case has its own scope, rather than the entire
    // switch having one scope, as is the usual convention. This makes it
    // easier to avoid conflicts in variable names.
    indent(switch_stmts_) << "case " << tfield->get_key() << ": {" << endl;
    indent_up();

    string key = full_property_name(tunion, tfield);
    indent(switch_stmts_) << "writeFieldBegin(oprot, " + key + ");" << endl;
    string value ="mMap.get(" + key + ")";
    output_write(tfield->get_type(), value, 0, true, switch_stmts_);
    indent(switch_stmts_) << "oprot.writeFieldEnd();" << endl;
    indent(switch_stmts_) << "break;" << endl;
    indent_down();
    indent(switch_stmts_) << "}" << endl;
    switch_stmts_ << endl;
  }
  indent(switch_stmts_) << "}" << endl;
}


void t_android_lite_generator::output_case_statement(t_struct *tstruct) {
  indent(switch_stmts_) << "case " << tstruct->get_name() << ": {" << endl;
  indent_up();

  if (tstruct->is_union()) {
    indent(switch_stmts_) << "if (this.mMap.size() < 1) {" << endl;
    indent_up();
    indent(switch_stmts_) << "throw new TProtocolException("
        "TProtocolException.MISSING_REQUIRED_FIELD, "
        "\"Cannot write a union with no set value!\");" << endl;
    indent_down();
    indent(switch_stmts_) << "} else if (this.mMap.size() > 1) {" << endl;
    indent_up();
    indent(switch_stmts_) << "throw new TProtocolException("
        "TProtocolException.INVALID_DATA, "
        "\"Cannot write a union with more than one set value!\");" << endl;
    indent_down();
    indent(switch_stmts_) << "}" << endl;
  }

  indent(switch_stmts_) << "oprot.writeStructBegin(new TStruct(\"" <<
      tstruct->get_name() << "\"));" << endl;

  // These are different because a union only needs to send its 1 value
  // but a struct needs to do a linear pass and send all of them.
  if (tstruct->is_union()) {
    output_case_body_union(tstruct);
  } else {
    output_case_body_struct(tstruct);
  }

  indent(switch_stmts_) << "oprot.writeFieldStop();" << endl;
  indent(switch_stmts_) << "oprot.writeStructEnd();" << endl;
  indent(switch_stmts_) << "break;" << endl;
  indent_down();
  indent(switch_stmts_) << "}" << endl;
  switch_stmts_ << endl; // extra blank line

}

void t_android_lite_generator::generate_struct(t_struct* tstruct) {
  const vector<t_field*>& members = tstruct->get_members();
  if (!members.empty()) {
    vector<t_field*>::const_iterator m_iter;
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      output_property(*m_iter, tstruct->get_name());
    }

    output_case_statement(tstruct);
  }
}

void t_android_lite_generator::generate_consts(vector<t_const*> tconsts) {
  if (tconsts.empty()) {
    return;
  }
  string f_consts_name = get_package_dir() + "/" + program_name_ +
    "Constants.java";
  ofstream consts_stream;
  consts_stream.open(f_consts_name.c_str());

  consts_stream << autogen_comment() << package_header() << endl;
  consts_stream << java_type_imports() << endl
                << android_thrift_imports() << endl
                << endl; // empty line at end
  consts_stream << "public class " <<  program_name_ << "Constants {"
                << endl;
  indent_up();

  vector<t_const*>::const_iterator c_iter;
  for (c_iter = tconsts.cbegin(); c_iter != tconsts.cend(); ++c_iter) {
    t_const *tconst = *c_iter;
    print_const_value(consts_stream, tconst->get_name(), tconst->get_type(),
        tconst->get_value(), false);
  }
  indent_down();
  consts_stream << "}" << endl;
  consts_stream.close();
}

string t_android_lite_generator::render_const_value(
    ostream& out,
    string name,
    t_type *type,
    const t_const_value* value) {
  // Everything can be handled by the call to super except enums
  if (!type->is_enum()) {
    return t_java_generator::render_const_value(out, name, type, value);
  }
  t_enum * tenum = (t_enum *)type;
  string enum_name = tenum->find_value(value->get_integer())->get_name();
  return full_enum_name(tenum, value->get_integer());
}

void t_android_lite_generator::print_const_value(
    ostream& out,
    string name,
    t_type* type,
    const t_const_value* value,
    bool in_static,
    bool defval) {
  if (!type->is_struct() && !type->is_enum()) {
    t_java_generator::print_const_value(out, name, type, value, in_static,
        defval);
    return;
  }

  if (!defval) {
    indent(out);
    out << (in_static ? "" : "public static final ") << type_name(type)
      << " ";
  }
  if (type->is_enum()) {
    t_enum * tenum = (t_enum *)type;
    out << name << " = " << full_enum_name(tenum, value->get_integer());

    if (!defval) {
      out << ";" << endl;
    }
  } else if (type->is_struct()) {
    string eventType_key = capitalize(type->get_program()->get_name()) +
      ".EventType." + type->get_name();
    out << name << " = new " <<  type_name(type, false, true) <<
      "(" << eventType_key << ");" << endl;
    if (!in_static) {
      indent(out) << "static {" << endl;
      indent_up();
    }

    const vector<t_field*>& fields = ((t_struct*)type)->get_members();
    vector<t_field*>::const_iterator f_iter;
    const vector<pair<t_const_value*, t_const_value*>>& vals = value->get_map();
    vector<pair<t_const_value*, t_const_value*>>::const_iterator v_iter;
    for (v_iter = vals.cbegin(); v_iter != vals.cend(); ++v_iter) {
      t_type* field_type = nullptr;
      for (f_iter = fields.cbegin(); f_iter != fields.end(); ++f_iter) {
        if ((*f_iter)->get_name() == v_iter->first->get_string()) {
          field_type = (*f_iter)->get_type();
          break;
        }
      }
      if (field_type == nullptr) {
        throw "type error: " + type->get_name() + " has no field " +
          v_iter->first->get_string();
      }
      string val = render_const_value(out, name, field_type, v_iter->second);
      indent(out) << name << ".addProperty("
                  << full_property_name((t_struct *)type, *f_iter)
                  << ", " << val << ");" << endl;
      }

    if (!in_static) {
      indent_down();
      indent(out) << "}" << endl;
    }
  }
}

void t_android_lite_generator::generate_enum(t_enum* tenum) {
  const vector<t_enum_value*> e_values = tenum->get_constants();
  vector<t_enum_value*>::const_iterator ev_iter;
  for (ev_iter = e_values.begin(); ev_iter != e_values.end(); ++ev_iter) {
    t_enum_value* val = *ev_iter;
    stringstream line;
    line << tenum->get_name() << "_" << val->get_name() << "(" <<
      val->get_value() << ")";
    enum_defns_.push_back(line.str());
  }
}

void t_android_lite_generator::generate_service(t_service* /*tservice*/) {
  throw "Services are not yet supported for Thrift on Android.";
}

void t_android_lite_generator::generate_xception(t_struct* /*txception*/) {
  throw "Exceptions are not yet supported for Thrift on Android.";
}

THRIFT_REGISTER_GENERATOR(android_lite, "Android",
"    annotate:        Generate annotations that help the linter.\n");
