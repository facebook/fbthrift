package com.facebook.thrift.perf;

import java.lang.InterruptedException;
import java.lang.Thread;
import java.lang.System;
import com.facebook.thrift.TException;
import com.facebook.fbcode.fb303.fb_status;
import com.facebook.fbcode.fb303.FacebookBase;
import java.util.List;

public class LoadTestHandler extends FacebookBase implements LoadTest.Iface  {
  public LoadTestHandler() {
    super("LoadTestHandler");
  }

  @Override
  public int getStatus() {
    return fb_status.ALIVE;
  }

  @Override
  public String getVersion() {
    return "";
  }

  public void noop() {
  }

  public void onewayNoop() {
  }

  public void asyncNoop() {
  }

  public long add(long a, long b) {
    return a + b;
  }

  public byte[] echo(byte[] data) {
    return data;
  }

  public void send(byte[] data) {
  }

  public byte[] recv(long recvBytes) {
    byte[] array = new byte[(int)recvBytes];
    return array;
  }

  public byte[] sendrecv(byte[] data, long recvBytes) {
    return recv(recvBytes);
  }

  public void onewaySend(byte[] data) {
  }

  public void onewayThrow(int code) throws TException {
    throw new TException();
  }

  public void throwUnexpected(int code) throws TException {
    throw new TException();
  }

  public void throwError(int code) throws LoadError {
    throw new LoadError(code);
  }

  public void sleep(long microseconds) {
    try {
      long ms = microseconds / 1000;
      int us = (int)(microseconds % 1000);
      Thread.sleep(ms, us);
    }
    catch (InterruptedException e) {
    }
  }

  public void onewaySleep(long microseconds) {
    sleep(microseconds);
  }

  public void badBurn(long microseconds) {
    burnImpl(microseconds);
  }

  public void badSleep(long microseconds) {
    burnImpl(microseconds);
  }

  public void onewayBurn(long microseconds) {
    burnImpl(microseconds);
  }

  public void burn(long microseconds) {
    burnImpl(microseconds);
  }

  private void burnImpl(long microseconds) {
    long end = System.nanoTime() + microseconds;
    while (System.nanoTime() < end) {}
  }

  public void largeContainer(List<BigStruct> items) throws TException {
  }

  public List<BigStruct> iterAllFields(List<BigStruct> items) throws TException {
    return items;
  }
}
