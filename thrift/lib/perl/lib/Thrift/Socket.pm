#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#

require 5.6.0;
use strict;
use warnings;

use Thrift;
use Thrift::Transport;

use IO::Socket::INET;
use IO::Select;

package Thrift::Socket;

use base('Thrift::Transport');

sub new
{
    my $classname    = shift;
    my $host         = shift || "localhost";
    my $port         = shift || 9090;
    my $debugHandler = shift;

    my $self = {
        host         => $host,
        port         => $port,
        debugHandler => $debugHandler,
        debug        => 0,
        timeout      => undef,
        handle       => undef,
    };

    return bless($self,$classname);
}


sub setTimeout
{
    my $self    = shift;
    my $timeout = shift;

    $self->{timeout} = $timeout;
}

#
#Sets debugging output on or off
#
# @param bool $debug
#
sub setDebug
{
    my $self  = shift;
    my $debug = shift;

    $self->{debug} = $debug;
}

#
# Tests whether this is open
#
# @return bool true if the socket is open
#
sub isOpen
{
    my $self = shift;

    if( defined $self->{handle} ){
        return $self->{handle}->connected;
    }

    return 0;
}

#
# Connects the socket.
#
sub open
{
    my $self = shift;
    my $sock = IO::Socket::INET->new(PeerAddr => $self->{host},
                                     PeerPort => $self->{port},
                                     Proto    => 'tcp',
                                     Timeout  => $self->{timeout} ? $self->{timeout}/1000 : undef);

    unless($sock) {
        my $error = 'TSocket: Could not connect to ' . $self->{host} . ':' .
            $self->{port}.' ('.$!.')';

        if ($self->{debug}) {
            $self->{debugHandler}->($error);
        }

        die new Thrift::TException($error);
    };

    $self->{handle} = $sock;
    $self->{selector} = IO::Select->new($sock);
}

#
# Closes the socket.
#
sub close
{
    my $self = shift;
    if(defined $self->{handle}) {
      $self->{handle}->close;
    }
    delete $self->{handle};
    delete $self->{selector};
}

sub can_read {
  my $self = shift;
  my @sockets = $self->{selector}->can_read(
    $self->{timeout} ? $self->{timeout} / 1000 : undef
  );
  return @sockets ? $sockets[0] : undef;
}

sub can_write {
  my $self = shift;
  my @sockets = $self->{selector}->can_write(
    $self->{timeout} ? $self->{timeout} / 1000 : undef
  );
  return @sockets ? $sockets[0] : undef;
}

#
# Uses stream get contents to do the reading
#
# @param int $len How many bytes
# @return string Binary data
#
sub readAll
{
    my $self = shift;
    my $len  = shift;


    my $pre = "";
    while (1) {
        unless($self->can_read) {
            die new Thrift::TException('TSocket: timed out reading '.$len.' bytes from '.
                                       $self->{host}.':'.$self->{port});
        }

        my ($buf,$sz);
        $self->{handle}->recv($buf, $len);

        if (!defined $buf || $buf eq '') {

            die new Thrift::TException('TSocket: Could not read '.$len.' bytes from '.
                               $self->{host}.':'.$self->{port}.": $!");

        } elsif (($sz = length($buf)) < $len) {

            $pre .= $buf;
            $len -= $sz;

        } else {
            return $pre.$buf;
        }
    }
}

#
# Read from the socket
#
# @param int $len How many bytes
# @return string Binary data
#
sub read
{
    my $self = shift;
    my $len  = shift;

    #check for timeout
    unless($self->can_read){
        die new Thrift::TException('TSocket: timed out reading '.$len.' bytes from '.
                                   $self->{host}.':'.$self->{port});
    }

    my ($buf,$sz);
    $self->{handle}->recv($buf, $len);

    if (!defined $buf || $buf eq '') {

        die new TException('TSocket: Could not read '.$len.' bytes from '.
                           $self->{host}.':'.$self->{port});

    }

    return $buf;
}


#
# Write to the socket.
#
# @param string $buf The data to write
#
sub write
{
    my $self = shift;
    my $buf  = shift;


    while (length($buf) > 0) {
        unless($self->can_write) {
            die new Thrift::TException('TSocket: timed out writing to bytes from '.
                                       $self->{host}.':'.$self->{port});
        }

        my $got = $self->{handle}->send($buf);

        if (!defined $got || $got == 0 ) {
            die new Thrift::TException('TSocket: Could not write '.length($buf).' bytes '.
                                 $self->{host}.':'.$self->{host});
        }

        $buf = substr($buf, $got);
    }
}

#
# Flush output to the socket.
#
sub flush
{
    my $self = shift;

    return unless defined $self->{handle};
    return $self->{handle}->flush;
}


#
# Build a ServerSocket from the ServerTransport base class
#
package  Thrift::ServerSocket;

our @ISA = qw( Thrift::Socket Thrift::ServerTransport );

use constant LISTEN_QUEUE_SIZE => 128;

sub new
{
    my $classname   = shift;
    my $port        = shift;

    my $self        = $classname->SUPER::new(undef, $port, undef);
    return bless($self,$classname);
}

sub listen
{
    my $self = shift;

    # Listen to a new socket
    my $sock = IO::Socket::INET->new(LocalAddr => undef, # any addr
                                     LocalPort => $self->{port},
                                     Proto     => 'tcp',
                                     Listen    => LISTEN_QUEUE_SIZE,
                                     ReuseAddr => 1)
        || do {
            my $error = 'TServerSocket: Could not bind to ' .
                        $self->{host} . ':' . $self->{port} . ' (' . $! . ')';

            if ($self->{debug}) {
                $self->{debugHandler}->($error);
            }

            die new Thrift::TException($error);
        };

    $self->{handle} = $sock;
}

sub timeout {
  my $self = shift;
 $self->{handle}->timeout(@_);
}

sub accept
{
    my $self = shift;

    if ( exists $self->{handle} and defined $self->{handle} )
    {
        my $client        = $self->{handle}->accept() or return;
        my $result        = ref($self)->new(
          $client->peerhost, $client->peerport
        );
        $result->{handle} = $client;
        $result->{selector} = new IO::Select($client);
        $result->{timeout} = $self->{timeout};
        return $result;
    }

    return 0;
}

1;
