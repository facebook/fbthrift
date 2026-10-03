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
from typing import TypedDict
from unittest import mock

from thrift.python.server_impl.execution.asyncio_worker import AsyncioWorker
from thrift.python.server_impl.execution.asyncio_worker_pool import AsyncioWorkerPool


class OneWorkerInitState(TypedDict, total=False):
    count: int
    thread: int
    thread_obj: threading.Thread
    loop: asyncio.AbstractEventLoop
    loop_running: bool


class TwoWorkerInitState(TypedDict, total=False):
    count: int
    threads: set[int]
    loops: set[int]
    thread_objs: list[threading.Thread]
    loops_list: list[asyncio.AbstractEventLoop]


class AsyncioWorkerPoolTest(unittest.TestCase):
    def test_zero_capacity_creates_no_workers_and_calls_no_initializers(self) -> None:
        # GIVEN
        initializer_called = False

        def initialize() -> None:
            nonlocal initializer_called
            initializer_called = True

        # WHEN
        pool = AsyncioWorkerPool(capacity=0, initializer=initialize)
        pool.start()
        pool.close()
        actual_initializer_called = initializer_called

        # THEN
        self.assertFalse(actual_initializer_called)

    def test_one_worker_owns_thread_and_loop_and_runs_initializer(self) -> None:
        # GIVEN
        calling_thread = threading.get_ident()
        initialization: OneWorkerInitState = {
            "count": 0,
        }
        expected_initialization_count = 1

        def initialize() -> None:
            initialization["thread_obj"] = threading.current_thread()
            initialization["thread"] = threading.get_ident()
            loop = asyncio.get_running_loop()
            initialization["loop"] = loop
            initialization["loop_running"] = loop.is_running()
            initialization["count"] = initialization["count"] + 1

        # WHEN
        pool = AsyncioWorkerPool(capacity=1, initializer=initialize)
        pool.start()
        worker_thread = initialization["thread_obj"]
        worker_alive_after_start = worker_thread.is_alive()
        pool.close()
        worker_alive_after_close = worker_thread.is_alive()
        actual_initialization_count = initialization["count"]
        actual_initializer_ran_off_calling_thread = (
            initialization["thread"] != calling_thread
        )
        actual_initializer_saw_running_loop = initialization["loop_running"]
        actual_worker_alive_after_start = worker_alive_after_start
        actual_worker_alive_after_close = worker_alive_after_close
        actual_loop_closed_after_close = initialization["loop"].is_closed()

        # THEN
        self.assertEqual(expected_initialization_count, actual_initialization_count)
        self.assertTrue(actual_initializer_ran_off_calling_thread)
        self.assertTrue(actual_initializer_saw_running_loop)
        self.assertTrue(actual_worker_alive_after_start)
        self.assertFalse(actual_worker_alive_after_close)
        self.assertTrue(actual_loop_closed_after_close)

    def test_two_workers_own_distinct_threads_and_loops(self) -> None:
        # GIVEN
        calling_thread = threading.get_ident()
        initialization: TwoWorkerInitState = {
            "count": 0,
            "threads": set(),
            "loops": set(),
            "thread_objs": [],
            "loops_list": [],
        }
        lock = threading.Lock()
        started_workers: list[AsyncioWorker] = []
        closed_workers: list[AsyncioWorker] = []
        real_start = AsyncioWorker.start
        real_close = AsyncioWorker.close
        expected_initialization_count = 2

        def initialize() -> None:
            loop = asyncio.get_running_loop()
            thread_obj = threading.current_thread()
            with lock:
                initialization["threads"].add(threading.get_ident())
                initialization["loops"].add(id(loop))
                initialization["count"] += 1
                initialization["thread_objs"].append(thread_obj)
                initialization["loops_list"].append(loop)

        def record_start(worker: AsyncioWorker) -> None:
            real_start(worker)
            started_workers.append(worker)

        def record_close(worker: AsyncioWorker) -> None:
            closed_workers.append(worker)
            real_close(worker)

        # WHEN
        with (
            mock.patch.object(AsyncioWorker, "start", new=record_start),
            mock.patch.object(AsyncioWorker, "close", new=record_close),
        ):
            pool = AsyncioWorkerPool(capacity=2, initializer=initialize)
            pool.start()
            thread_objs = initialization["thread_objs"]
            loops_list = initialization["loops_list"]
            both_alive_after_start = all(t.is_alive() for t in thread_objs)
            pool.close()
        both_dead_after_close = all(not t.is_alive() for t in thread_objs)
        both_loops_closed_after_close = all(loop.is_closed() for loop in loops_list)
        actual_initialization_count = initialization["count"]
        actual_distinct_threads = len(initialization["threads"]) == 2
        actual_distinct_loops = len(initialization["loops"]) == 2
        actual_all_off_calling_thread = calling_thread not in initialization["threads"]
        actual_both_alive_after_start = both_alive_after_start
        actual_both_dead_after_close = both_dead_after_close
        actual_both_loops_closed_after_close = both_loops_closed_after_close
        actual_only_fully_started_workers_closed = closed_workers == started_workers

        # THEN
        self.assertEqual(expected_initialization_count, actual_initialization_count)
        self.assertTrue(actual_distinct_threads)
        self.assertTrue(actual_distinct_loops)
        self.assertTrue(actual_all_off_calling_thread)
        self.assertTrue(actual_both_alive_after_start)
        self.assertTrue(actual_both_dead_after_close)
        self.assertTrue(actual_both_loops_closed_after_close)
        self.assertTrue(actual_only_fully_started_workers_closed)

    def test_pool_cleans_up_started_workers_when_later_initializer_raises_base_exception(
        self,
    ) -> None:
        # GIVEN
        class WorkerStartupFailure(BaseException):
            pass

        initialization: OneWorkerInitState = {
            "count": 0,
        }
        initialization_failure = WorkerStartupFailure("second initializer failed")
        expected_initialization_count = 2

        def initialize() -> None:
            if initialization["count"] == 0:
                initialization["thread_obj"] = threading.current_thread()
                loop = asyncio.get_running_loop()
                initialization["loop"] = loop
                initialization["count"] = 1
            else:
                initialization["count"] = 2
                raise initialization_failure

        # WHEN
        pool = AsyncioWorkerPool(capacity=2, initializer=initialize)
        self.addCleanup(pool.close)
        raised_exception: BaseException | None = None
        try:
            pool.start()
        except WorkerStartupFailure as exception:
            raised_exception = exception
        worker_thread = initialization.get("thread_obj")
        first_loop = initialization.get("loop")
        actual_initialization_count = initialization["count"]
        actual_same_exception_propagated = raised_exception is initialization_failure
        actual_first_worker_stopped_after_second_initializer_failed = (
            not worker_thread.is_alive() if worker_thread is not None else False
        )
        actual_first_loop_closed_after_second_initializer_failed = (
            first_loop.is_closed() if first_loop is not None else False
        )

        # THEN
        self.assertEqual(expected_initialization_count, actual_initialization_count)
        self.assertTrue(actual_same_exception_propagated)
        self.assertTrue(actual_first_worker_stopped_after_second_initializer_failed)
        self.assertTrue(actual_first_loop_closed_after_second_initializer_failed)

    def test_pool_rolls_back_when_later_async_initializer_fails(self) -> None:
        # GIVEN
        startup_failure = ValueError("second async initializer failed")
        worker_loops: list[asyncio.AbstractEventLoop] = []
        worker_threads: list[threading.Thread] = []
        expected_initialization_count = 2

        async def initialize() -> None:
            worker_loops.append(asyncio.get_running_loop())
            worker_threads.append(threading.current_thread())
            await asyncio.sleep(0)
            if len(worker_threads) == 2:
                raise startup_failure

        # WHEN
        pool = AsyncioWorkerPool(capacity=2, initializer=initialize)
        self.addCleanup(pool.close)
        raised_exception: BaseException | None = None
        try:
            pool.start()
        except ValueError as exception:
            raised_exception = exception
        actual_initialization_count = len(worker_threads)
        actual_same_exception_propagated = raised_exception is startup_failure
        actual_worker_loops_closed = all(loop.is_closed() for loop in worker_loops)
        actual_worker_threads_stopped = all(
            not worker.is_alive() for worker in worker_threads
        )

        # THEN
        self.assertEqual(expected_initialization_count, actual_initialization_count)
        self.assertTrue(actual_same_exception_propagated)
        self.assertTrue(actual_worker_loops_closed)
        self.assertTrue(actual_worker_threads_stopped)

    def test_startup_rolls_back_started_workers_in_reverse_order(self) -> None:
        # GIVEN
        startup_failure = ValueError("third initializer failed")
        cleanup_failures = (
            RuntimeError("first context close failed"),
            RuntimeError("second context close failed"),
        )
        closed_contexts: list[int] = []
        worker_loops: list[asyncio.AbstractEventLoop] = []
        worker_threads: list[threading.Thread] = []
        started_workers: list[AsyncioWorker] = []
        closed_workers: list[AsyncioWorker] = []
        real_start = AsyncioWorker.start
        real_close = AsyncioWorker.close
        expected_cleanup_notes = (
            "Worker-pool rollback also failed: "
            "RuntimeError('second context close failed')",
            "Worker-pool rollback also failed: "
            "RuntimeError('first context close failed')",
        )
        expected_closed_contexts = (1, 0)

        class FailingContext:
            def __init__(self, index: int) -> None:
                self.index = index

            async def aclose(self) -> None:
                closed_contexts.append(self.index)
                raise cleanup_failures[self.index]

        def initialize() -> FailingContext:
            index = len(worker_threads)
            if index == 2:
                raise startup_failure
            worker_loops.append(asyncio.get_running_loop())
            worker_threads.append(threading.current_thread())
            return FailingContext(index)

        def record_start(worker: AsyncioWorker) -> None:
            real_start(worker)
            started_workers.append(worker)

        def record_close(worker: AsyncioWorker) -> None:
            closed_workers.append(worker)
            real_close(worker)

        # WHEN
        pool = AsyncioWorkerPool(capacity=3, initializer=initialize)
        self.addCleanup(pool.close)
        raised_exception: BaseException | None = None
        with (
            mock.patch.object(AsyncioWorker, "start", new=record_start),
            mock.patch.object(AsyncioWorker, "close", new=record_close),
        ):
            try:
                pool.start()
            except ValueError as exception:
                raised_exception = exception
        actual_cleanup_notes = (
            tuple(getattr(raised_exception, "__notes__", ()))
            if raised_exception is not None
            else ()
        )
        actual_closed_contexts = tuple(closed_contexts)
        actual_same_exception_propagated = raised_exception is startup_failure
        actual_worker_loops_closed = all(loop.is_closed() for loop in worker_loops)
        actual_worker_threads_stopped = all(
            not worker.is_alive() for worker in worker_threads
        )
        actual_only_started_workers_closed_in_reverse_order = closed_workers == list(
            reversed(started_workers)
        )

        # THEN
        self.assertEqual(expected_cleanup_notes, actual_cleanup_notes)
        self.assertEqual(expected_closed_contexts, actual_closed_contexts)
        self.assertTrue(actual_same_exception_propagated)
        self.assertTrue(actual_worker_loops_closed)
        self.assertTrue(actual_worker_threads_stopped)
        self.assertTrue(actual_only_started_workers_closed_in_reverse_order)

    def test_close_stops_every_worker_when_a_context_close_fails(self) -> None:
        # GIVEN
        close_failure = ValueError("first context close failed")
        closed_contexts: list[int] = []
        worker_loops: list[asyncio.AbstractEventLoop] = []
        worker_threads: list[threading.Thread] = []
        expected_closed_contexts = (0, 1)

        class WorkerContext:
            def __init__(self, index: int) -> None:
                self.index = index

            async def aclose(self) -> None:
                closed_contexts.append(self.index)
                if self.index == 0:
                    raise close_failure

        def initialize() -> WorkerContext:
            index = len(worker_loops)
            worker_loops.append(asyncio.get_running_loop())
            worker_threads.append(threading.current_thread())
            return WorkerContext(index)

        # WHEN
        pool = AsyncioWorkerPool(capacity=2, initializer=initialize)
        pool.start()
        raised_exception: BaseException | None = None
        try:
            pool.close()
        except ValueError as exception:
            raised_exception = exception
        actual_closed_contexts = tuple(closed_contexts)
        actual_same_exception_propagated = raised_exception is close_failure
        actual_worker_loops_closed = all(loop.is_closed() for loop in worker_loops)
        actual_worker_threads_stopped = all(
            not worker.is_alive() for worker in worker_threads
        )

        # THEN
        self.assertEqual(expected_closed_contexts, actual_closed_contexts)
        self.assertTrue(actual_same_exception_propagated)
        self.assertTrue(actual_worker_loops_closed)
        self.assertTrue(actual_worker_threads_stopped)

    def test_close_aggregates_multiple_context_failures_in_worker_order(
        self,
    ) -> None:
        # GIVEN
        first_failure = ValueError("first context close failed")
        second_failure = RuntimeError("second context close failed")
        cleanup_failures = (first_failure, second_failure)
        closed_contexts: list[int] = []
        worker_loops: list[asyncio.AbstractEventLoop] = []
        worker_threads: list[threading.Thread] = []
        expected_closed_contexts = (0, 1)
        expected_exception_message = "Worker cleanup failed"

        class WorkerContext:
            def __init__(self, index: int) -> None:
                self.index = index

            async def aclose(self) -> None:
                closed_contexts.append(self.index)
                raise cleanup_failures[self.index]

        def initialize() -> WorkerContext:
            index = len(worker_loops)
            worker_loops.append(asyncio.get_running_loop())
            worker_threads.append(threading.current_thread())
            return WorkerContext(index)

        # WHEN
        pool = AsyncioWorkerPool(capacity=2, initializer=initialize)
        pool.start()
        raised_exception: BaseExceptionGroup | None = None
        try:
            pool.close()
        except BaseExceptionGroup as exception:
            raised_exception = exception
        actual_closed_contexts = tuple(closed_contexts)
        actual_exception_message = (
            raised_exception.message if raised_exception is not None else ""
        )
        actual_failures_preserved_in_worker_order = (
            raised_exception.exceptions == cleanup_failures
            if raised_exception is not None
            else False
        )
        actual_worker_loops_closed = all(loop.is_closed() for loop in worker_loops)
        actual_worker_threads_stopped = all(
            not worker.is_alive() for worker in worker_threads
        )

        # THEN
        self.assertEqual(expected_closed_contexts, actual_closed_contexts)
        self.assertEqual(expected_exception_message, actual_exception_message)
        self.assertTrue(actual_failures_preserved_in_worker_order)
        self.assertTrue(actual_worker_loops_closed)
        self.assertTrue(actual_worker_threads_stopped)

    def test_close_is_idempotent(self) -> None:
        # GIVEN
        initialization: OneWorkerInitState = {
            "count": 0,
        }
        expected_initialization_count = 1

        def initialize() -> None:
            initialization["thread_obj"] = threading.current_thread()
            initialization["count"] = initialization["count"] + 1
            loop = asyncio.get_running_loop()
            initialization["loop"] = loop

        # WHEN
        pool = AsyncioWorkerPool(capacity=1, initializer=initialize)
        pool.start()
        pool.close()
        second_close_did_not_raise = True
        try:
            pool.close()
        except Exception:
            second_close_did_not_raise = False
        worker_thread = initialization["thread_obj"]
        actual_initialization_count = initialization["count"]
        actual_second_close_did_not_raise = second_close_did_not_raise
        actual_worker_dead_after_second_close = not worker_thread.is_alive()

        # THEN
        self.assertEqual(expected_initialization_count, actual_initialization_count)
        self.assertTrue(actual_second_close_did_not_raise)
        self.assertTrue(actual_worker_dead_after_second_close)

    def test_nested_start_is_rejected_without_rebuild(self) -> None:
        # GIVEN
        initialization: OneWorkerInitState = {
            "count": 0,
        }
        expected_initialization_count = 1
        expected_nested_start_message = "AsyncioWorkerPool has already been started"

        def initialize() -> None:
            initialization["thread_obj"] = threading.current_thread()
            initialization["count"] = initialization["count"] + 1
            loop = asyncio.get_running_loop()
            initialization["loop"] = loop

        # WHEN
        pool = AsyncioWorkerPool(capacity=1, initializer=initialize)
        self.addCleanup(pool.close)
        pool.start()
        nested_rejection: BaseException | None = None
        try:
            pool.start()
        except RuntimeError as exception:
            nested_rejection = exception
        worker_thread = initialization["thread_obj"]
        actual_initialization_count = initialization["count"]
        actual_nested_start_message = (
            str(nested_rejection) if nested_rejection is not None else None
        )
        actual_nested_start_rejected = isinstance(nested_rejection, RuntimeError)
        actual_worker_alive_after_rejected_start = worker_thread.is_alive()

        # THEN
        self.assertEqual(expected_initialization_count, actual_initialization_count)
        self.assertEqual(expected_nested_start_message, actual_nested_start_message)
        self.assertTrue(actual_nested_start_rejected)
        self.assertTrue(actual_worker_alive_after_rejected_start)

    def test_restart_after_close_is_rejected_without_rebuild(self) -> None:
        # GIVEN
        initialization: OneWorkerInitState = {
            "count": 0,
        }
        expected_initialization_count = 1
        expected_restart_message = "AsyncioWorkerPool cannot be restarted after close"

        def initialize() -> None:
            initialization["count"] = initialization["count"] + 1

        # WHEN
        pool = AsyncioWorkerPool(capacity=1, initializer=initialize)
        self.addCleanup(pool.close)
        pool.start()
        pool.close()
        restart_rejection: BaseException | None = None
        try:
            pool.start()
        except RuntimeError as exception:
            restart_rejection = exception
        actual_initialization_count = initialization["count"]
        actual_restart_message = (
            str(restart_rejection) if restart_rejection is not None else None
        )
        actual_restart_rejected = isinstance(restart_rejection, RuntimeError)

        # THEN
        self.assertEqual(expected_initialization_count, actual_initialization_count)
        self.assertEqual(expected_restart_message, actual_restart_message)
        self.assertTrue(actual_restart_rejected)

    def test_restart_after_rollback_is_rejected_without_rebuild(self) -> None:
        # GIVEN
        initialization: OneWorkerInitState = {
            "count": 0,
        }
        initialization_failure = ValueError("pool init failed")
        expected_initialization_count = 1
        expected_restart_message = (
            "AsyncioWorkerPool cannot be restarted after rollback"
        )

        def initialize() -> None:
            initialization["count"] = initialization["count"] + 1
            raise initialization_failure

        # WHEN
        pool = AsyncioWorkerPool(capacity=1, initializer=initialize)
        self.addCleanup(pool.close)
        raised_exception: BaseException | None = None
        try:
            pool.start()
        except ValueError as exception:
            raised_exception = exception
        restart_rejection: BaseException | None = None
        try:
            pool.start()
        except Exception as exception:
            restart_rejection = exception
        actual_initialization_count = initialization["count"]
        actual_restart_message = (
            str(restart_rejection) if restart_rejection is not None else None
        )
        actual_restart_rejected = isinstance(restart_rejection, RuntimeError)
        actual_same_exception_propagated = raised_exception is initialization_failure

        # THEN
        self.assertEqual(expected_initialization_count, actual_initialization_count)
        self.assertEqual(expected_restart_message, actual_restart_message)
        self.assertTrue(actual_restart_rejected)
        self.assertTrue(actual_same_exception_propagated)
