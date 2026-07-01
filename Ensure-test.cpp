/*
 * Copyright 2026 L. Richard Moore Jr.
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

#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <stdexcept>
#include "Ensure.hpp"

// Redirects std::cerr for the duration of fn(), returns what was written.
template<typename F>
static std::string captureStderr(F fn) {
    std::ostringstream buf;
    std::streambuf* old = std::cerr.rdbuf(buf.rdbuf());
    fn();
    std::cerr.rdbuf(old);
    return buf.str();
}

// ── ensure ────────────────────────────────────────────────────────────────────
// The terminating path of ensure (condition == false) cannot be tested
// in-process because it calls _exit(). See Ensure-test-main.cpp.

TEST_CASE("ensure: silent and non-terminating when condition is true") {
    std::string output = captureStderr([] {
        ensure(true);
        ensure(true, "no output expected");
        ensure(true, "formatted {} {}", 42, "value");
        ensure(1 + 1 == 2);
    });
    CHECK(output.empty());
}

// ── warn ──────────────────────────────────────────────────────────────────────

TEST_CASE("caution:writes 'Warning' prefix and filename to stderr") {
    std::string output = captureStderr([] {
        caution("hello");
    });
    CHECK(output.find("Caution") != std::string::npos);
    CHECK(output.find("Ensure-test.cpp") != std::string::npos);
    CHECK(output.find("hello") != std::string::npos);
}

TEST_CASE("caution:formats args into message") {
    std::string output = captureStderr([] {
        caution("value is {}", 42);
    });
    CHECK(output.find("value is 42") != std::string::npos);
}

TEST_CASE("caution:multiple format args") {
    std::string output = captureStderr([] {
        caution("{} + {} = {}", 1, 2, 3);
    });
    CHECK(output.find("1 + 2 = 3") != std::string::npos);
}

// ── warn_if ───────────────────────────────────────────────────────────────────

TEST_CASE("caution_if:silent when condition is false") {
    std::string output = captureStderr([] {
        caution_if(false, "should not appear");
    });
    CHECK(output.empty());
}

TEST_CASE("caution_if:prints when condition is true") {
    std::string output = captureStderr([] {
        caution_if(true, "appeared");
    });
    CHECK(output.find("appeared") != std::string::npos);
    CHECK(output.find("Caution") != std::string::npos);
}

TEST_CASE("caution_if:formats args") {
    std::string output = captureStderr([] {
        caution_if(true, "count is {}", 99);
    });
    CHECK(output.find("count is 99") != std::string::npos);
}

TEST_CASE("caution_if:usable as unbraced if/else body") {
    // caution_if is a function, so it is always safe in unbraced if/else contexts.
    bool elseTaken = false;
    if (false)
        caution_if(true, "not reached");
    else
        elseTaken = true;
    CHECK(elseTaken);
}

// ── throw_if ──────────────────────────────────────────────────────────────────

TEST_CASE("throw_if: does not throw when condition is false") {
    REQUIRE_NOTHROW(throw_if<std::runtime_error>(false, "won't throw"));
}

TEST_CASE("throw_if: throws correct type when condition is true") {
    REQUIRE_THROWS_AS(throw_if<std::runtime_error>(true,    "msg"), std::runtime_error);
    REQUIRE_THROWS_AS(throw_if<std::overflow_error>(true,   "msg"), std::overflow_error);
    REQUIRE_THROWS_AS(throw_if<std::invalid_argument>(true, "msg"), std::invalid_argument);
}

TEST_CASE("throw_if: exception carries the provided message") {
    try {
        throw_if<std::runtime_error>(true, "specific message");
        FAIL("should have thrown");
    } catch (const std::runtime_error& e) {
        CHECK(std::string(e.what()) == "specific message");
    }
}

TEST_CASE("throw_if: false condition leaves control flow unchanged") {
    int reached = 0;
    throw_if<std::runtime_error>(false, "no throw");
    reached = 1;
    CHECK(reached == 1);
}
