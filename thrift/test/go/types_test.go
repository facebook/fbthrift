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

package gotest

import (
	"reflect"
	"sort"
	"testing"
	"thrift/test/go/if/thrifttest"
)

func TestStructMapKey(t *testing.T) {
	k1a := thrifttest.MapKey{Num: 1, Strval: "a"}
	k1b := thrifttest.MapKey{Num: 1, Strval: "a"}
	k2 := thrifttest.MapKey{Num: 2, Strval: "b"}

	maps := thrifttest.Maps{Struct2str: map[thrifttest.MapKey]string{}}
	maps.Struct2str[k1a] = "x"
	maps.Struct2str[k1b] = "y"
	maps.Struct2str[k2] = "z"

	if maps.Struct2str[k1a] != "y" {
		t.Fatalf("unexpected map value (should have been overwritten to 'y')")
	}

	if maps.Struct2str[k2] != "z" {
		t.Fatalf("unexpected map value (should have been 'z')")
	}

	if len(maps.Struct2str) != 2 {
		t.Fatalf("unexpected map length (should have been 2)")
	}
}

func TestEnumValues(t *testing.T) {
	generatedValues := thrifttest.NumberzValues
	sort.Slice(generatedValues, func(i, j int) bool { return generatedValues[i] < generatedValues[j] })

	values := []thrifttest.Numberz{}
	for k := range thrifttest.NumberzToName {
		values = append(values, k)
	}
	sort.Slice(values, func(i, j int) bool { return values[i] < values[j] })

	if !reflect.DeepEqual(generatedValues, values) {
		t.Fatalf("values slices are not equal (should be equal)")
	}
}

func TestEnumNames(t *testing.T) {
	generatedNames := thrifttest.NumberzNames
	sort.Strings(generatedNames)

	names := []string{}
	for k := range thrifttest.NumberzToValue {
		names = append(names, k)
	}
	sort.Strings(names)

	if !reflect.DeepEqual(generatedNames, names) {
		t.Fatalf("names slices are not equal (should be equal)")
	}
}
