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

#include "header.h"

using namespace openrasp;

V8Class v8_class;
ContextClass ctx_class;
bool isInitialized = false;
Snapshot* snapshot = nullptr;
std::mutex mtx;

void openrasp::plugin_info(Isolate* isolate, const std::string& message) {
  auto data = isolate->GetData();
  auto custom_data = GetCustomData(isolate);
  if (!custom_data || !custom_data->env) {
    return;
  }
  auto env = custom_data->env;
  auto msg = env->NewStringUTF(message.c_str());
  env->CallStaticVoidMethod(v8_class.cls, v8_class.plugin_log, msg);
}
void openrasp::alarm_info(Isolate* isolate,
                          v8::Local<v8::String> type,
                          v8::Local<v8::Object> params,
                          v8::Local<v8::Object> result) {}

Isolate* GetIsolate() {
  static thread_local IsolatePtr isolate_ptr;
  Isolate* isolate = isolate_ptr.get();
  if (snapshot) {
    if (!isolate || isolate->IsExpired(snapshot->timestamp)) {
      std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
      if (lock) {
        auto duration = std::chrono::system_clock::now().time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        isolate = Isolate::New(snapshot, millis);
        isolate->GetData()->custom_data = new CustomData();
        isolate_ptr.reset(isolate);
      }
    }
  }
  return isolate;
}