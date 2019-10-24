/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#import <Foundation/Foundation.h>

#import "TProtocol.h"

/** The following two types are defined and used to conform to the
   [makeImmutable] API defined below. An instance of TBaseStruct that was
   [makeImmutable] will convert the mutable containers that it holds. for
   example: NSMutableArray -> NSArray NSMutableDictionary -> NSDictionary */
typedef NSMutableArray TBaseStructArray;
typedef NSMutableDictionary TBaseStructDictionary;
typedef NSMutableSet TBaseStructSet;

@interface TBaseStruct : NSObject<NSCopying, NSMutableCopying>

/**
 * convert this instance to immutable
 *
 * @return YES in case the object is immutable.
 */
- (BOOL)makeImmutable;

/**
 * check whether this instance is immutable
 */
- (BOOL)isImmutable;

/**
 * check whether this instance is mutable
 */
- (BOOL)isMutable;

/**
 * throw an exception in case this instance is immutable.
 */
- (void)throwExceptionIfImmutable;

@end
