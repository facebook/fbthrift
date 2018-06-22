package main

import (
	"flag"
	"net"
	"os"
	"os/signal"
	"syscall"

	"github.com/golang/glog"
)

func init() {
	flag.Set("logtostderr", "true") // make glog log to stderr by default
}

func main() {
	host := flag.String("host", "", "Server host")
	port := flag.String("port", "7777", "Server port")
	flag.Parse()

	sig := make(chan os.Signal, 1)
	signal.Notify(sig, syscall.SIGTERM, os.Interrupt)
	go func() {
		<-sig
		glog.Info("Shutting down.")
		// Do any cleanup work here
		os.Exit(0)
	}()
	addr := net.JoinHostPort(*host, *port)
	// Start the thrift server
	glog.Fatal(Serve(addr))
}
