/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fstream>
#include <sstream>

#include "base/task_runner.h"
#include "ftrace_reader/ftrace_controller.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::HasSubstr;
using testing::Not;

namespace perfetto {
namespace {

class FakeTaskRunner : public base::TaskRunner {
  void PostTask(std::function<void()>) override {}
  void PostDelayedTask(std::function<void()>, int delay_ms) override {}
  void AddFileDescriptorWatch(int fd, std::function<void()>) override {}
  void RemoveFileDescriptorWatch(int fd) override {}
};

const char kTracePath[] = "/sys/kernel/debug/tracing/trace";

std::string GetTraceOutput() {
  std::ifstream fin(kTracePath, std::ios::in);
  if (!fin) {
    ADD_FAILURE() << "Could not read trace output";
    return "";
  }
  std::ostringstream stream;
  stream << fin.rdbuf();
  fin.close();
  return stream.str();
}

}  // namespace

TEST(FtraceControllerIntegrationTest, ClearTrace) {
  FakeTaskRunner runner;
  std::unique_ptr<FtraceController> ftrace = FtraceController::Create(&runner);
  ftrace->WriteTraceMarker("Hello, World!");
  ftrace->ClearTrace();
  EXPECT_THAT(GetTraceOutput(), Not(HasSubstr("Hello, World!")));
}

TEST(FtraceControllerIntegrationTest, TraceMarker) {
  FakeTaskRunner runner;
  std::unique_ptr<FtraceController> ftrace = FtraceController::Create(&runner);
  ftrace->WriteTraceMarker("Hello, World!");
  EXPECT_THAT(GetTraceOutput(), HasSubstr("Hello, World!"));
}

TEST(FtraceControllerIntegrationTest, EnableDisableEvent) {
  FakeTaskRunner runner;
  std::unique_ptr<FtraceController> ftrace = FtraceController::Create(&runner);
  ftrace->EnableEvent("sched", "sched_switch");
  sleep(1);
  EXPECT_THAT(GetTraceOutput(), HasSubstr("sched_switch"));

  ftrace->DisableEvent("sched", "sched_switch");
  ftrace->ClearTrace();
  sleep(1);
  EXPECT_THAT(GetTraceOutput(), Not(HasSubstr("sched_switch")));
}

TEST(FtraceControllerIntegrationTest, EnableDisableTracing) {
  FakeTaskRunner runner;
  std::unique_ptr<FtraceController> ftrace = FtraceController::Create(&runner);
  ftrace->ClearTrace();
  EXPECT_TRUE(ftrace->IsTracingEnabled());
  ftrace->WriteTraceMarker("Before");
  ftrace->DisableTracing();
  EXPECT_FALSE(ftrace->IsTracingEnabled());
  ftrace->WriteTraceMarker("During");
  ftrace->EnableTracing();
  EXPECT_TRUE(ftrace->IsTracingEnabled());
  ftrace->WriteTraceMarker("After");
  EXPECT_THAT(GetTraceOutput(), HasSubstr("Before"));
  EXPECT_THAT(GetTraceOutput(), Not(HasSubstr("During")));
  EXPECT_THAT(GetTraceOutput(), HasSubstr("After"));
}

}  // namespace perfetto
