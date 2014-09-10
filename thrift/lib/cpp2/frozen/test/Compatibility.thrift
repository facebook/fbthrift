namespace cpp compatibility1
namespace cpp2 compatibility2

struct Person {
  3: optional double dob,
  5: string name,
}

struct Root {
  1: string title,
  2: hash_map<i64, Person> people,
}

struct Case {
  1: string name,
  2: optional Root root,
  3: bool fails = 0,
}

const list<Case> kTestCases = [
  {
    "name": "beforeUnique",
    "root": {
      "title": "version 0",
      "people": {
        101 : {
          "dob": 1.23e9,
          "name": "alice",
        },
        102 : {
          "dob": 1.21e9,
          "name": "bob",
        },
      },
    },
  },
  {
    "name": "afterUnique",
    "root": {
      "title": "version 0",
      "people": {
        101 : {
          "dob": 1.23e9,
          "name": "alice",
        },
        102 : {
          "dob": 1.21e9,
          "name": "bob",
        },
      },
    },
  },
  {
    "name": "withFileVersion",
    "root": {
      "title": "version 0",
      "people": {
        101 : {
          "dob": 1.23e9,
          "name": "alice",
        },
        102 : {
          "dob": 1.21e9,
          "name": "bob",
        },
      },
    },
  },
  {
    "name": "futureVersion",
    "fails": 1,
  },
  {
    "name": "missing",
    "fails": 1,
  },
]
