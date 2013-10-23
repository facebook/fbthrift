/**
 * Copyright 2011 Facebook
 * @author Wei Chen (weichen@fb.com)
 *
 * A class to manage moving average stats. The class can be used to
 * calculate 1-minute, 10-minute, 60-minute simple moving averages.
 */

package com.facebook.thrift.direct_server;

import java.util.concurrent.ConcurrentLinkedQueue;

import com.facebook.fbcode.fb303.FacebookBase;


public class MovingAvgStat {

  // An array that saves per minute history
  public long vals_[];

  // Stores 10 minutes sum
  public long sumOf10Min_;

  // Stores 1 hour sum
  public long sumOf60Min_;

  // Total sum for the stat
  public long sumOfAll_;

  // For average stats, this is the data source for denominator.
  // Can be null if the stat only cares about sum.
  public MovingAvgStat base_;

  // An index into history array @vals_.
  public int idx_;

  // The object that display stats values
  protected FacebookBase handler_;


  protected final String nameOf1Min_;

  protected final String nameOf10Min_;

  protected final String nameOf60Min_;

  protected final String nameOfAll_;

  protected final String nameOf1MinAvg_;

  protected final String nameOf10MinAvg_;

  protected final String nameOf60MinAvg_;

  protected final String nameOfAllAvg_;


  // A singleton that updates fb303 counters periodically.
  static class StatsManager extends Thread {

    // All stats, registered within MovingAvgStat constructor.
    private ConcurrentLinkedQueue<MovingAvgStat> stats_;

    // Update fb303 counters every 20 seconds.
    static private int kUpdateInterval = 20000;

    StatsManager() {
      stats_ = new ConcurrentLinkedQueue<MovingAvgStat>();
      start();
    }

    // Called by MovingAvgStat constructor to register.
    void add(MovingAvgStat m) {
      stats_.add(m);
    }

    // Refresh fb303 counters periodically.
    @Override public void run() {
      long dur = 60000;
      long ms = System.currentTimeMillis();
      long nextUpdate = (ms / dur + 1) * dur;

      while (true) {
        if (ms < nextUpdate) {
          // Do not rotate history
          for (MovingAvgStat s : stats_) {
            synchronized(s) {
              s.updateFb303(false);
            }
          }

        } else {
          nextUpdate = (ms / dur + 1) * dur;
          for (MovingAvgStat s : stats_) {
            // Rotate history
            synchronized(s) {
              s.updateFb303(true);

              int nidx = (s.idx_ + 1) % MovingAvgStat.valueTotalSize();

              s.sumOf10Min_ -= s.vals_[nidx];
              s.sumOf60Min_ -= s.vals_[nidx];

              s.vals_[nidx] = 0;
              s.idx_ = nidx;
            }
          }
        }

        ms = System.currentTimeMillis();

        // Only sleep if we haven't already passed nextUpdate
        if (ms <= nextUpdate) {
          long exp = (ms + kUpdateInterval > nextUpdate) ?
            nextUpdate - ms : kUpdateInterval;

          try {
            Thread.sleep(exp);
          } catch (InterruptedException e) {
            // Do nothing, start next iteration.
          }

          ms = System.currentTimeMillis();
        }
      }
    }
  };


  // A singleton StatsManager that update fb303 counters periodically
  static StatsManager manager_ = new StatsManager();


  // Return array length of @vals_.
  final public static int valueTotalSize() {
    return 60;
  }


  public MovingAvgStat(FacebookBase h, String fName, MovingAvgStat b) {
    handler_ = h;
    vals_ = new long[valueTotalSize()];
    base_ = b;

    for (int i = 0; i < vals_.length; ++i) {
      vals_[i] = 0;
    }

    nameOf1Min_ = "thrift." + fName + ".sum.60";
    nameOf10Min_ = "thrift." + fName + ".sum.600";
    nameOf60Min_ = "thrift." + fName + ".sum.3600";
    nameOfAll_ = "thrift." + fName + ".sum";

    nameOf1MinAvg_ = "thrift." + fName + ".avg.60";
    nameOf10MinAvg_ = "thrift." + fName + ".avg.600";
    nameOf60MinAvg_ = "thrift." + fName + ".avg.3600";
    nameOfAllAvg_ = "thrift." + fName + ".avg";

    idx_ = 0;
    manager_.add(this);
  }

  public void addValue(long v) {
    synchronized(this) {
      int i = idx_ % valueTotalSize();
      vals_[i] += v;
      sumOf10Min_ += v;
      sumOf60Min_ += v;
      sumOfAll_ += v;
    }
  }

  public void updateFb303(boolean r) {
    handler_.setCounter(nameOf1Min_, vals_[idx_]);
    handler_.setCounter(nameOf10Min_, sumOf10Min_);
    handler_.setCounter(nameOf60Min_, sumOf60Min_);
    handler_.setCounter(nameOfAll_, sumOfAll_);

    if (base_ != null) {
      int temp_index = base_.idx_ - (r ? 1 : 0);
      if (temp_index < 0) {
        temp_index += MovingAvgStat.valueTotalSize();
      }
      long tmp = base_.vals_[temp_index];
      handler_.setCounter(nameOf1MinAvg_, tmp > 0 ? vals_[idx_] / tmp : 0);

      tmp = base_.sumOf10Min_;
      handler_.setCounter(nameOf10MinAvg_, tmp > 0 ? sumOf10Min_ / tmp : 0);

      tmp = base_.sumOf60Min_;
      handler_.setCounter(nameOf60MinAvg_, tmp > 0 ? sumOf60Min_ / tmp : 0);

      tmp = base_.sumOfAll_;
      handler_.setCounter(nameOfAllAvg_, tmp > 0 ? sumOfAll_ / tmp : 0);
    } else {
      handler_.setCounter(nameOf1MinAvg_, vals_[idx_] / 60);
      handler_.setCounter(nameOf10MinAvg_, sumOf10Min_ / 600);
      handler_.setCounter(nameOf60MinAvg_, sumOf60Min_ / 3600);
      handler_.setCounter(nameOfAllAvg_, 0);
    }
  }
};
