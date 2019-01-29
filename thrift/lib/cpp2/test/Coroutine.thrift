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

  SumResponse computeSum(1: SumRequest request) (coroutine);
  i32 computeSumPrimitive(1: i32 x, 2: i32 y) (coroutine);
  SumResponse computeSumEb(1: SumRequest request) (coroutine, thread='eb');

  void computeSumVoid(1: i32 x, 2: i32 y) (coroutine);

  SumResponse computeSumUnimplemented(1: SumRequest request) (coroutine);
  i32 computeSumUnimplementedPrimitive(1: i32 x, 2: i32 y) (coroutine);

  SumResponse computeSumThrows(1: SumRequest request) (coroutine);
  i32 computeSumThrowsPrimitive(1: i32 x, 2: i32 y) (coroutine);

  i32 noParameters() (coroutine);

  SumResponse implementedWithFutures() (coroutine);
  i32 implementedWithFuturesPrimitive() (coroutine);
}
