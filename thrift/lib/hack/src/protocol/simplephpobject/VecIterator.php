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
class VecIterator<T> implements Iterator<T> {
  private int $currentIndex;
  public function __construct(private vec<T> $src) {
    $this->currentIndex = PHP\fb\key($src) ?? 0;
  }

  public function current(): T {
    invariant($this->currentIndex < C\count($this->src), 'iterator not valid');
    return $this->src[$this->currentIndex];
  }

  public function key(): int {
    invariant($this->currentIndex < C\count($this->src), 'iterator not valid');
    return $this->currentIndex;
  }

  public function next(): void {
    $this->currentIndex++;
  }

  public function rewind(): void {
    $this->currentIndex = 0;
  }

  public function valid(): bool {
    return $this->currentIndex < C\count($this->src);
  }
}
