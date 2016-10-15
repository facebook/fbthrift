
cdef public void call_cy_SimpleService_get_five(
    object self,
    shared_ptr[cFollyPromise[int32_t]] cPromise
) with gil:
    promise = Promise_i32.create(cPromise)

    func = functools.partial(
        asyncio.ensure_future,
        SimpleService_get_five_coro(
            self,
            promise),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def SimpleService_get_five_coro(
    object self,
    Promise_i32 promise
):
    try:
      result = await self.get_five()
    except Exception as ex:
        print(
            "Unexpected error in service handler get_five:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(<int32_t> result)

cdef public void call_cy_SimpleService_add_five(
    object self,
    shared_ptr[cFollyPromise[int32_t]] cPromise,
    int32_t num
) with gil:
    promise = Promise_i32.create(cPromise)
    arg_num = num

    func = functools.partial(
        asyncio.ensure_future,
        SimpleService_add_five_coro(
            self,
            promise,
            arg_num),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def SimpleService_add_five_coro(
    object self,
    Promise_i32 promise,
    num
):
    try:
      result = await self.add_five(
          num)
    except Exception as ex:
        print(
            "Unexpected error in service handler add_five:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(<int32_t> result)

cdef public void call_cy_SimpleService_do_nothing(
    object self,
    shared_ptr[cFollyPromise[cFollyUnit]] cPromise
) with gil:
    promise = Promise_void.create(cPromise)

    func = functools.partial(
        asyncio.ensure_future,
        SimpleService_do_nothing_coro(
            self,
            promise),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def SimpleService_do_nothing_coro(
    object self,
    Promise_void promise
):
    try:
      result = await self.do_nothing()
    except Exception as ex:
        print(
            "Unexpected error in service handler do_nothing:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(c_unit)

cdef public void call_cy_SimpleService_concat(
    object self,
    shared_ptr[cFollyPromise[unique_ptr[string]]] cPromise,
    unique_ptr[string] first,
    unique_ptr[string] second
) with gil:
    promise = Promise_string.create(cPromise)
    arg_first = (deref(first.get())).decode()
    arg_second = (deref(second.get())).decode()

    func = functools.partial(
        asyncio.ensure_future,
        SimpleService_concat_coro(
            self,
            promise,
            arg_first,
            arg_second),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def SimpleService_concat_coro(
    object self,
    Promise_string promise,
    first,
    second
):
    try:
      result = await self.concat(
          first,
          second)
    except Exception as ex:
        print(
            "Unexpected error in service handler concat:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(make_unique[string](<string> result.encode('UTF-8')))

cdef public void call_cy_SimpleService_get_value(
    object self,
    shared_ptr[cFollyPromise[int32_t]] cPromise,
    unique_ptr[cSimpleStruct] simple_struct
) with gil:
    promise = Promise_i32.create(cPromise)
    arg_simple_struct = SimpleStruct.create(move(simple_struct))

    func = functools.partial(
        asyncio.ensure_future,
        SimpleService_get_value_coro(
            self,
            promise,
            arg_simple_struct),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def SimpleService_get_value_coro(
    object self,
    Promise_i32 promise,
    simple_struct
):
    try:
      result = await self.get_value(
          simple_struct)
    except Exception as ex:
        print(
            "Unexpected error in service handler get_value:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(<int32_t> result)

cdef public void call_cy_SimpleService_negate(
    object self,
    shared_ptr[cFollyPromise[cbool]] cPromise,
    cbool input
) with gil:
    promise = Promise_bool.create(cPromise)
    arg_input = input

    func = functools.partial(
        asyncio.ensure_future,
        SimpleService_negate_coro(
            self,
            promise,
            arg_input),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def SimpleService_negate_coro(
    object self,
    Promise_bool promise,
    input
):
    try:
      result = await self.negate(
          input)
    except Exception as ex:
        print(
            "Unexpected error in service handler negate:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(<cbool> result)

cdef public void call_cy_SimpleService_tiny(
    object self,
    shared_ptr[cFollyPromise[int8_t]] cPromise,
    int8_t input
) with gil:
    promise = Promise_byte.create(cPromise)
    arg_input = input

    func = functools.partial(
        asyncio.ensure_future,
        SimpleService_tiny_coro(
            self,
            promise,
            arg_input),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def SimpleService_tiny_coro(
    object self,
    Promise_byte promise,
    input
):
    try:
      result = await self.tiny(
          input)
    except Exception as ex:
        print(
            "Unexpected error in service handler tiny:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(<int8_t> result)

cdef public void call_cy_SimpleService_small(
    object self,
    shared_ptr[cFollyPromise[int16_t]] cPromise,
    int16_t input
) with gil:
    promise = Promise_i16.create(cPromise)
    arg_input = input

    func = functools.partial(
        asyncio.ensure_future,
        SimpleService_small_coro(
            self,
            promise,
            arg_input),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def SimpleService_small_coro(
    object self,
    Promise_i16 promise,
    input
):
    try:
      result = await self.small(
          input)
    except Exception as ex:
        print(
            "Unexpected error in service handler small:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(<int16_t> result)

cdef public void call_cy_SimpleService_big(
    object self,
    shared_ptr[cFollyPromise[int64_t]] cPromise,
    int64_t input
) with gil:
    promise = Promise_i64.create(cPromise)
    arg_input = input

    func = functools.partial(
        asyncio.ensure_future,
        SimpleService_big_coro(
            self,
            promise,
            arg_input),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def SimpleService_big_coro(
    object self,
    Promise_i64 promise,
    input
):
    try:
      result = await self.big(
          input)
    except Exception as ex:
        print(
            "Unexpected error in service handler big:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(<int64_t> result)

cdef public void call_cy_SimpleService_two(
    object self,
    shared_ptr[cFollyPromise[double]] cPromise,
    double input
) with gil:
    promise = Promise_double.create(cPromise)
    arg_input = input

    func = functools.partial(
        asyncio.ensure_future,
        SimpleService_two_coro(
            self,
            promise,
            arg_input),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def SimpleService_two_coro(
    object self,
    Promise_double promise,
    input
):
    try:
      result = await self.two(
          input)
    except Exception as ex:
        print(
            "Unexpected error in service handler two:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(<double> result)

cdef public void call_cy_SimpleService_expected_exception(
    object self,
    shared_ptr[cFollyPromise[cFollyUnit]] cPromise
) with gil:
    promise = Promise_void.create(cPromise)

    func = functools.partial(
        asyncio.ensure_future,
        SimpleService_expected_exception_coro(
            self,
            promise),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def SimpleService_expected_exception_coro(
    object self,
    Promise_void promise
):
    try:
      result = await self.expected_exception()
    except SimpleException as ex:
        deref(promise.cPromise).setException(deref((<SimpleException> ex).c_SimpleException.get()))
    except Exception as ex:
        print(
            "Unexpected error in service handler expected_exception:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(c_unit)

cdef public void call_cy_SimpleService_unexpected_exception(
    object self,
    shared_ptr[cFollyPromise[int32_t]] cPromise
) with gil:
    promise = Promise_i32.create(cPromise)

    func = functools.partial(
        asyncio.ensure_future,
        SimpleService_unexpected_exception_coro(
            self,
            promise),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def SimpleService_unexpected_exception_coro(
    object self,
    Promise_i32 promise
):
    try:
      result = await self.unexpected_exception()
    except Exception as ex:
        print(
            "Unexpected error in service handler unexpected_exception:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(<int32_t> result)

