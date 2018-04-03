namespace cpp2 streaming_tests

service DiffTypesStreamingService {
  stream i32 downloadObject(1: i64 object_id);
}
