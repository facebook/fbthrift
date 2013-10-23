namespace cpp tutorial.async.fetcher

/**
 * Exception thrown if an error occurs in fetchHttp().
 */
exception HttpError {
  1: string message;
}

/**
 * A service that fetches remote resources.
 */
service Fetcher {
  binary fetchHttp(1: string ip, 2: string path) throws(1: HttpError err);
}
