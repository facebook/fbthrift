#!/usr/local/bin/python2.6 -tt
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import errno
import os
import json

from thrift_compiler import frontend
# Easy access to the enum of t_base_type::t_base
from thrift_compiler.frontend import t_base
# Easy access to the enum of t_field::e_req
from thrift_compiler.frontend import e_req

from thrift_compiler.frontend import t_enum
from thrift_compiler.frontend import t_list
from thrift_compiler.frontend import t_set
from thrift_compiler.frontend import t_map
from thrift_compiler.frontend import t_struct

from thrift_compiler.generate import t_generator

from thrift_compiler.generate.t_schema_context import SchemaOutputContext
from thrift_compiler.generate.t_output import IndentedOutput

# ---------------------------------------------------------------
# Generator
# ---------------------------------------------------------------


class CompilerError(RuntimeError):
    pass

class PropertyBag(object):
    pass

class SchemaGenerator(t_generator.Generator):
    '''
    Generates JSON representation of thrift object schema.
    '''

    short_name = 'schema'
    long_name = 'Schema Version 1'
    supported_flags = {}
    _out_dir_base = 'gen-schema'

    _base_to_typename = {
        t_base.void: 'void',
        t_base.string: 'string',
        t_base.bool: 'bool',
        t_base.byte: 'byte',
        t_base.i16: 'i16',
        t_base.i32: 'i32',
        t_base.i64: 'i64',
        t_base.double: 'double',
        t_base.float: 'float',
    }

    def __init__(self, *args, **kwargs):
        # super constructor
        t_generator.Generator.__init__(self, *args, **kwargs)

    @property
    def out_dir(self):
        return os.path.join(self._program.out_path, self._out_dir_base)

    def in_out_dir(self, filename):
        return os.path.join(self.out_dir, filename)

    def _write_to(self, to):
        return IndentedOutput(open(self.in_out_dir(to), 'w'))

    def _make_context(self, filename):
        'Convenience method to get set up context and output files'

        # open files and instantiate outputs
        schema_file = self._write_to(filename + "_schema.json")

        context = SchemaOutputContext(schema_file)
        return context

    def _get_program_deps(self, tprogram, output, recurse=True):
        output.add(tprogram)
        for include in tprogram.includes:
            if include in output:
                continue

            if recurse:
                self._get_program_deps(include, output, recurse)

    def _get_metadata_for_program(self, tprogram):
        output = PropertyBag()
        output.namespaces = {}
        for key in tprogram.namespaces.keys():
            output.namespaces[key] = tprogram.namespaces[key]
        return output

    def init_generator(self):
        name = self._program.name
        # Make output directory
        try:
            os.mkdir(self.out_dir)
        except OSError as exc:
            if exc.errno == errno.EEXIST and os.path.isdir(self.out_dir):
                pass
            else:
                raise

        self._schema = PropertyBag()
        self._schema.dataTypes = {}
        self._schema.names = {}
        self._schema.services = {}

        metadata = PropertyBag()
        metadata.comment = self._autogen_comment
        metadata.path = self._program.path
        metadata.fileMetadata = {}

        includes = set()
        self._get_program_deps(self._program, includes)

        for program in includes:
            metadata.fileMetadata[
                program.name] = self._get_metadata_for_program(
                    program
                )

        self._schema._metadata = metadata

        context = self._make_context(name)

        self._schema_out = context.schema

    def close_generator(self):
        serialized = json.dumps(
            self._schema,
            default=lambda o: o.__dict__,
            sort_keys=True,
            indent=4,
            separators=(',', ': ')
        )

        print(serialized, file=self._schema_out)

    def _generate_data(self):
        pass

    def _generate_consts(self, constants):
        pass

    def _get_true_type(self, ttype):
        'Get the true type behind a series of typedefs and streams'
        while ttype.is_typedef or ttype.is_stream:
            if ttype.is_typedef:
                ttype = ttype.as_typedef.type
            else:
                ttype = ttype.as_stream.elem_type
        return ttype

    def _is_already_processed(self, ttype):
        return ttype.type_id in self._schema.dataTypes

    def _is_optional(self, f):
        return f.req == e_req.optional

    def _generate_schema_for(self, ttype):
        if self._is_already_processed(ttype):
            return

        id = ttype.type_id
        tname = ttype.full_name

        self._schema.dataTypes[id] = PropertyBag()
        self._schema.names[tname] = id

        info = self._schema.dataTypes[id]
        info.name = tname

        ttype = self._get_true_type(ttype)

        info.type_value = (int(ttype.type_value), str(ttype.type_value))

        type_annotations = ttype.annotations
        if type_annotations is not None and len(type_annotations) != 0:
            info.annotations = {}
            for key in type_annotations.keys():
                info.annotations[key] = type_annotations[key]

        if ttype.is_base_type:
            pass
        elif ttype.is_struct or ttype.is_xception:
            tstruct = ttype if isinstance(ttype, t_struct) else ttype.as_struct
            info.fields = {}
            fieldOrder = 0
            for field in tstruct.members:
                schemaField = PropertyBag()
                schemaField.is_required = not self._is_optional(field)
                schemaField.name = field.name

                field_annotations = field.annotations
                if field_annotations is not None and \
                   len(field_annotations) != 0:
                    schemaField.annotations = {}
                    for key in field_annotations.keys():
                        schemaField.annotations[key] = field_annotations[key]

                schemaField.order = fieldOrder
                schemaField.type_id = str(field.type.type_id)

                fieldOrder = fieldOrder + 1
                info.fields[field.key] = schemaField
                self._generate_schema_for(field.type)
        elif ttype.is_map:
            tmap = ttype if isinstance(ttype, t_map) else ttype.as_map
            info.key_type_id = str(tmap.key_type.type_id)
            info.value_type_id = str(tmap.value_type.type_id)

            self._generate_schema_for(tmap.key_type)
            self._generate_schema_for(tmap.value_type)
        elif ttype.is_set:
            tset = ttype if isinstance(ttype, t_set) else ttype.as_set
            info.value_type_id = str(tset.elem_type.type_id)
            self._generate_schema_for(tset.elem_type)
        elif ttype.is_list:
            tlist = ttype if isinstance(ttype, t_list) else ttype.as_list
            info.value_type_id = str(tlist.elem_type.type_id)
            self._generate_schema_for(tlist.elem_type)
        elif ttype.is_enum:
            tenum = ttype if isinstance(ttype, t_enum) else ttype.as_enum
            info.enum_values = {}
            for const in tenum.constants:
                info.enum_values[const.name] = const.value
        else:
            raise CompilerError(
                "[Schema] Don't know how to handle {} (TypeValue: {})".format(
                    tname, ttype.type_value
                )
            )

    def _generate_service(self, svc):
        service_info = self._schema.services[svc.name] = {}

        for fn in svc.functions:
            fn_info = service_info[fn.name] = PropertyBag()

            if fn.arglist is not None:
                fn_info.message_type_id = str(fn.arglist.type_id)
                self._generate_object(fn.arglist)

            if fn.xceptions is not None:
                #
                # Note that thrift parser generates nameless t_struct
                # objects for function "throws" lists which are passed
                # in to t_function objects.
                #
                # This generates a name for it to avoid name collision
                # with other exception lists.
                #
                if len(fn.xceptions.name) == 0:
                    fn.xceptions.name = str("{}_exceptions".format(fn.name))

                fn_info.exception_type_id = str(fn.xceptions.type_id)
                self._generate_object(fn.xceptions)

    def _generate_enum(self, what):
        self._generate_schema_for(self._get_true_type(what))

    def _generate_object(self, obj):
        self._generate_schema_for(self._get_true_type(obj))

    _generate_map = {
        frontend.t_enum: _generate_enum,
        frontend.t_struct: _generate_object,
        frontend.t_service: _generate_service,
    }

    def _generate(self, what):
        try:
            gen_func = self._generate_map[what.__class__]
            gen_func(self, what)
        except KeyError:
            print("Warning: Did not generate {}".format(what))

    def _generate_comment(self, text):
        return filter(None, text.split('\n'))

    def _gen_forward_declaration(self, param):
        pass

# register the generator factory
t_generator.GeneratorFactory(SchemaGenerator)
