namespace cpp2 apache.thrift.tutorial.fetcher

/**
 * Exception thrown if an error occurs in fetchHttp().
 */
exception HttpError {
  1: string message;
}

struct FetchHttpRequest {
  1: string addr;
  2: i32 port;
  3: string path;
}

/**
 * A service that fetches remote resources.
 */
service Fetcher {
  binary fetchHttp(1: FetchHttpRequest request) throws(1: HttpError err);
}
