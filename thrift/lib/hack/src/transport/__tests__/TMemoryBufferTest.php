<?hh
/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

<<Oncalls('thrift')>>
final class TMemoryBufferTest extends WWWTest {

  use ClassLevelTest;

  public static function providePeekCases(): dict<string, shape(
    'buffer_contents' => string,
    'length' => int,
    'expected' => string,
  )> {
    return dict[
      'a single character' => shape(
        'buffer_contents' => 'abc',
        'length' => 1,
        'expected' => 'a',
      ),
      'a multiple characters' => shape(
        'buffer_contents' => 'abc',
        'length' => 2,
        'expected' => 'ab',
      ),
      'an empty buffer' => shape(
        'buffer_contents' => '',
        'length' => 1,
        'expected' => '',
      ),
    ];
  }

  <<DataProvider('providePeekCases')>>
  public function testPeek(
    string $buffer_contents,
    int $length,
    string $expected,
  ): void {
    $buffer = new TMemoryBuffer($buffer_contents);
    expect($buffer->peek($length))->toBePHPEqual($expected);
  }

  public function testPeekPastTheEnd(): void {
    $buffer = new TMemoryBuffer('abc');
    expect(vec[$buffer->peek(1, 3), $buffer->peek(1, 5)])->toEqual(vec['', '']);
  }

  // The return value, or the exception class and message, as one string.
  private static function outcome((function(): string) $fn): string {
    try {
      return 'ok '.PHP\bin2hex($fn());
    } catch (Exception $e) {
      return 'threw '.Classnames::getx($e).': '.$e->getMessage();
    }
  }

  public static function provideReadAllCases(): dict<
    string,
    shape('initial' => string, 'reads' => vec<int>, 'expected' => vec<string>),
  > {
    return dict[
      'exact reads' => shape(
        'initial' => 'abcdef',
        'reads' => vec[1, 2, 3],
        'expected' => vec['ok 61', 'ok 6263', 'ok 646566'],
      ),
      'zero and negative lengths' => shape(
        'initial' => 'abc',
        'reads' => vec[0, -5, 1],
        'expected' => vec['ok ', 'ok ', 'ok 61'],
      ),
      'a short read consumes nothing' => shape(
        'initial' => 'abc',
        'reads' => vec[1, 4, 2],
        'expected' => vec[
          'ok 61',
          'threw TTransportException: TMemoryBuffer: Could not read 4 bytes '.
          'from buffer. Original length is 3 Current index is 1',
          'ok 6263',
        ],
      ),
    ];
  }

  <<DataProvider('provideReadAllCases')>>
  public function testReadAll(
    string $initial,
    vec<int> $reads,
    vec<string> $expected,
  ): void {
    $buffer = new TMemoryBuffer($initial);
    expect(
      Vec\map($reads, $len ==> self::outcome(() ==> $buffer->readAll($len))),
    )
      ->toEqual($expected);
  }

  // readAll() as a loop over read(), which is how it was implemented before it
  // read from the buffer directly.
  private static function loopReadAll(TMemoryBuffer $buffer, int $len): string {
    $data = '';
    while (Str\length($data) < $len) {
      $data .= $buffer->read($len - Str\length($data));
    }
    return $data;
  }

  // getBuffer() returns false, against its soft string return type, once
  // every byte has been read; peek() has no such case.
  private static function remaining(TMemoryBuffer $buffer): string {
    $available = $buffer->available();
    return $available === 0 ? '' : $buffer->peek($available);
  }

  private static function nextRandom(inout int $state, int $n): int {
    $state ^= $state << 13;
    $state ^= ($state >> 7) & 0x01ffffffffffffff;
    $state ^= $state << 17;
    return ($state & 0x7fffffffffffffff) % $n;
  }

  // Seeded random operation sequences against a buffer that reads through the
  // loop. The only intended difference is a short read, which now throws
  // before consuming anything.
  public function testRandomOperationsMatchLoopReadAll(): void {
    $state = 0x2545f4914f6cdd1d;
    for ($run = 0; $run < 50; $run++) {
      $buffer = new TMemoryBuffer();
      $reference = new TMemoryBuffer();
      for ($step = 0; $step < 60; $step++) {
        $letter = PHP\chr(65 + self::nextRandom(inout $state, 26));
        $bytes = Str\repeat($letter, self::nextRandom(inout $state, 6));
        $len = self::nextRandom(inout $state, 9) - 2;
        $available = $buffer->available();
        switch (self::nextRandom(inout $state, 4)) {
          case 0:
            $buffer->write($bytes);
            $reference->write($bytes);
            break;
          case 1:
            $buffer->putBack($bytes);
            $reference->putBack($bytes);
            break;
          case 2:
            $peek = Math\minva(Math\maxva($len, 0), $available);
            expect($buffer->peek($peek))->toEqual($reference->peek($peek));
            break;
          default:
            $result = self::outcome(() ==> $buffer->readAll($len));
            if ($len > $available) {
              expect(Str\starts_with(
                $result,
                'threw TTransportException: TMemoryBuffer: Could not read '.
                $len.
                ' bytes',
              ))->toBeTrue();
            } else {
              expect($result)->toEqual(
                self::outcome(() ==> self::loopReadAll($reference, $len)),
              );
            }
        }
        expect(tuple($buffer->available(), self::remaining($buffer)))
          ->toEqual(
            tuple($reference->available(), self::remaining($reference)),
          );
      }
    }
  }
}
