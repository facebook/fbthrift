enum HasOptionalsTestEnum {
  FOO = 1,
  BAR = 2,
}

struct HasOptionalsExtra {
  1: i64 extraInt64Req;
  2: optional i64 extraInt64Opt;
  3: string extraStringReq;
  4: optional string extraStringOpt;
  5: set<i64> extraSetReq;
  6: optional set<i64> extraSetOpt;
  7: list<i64> extraListReq;
  8: optional list<i64> extraListOpt;
  9: map<i64, i64> extraMapReq;
  10: optional map<i64, i64> extraMapOpt;
  11: HasOptionalsTestEnum extraEnumReq;
  12: optional HasOptionalsTestEnum extraEnumOpt;
}

struct HasOptionals {
  1: i64 int64Req;
  2: optional i64 int64Opt;
  3: string stringReq;
  4: optional string stringOpt;
  5: set<i64> setReq;
  6: optional set<i64> setOpt;
  7: list<i64> listReq;
  8: optional list<i64> listOpt;
  9: map<i64, i64> mapReq;
  10: optional map<i64, i64> mapOpt;
  11: HasOptionalsTestEnum enumReq;
  12: optional HasOptionalsTestEnum enumOpt;
  13: HasOptionalsExtra structReq;
  14: optional HasOptionalsExtra structOpt;
}
