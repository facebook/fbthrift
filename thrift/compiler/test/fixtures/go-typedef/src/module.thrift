typedef string Plate
typedef string State
typedef i32 Year
typedef list<string> Drivers
typedef set<string> Accessories
typedef map<i32, string> PartName

struct Automobile {
  1: Plate plate;
  2: optional Plate previous_plate;
  3: optional Plate first_plate = "0000";
  4: Year year;
  5: Drivers drivers;
}

typedef Automobile Car

service Finder {
  Automobile byPlate(
    1: Plate plate);

  Car aliasByPlate(
    1: Plate plate);

  Plate previousPlate(
    1: Plate plate);
}

struct Pair {
  1: Automobile automobile;
  2: Car car;
}

struct Collection {
  1: list<Automobile> automobiles;
  2: list<Car> cars;
}
