from cython.operator cimport dereference as deref
from libc.stdint cimport int32_t
from libcpp.memory cimport shared_ptr, make_shared

cimport testing.types
import testing.types

def simulate_HardError(str errortext, int32_t code):
    cdef shared_ptr[testing.types.cHardError] c_inst
    c_inst = make_shared[testing.types.cHardError]()
    deref(c_inst).errortext = <bytes>errortext.encode('utf-8')
    deref(c_inst).__isset.errortext = True
    deref(c_inst).code = code
    return testing.types.HardError.create(c_inst)

def simulate_UnusedError(str message):
    cdef shared_ptr[testing.types.cUnusedError] c_inst
    c_inst = make_shared[testing.types.cUnusedError]()
    deref(c_inst).message = <bytes>message.encode('utf-8')
    deref(c_inst).__isset.message = True
    return testing.types.UnusedError.create(c_inst)
