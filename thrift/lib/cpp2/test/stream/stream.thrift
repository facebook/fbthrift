namespace cpp apache.thrift.test

struct Param {
  1: string msg
}

struct Verb {
  1: string be
}

struct Subject {
  1: Verb will
}

exception SimpleException {
  1: string msg,
} (message = 'msg')

service StreamService {
  string sendResponse(1:i64 size)
  oneway void noResponse(1:i64 size)
  oneway void subjectOneWay(1:Subject arg)

  void doNothing()
  double returnSomething()
  i32 returnSimple(1: i32 zero)
  Subject returnSubject(1: Subject param)
  stream<i32> returnStream(1: string which)

  void doNothingWithStreams(1: stream<i32> intStream,
                            2: i32 num,
                            3: list<string> words,
                            4: Subject subject,
                            5: stream<string> another)
  i16 returnSimpleWithStreams(1: stream<i32> intStream,
                              2: i32 num,
                              3: list<string> words,
                              4: Subject subject,
                              5: stream<string> another)
  Subject returnSubjectWithStreams(1: stream<i32> intStream,
                                   2: i32 num,
                                   3: list<string> words,
                                   4: Subject subject,
                                   5: stream<string> another)
  stream<list<Subject>> returnStreamWithStreams(1: stream<i32> intStream,
                                                2: i32 num,
                                                3: list<string> words,
                                                4: Subject subject,
                                                5: stream<list<string>> another)

  stream<i32> throwException(1: i32 size) throws (1: SimpleException error)
  stream<i32> multiStream(1: stream<i32> one (cpp.type = "this is ignored")
                          2: stream<i32> two (cpp.template = "also ignored")
                          3: i32 notStream = 123)

  stream<i32> simpleTwoWayStream(1: stream<i32> input)
  void simpleOneWayStream(1: stream<i32> input)
  i32 sum(1: stream<i32> nums);
  stream<i32> ignoreStreams(1: stream<i32> input)

  stream<i32> errorStreams(1: stream<i32> input);
}

service SimilarService {
  stream<i32> doNothing(1: stream<i32> input)
  i32 ignoreStreams()
}
