#!/usr/bin/env python
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
'fbcode_builder steps to build Facebook Thrift'

import specs.fbthrift as fbthrift


def fbcode_builder_spec(builder):
    return {
        'depends_on': [fbthrift],
    }


config = {
    'github_project': 'facebook/fbthrift',
    'fbcode_builder_spec': fbcode_builder_spec,
}
