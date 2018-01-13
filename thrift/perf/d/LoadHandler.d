import std.conv, std.getopt, std.stdio, core.time, core.thread;

import load_types;
import LoadTest;

class LoadHandler : LoadTest {
  void burnImpl(long microseconds) {
    auto end = TickDuration.currSystemTick() + TickDuration.from!"usecs"(microseconds);
    while (end > TickDuration.currSystemTick()) {
    }
  }
  void noop(){}
  void onewayNoop(){}
  void asyncNoop(){}
  void sleep(long microseconds){
    Thread.sleep( dur!("usecs")(microseconds));
  }
  void onewaySleep(long microseconds){
    Thread.sleep( dur!("usecs")(microseconds));
  }
  void badSleep(long microseconds){
    Thread.sleep( dur!("usecs")(microseconds));
  }
  void burn(long microseconds) {
    burnImpl(microseconds);
  }
  void onewayBurn(long microseconds) {
    burnImpl(microseconds);
  }
  void badBurn(long microseconds){
    burnImpl(microseconds);
  }
  void throwImpl(int code) {
    auto err = new LoadError;
    err.code = code;
    throw err;
  }
  void throwError(int code){
    throwImpl(code);
  }
  void throwUnexpected(int code){
    throwImpl(code);
  }
  void onewayThrow(int code){
    throwImpl(code);
  }
  void send(string data){}
  void onewaySend(string data){}
  string recv(long data) {
    string ret;
    ret.length = data;
    return ret;
  }
  string sendrecv(string data, long recvBytes) {
    string ret;
    ret.length = recvBytes;
    return ret;
  }
  string echo(string data) {
    return data;
  }
  long add(long a, long b) {
    return a + b;
  }
  void largeContainer(BigStruct[] items) {
  }
  BigStruct[] iterAllFields(BigStruct[] items) {
  return items;
  }
}
