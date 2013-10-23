#!/bin/env python2.7

import sys
import subprocess
import re
import os
import argparse
import time

machine_prefix = ""
server_machine = ""
client_machine = ""
machines = []

def twstate():
    out = subprocess.check_output(["twutil", "search",
                                   "thriftperf"])
    for line in out.split('\n'):
        if line[0:5] == "State":
            return line[7:]

def twmachines():
    out = subprocess.check_output(["twutil", "search",
                                   "thriftperf"])
    for line in out.split('\n'):
        if line[0:15] == "Taskid: 0 Job: ":
            global machine_prefix
            if machine_prefix != "": # Only find the first one
                break
            machine_prefix = line[15:]
            print "Machine prefix is: " + machine_prefix
        if line[0:10] == "Hostport: ":
            machines.append(line[10:-5])
    global server_machine
    global client_machine
    server_machine = machine_prefix + "/0"
    client_machine = machine_prefix + "/1"

def twstart():
    print "===Deploying to Tupperware Service Pool==="
    subprocess.check_call(["twdeploy", "start", "--silent",
                           "tupperware/config/thrift/thrift_perf.tw"])
    state = twstate()
    while state and state[0:18] != "Running[Running:2]":
        print state
        state = twstate()
        time.sleep(1)
    print

def twstop():
    print "===Undeploying to Tupperware Service Pool==="
    subprocess.check_call(["twdeploy", "stop", "--silent",
                           "tupperware/config/thrift/thrift_perf.tw"])
    subprocess.check_call(["twdeploy", "wait_stop",
                           "tupperware/config/thrift/thrift_perf.tw"])

def build():
    out = subprocess.check_output(["git", "diff"])
    if out != "":
        print "You have uncommited changes."
        sys.exit()

    out = subprocess.check_output(
        ["git", "merge-base", "origin/master", "HEAD"])
    merge_point = out.strip()
    print "Your current branch is from " + merge_point

    out = subprocess.check_output(["git", "diff", merge_point])
    if out == "":
        print "You do not have any local changes."
        sys.exit()

    print "===Your local commits==="
    print subprocess.check_output(
        ["git", "log", "--pretty=oneline", merge_point + "..HEAD"]).strip()

    print "===Building Ephemeral Fbpackage==="
    subprocess.check_call(["fbpackage", "-E", "--tag", "base",
                           "build", "thrift/perftest"])
    print "===Building origin/master Ephemeral Fbpackage==="
    subprocess.check_call(["git", "checkout", merge_point])
    subprocess.check_call(["fbpackage", "-E", "--tag", "test",
                           "build", "thrift/perf"])

    subprocess.check_call(["git", "checkout", "@{-1}"])

def start_server(server, test):
    f = open("/dev/null", "w")
    subprocess.Popen(["twutil", "ssh", server_machine,
                           "sh -c '( (nohup " + server + " " + test +
                           " 2>&1 >/dev/null </dev/null) & )'"], stdout=f)
    # Wait for other process to start
    time.sleep(2)
    f.close()

def stop_server(server):
    subprocess.check_output(["twutil", "ssh", server_machine,
                           "killall " + server.split('/')[-1]])

def get_cpu_used(before, after):
    before = before.splitlines()[0]
    after = after.splitlines()[0]
    before = before.split(' ')[2:9]
    after = after.split(' ')[2:9]
    before = [int(a) for a in before]
    after = [int(a) for a in after]
    before_tot = sum(before)
    after_tot = sum(after)
    tot_diff = after_tot - before_tot
    idle_diff = after[3] - before[3]
    cpu_percent_used = float(idle_diff) / float(tot_diff) * 100.0
    return cpu_percent_used

def print_server_hardware_info():
    out = subprocess.check_output(["twutil", "ssh", server_machine,
        "cat /proc/cpuinfo"])
    info = out.strip().split('\n')
    num_cores = len([l for l in info if l.startswith("processor")])
    model = [l for l in info if l.startswith("model name")][0].split(":")[1]

    print "Server {0} has ".format(server_machine)
    print "%d %s" % (num_cores, model)

    out = subprocess.check_output(["twutil", "ssh", server_machine,
        "cat /proc/meminfo"])
    info = out.strip().split('\n')
    print info[0]
    print info[1]

    out = subprocess.check_output(["twutil", "ssh", server_machine,
        "hostname"])
    print "Hostname: " + out.strip()
    print ("Run \"twutil ssh {0} cat /proc/cpuinfo\" or \"twutil ssh {0}" +
            " cat /proc/meminfo\" to get more info\n").format(server_machine)


def run_loadtest(prog, test):
    before_cpu = subprocess.check_output(
        ["twutil", "ssh", server_machine,
         "cat /proc/stat"])
    out = subprocess.check_output(
        ["twutil", "ssh", client_machine,
         "bash -c '(" + prog +
         " --server=" + machines[0] + " " +
         test + "& sleep " + str(args.time) + " && killall loadgen )' 2>&1"])
    after_cpu = subprocess.check_output(
        ["twutil", "ssh", server_machine,
         "cat /proc/stat"])
    serverload = "%.2f" % get_cpu_used(before_cpu, after_cpu)
    line = out.strip().split('\n')[-1].strip().strip('|')
    run = [arg for arg in re.split(' |/', line) if arg != '']
    run.insert(0, serverload)
    return run

def test():
    twmachines()  # Get the machines to use
    print_server_hardware_info()

    ourtests = []
    for i in range(0, len(tests)):
        if i in args.run_test:
            ourtests.append(tests[i])
    for server, labels, servertest, clienttest in ourtests:
        try:
            print "===Test " + server + " " + \
                servertest + ", " + clienttest + "==="
            print "===Starting Base Loadtest for " + server + "==="
            start_server("/packages/thrift/perf/" + server, servertest)
            base = run_loadtest("/packages/thrift/perf/loadgen", clienttest)
            stop_server("/packages/thrift/perf/" + server)
            print "===Starting Canary Loadtest for " + server + "==="
            start_server("/packages/thrift/perftest/" + server, servertest)
            canary = run_loadtest(
                "/packages/thrift/perftest/loadgen", clienttest)
            stop_server("/packages/thrift/perftest/" + server)
            diff = []
            print "===Results==="
            try:
                for i in range(0, len(base)):
                    if float(base[i]) == 0:
                        diff.append(0)
                    else:
                        diff.append((float(canary[i]) - float(base[i])) / \
                                        float(base[i]) * 100.0)
                label = labels.split(",")
                label.insert(0, "Server CPU")
                for i in range(0, len(label)):
                    print label[i] + " Base: " + base[i] + " Canary: " + \
                        canary[i] + " diff: " + ("%.2f" % diff[i]) + "%",
                    print " WARNING" * min(3, int(abs(diff[i]) / 10))
            except:
                print "FAILED PRINTING RESULTS - BAD DATA?"
        except:
            print "FAILED TO RUN TEST"

tests = [
    ("ThriftServer",
     "QPS,Latency,stddev",
     "--num_threads=32 -num_queue_threads=32 --enable_service_framework",
     "-weight_noop=1 -cpp2 -ops_per_conn=100000"
     " -async -num_threads=32 -async_ops=1000 -enable_service_framework"),
    ("ThriftServer",
     "QPS,Latency,stddev",
     "--num_threads=32 -num_queue_threads=32 --enable_service_framework",
     "-weight_noop=1 -cpp2 -ops_per_conn=100000"
     " -async -num_threads=32 -async_ops=1000"
     " -cpp2 -enable_service_framework"),
    ("ThriftServer",
     "QPS,Latency,stddev",
     "--num_threads=32 -num_queue_threads=32 --enable_service_framework",
     "-weight_burn=1 -cpp2 -ops_per_conn=100000"
     " -async -num_threads=32 -burn_avg=0 -cpp2 -enable_service_framework"),
    ("ThriftServer",
     "QPS,Latency,stddev",
     "--num_threads=32 --num_queue_threads=200 --enable_service_framework",
     "-weight_burn=1 -cpp2 -ops_per_conn=100000"
     " -async -num_threads=32 -burn_avg=0 -cpp2 -enable_service_framework"),
    ("ThriftServer",
     "QPS,Latency,stddev",
     "--num_threads=32 --num_queue_threads=200 --enable_service_framework",
     "-weight_burn=1 -cpp2 -ops_per_conn=100000"
     " -async -num_threads=32 -burn_avg=1000"
     " -cpp2 -enable_service_framework"),
    ("ThriftServer",
     "QPS,Latency,stddev",
     "--num_threads=32 --num_queue_threads=32 --enable_service_framework",
     "-weight_echo=1 -cpp2 -ops_per_conn=100000"
     " -async -num_threads=32 -burn_avg=1000"
     " -cpp2 -enable_service_framework"),
    ("EventServer",
     "QPS,Latency,stddev",
     "--num_threads=32 --enable_service_framework",
     "-weight_noop=1 -async -num_threads=32 -async_ops=1000"
     " -enable_service_framework"),
    ("EventServer",
     "QPS,Latency,stddev",
     "--num_threads=32 --task_queue_mode --num_queue_threads=200"
     " --enable_service_framework",
     "-weight_burn=1 -async -num_threads=32 -burn_avg=0"
     " -enable_service_framework"),
    ]

parser = argparse.ArgumentParser(description="Thrift Loadtest Canary")
parser.add_argument('-d', '--deploy',
                    help='Do not build and build the fbpackages',
                    default=False, action='store_true')
parser.add_argument('-T', '--time', type=int, default=60,
                    help='Time to run each test for')
parser.add_argument('--print_tests', help='Print the tests',
                    action='store_true')
parser.add_argument('-r', '--run_test', type=int,
                    help='Tests to run', action='append')
parser.add_argument('-f', '--force',
                    help='Force run the tests without reserving',
                    action='store_true')

args = parser.parse_args()

if args.time < 2:
    print "Test time must be > 1"
    sys.exit()

if args.force:
    twstop()

if not args.run_test:
    args.run_test = range(0, len(tests))

if args.print_tests:
    for x in range(0, len(tests)):
        print str(x) + ": " + str(tests[x])
    sys.exit()

state = twstate()
if state and state[0:7] == "Running":
    print "Loadtest is already running, force quit with -f"
    sys.exit()

if not args.deploy:
    build()

try:
    twstart()

    test()
finally:
    twstop()

print "===Done==="
