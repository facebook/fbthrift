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

from thrift_compiler.generate.t_output import CompositeOutput
from thrift_compiler.generate.t_output_aggregator import OutputContext

# ---------------------------------------------------------------
# OutputContext
# ---------------------------------------------------------------


class SchemaOutputContext(OutputContext):
    def __init__(self, schema_file):
        self._schema_file = schema_file

        outputs = [schema_file]

        # shorthand to write to all outputs at the same time
        self._all_outputs = CompositeOutput(*outputs)
        # start writing in the header
        self.output = schema_file

    @property
    def schema(self):
        return self._schema_file

    @property
    def output(self):
        return self._output_crt

    @output.setter
    def output(self, output):
        self._output_crt = output

    @property
    def outputs(self):
        return self._all_outputs

    def _enter_scope_handler(self, scope, physical_scope=True):
        raise NotImplementedError

    def _exit_scope_handler(self, scope, physical_scope=True):
        raise NotImplementedError
