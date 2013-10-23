namespace cpp test
namespace cpp2 test
namespace py test.reverse

struct Request {
  1: string str;
}

struct Response {
  1: string str;
}

service ReverseService {
  Response reverse(1: Request req);
}
