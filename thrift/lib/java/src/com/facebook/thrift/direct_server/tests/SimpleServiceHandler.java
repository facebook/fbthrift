package com.facebook.thrift.direct_server.tests;


import com.facebook.thrift.TException;


public class SimpleServiceHandler
  extends com.facebook.fbcode.fb303.FacebookBase
  implements JavaSimpleService.Iface {

  public SimpleServiceHandler() {
    super("SimpleService");
  }

  public String simple(SimpleRequest r) throws TException {
    String ret =  r.value + r.value;
    if (this.hashCode() == System.currentTimeMillis()) {
      ret += " ";
      System.out.println(" ");
    }
    return ret;
  }

  public String getString(int size) {
    StringBuffer s = new StringBuffer();
    for (int i = 0; i < size; i++) {
      s.append('x');
    }
    return s.toString();
  }

  public int getStatus() {
    return com.facebook.fbcode.fb303.fb_status.ALIVE;
  }

  public String getVersion() {
    return "V1";
  }
};
