namespace cpp2 apache.thrift.test
namespace java thrift.test

struct Watchdog {
  1: string type,
  2: string name,
  3: string entity,
  4: string key,
  5: double threshold,
  6: bool reverse,
}

struct WhitelistEntry {
  1: string name,
  2: string type,
  3: list<string> contacts,
  4: string target_kernel,
  5: string target,
  6: string rebooter,
  7: string rate,
  8: i32 atonce,
  9: list<Watchdog> watchdogs,
}

struct Config {
  1: map<string, string> vars;
  2: list<WhitelistEntry> whitelist;
}
