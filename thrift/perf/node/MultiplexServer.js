var options = require('optimist')
  .demand(['port'])
  .argv;

var JSService = require('load/LoadTest');

var fb303_types = require('fb303/fb303_types');
var microtime = require('microtime');
var thrift = require('thrift');
var ttransport = require('thrift/lib/thrift/transport');
var TException = thrift.Thrift.TException;

var VERSION = '3';

var methods = {
  noop : function(callback) {
    callback(null);
  },
  onewayNoop : function(callback) {
    callback(null);
  },
  asyncNoop : function(callback) {
    process.nextTick(function() { callback(null); });
  },
  sleep : function(us, callback) {
    setTimeout(function() { callback(null); }, us/1000);
  },
  badSleep : function(us, callback) {
    var stop = new Date().getTime();
    while(new Date().getTime() < stop + us/1000) {
      ;
    }
    callback(null);
  },
  badBurn : function(us, callback) {
    var stop = new Date().getTime();
    while(new Date().getTime() < stop + us/1000) {
      ;
    }
    callback(null);
  },
  burn : function(us, callback) {
    var stop = new Date().getTime();
    while(new Date().getTime() < stop + us/1000) {
      ;
    }
    callback(null);
  },
  throwError : function(code, callback) {
    var err = new LoadError();
    err.code = code;
    callback(err);
  },
  throwUnexpected : function(code, callback) {
    var err = new LoadError();
    err.code = code;
    callback(err);
  },
  onewayThrow : function(code, callback) {
    var err = new LoadError();
    err.code = code;
    callback(err);
  },
  send : function(data, callback) {
    callback(null);
  },
  onewaySend : function(data) {
  },
  recv : function(bytes, callback) {
    data = "";
    for (var i=0; i < bytes; i++) {
      data.concat("x");
    }
    callback(null, data);
  },
  sendrecv : function(data, bytes, callback) {
    newdata = "";
    for (var i=0; i < bytes; i++) {
      newdata.concat("x");
    }
    callback(null, newdata);
  },
  echo : function(data, callback) {
    callback(null, data);
  },
  add : function(a, b, callback) {
    callback(null, a + b);
  },
  getStatus: function(callback) {
    callback(null, fb303_types.fb_status.ALIVE);
  }
};

var app = thrift.createServer(
  JSService,
  methods,
  {transport: ttransport.TFramedTransport}
);
app.on('error', function(error) {
  console.warn(error);
});
app.listen(options.port, '::');
