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

abstract class ThriftAsyncProcessor extends ThriftProcessorBase
  implements IThriftAsyncProcessor {

  abstract const type TThriftIf as IThriftAsyncIf;

  final public async function processAsync(
    TProtocol $input,
    TProtocol $output,
  ): Awaitable<bool> {
    $rseqid = 0;
    $fname = '';
    $mtype = 0;

    $input->readMessageBegin($fname, $mtype, $rseqid);
    $methodname = 'process_'.$fname;
    if (!method_exists($this, $methodname)) {
      $handler_ctx = $this->eventHandler_->getHandlerContext($fname);
      $this->eventHandler_->preRead($handler_ctx, $fname, array());
      $input->skip(TType::STRUCT);
      $input->readMessageEnd();
      $this->eventHandler_->postRead($handler_ctx, $fname, array());
      $x = new TApplicationException(
        'Function '.$fname.' not implemented.',
        TApplicationException::UNKNOWN_METHOD,
      );
      $this->eventHandler_->handlerError($handler_ctx, $fname, $x);
      $output->writeMessageBegin($fname, TMessageType::EXCEPTION, $rseqid);
      $x->write($output);
      $output->writeMessageEnd();
      $output->getTransport()->flush();
      return true;
    }
    /* UNSAFE_EXPR[2011]: This is safe */
    await $this->$methodname($rseqid, $input, $output);
    return true;
  }

  final public function process(TProtocol $input, TProtocol $output): bool {
    return HH\Asio\join($this->processAsync($input, $output)->getWaitHandle());
  }
}
