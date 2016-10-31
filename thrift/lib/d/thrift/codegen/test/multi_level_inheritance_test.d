import thrift.codegen.client;
import thrift.protocol.compact;
import thrift.transport.memory;

import thrift.test.Top;
import thrift.test.Mid;
import thrift.test.Bot;

int main(string[]) {
  // Tests that `tClient!Bot` can compile.
  auto transport = new TMemoryBuffer();
  auto protocol = new TCompactProtocol!TMemoryBuffer(transport);
  auto clientTop = tClient!Top(protocol);
  auto clientMid = tClient!Mid(protocol);
  auto clientBot = tClient!Bot(protocol);
  return 0;
}
