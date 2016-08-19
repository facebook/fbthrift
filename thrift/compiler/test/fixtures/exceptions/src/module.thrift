namespace java.swift test.fixtures.exceptions

exception Banal { }
exception Fiery {
  1: required string message,
}

service Raiser {
  void doBland(),
  void doRaise() throws (1: Banal b, 2: Fiery f),
  string get200(),
  string get500() throws (1: Fiery f, 2: Banal b),
}
