var Thrift = require('thrift').Thrift;
var CppThriftServer = require('CppThriftServer/CppThriftServer').CppThriftServer;
var TBinaryProtocol = require('thrift/lib/thrift/protocol').TBinaryProtocol;

var ttransport = require('thrift/lib/thrift/transport');

var TBufferedTransport = ttransport.TBufferedTransport;

function ThriftServer(service, methods) {
  this.server = new CppThriftServer();
  this.server.processor = new service.Processor(methods);
  this.server.setInterface(this.wrappedProcessor);
}

ThriftServer.prototype.listen = function(port) {
  this.server.setPort(port);
  this.server.serve();
}

ThriftServer.prototype.wrappedProcessor = function(server, callback, datain) {
  var transin = new TBufferedTransport(datain);
  var processor = server.processor
  TBufferedTransport.receiver(function(transin) {
    var protin = new TBinaryProtocol(transin);
    var transout = new TBufferedTransport(undefined, function(buf) {
      callback.sendReply(buf);
    });
    var protout = new TBinaryProtocol(transout);
    try {
     processor.process(protin, protout);
    } catch (err) {
      console.log(err);
      var x = new Thrift.TApplicationException(
        Thrift.TApplicationExceptionType.UNKNOWN_METHOD,
        'Unknown error'
      );
      protout.writeMessageBegin('', Thrift.MessageType.Exception, 0);
      x.write(protout);
      protout.writeMessageEnd();
      protout.flush();
    }
  })(datain);
}

module.exports = ThriftServer;
