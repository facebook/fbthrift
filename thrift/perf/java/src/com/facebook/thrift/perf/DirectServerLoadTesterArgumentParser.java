package com.facebook.thrift.perf;

import org.apache.commons.cli.*;

/*
 * Options specific to load-testing DirectServer
 */
public class DirectServerLoadTesterArgumentParser
  extends LoadTesterArgumentParser {

  public boolean getHsHaMode() {
    return commandLine.hasOption(OPT_HSHA_MODE);
  }

  public int getNumberOfPendingOperations() {
    String value =
      commandLine.getOptionValue(OPT_NUM_PENDING,
                                 Integer.toString(getNumberOfProcessors()));
    return Integer.decode(value);
  }

  protected void registerOptions() {
    super.registerOptions();

    options.addOption(
      OptionBuilder.withDescription("Run in half-sync half-async mode")
      .withLongOpt(OPT_HSHA_MODE)
      .create());

    options.addOption(
      OptionBuilder.withDescription("Number of pending operations")
      .withLongOpt(OPT_NUM_PENDING)
      .create());
  }

  private static final String OPT_HSHA_MODE = "hsha_mode";
  private static final String OPT_NUM_PENDING = "num_pending";
}
