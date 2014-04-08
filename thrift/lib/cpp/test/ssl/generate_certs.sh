#!/bin/bash

# This script exists to generate the test_key.pem, test_cert.pem,
# ca_key.pem, and ca_cert.pem files.  It shouldn't really ever be run again
# until the year 2041 (when these certs expire)., unless you need to change
# or update attributes of the certificate (Common Name, Organization,
# whatever).

DAYS=10000

# Generate Certificate Authority Key
openssl genrsa -out ca-key.pem 2048

# Generate Self-Signed Certificate Authority Cert
openssl req -x509 -new -nodes \
    -key ca-key.pem \
    -days ${DAYS} \
    -out ca-cert.pem \
    -subj '/C=US/O=Thrift/CN=Thrift Certificate Authority'

# CA serial number
echo 00000009 > ca-cert.srl

# Generate the test key
openssl genrsa -out tests-key.pem 2048

# Generate the test key certificate request
openssl req -new -nodes \
    -key tests-key.pem \
    -days ${DAYS} \
    -out tests-cert.csr \
    -subj '/C=US/O=Asox/CN=test.thrift.org'

# Sign the test key
openssl x509 -req \
    -in tests-cert.csr \
    -CA ca-cert.pem \
    -CAkey ca-key.pem \
    -out tests-cert.pem

# Clean up the signing request as well as the serial number
rm ca-cert.srl tests-cert.csr
