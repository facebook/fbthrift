/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "thrift/lib/cpp2/security/KerberosSASLHandshakeClient.h"
#include "thrift/lib/cpp2/security/KerberosSASLHandshakeServer.h"
#include "common/init/Init.h"

using namespace apache::thrift;
using namespace std;
using namespace facebook;

DEFINE_string(client_principal, "", "Client principal");
DEFINE_string(service_principal, "", "Service principal");

/**
 * Simple test program for the Kerberos SASL Handshake class.
 */
int main(int argc, char** argv) {
  initFacebook(&argc, &argv);

  auto cc_manager = std::make_shared<krb5::Krb5CredentialsCacheManager>();

  KerberosSASLHandshakeClient client;
  client.setCredentialsCacheManager(cc_manager);
  KerberosSASLHandshakeServer service;

  CHECK(FLAGS_service_principal != "");

  client.setRequiredClientPrincipal(FLAGS_client_principal);
  client.setRequiredServicePrincipal(FLAGS_service_principal);

  client.startClientHandshake();
  auto cl_token = client.getTokenToSend();
  if (cl_token)
    cout << "Client sending token of size: " << cl_token->size() << endl;
  std::shared_ptr<string> sv_token;

  while (!client.isContextEstablished() || !service.isContextEstablished()) {
    if (!client.isContextEstablished() && sv_token != nullptr) {
      client.handleResponse(*sv_token);
      sv_token = nullptr;
      cl_token = client.getTokenToSend();
      if (cl_token)
        cout << "Client sending token of size: " << cl_token->size() << endl;
    }
    if (!client.isContextEstablished() && cl_token != nullptr) {
      service.handleResponse(*cl_token);
      cl_token = nullptr;
      sv_token = service.getTokenToSend();
      if (sv_token)
        cout << "Server sending token of size: " << sv_token->size() << endl;
    }
  }

  cout << "Context established" << endl;
  cout << "Client - client: " <<
    client.getEstablishedClientPrincipal() << endl;
  cout << "Client - service: " <<
    client.getEstablishedServicePrincipal() << endl;
  cout << "Service - client: " <<
    service.getEstablishedClientPrincipal() << endl;
  cout << "Service - service: " <<
    service.getEstablishedServicePrincipal() << endl;
  return 0;
}
