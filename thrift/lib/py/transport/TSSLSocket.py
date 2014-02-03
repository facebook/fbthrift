from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from .TSocket import *
from .TTransport import *
import socket
import ssl
import traceback
import sys

# workaround for a python bug.  see http://bugs.python.org/issue8484
import hashlib

class TSSLSocket(TSocket):
    """Socket implementation that communicates over an SSL/TLS encrypted
    channel."""
    def __init__(self, host='localhost', port=9090, unix_socket=None,
                 ssl_version=ssl.PROTOCOL_TLSv1,
                 cert_reqs=ssl.CERT_NONE,
                 ca_certs=None,
                 verify_name=False,
                 keyfile=None,
                 certfile=None):
        """Initialize a TSSLSocket.

        @param ssl_version(int)  protocol version. see ssl module.
        @param cert_reqs(int)    whether to verify peer certificate. see ssl
                                 module.
        @param ca_certs(str)     filename containing trusted root certs.
        @param verify_name       if False, no peer name validation is performed
                                 if True, verify subject name of peer vs 'host'
                                 if a str, verify subject name of peer vs given
                                 str
        @param keyfile           filename containing the client's private key
        @param certfile          filename containing the client's cert and
                                 optionally the private key
        """
        TSocket.__init__(self, host, port, unix_socket)
        self.cert_reqs = cert_reqs
        self.ca_certs = ca_certs
        self.ssl_version = ssl_version
        self.verify_name = verify_name
        self.client_keyfile = keyfile
        self.client_certfile = certfile

    def open(self):
        TSocket.open(self)
        try:
            sslh = ssl.SSLSocket(self.handle,
                                 ssl_version=self.ssl_version,
                                 cert_reqs=self.cert_reqs,
                                 ca_certs=self.ca_certs,
                                 keyfile=self.client_keyfile,
                                 certfile=self.client_certfile)
            if self.verify_name:
                # validate the peer certificate commonName against the
                # hostname (or given name) that we were expecting.
                cert = sslh.getpeercert()
                str_type = (str, unicode) if sys.version_info[0] < 3 else str
                if isinstance(self.verify_name, str_type):
                    valid_names = self._getCertNames(cert)
                    name = self.verify_name
                else:
                    valid_names = self._getCertNames(cert, "DNS")
                    name = self.host
                match = False
                for valid_name in valid_names:
                    if self._matchName(name, valid_name):
                        match = True
                        break
                if not match:
                    sslh.close()
                    raise TTransportException(TTransportException.NOT_OPEN,
                            "failed to verify certificate name")
            self.handle = sslh
        except ssl.SSLError as e:
            raise TTransportException(TTransportException.NOT_OPEN,
                            "SSL error during handshake: " + str(e))
        except socket.error as e:
            raise TTransportException(TTransportException.NOT_OPEN,
                            "socket error during SSL handshake: " + str(e))

    @staticmethod
    def _getCertNames(cert, includeAlt=None):
        """Returns a set containing the common name(s) for the given cert. If
        includeAlt is not None, then X509v3 alternative names of type includeAlt
        (e.g. 'DNS', 'IPADD') will be included as potential matches."""
        # The "subject" field is a tuple containing the sequence of relative
        # distinguished names (RDNs) given in the certificate's data structure
        # for the principal, and each RDN is a sequence of name-value pairs.
        names = set()
        for rdn in cert.get('subject', ()):
            for k, v in rdn:
                if k == 'commonName':
                    names.add(v)
        if includeAlt:
            for k, v in cert.get('subjectAltName', ()):
                if k == includeAlt:
                    names.add(v)
        return names

    @staticmethod
    def _matchName(name, pattern):
        """match a DNS name against a pattern. match is not case sensitive.
        a '*' in the pattern will match any single component of name."""
        name_parts = name.split('.')
        pattern_parts = pattern.split('.')
        if len(name_parts) != len(pattern_parts):
            return False
        for n, p in zip(name_parts, pattern_parts):
            if p != '*' and (n.lower() != p.lower()):
                return False
        return True


class TSSLServerSocket(TServerSocket):
    """
    SSL implementation of TServerSocket

    Note that this does not support TNonblockingServer

    This uses the ssl module's wrap_socket() method to provide SSL
    negotiated encryption.
    """
    SSL_VERSION = ssl.PROTOCOL_TLSv1

    def __init__(self, port=9090, certfile='cert.pem', unix_socket=None):
        """Initialize a TSSLServerSocket

        @param certfile: The filename of the server certificate file, defaults
                         to cert.pem
        @type certfile: str
        @param port: The port to listen on for inbound connections.
        @type port: int
        """
        self.setCertfile(certfile)
        self.setCertReqs(ssl.CERT_NONE, None)
        TServerSocket.__init__(self, port, unix_socket)

    def setCertfile(self, certfile):
        """Set or change the server certificate file used to wrap new
        connections.

        @param certfile: The filename of the server certificate, i.e.
                         '/etc/certs/server.pem'
        @type certfile: str

        Raises an IOError exception if the certfile is not present or
        unreadable.
        """
        if not os.access(certfile, os.R_OK):
            raise IOError('No such certfile found: %s' % (certfile))
        self.certfile = certfile

    def setCertReqs(self, cert_reqs, ca_certs):
        """Set or change the parameters used to validate the client's
        certificate.  The parameters behave the same as the arguments to
        python's ssl.wrap_socket() method with the same name.
        """
        self.cert_reqs = cert_reqs
        self.ca_certs = ca_certs

    def accept(self):
        plain_client, addr = self._sock_accept()
        try:
            client = ssl.wrap_socket(plain_client,
                                     certfile=self.certfile,
                                     server_side=True,
                                     ssl_version=self.SSL_VERSION,
                                     cert_reqs=self.cert_reqs,
                                     ca_certs=self.ca_certs)
        except ssl.SSLError as ssl_exc:
            # failed handshake/ssl wrap, close socket to client
            plain_client.close()
            # raise ssl_exc
            # We can't raise the exception, because it kills most TServer
            # derived serve() methods.
            # Instead, return None, and let the TServer instance deal with it
            # in other exception handling.  (but TSimpleServer dies anyway)
            print(traceback.print_exc())
            return None

        return self._makeTSocketFromAccepted((client, addr))
