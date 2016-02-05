struct TestStruct {
  1: i64 aLong,
  2: string aString,
}

service TestService {
  void sleep();
  bool isPrime(i64 num);
  TestStruct getResult();
}
