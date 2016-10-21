typedef i64 ColorID

struct House {
  1: ColorID id,
  2: string houseName,
  3: optional set<ColorID> houseColors (deprecated="Not used anymore")
} (deprecated)

struct Field {
  1: ColorID id,
  2: i32 fieldType = 5 (deprecated)
} (deprecated="No longer supported")
