package com.facebook.thrift.perf;

import org.apache.commons.cli.*;

/*
 * Basic parser for load tester command line arguments.
 *
 * Add common options here, or derive from this class to add
 * options to affect load-testing of specific servers.
 */
public class LoadTesterArgumentParser {
  public void parseOptions(String[] args) throws Exception {
    registerOptions();
    PosixParser parser = new PosixParser();
    commandLine = parser.parse(options, args);
  }

  public void printUsage(Class mainClass) {
    HelpFormatter help = new HelpFormatter();
    String programName = mainClass.getSimpleName();
    String usageHeader = String.format("%s [options]", programName);
    help.printHelp(usageHeader, options);
  }

  public int getListenPort() {
    String value =
      commandLine.getOptionValue(OPT_PORT,
                                 Integer.toString(OPT_PORT_DEFAULT));
    return Integer.decode(value);
  }

  public int getNumberOfThreads() {
    String value =
      commandLine.getOptionValue(OPT_NUM_THREADS,
                                 Integer.toString(getNumberOfProcessors()));
    return Integer.decode(value);
  }

  public boolean getPrintUsage() {
    return commandLine.hasOption(OPT_HELP);
  }

  public int getNumberOfProcessors() {
    return Runtime.getRuntime().availableProcessors();
  }

  protected void registerOptions() {
    options.addOption(
      OptionBuilder.withLongOpt(OPT_HELP)
      .withDescription("Print this usage message")
      .create());

    options.addOption(
      OptionBuilder.withLongOpt(OPT_PORT)
      .hasArg()
      .withArgName("number")
      .withDescription("Port to bind for listening")
      .create());

    options.addOption(
      OptionBuilder.withLongOpt(OPT_NUM_THREADS)
      .hasArg()
      .withArgName("count")
      .withDescription("Number of task threads")
      .create());
  }

  protected Options options = new Options();
  protected CommandLine commandLine = null;

  private static final String OPT_HELP = "help";
  private static final String OPT_PORT = "port";
  private static final String OPT_NUM_THREADS = "num_threads";
  private static final int OPT_PORT_DEFAULT = 1234;
}
