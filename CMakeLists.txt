# Copyright 2026 L. Richard Moore Jr.
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

cmake_minimum_required(VERSION 3.18)
project(ensure VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Catch2 3 REQUIRED)

add_executable(Ensure-test Ensure-test.cpp Ensure.hpp)
target_link_libraries(Ensure-test PRIVATE Catch2::Catch2WithMain)

# Ensure-test-main exercises the _exit() path; run it manually, not via ctest.
add_executable(Ensure-test-main Ensure-test-main.cpp Ensure.hpp)

enable_testing()
include(Catch)
catch_discover_tests(Ensure-test)
