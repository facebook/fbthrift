namespace cpp2 static_reflection.demo

typedef map<string, string> legacy_config

const legacy_config example = {
  "host-name": "localhost",
  "host-port": "80",
  "client-name": "my_client",
  "socket-send-timeout": "100",
  "socket-receive-timeout": "120",
  "transport-frame-size": "1024",
  "apply-compression": "1",
  "log-sampling-rate": ".01"
}
