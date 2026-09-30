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
 */

package org.apache.thrift;

import static org.junit.jupiter.api.Assertions.assertEquals;

import org.junit.jupiter.api.Test;

public class ExceptionClassificationTest {

  private static class UnclassifiedException extends TBaseException {}

  @Test
  public void testDefaultsAreUnspecified() {
    UnclassifiedException e = new UnclassifiedException();
    assertEquals(ExceptionBlame.UNSPECIFIED, e.getExceptionBlame());
    assertEquals(ExceptionKind.UNSPECIFIED, e.getExceptionKind());
    assertEquals(ExceptionSafety.UNSPECIFIED, e.getExceptionSafety());
  }

  @Test
  public void testOverridesAreVisible() {
    TBaseException e =
        new TBaseException() {
          @Override
          public ExceptionBlame getExceptionBlame() {
            return ExceptionBlame.CLIENT;
          }

          @Override
          public ExceptionKind getExceptionKind() {
            return ExceptionKind.PERMANENT;
          }

          @Override
          public ExceptionSafety getExceptionSafety() {
            return ExceptionSafety.SAFE;
          }
        };
    assertEquals(ExceptionBlame.CLIENT, e.getExceptionBlame());
    assertEquals(ExceptionKind.PERMANENT, e.getExceptionKind());
    assertEquals(ExceptionSafety.SAFE, e.getExceptionSafety());
  }
}
