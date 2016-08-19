namespace java.swift test.fixtures.optionals

struct Color {
  1: double red;
  2: double green;
  3: double blue;
  4: double alpha;
}

enum Animal {
  DOG = 1,
  CAT,
  TARANTULA,
}

struct Vehicle {
  1: Color color;
  2: optional string licensePlate;
  3: optional string description;
  4: optional string name;
}

typedef i64 PersonID

struct Person {
  1: PersonID id;
  2: string name;
  3: optional i16 age;
  4: optional string address;
  5: optional Color favoriteColor;
  6: optional set<PersonID> friends;
  7: optional PersonID bestFriend;
  8: optional map<Animal, string> petNames;
  9: optional Animal afraidOfAnimal;
  10: optional list<Vehicle> vehicles;
}
