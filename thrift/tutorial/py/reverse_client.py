# Copyright 2004-present Facebook. All Rights Reserved.
#
# Author: Marc Horowitz (mhorowitz@fb.com)
#

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
# this interferes with ServiceRouter/SWIG
# @lint-avoid-python-3-compatibility-imports
#from __future__ import unicode_literals

# @lint-avoid-servicerouter-import-warning
from ServiceRouter import ServiceOptions, ConnConfigs, ServiceRouter
from libfb import decorators
from test.reverse import ttypes as reverse
from test.reverse import ReverseService

TIMEOUT = 60 * 1000  # milliseconds

@decorators.memoize_forever
def getClient():
    options = ServiceOptions({
        "single_host": ("127.0.0.1", "9999")
        })

    overrides = ConnConfigs({
        # milliseconds
        "sock_sendtimeout": str(TIMEOUT),
        "sock_recvtimeout": str(TIMEOUT),
        "thrift_transport": "header",
        "thrift_security": "permitted",  # "required",
        "thrift_security_service_tier": "host",
        })

    return ServiceRouter().getClient2(
        ReverseService.Client,
        "",  # tier name; not needed since we're using single_host
        options, overrides, False)

req = reverse.Request(str="hello, world")
resp = getClient().reverse(req)
print(resp.str)
