/*
 * Copyright 2017-2019 Baidu Inc.
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

#include <chrono>
#include <mutex>
#include <thread>
#include "bundle.h"

namespace openrasp {
TimeoutTask::TimeoutTask(v8::Isolate* _isolate, int _milliseconds) : isolate(_isolate) {
  time_point = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(_milliseconds);
}

void TimeoutTask::Run() {
  do {
    // try_lock_until is allowed to fail spuriously and return false
    // even if the mutex is not currently locked by any other thread
    // http://en.cppreference.com/w/cpp/thread/timed_mutex/try_lock_until
    if (mtx.try_lock_until(time_point)) {
      mtx.unlock();
      return;
    }
  } while (std::chrono::high_resolution_clock::now() < time_point);

  // TerminateExecution can be used by any thread
  // even if that thread has not acquired the V8 lock with a Locker object
  isolate->TerminateExecution();
  is_timeout = true;
  // wait until check process exited
  std::lock_guard<std::timed_mutex> lock(mtx);
}

std::timed_mutex& TimeoutTask::GetMtx() {
  return mtx;
}
}  // namespace openrasp