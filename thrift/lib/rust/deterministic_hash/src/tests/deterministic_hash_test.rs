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

use fbthrift::builtin_types::{BTreeMap, BTreeSet};
use fbthrift_deterministic_hash::{deterministic_hash, Sha256Hasher};
use maplit::btreemap;
use teststructs::ComplexStruct;

#[test]
fn hash_test_complex_structure() {
    let complex_struct = ComplexStruct {
        l: vec!["a".to_string(), "ab".to_string(), "abc".to_string()],
        s: BTreeSet::from(["a".to_string(), "ab".to_string(), "abc".to_string()]),
        m: btreemap! {
            "aa".to_owned() => "bb".to_owned(),
            "cc".to_owned() => "dd".to_owned(),
            "ee".to_owned() => "ff".to_owned(),
        },
        ml: btreemap! {
            "aa".to_owned() => vec![1, 2, 3],
            "bb".to_owned() => vec![],
            "cc".to_owned() => vec![3, 2],
            "ee".to_owned() => vec![1, 2, 3],
            "ff".to_owned() => vec![],
        },
        mm: btreemap! {
            "aa".to_owned() => BTreeMap::from([(1, 2)]),
            "bb".to_owned() => BTreeMap::from([]),
            "cc".to_owned() => BTreeMap::from([(2, 3), (3, 2)]),
            "ee".to_owned() => BTreeMap::from([(3, 4)]),
            "ff".to_owned() => BTreeMap::from([]),
        },
        ..Default::default()
    };
    let result =
        deterministic_hash(&complex_struct, Sha256Hasher::default).expect("expected no error");
    assert_eq!(
        result,
        [
            215, 3, 60, 5, 187, 190, 35, 151, 36, 121, 90, 142, 117, 193, 153, 144, 17, 19, 156,
            131, 209, 173, 91, 93, 59, 156, 245, 18, 29, 76, 76, 172
        ]
    );
}
