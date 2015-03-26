var Thrift = require('thrift').Thrift;
var CppThriftServer = require('CppThriftServer/CppThriftServer').CppThriftServer;
var TBinaryProtocol = require('thrift/lib/thrift/protocol').TBinaryProtocol;

var ttransport = require('thrift/lib/thrift/transport');

var TBufferedTransport = ttransport.TBufferedTransport;

function wrappedProcessor(server, callback, datain) {
  var transin = new TBufferedTransport(datain);
  var processor = server.processor;
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

function ThriftServer(service, methods) {
  // Setup environment variable for secure thrift.
  process.env['KRB5_CONFIG'] = '/etc/krb5-thrift.conf';
  process.env['KRB5RCACHETYPE'] = 'none';

  this.server = new CppThriftServer();
  this.server.processor = new service.Processor(methods);
  this.server.setInterface(wrappedProcessor);
}

ThriftServer.prototype.listen = function(port) {
  this.server.setPort(port);
  this.server.serve();
  return this;
}

ThriftServer.prototype.setTimeout = function(timeout) {
  this.server.setTimeout(timeout);
  return this;
}

module.exports = ThriftServer;
