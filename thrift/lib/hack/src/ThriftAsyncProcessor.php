<?hh

/*
 * Copyright 2006-present Facebook, Inc.
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

    $input->readMessageBegin(inout $fname, inout $mtype, inout $rseqid);
    RelativeScript::setMinorPath($fname);
    $methodname = 'process_'.$fname;
    if (!PHP\method_exists($this, $methodname)) {
      $handler_ctx = $this->eventHandler_->getHandlerContext($fname);
      $this->eventHandler_->preRead($handler_ctx, $fname, darray[]);
      $input->skip(TType::STRUCT);
      $input->readMessageEnd();
      $this->eventHandler_->postRead($handler_ctx, $fname, darray[]);
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
    /* HH_FIXME[2011]: This is safe */
    await $this->$methodname($rseqid, $input, $output);
    return true;
  }

  public function process(TProtocol $input, TProtocol $output): bool {
    return Asio::awaitSynchronously($this->processAsync($input, $output));
  }
}
