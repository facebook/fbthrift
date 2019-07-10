<?hh

/*
 * Copyright 2006-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/** Inherits from Socket */

/**
 * Sockets implementation of the TTransport interface that allows connection
 * to a pool of servers.
 *
 * @package thrift.transport
 */
class TSocketPool extends TSocket {
  /**
   * Remote servers. List of host:port pairs.
   */
  private varray<(string, int)> $servers_ = varray[];

  /**
   * How many times to retry each host in connect
   *
   * @var int
   */
  private int $numRetries_ = 1;

  /**
   * Retry interval in seconds, how long to not try a host if it has been
   * marked as down.
   *
   * @var int
   */
  private int $retryInterval_ = 60;

  /**
   * Max consecutive failures before marking a host down.
   *
   * @var int
   */
  private int $maxConsecutiveFailures_ = 1;

  /**
   * Try hosts in order? or Randomized?
   *
   * @var bool
   */
  private bool $randomize_ = true;

  /**
   * Always try last host, even if marked down?
   *
   * @var bool
   */
  private bool $alwaysTryLast_ = true;

  /**
   * Always retry the host without wait if there was a transient
   * connection failure (such as Resource temporarily unavailable).
   * If this is set, the wait_and_retry mechanism will ignore the
   * value in $retryInterval_ for transient failures.
   *
   * @var bool
   */
  private bool $alwaysRetryForTransientFailure_ = false;

  const int ERROR_RESOURCE_TEMPORARILY_UNAVAILABLE = 11;

  /**
   * Socket pool constructor
   *
   * @param array  $hosts        List of remote hostnames
   * @param mixed  $ports        Array of remote ports, or a single common port
   * @param bool   $persist      Whether to use a persistent socket
   * @param mixed  $debugHandler Function for error logging
   */
  public function __construct(
    KeyedContainer<int, string> $hosts = varray['localhost'],
    KeyedContainer<int, int> $ports = varray[9090],
    bool $persist = false,
    ?Predicate<string> $debugHandler = null,
  ) {
    parent::__construct('', 0, $persist, $debugHandler);

    foreach ($hosts as $key => $host) {
      $this->servers_[] = tuple((string) $host, (int) $ports[$key]);
    }
  }

  /**
   * Add a server to the pool
   *
   * This function does not prevent you from adding a duplicate server entry.
   *
   * @param string $host hostname or IP
   * @param int $port port
   */
  public function addServer(string $host, int $port): void {
    $this->servers_[] = tuple($host, $port);
  }

  /**
   * Sets how many time to keep retrying a host in the connect function.
   *
   * @param int $numRetries
   */
  public function setNumRetries(int $numRetries): void {
    $this->numRetries_ = $numRetries;
  }

  /**
   * Sets how long to wait until retrying a host if it was marked down
   *
   * @param int $numRetries
   */
  public function setRetryInterval(int $retryInterval): void {
    $this->retryInterval_ = $retryInterval;
  }

  /**
   * Sets how many time to keep retrying a host before marking it as down.
   *
   * @param int $numRetries
   */
  public function setMaxConsecutiveFailures(
    int $maxConsecutiveFailures,
  ): void {
    $this->maxConsecutiveFailures_ = $maxConsecutiveFailures;
  }

  /**
   * Turns randomization in connect order on or off.
   *
   * @param bool $randomize
   */
  public function setRandomize(bool $randomize): void {
    $this->randomize_ = $randomize;
  }

  /**
   * Whether to always try the last server.
   *
   * @param bool $alwaysTryLast
   */
  public function setAlwaysTryLast(bool $alwaysTryLast): void {
    $this->alwaysTryLast_ = $alwaysTryLast;
  }

  /**
   * Whether to always retry the host without wait for
   * transient connection failures
   *
   * @param bool $alwaysRetry
   */
  public function setAlwaysRetryForTransientFailure(bool $alwaysRetry): void {
    $this->alwaysRetryForTransientFailure_ = $alwaysRetry;
  }

  /**
   * Connects the socket by iterating through all the servers in the pool
   * and trying to find one that:
   * 1. is not marked down after consecutive failures
   * 2. can really be connected to
   *
   * @return bool  false: any IP in the pool failed to connect before returning
   *               true: no failures
   */
  <<__Override>>
  public function open(): void {
    // Check if we want order randomization
    if ($this->randomize_) {
      // warning: don't use shuffle here because it leads to uneven
      // load distribution
      $n = PHP\count($this->servers_);
      $s = $this->servers_;
      for ($i = 1; $i < $n; $i++) {
        $j = PHP\mt_rand(0, $i);
        $tmp = $s[$i];
        $s[$i] = $s[$j];
        $s[$j] = $tmp;
      }
      $this->servers_ = $s;
    }

    // Count servers to identify the "last" one
    $numServers = PHP\count($this->servers_);
    $has_conn_errors = false;

    $fail_reason = darray[]; // reasons of conn failures
    for ($i = 0; $i < $numServers; ++$i) {

      // host port is stored as an array
      list($host, $port) = $this->servers_[$i];

      $failtimeKey = TSocketPool::getAPCFailtimeKey($host, $port);
      // Cache miss? Assume it's OK
      $lastFailtime = (int) PHP\fb\apc_fetch_no_success_check($failtimeKey);

      $retryIntervalPassed = false;

      // Cache hit...make sure enough the retry interval has elapsed
      if ($lastFailtime > 0) {
        $elapsed = PHP\time() - $lastFailtime;
        if ($elapsed > $this->retryInterval_) {
          $retryIntervalPassed = true;
          if ($this->debug_ && $this->debugHandler_ !== null) {
            $dh = $this->debugHandler_;
            $dh(
              'TSocketPool: retryInterval '.
              '('.
              $this->retryInterval_.
              ') '.
              'has passed for host '.
              $host.
              ':'.
              $port,
            );
          }
        }
      }

      // Only connect if not in the middle of a fail interval, OR if this
      // is the LAST server we are trying, just hammer away on it
      $isLastServer = false;
      if ($this->alwaysTryLast_) {
        $isLastServer = ($i == ($numServers - 1));
      }

      if (($lastFailtime === 0) ||
          ($isLastServer) ||
          ($lastFailtime > 0 && $retryIntervalPassed)) {

        // Set underlying TSocket params to this one
          $this->host_ = $host;
        $this->port_ = $port;

        // Try up to numRetries_ connections per server
        for ($attempt = 0; $attempt < $this->numRetries_; $attempt++) {
          try {
            // Use the underlying TSocket open function
            parent::open();

            // Only clear the failure counts if required to do so
            if ($lastFailtime > 0) {
              PHP\apc_store($failtimeKey, 0);
            }

            // Successful connection, return now
            return;

          } catch (TException $tx) {
            // Connection failed
            // keep the reason for the last try
            $errstr = $this->getErrStr();
            $errno = $this->getErrNo();
            if ($errstr !== null || $errno !== null) {
              $fail_reason[$i] = '('.$errstr.'['.$errno.'])';
            } else {
              $fail_reason[$i] = '(?)';
            }
          }
        }

        // For transient errors (like Resource temporarily unavailable),
        // we might want not to cache the failure.
        if ($this->alwaysRetryForTransientFailure_ &&
            $this->isTransientConnectFailure($this->getErrNo())) {
          continue;
        }

        $dh = ($this->debug_ ? $this->debugHandler_ : null);

        $has_conn_errors = $this->recordFailure(
          $host,
          $port,
          $this->maxConsecutiveFailures_,
          $this->retryInterval_,
          $dh,
        );
      } else {
        $fail_reason[$i] = '(cached-down)';
      }
    }

    // Holy shit we failed them all. The system is totally ill!
    $error = 'TSocketPool: All hosts in pool are down. ';
    $hosts = varray[];
    foreach ($this->servers_ as $i => $server) {
      // array(host, port) (reasons, if exist)
      list($host, $port) = $server;
      $h = $host.':'.$port;
      if (PHP\array_key_exists($i, $fail_reason)) {
        $h .= (string) $fail_reason[$i];
      }
      $hosts[] = $h;
    }
    $hostlist = PHP\implode(',', $hosts);
    $error .= '('.$hostlist.')';
    if ($this->debug_ && $this->debugHandler_ !== null) {
      $dh = $this->debugHandler_;
      $dh($error);
    }
    throw new TTransportException($error);
  }

  public static function getAPCFailtimeKey(string $host, int $port): string {
    // Check APC cache for a record of this server being down
    return 'thrift_failtime:'.$host.':'.$port.'~';
  }

  /**
   * Record the failure in APC
   * @param string  host  dest IP
   * @param int     port  dest port
   * @param int     max_failures   max consec errors before mark host/port down
   * @param int     down_period    how long to mark the host/port down
   *
   * @return bool  if mark a host/port down
   */
  public function recordFailure(
    string $host,
    int $port,
    int $max_failures,
    int $down_period,
    ?Predicate<string> $log_handler = null,
  ): bool {
    $marked_down = false;
    // Mark failure of this host in the cache
    $consecfailsKey = 'thrift_consecfails:'.$host.':'.$port.'~';

    // Ignore APC misses (treat as 0)
    $consecfails = ((int) PHP\fb\apc_fetch_no_success_check($consecfailsKey)) + 1;

    // Log and cache this failure
    if ($consecfails >= $max_failures) {
      if ($log_handler !== null) {
        $log_handler(
          'TSocketPool: marking '.
          $host.
          ':'.
          $port.
          ' as down for '.
          $down_period.
          ' secs '.
          'after '.
          $consecfails.
          ' failed attempts.',
        );
      }
      // Store the failure time
      $failtimeKey = self::getAPCFailtimeKey($host, $port);
      $curr_time = PHP\time();
      PHP\apc_store($failtimeKey, $curr_time);
      $marked_down = true;

      // Clear the count of consecutive failures
      PHP\apc_store($consecfailsKey, 0);
    } else {
      PHP\apc_store($consecfailsKey, $consecfails);
    }

    return $marked_down;
  }

  /**
   * Whether a connection failure is a transient failure
   * based on the error-code
   *
   * @param int $error_code
   * @return bool true or false
   */
  private function isTransientConnectFailure(?int $error_code): bool {
    if ($error_code === null) {
      return false;
    }
    // todo: add more to this list
    switch ($error_code) {
      case self::ERROR_RESOURCE_TEMPORARILY_UNAVAILABLE:
        return true;
      default:
        return false;
    }
  }
}
