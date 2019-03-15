namespace cpp2 apache.thrift.test

struct SumRequest {
  1: i32 x,
  2: i32 y
}

struct SumResponse {
  1: i32 sum,
}

service Coroutine {
  SumResponse computeSumNoCoro(1: SumRequest request);

  SumResponse computeSum(1: SumRequest request) (cpp.coroutine);
  i32 computeSumPrimitive(1: i32 x, 2: i32 y) (cpp.coroutine);
  SumResponse computeSumEb(1: SumRequest request) (cpp.coroutine, thread='eb');

  void computeSumVoid(1: i32 x, 2: i32 y) (cpp.coroutine);

  SumResponse computeSumUnimplemented(1: SumRequest request) (cpp.coroutine);
  i32 computeSumUnimplementedPrimitive(1: i32 x, 2: i32 y) (cpp.coroutine);

  SumResponse computeSumThrows(1: SumRequest request) (cpp.coroutine);
  i32 computeSumThrowsPrimitive(1: i32 x, 2: i32 y) (cpp.coroutine);

  i32 noParameters() (cpp.coroutine);

  SumResponse implementedWithFutures() (cpp.coroutine);
  i32 implementedWithFuturesPrimitive() (cpp.coroutine);

  i32 takesRequestParams() (cpp.coroutine);
}
