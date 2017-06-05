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
  // Also see libfb/py/controller/base.py:makeThriftService
  // and common/config/KerberosConfig.h:setupSecureThrift
  process.env['KRB5_CONFIG'] = '/etc/krb5-thrift.conf';
  process.env['KRB5RCACHETYPE'] = 'none';

  this.server = new CppThriftServer();
  this.server.processor = new service.Processor(methods);
  this.server.setInterface(wrappedProcessor);
}

ThriftServer.SSLPolicy = Object.freeze({
  PERMITTED: "permitted",
  REQUIRED: "required"
});

ThriftServer.SSLVerify = Object.freeze({
  IF_SENT: "verify",
  REQUIRED: "verify_required",
  NONE: "no_verify"
});

ThriftServer.SSLConfig = function() {
  this.certPath = "";
  this.keyPath = "";
  this.keyPwPath = "";
  this.clientCaPath = "";
  this.eccCurveName = "";
  this.verify = ThriftServer.SSLVerify.IF_SENT;
  this.policy = ThriftServer.SSLPolicy.PERMITTED;
  this.ticketFilePath = "";
  this.alpnProtocols = [];
  this.sessionContext = "";
};

ThriftServer.prototype.setSSLConfig = function(config) {
  if (!(config instanceof ThriftServer.SSLConfig)) {
    throw new Error("Config must be instance of ThriftServer.SSLConfig");
  }
  // validate verify and policy
  var validVerify = ["verify", "verify_required", "no_verify"];
  if (validVerify.indexOf(config.verify) === -1) {
    throw new Error("config.verify is invalid.  Must be one of: " +
      validVerify.join(", "));
  }
  var validPolicy = ["permitted", "required"];
  if (validPolicy.indexOf(config.policy) === -1) {
    throw new Error("config.policy is invalid.  Must be one of: " +
      validPolicy.join(", "));
  }
  this.server.setSSLConfig(config);
};

ThriftServer.prototype.listen = function(port) {
  this.server.setPort(port);
  this.server.serve();
  return this;
};

ThriftServer.prototype.setTimeout = function(timeout) {
  this.server.setTimeout(timeout);
  return this;
};

module.exports = ThriftServer;
