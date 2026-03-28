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

#nullable enable
using System;

namespace FBThrift
{
    /// <summary>
    /// Shared null-guard helpers used by generated struct and union property setters.
    /// Centralizes the null-check pattern to reduce generated code size.
    /// </summary>
    public static class ThriftNullGuard
    {
        /// <summary>
        /// Throws <see cref="ArgumentNullException"/> if the given value is null.
        /// Used by generated struct property setters for non-nullable fields.
        /// </summary>
        public static void ThrowIfNull(object? value, string fieldName)
        {
            if (value == null)
            {
                throw new ArgumentNullException(
                    nameof(value),
                    $"Struct field '{fieldName}' is non-nullable and cannot be set to null");
            }
        }

        /// <summary>
        /// Throws <see cref="ArgumentNullException"/> if the given value is null.
        /// Used by generated union property setters for reference-type fields.
        /// </summary>
        public static void ThrowIfNullUnion<T>(T? value, string fieldName) where T : class
        {
            if (value == null)
            {
                throw new ArgumentNullException(
                    nameof(value),
                    $"Union field '{fieldName}' cannot be set to null");
            }
        }

        /// <summary>
        /// Throws <see cref="ArgumentNullException"/> if the given value is null.
        /// Used by generated union property setters for value-type fields.
        /// Generic overload avoids boxing of Nullable&lt;T&gt; into object.
        /// </summary>
        public static void ThrowIfNullUnion<T>(T? value, string fieldName) where T : struct
        {
            if (!value.HasValue)
            {
                throw new ArgumentNullException(
                    nameof(value),
                    $"Union field '{fieldName}' cannot be set to null");
            }
        }
    }
}
