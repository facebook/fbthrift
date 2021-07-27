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

use anyhow::Error;

pub trait RequestContext {
    type ContextStack;
    type Name;

    fn get_context_stack(
        &self,
        service_name: &Self::Name,
        fn_name: &Self::Name,
    ) -> Result<Self::ContextStack, Error>;

    fn set_user_exception_header(&self, ex_type: &str, ex_reason: &str) -> Result<(), Error>;
}

impl RequestContext for () {
    type ContextStack = ();
    type Name = const_cstr::ConstCStr;

    fn get_context_stack(
        &self,
        _service_name: &Self::Name,
        _fn_name: &Self::Name,
    ) -> Result<Self::ContextStack, Error> {
        Ok(())
    }

    fn set_user_exception_header(&self, _ex_type: &str, _ex_reason: &str) -> Result<(), Error> {
        Ok(())
    }
}
