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

pub trait ContextStack {
    /// Called before the request is read.
    fn pre_read(&mut self) -> Result<(), Error>;

    /// Called after the request is read.
    fn post_read(&mut self, bytes: u32) -> Result<(), Error>;

    /// Called before a response is writen.
    fn pre_write(&mut self) -> Result<(), Error>;

    /// Called after a response a written.
    fn post_write(&mut self, bytes: u32) -> Result<(), Error>;
}

impl ContextStack for () {
    fn pre_read(&mut self) -> Result<(), Error> {
        Ok(())
    }

    fn post_read(&mut self, _bytes: u32) -> Result<(), Error> {
        Ok(())
    }

    fn pre_write(&mut self) -> Result<(), Error> {
        Ok(())
    }

    fn post_write(&mut self, _bytes: u32) -> Result<(), Error> {
        Ok(())
    }
}
