<?hh // strict

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift
*/

/**
 * This allows clients to perform calls as generators
 */
class TClientAsyncHandler {
  // Called before send in gen_methodName() calls
  public async function genBefore(): Awaitable<void> {
    // Do nothing
  }

  // Called between the send and recv for gen_methodName() calls
  public async function genWait(int $sequence_id): Awaitable<void> {
    // Do nothing
  }

  // Called after recv in gen_methodName() calls
  public async function genAfter(): Awaitable<void> {
    // Do nothing
  }
}
