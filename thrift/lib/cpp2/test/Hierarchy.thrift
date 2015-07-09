namespace cpp2 apache.thrift.test

service MyRoot {
  string doRoot(),
}

service MyNode extends MyRoot {
  string doNode(),
}

service MyLeaf extends MyNode {
  string doLeaf(),
}
