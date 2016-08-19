namespace java.swift test.fixtures.inheritance

service MyRoot {
  void do_root(),
}

service MyNode extends MyRoot {
  void do_mid(),
}

service MyLeaf extends MyNode {
  void do_leaf(),
}
