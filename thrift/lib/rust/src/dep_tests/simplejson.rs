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

use anyhow::Result;
use fbthrift::simplejson_protocol::{deserialize, serialize};
use fbthrift_test_if::{Basic, En, MainStruct, MainStructNoBinary, Small, SubStruct, Un, UnOne};
use std::collections::BTreeMap;
use std::default::Default;

#[test]
fn test_large_roundtrip() -> Result<()> {
    // Build the big struct
    let mut m = BTreeMap::new();
    m.insert("m1".to_string(), 1);
    m.insert("m2".to_string(), 2);

    let sub = SubStruct {
        ..Default::default()
    };

    let u = Un::un1(UnOne { one: 1 });
    let e = En::TWO;

    let mut int_keys = BTreeMap::new();
    int_keys.insert(42, 43);
    int_keys.insert(44, 45);

    let r = MainStruct {
        foo: "foo".to_string(),
        m,
        bar: "test".to_string(),
        s: sub,
        l: vec![Small { num: 1, two: 2 }, Small { num: 2, two: 3 }],
        u,
        e,
        int_keys,
        opt: None,
    };

    // serialize it and assert that it serializes correctly
    let s = String::from_utf8(serialize(&r).to_vec()).unwrap();

    // Note that default optionals are there
    // but non-default optionals are not there
    // That is an artifact on how the serialize trait works
    let expected_string = r#"{
        "foo":"foo",
        "m":{"m1":1,"m2":2},
        "bar":"test",
        "s":{"opt_def":"IAMOPT","req_def":"IAMREQ","bin":""},
        "l":[{"num":1,"two":2},{"num":2,"two":3}],
        "u":{"un1":{"one":1}},
        "e":2,
        "int_keys":{"42":43,"44":45}
    }"#
    .replace(" ", "")
    .replace("\n", "");
    assert_eq!(expected_string, s);

    // It at least needs to be valid json, the serialize then
    // deserialize and compare will come in the next diff
    let v: serde_json::Result<serde_json::Value> = serde_json::from_str(&s);
    assert!(v.is_ok());

    // Assert that deserialize builts the exact same struct
    assert_eq!(r, deserialize(s).unwrap());

    Ok(())
}

#[test]
fn test_struct_key() -> Result<()> {
    // See the `structKey` test in cpp_compat_test

    let mut h = std::collections::BTreeMap::new();
    h.insert(
        Small {
            ..Default::default()
        },
        1,
    );
    let sub = SubStruct {
        key_map: Some(h),
        // In rust we need to specify optionals with defaults as None
        // instead of relying on ..Default::default()
        opt_def: None,
        ..Default::default()
    };

    let s = String::from_utf8(serialize(&sub).to_vec()).unwrap();
    let expected_string = r#"{
        "req_def":"IAMREQ",
        "key_map":{{"num":0,"two":0}:1},
        "bin":""
    }"#
    .replace(" ", "")
    .replace("\n", "");
    assert_eq!(expected_string, s);

    // It's definitely not JSON...
    let v: serde_json::Result<serde_json::Value> = serde_json::from_str(&s);
    assert!(v.is_err());

    // ...but it needs to deserialize
    assert_eq!(sub, deserialize(s).unwrap());

    Ok(())
}

#[test]
fn test_weird_text() -> Result<()> {
    // See the `weirdText` test in cpp_compat_test

    let mut sub = SubStruct {
        opt_def: Some("stuff\twith\nescape\\characters'...\"lots{of}fun</xml>".to_string()),
        bin: "1234".as_bytes().to_vec(),
        ..Default::default()
    };

    let s = String::from_utf8(serialize(&sub).to_vec()).unwrap();
    let expected_string = r#"{
        "opt_def":"stuff\twith\nescape\\characters'...\"lots{of}fun</xml>",
        "req_def":"IAMREQ",
        "bin":"MTIzNA"
    }"#
    .replace(" ", "")
    .replace("\n", "");
    assert_eq!(expected_string, s);
    // Make sure its equal
    assert_eq!(sub, deserialize(s).unwrap());

    // Unicode escaping
    sub.opt_def = Some("UNICODE\u{1F60A}UH OH".to_string());

    let s = String::from_utf8(serialize(&sub).to_vec()).unwrap();
    let expected_string = r#"{
        "opt_def":"UNICODE😊UH OH",
        "req_def":"IAMREQ",
        "bin":"MTIzNA"
    }"#
    // Double-space to deal with "tabs"
    .replace("  ", "")
    .replace("\n", "");
    assert_eq!(expected_string, s);
    // Make sure its equal
    assert_eq!(sub, deserialize(s).unwrap());

    Ok(())
}

#[test]
fn test_skip_complex() -> Result<()> {
    // See the `skipComplex` test in cpp_compat_test

    let sub = SubStruct {
        opt_def: Some("thing".to_string()),
        bin: "1234".as_bytes().to_vec(),
        ..Default::default()
    };

    let input = r#"{
        "opt_def":"thing",
        "req_def":"IAMREQ",
        "bin":"MTIzNA"
        "extra":[1,{"thing":"thing2"}],
        "extra_map":{"thing":null,"thing2":2}
    }"#
    .replace(" ", "")
    .replace("\n", "");
    // Make sure everything is skipped properly
    assert_eq!(sub, deserialize(input).unwrap());

    Ok(())
}

#[test]
fn test_null_stuff() -> Result<()> {
    // See the `nullStuff` test in cpp_compat_test

    let sub = SubStruct {
        opt_def: None,
        bin: "1234".as_bytes().to_vec(),
        ..Default::default()
    };

    let input = r#"{
        "opt_def":null,
        "req_def":"IAMREQ",
        "bin":"MTIzNA"
    }"#
    .replace(" ", "")
    .replace("\n", "");
    // Make sure everything is skipped properly
    assert_eq!(sub, deserialize(input).unwrap());

    Ok(())
}

#[test]
fn infinite_spaces() -> Result<()> {
    let mut m = BTreeMap::new();
    m.insert("m1".to_string(), 1);
    m.insert("m2".to_string(), 2);

    let sub = SubStruct {
        ..Default::default()
    };

    let u = Un::un1(UnOne { one: 1 });
    let e = En::TWO;

    let mut int_keys = BTreeMap::new();
    int_keys.insert(42, 43);
    int_keys.insert(44, 45);

    let r = MainStruct {
        foo: "foo".to_string(),
        m,
        bar: "test".to_string(),
        s: sub,
        l: vec![Small { num: 1, two: 2 }, Small { num: 2, two: 3 }],
        u,
        e,
        int_keys,
        opt: None,
    };

    let input = r#"{
         "foo"  :  "foo" ,
          "m" : { "m1" :  1   , "m2" : 2 }  ,
        "bar":"test",
        "s":{"opt_def":  "IAMOPT"  ,"req_def":  "IAMREQ","bin": ""  },
        "l":[{"num":1,"two":2},{"num"  :2 ," two" : 3 } ],
        "u":{"un1":{"one":  1  } },
        "e":  2  ,
        "int_keys"  :{"42"   :  43,  "44":45}
    }"#;

    // Assert that deserialize builts the exact same struct
    assert_eq!(r, deserialize(input).unwrap());

    Ok(())
}

#[test]
fn test_bool() -> Result<()> {
    let b = Basic {
        b: true,
        b2: false,
        ..Default::default()
    };
    // serialize it and assert that it serializes correctly
    let s = String::from_utf8(serialize(&b).to_vec()).unwrap();

    // Assert that deserialize builts the exact same struct
    assert_eq!(b, deserialize(s).unwrap());

    Ok(())
}

#[test]
fn test_serde_compat() -> Result<()> {
    // Build the big struct
    let mut m = BTreeMap::new();
    m.insert("m1".to_string(), 1);
    m.insert("m2".to_string(), 2);

    let u = Un::un1(UnOne { one: 1 });
    let e = En::TWO;

    let mut int_keys = BTreeMap::new();
    int_keys.insert(42, 43);
    int_keys.insert(44, 45);

    let r = MainStructNoBinary {
        foo: "foo".to_string(),
        m,
        bar: "test".to_string(),
        l: vec![Small { num: 1, two: 2 }, Small { num: 2, two: 3 }],
        u,
        e,
        int_keys,
        opt: None,
    };

    let fbthrift_s = String::from_utf8(serialize(&r).to_vec()).unwrap();
    // We aren't going to get full compat, but at least make it so fbthrift
    // can deserialize
    // what serde has written out
    let serde_s = serde_json::to_string(&r).unwrap();

    // but passing between them should work
    assert_eq!(r, serde_json::from_str(&fbthrift_s).unwrap());
    assert_eq!(r, deserialize(&serde_s).unwrap());
    Ok(())
}
