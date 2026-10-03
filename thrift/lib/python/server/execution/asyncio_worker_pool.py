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

from collections.abc import Callable, Iterable
from typing import Any, Literal

from thrift.python.server_impl.execution.asyncio_worker import AsyncioWorker


class AsyncioWorkerPool:
    def __init__(self, capacity: int, initializer: Callable[[], Any]) -> None:
        self._capacity = capacity
        self._initializer = initializer
        self.__workers: list[AsyncioWorker] = []
        self.__state: Literal["created", "started", "closed", "rolled_back"] = "created"

    def start(self) -> None:
        if self.__state == "started":
            raise RuntimeError("AsyncioWorkerPool has already been started")
        if self.__state == "closed":
            raise RuntimeError("AsyncioWorkerPool cannot be restarted after close")
        if self.__state == "rolled_back":
            raise RuntimeError("AsyncioWorkerPool cannot be restarted after rollback")
        if self._capacity == 0:
            self.__state = "started"
            return
        try:
            for _ in range(self._capacity):
                worker = AsyncioWorker(self._initializer)
                worker.start()
                self.__workers.append(worker)
        except BaseException as startup_failure:  # noqa: B036 - rollback must cover all startup failures
            for cleanup_failure in self._close_workers(reversed(self.__workers)):
                startup_failure.add_note(
                    f"Worker-pool rollback also failed: {cleanup_failure!r}"
                )
            self.__state = "rolled_back"
            raise
        self.__state = "started"

    def close(self) -> None:
        cleanup_failures = self._close_workers(self.__workers)
        self.__state = "closed"
        if len(cleanup_failures) == 1:
            raise cleanup_failures[0]
        if cleanup_failures:
            raise BaseExceptionGroup("Worker cleanup failed", cleanup_failures)

    def _close_workers(self, workers: Iterable[AsyncioWorker]) -> list[BaseException]:
        cleanup_failures: list[BaseException] = []
        for worker in workers:
            try:
                worker.close()
            except BaseException as exception:  # noqa: B036 - aggregate cleanup
                cleanup_failures.append(exception)
        self.__workers.clear()
        return cleanup_failures
