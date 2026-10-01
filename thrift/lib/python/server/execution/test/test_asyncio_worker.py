# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import asyncio
import threading
import unittest
from collections.abc import Awaitable
from typing import TypedDict
from unittest import mock

from thrift.python.server_impl.execution.asyncio_worker import AsyncioWorker


class WorkerInitializationState(TypedDict, total=False):
    thread: int
    loop: asyncio.AbstractEventLoop
    loop_running: bool


class AsyncioWorkerTest(unittest.TestCase):
    def setUp(self) -> None:
        self.__workers: list[AsyncioWorker] = []
        self.addCleanup(self.__close_workers)

    def __register_worker(self, worker: AsyncioWorker) -> None:
        self.__workers.append(worker)

    def __close_workers(self) -> None:
        for worker in reversed(self.__workers):
            if worker.is_alive():
                worker.close()

    def test_worker_creates_and_owns_asyncio_loop_thread(self) -> None:
        # GIVEN
        calling_thread = threading.get_ident()
        initialization: WorkerInitializationState = {}

        def initialize() -> None:
            initialization["thread"] = threading.get_ident()
            loop = asyncio.get_running_loop()
            initialization["loop"] = loop
            initialization["loop_running"] = loop.is_running()

        # WHEN
        worker = AsyncioWorker(initialize)
        self.__register_worker(worker)
        worker.start()
        actual_worker_alive_after_start = worker.is_alive()
        worker.close()
        actual_loop_closed_after_close = initialization["loop"].is_closed()
        actual_worker_alive_after_close = worker.is_alive()
        actual_initializer_ran_off_calling_thread = (
            initialization["thread"] != calling_thread
        )
        actual_initializer_saw_running_loop = initialization["loop_running"]

        # THEN
        self.assertTrue(actual_initializer_ran_off_calling_thread)
        self.assertTrue(actual_initializer_saw_running_loop)
        self.assertTrue(actual_worker_alive_after_start)
        self.assertFalse(actual_worker_alive_after_close)
        self.assertTrue(actual_loop_closed_after_close)

    def test_worker_waits_for_async_initializer(self) -> None:
        # GIVEN
        calling_thread = threading.get_ident()
        initialization: WorkerInitializationState = {}
        initialization_finished = threading.Event()

        async def initialize() -> None:
            initialization["thread"] = threading.get_ident()
            loop = asyncio.get_running_loop()
            initialization["loop"] = loop
            initialization["loop_running"] = loop.is_running()
            await asyncio.sleep(0)
            initialization_finished.set()

        # WHEN
        worker = AsyncioWorker(initialize)
        self.__register_worker(worker)
        worker.start()
        actual_initializer_finished_before_start_returns = (
            initialization_finished.is_set()
        )
        actual_worker_alive_after_start = worker.is_alive()
        worker.close()
        actual_worker_alive_after_close = worker.is_alive()
        actual_initializer_ran_off_calling_thread = (
            initialization["thread"] != calling_thread
        )
        actual_initializer_saw_running_loop = initialization["loop_running"]

        # THEN
        self.assertTrue(actual_initializer_finished_before_start_returns)
        self.assertTrue(actual_initializer_ran_off_calling_thread)
        self.assertTrue(actual_initializer_saw_running_loop)
        self.assertTrue(actual_worker_alive_after_start)
        self.assertFalse(actual_worker_alive_after_close)

    def test_worker_closes_generic_awaitable_initializer_result(self) -> None:
        # GIVEN
        initialization: WorkerInitializationState = {}
        cleanup: WorkerInitializationState = {}

        class InitializationResult:
            async def aclose(self) -> None:
                cleanup["loop"] = asyncio.get_running_loop()
                cleanup["thread"] = threading.get_ident()

        def initialize() -> Awaitable[InitializationResult]:
            initialization["loop"] = asyncio.get_running_loop()
            initialization["thread"] = threading.get_ident()
            result = initialization["loop"].create_future()
            result.set_result(InitializationResult())
            return result

        worker = AsyncioWorker(initialize)
        self.__register_worker(worker)

        # WHEN
        worker.start()
        worker.close()
        actual_cleanup_called = "loop" in cleanup
        actual_cleanup_loop_matches = (
            "loop" in cleanup
            and "loop" in initialization
            and cleanup["loop"] is initialization["loop"]
        )
        actual_cleanup_thread_matches = (
            "thread" in cleanup
            and "thread" in initialization
            and cleanup["thread"] == initialization["thread"]
        )
        actual_loop_closed_after_close = (
            "loop" in initialization and initialization["loop"].is_closed()
        )
        actual_worker_alive_after_close = worker.is_alive()

        # THEN
        self.assertTrue(actual_cleanup_called)
        self.assertTrue(actual_cleanup_loop_matches)
        self.assertTrue(actual_cleanup_thread_matches)
        self.assertTrue(actual_loop_closed_after_close)
        self.assertFalse(actual_worker_alive_after_close)

    def test_initializer_failure_stops_worker_and_propagates(self) -> None:
        # GIVEN
        initialization: WorkerInitializationState = {}
        initialization_failure = ValueError("initializer failed")

        def initialize() -> None:
            initialization["loop"] = asyncio.get_running_loop()
            raise initialization_failure

        # WHEN
        worker = AsyncioWorker(initialize)
        self.__register_worker(worker)
        raised_exception: BaseException | None = None
        try:
            worker.start()
        except ValueError as exception:
            raised_exception = exception
        actual_worker_alive_after_failure = worker.is_alive()
        actual_no_notes_attached = getattr(raised_exception, "__notes__", ()) == ()
        actual_same_exception_propagated = raised_exception is initialization_failure
        actual_loop_closed_after_failure = initialization["loop"].is_closed()

        # THEN
        self.assertTrue(actual_no_notes_attached)
        self.assertTrue(actual_same_exception_propagated)
        self.assertFalse(actual_worker_alive_after_failure)
        self.assertTrue(actual_loop_closed_after_failure)

    def test_async_initializer_failure_stops_worker_and_propagates(self) -> None:
        # GIVEN
        initialization: WorkerInitializationState = {}
        initialization_failure = ValueError("async initializer failed")

        async def initialize() -> None:
            initialization["loop"] = asyncio.get_running_loop()
            await asyncio.sleep(0)
            raise initialization_failure

        worker = AsyncioWorker(initialize)
        self.__register_worker(worker)

        # WHEN
        raised_exception: BaseException | None = None
        try:
            worker.start()
        except ValueError as exception:
            raised_exception = exception
        actual_loop_closed_after_failure = initialization["loop"].is_closed()
        actual_same_exception_propagated = raised_exception is initialization_failure
        actual_worker_alive_after_failure = worker.is_alive()

        # THEN
        self.assertTrue(actual_loop_closed_after_failure)
        self.assertTrue(actual_same_exception_propagated)
        self.assertFalse(actual_worker_alive_after_failure)

    def test_worker_propagates_event_loop_creation_failure(self) -> None:
        # GIVEN
        creation_failure = RuntimeError("event-loop creation failed")
        raised_exception: BaseException | None = None

        def initialize() -> None:
            pass

        worker = AsyncioWorker(initialize)
        self.__register_worker(worker)

        def start_worker() -> None:
            nonlocal raised_exception
            try:
                worker.start()
            except RuntimeError as exception:
                raised_exception = exception

        with mock.patch.object(
            asyncio,
            "new_event_loop",
            autospec=True,
            side_effect=creation_failure,
        ):
            start_thread = threading.Thread(target=start_worker, daemon=True)

            # WHEN
            start_thread.start()
            start_thread.join(timeout=1.0)
            actual_same_exception_propagated = raised_exception is creation_failure
            actual_start_waiter_released = not start_thread.is_alive()
            actual_worker_alive_after_failure = worker.is_alive()

        # THEN
        self.assertTrue(actual_same_exception_propagated)
        self.assertTrue(actual_start_waiter_released)
        self.assertFalse(actual_worker_alive_after_failure)
