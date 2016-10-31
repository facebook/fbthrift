namespace d thrift.test

service Top {
  void top(),
}

service Mid extends Top {
  void mid(),
}

service Bot extends Mid {
  void bot(),
}
