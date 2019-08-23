// Copyright 2004-present Facebook. All Rights Reserved.

package com.facebook.thrift.lite.annotations;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * An annotation that goes with ThriftProperty's and flags them as optional.
 *
 * <p>The String value is the name of the struct that the property belongs to. The name 'value' is
 * special, so that you can say @TOptional("MyStruct"). Using a more descriptive name would have
 * forced people to write code like @TOptional(struct = "MyStruct").
 */
@Target(ElementType.FIELD)
@Retention(RetentionPolicy.SOURCE)
public @interface TOptional {
  String value();
}
