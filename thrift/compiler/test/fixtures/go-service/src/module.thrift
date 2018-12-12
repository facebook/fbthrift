struct GetEntityRequest {
  1: string id;
}

struct GetEntityResponse {
  1: string entity;
}

service GetEntity {
  GetEntityResponse getEntity(1: GetEntityRequest r);

  bool getBool();
  byte getByte();
  i16 getI16();
  i32 getI32();
  i64 getI64();
  double getDouble();
  string getString();
  binary getBinary();
  map<string, string> getMap();
  set<string> getSet();
  list<string> getList();
}
