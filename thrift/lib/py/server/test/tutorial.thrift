namespace py.asyncio thrift_asyncio.tutorial

service Calculator {
  i32 add(
    1: i32 num1,
    2: i32 num2,
  );

  i32 calculate(
    1: i32 logid,
    2: string work,
  );

  oneway void zip();
}
