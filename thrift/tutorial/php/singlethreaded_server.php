#!/usr/bin/env php
<?php
/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

$GLOBALS['THRIFT_ROOT'] = realpath(dirname(__FILE__).'/../..').'/lib/php/src';

require_once realpath(dirname(__FILE__)).'/CalculatorLib.php';

require_once $GLOBALS['THRIFT_ROOT'].'/transport/TSocket.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/THttpClient.php';
require_once $GLOBALS['THRIFT_ROOT'].'/server/TServer.php';

/**
 * DO NOT USE THIS PHP SERVER IN PRODUCTION!!!
 *
 * PHP was never designed to run for more than a single request,
 * so you will discover that this server leaks memory like mad.
 *
 * Acceptable uses for a php thrift server include:
 *       -- demo thrift services
 *       -- internal-only non-critical servers you have designed to restart
 *          as often as necessary to keep the memory footprint reasonable
 */

try {
  $handler              = new CalculatorHandler();
  $processor            = new CalculatorProcessor($handler);
  $singlethreadedserver = new TSimpleServer($processor,
                                            new TServerSocket(9090),
                                            new TTransportFactoryBase(),
                                            new TBinaryProtocolFactory());
  $singlethreadedserver->serve();
} catch (TException $tx) {
  echo 'TException: ' . $tx->getMessage() . "\n";
}

?>
