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
#include <stdlib.h>
#include <iostream>
#include <gflags/gflags.h>
#include <folly/Conv.h>

#include <common/init/Init.h>
#include <thrift/lib/cpp2/security/packet_decryption_tool/DecryptionManager.h>

using std::string;
using std::exception;
using apache::thrift::DecryptionManager;

DEFINE_string(ccache_file, "",
    "Name of the krb5 ccache file to use. If neither this nor server keytab "
    "is not specified, "
    "environment variable KRB5CCNAME will be checked. "
    "If this environment variable isn't set, /tmp/krb5cc_$(id -u) "
    "will be used. If this file isn't usable the program terminates");

DEFINE_string(server_keytab, "",
    "Name of a keytab file in the server side. If this is specified, "
    "it will be used to decrypt the service ticket in the security "
    "negotiation message and ccache_file will be ignored.");

DEFINE_string(pcap_input_file, "",
    "Name of the pcap file to decrypt from, cannot be empty");

DEFINE_string(pcap_output_file, "",
    "Name of the pcap file to write decrypted packets, cannot be empty");

DEFINE_bool(verbose, false, "Print debugging messages");

static void usage() {
  std::cerr <<
    "\n"
    "This tool decrypts an encrypted thrift traffic from a pcap file\n"
    "and writes the result to another pcap file. The only difference\n"
    "between the input and output pcap file is the encrypted application\n"
    "packets are substituted with decrypted packets. The tcp handshake \n"
    "packets, security handshake packets are kept unchanged."
    "\n"
    "It relies on a credentials cache file or a service keytab file to\n"
    "fetch appropriate credentials to start the decryption process.\n"
    "If the credentials cache file (only available in the client side) is\n"
    "unavailable, outdated or already updated a service keytab file is \n"
    "needed.\n"
    "\n"
    "It works by grab the session key from the credentials cache file if\n"
    "provided. If not, it grabs the private service key from the service\n"
    "keytab file and use this key to decrypt the service ticket which\n"
    "contains the session key. The service ticket is part of the first\n"
    "security negotiation message. With this session key it is able to\n"
    "decrypt the security negotiation messages to grab the subsession key\n"
    "negotiated by the client and server to use for application message\n"
    "encryption/decryption.\n"
    "\n"
    "If a service keytab file is provided you may need to run this tool as\n"
    "root to read the service keytab file.\n"
    "\n"
    "Usage example:\n"
    "\tUse ccache file from client side:\n"
    "\t$> decryptor\n"
    "\t     --ccache_file /tmp/some_ccache\n"
    "\t     --pcap_input_file /tmp/input_pcap\n"
    "\t     --pcap_output_file /tmp/output_pcap\n"
    "\tOr use server keytab file from server side:\n"
    "\t$> decryptor\n"
    "\t     --server_keytab /var/facebook/krb5/krb5.keytab\n"
    "\t     --pcap_input_file /tmp/input_pcap\n"
    "\t     --pcap_output_file /tmp/output_pcap\n";
}

int main(int argc, char **argv) {
  facebook::initFacebook(&argc, &argv);

  try {
    if (FLAGS_ccache_file == "" && FLAGS_server_keytab == "") {
      char* ccache = getenv("KRB5CCNAME");
      if (ccache != nullptr) {
        FLAGS_ccache_file = string(ccache);
      } else {
        FLAGS_ccache_file = string("/tmp/krb5cc_") +
          folly::to<string>(getuid());
      }
    }

    if (FLAGS_pcap_input_file == "") {
      LOG(ERROR) << "pcap_input_file is empty";
      usage();
      return 1;
    }

    if (FLAGS_pcap_output_file == "") {
      LOG(ERROR) << "pcap_output_file is empty";
      usage();
      return 1;
    }

    if (FLAGS_server_keytab != "" && FLAGS_ccache_file != "" ) {
      LOG(ERROR) << "Only need one of server keytab and ccache_file";
      usage();
      return 1;
    }

    DecryptionManager manager(FLAGS_ccache_file,
                              FLAGS_server_keytab,
                              FLAGS_pcap_input_file,
                              FLAGS_pcap_output_file);
    manager.decrypt();
    manager.printSummary();
  } catch (const exception& e) {
    LOG(ERROR) << "Failed to decrypt: " << e.what();
    return 1;
  }

  LOG(INFO) << "Decryption completed successfully.";
  return 0;
}
