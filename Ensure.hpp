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

/**
 * @file Ensure.hpp
 * @brief Assertion, warning, and conditional-throw utilities for debug builds.
 *
 * Provides four public facilities:
 *
 * | Facility        | Behaviour on trigger                                  |
 * |-----------------|-------------------------------------------------------|
 * | `ensure()`      | Prints to stderr and calls `_exit(EXIT_FAILURE)`      |
 * | `caution()`     | Always writes to stderr; execution continues          |
 * | `caution_if()`  | Writes to stderr if condition is true; continues      |
 * | `throw_if<T>()` | Throws exception of type T if condition is true       |
 *
 * All message arguments use `std::format`-style format strings.
 * `caution` and `caution_if` capture the call-site file and line automatically
 * via `std::source_location::current()` — no macros are required for them.
 *
 * **NDEBUG**: When `NDEBUG` is defined:
 * - `ensure` compiles to `((void)0)` — the condition expression is **not**
 *   evaluated.
 * - `caution` and `caution_if` produce no output, but their arguments **are**
 *   still evaluated (they are constructed objects, not macros).
 * - `throw_if` is **not** affected by `NDEBUG` and is always active.
 *
 * **Backtraces**: Define `ENSURE_WITH_BACKTRACE` before including this header
 * to print a stack trace to stderr when `ensure` triggers (POSIX only —
 * no effect on Windows or Emscripten).
 */
#pragma once

#include <cstdlib>
#include <iostream>
#include <format>
#include <source_location>
#include <utility>
#if !defined(__EMSCRIPTEN__) && !defined(_WIN32)
#include <unistd.h>
#include <execinfo.h>
#endif

/**
 * @brief Print an unconditional caution message to `stderr`.
 *
 * Use as a function call: `caution("fmt", args...)`.
 *
 * Writes a formatted message to `stderr` prefixed with
 * `"Caution in <file> line <line>: "`. Execution continues normally.
 * The call-site location is captured automatically via
 * `std::source_location::current()`.
 *
 * Produces no output when `NDEBUG` is defined, but arguments are still
 * evaluated.
 *
 * @param fmt  A `std::format`-style format string.
 * @param args Optional format arguments.
 */
// caution and caution_if are CTAD structs rather than functions because C++
// requires a function parameter pack to be the last parameter, which prevents
// placing a defaulted std::source_location after Args&&... args. The class-
// template constructor workaround is valid because Args is the class's pack,
// not a function-template pack; by instantiation time it's fully concrete.
// Collapse these back to plain functions once the language limitation is lifted.
template<class... Args>
struct caution {
    caution(std::format_string<Args...> fmt,
            Args&&... args,
            std::source_location loc = std::source_location::current()) {
#ifndef NDEBUG
        std::cerr << "Caution in " << loc.file_name() << " line " << loc.line()
                  << ": " << std::vformat(fmt.get(), std::make_format_args(args...))
                  << '\n';
#endif
    }
};
template<class... Args>
caution(std::format_string<Args...>, Args&&...) -> caution<Args...>;

/**
 * @brief Print a caution message to `stderr` if a condition is true.
 *
 * Use as a function call: `caution_if(condition, "fmt", args...)`.
 *
 * When @p condition evaluates to `true`, writes a formatted message to
 * `stderr` prefixed with `"Caution in <file> line <line>: "`. Execution
 * continues normally in either branch.
 * The call-site location is captured automatically via
 * `std::source_location::current()`.
 *
 * Produces no output when `NDEBUG` is defined, but arguments are still
 * evaluated.
 *
 * @param condition  If `true`, the caution message is printed.
 * @param fmt        A `std::format`-style format string.
 * @param args       Optional format arguments.
 */
template<class... Args>
struct caution_if {
    caution_if(bool condition,
               std::format_string<Args...> fmt,
               Args&&... args,
               std::source_location loc = std::source_location::current()) {
        if (condition)
            caution<Args...>(fmt, std::forward<Args>(args)..., loc);
    }
};
template<class... Args>
caution_if(bool, std::format_string<Args...>, Args&&...) -> caution_if<Args...>;

// ensure remains a macro so that NDEBUG strips the condition expression
// entirely — including any side effects it may have.

#ifdef NDEBUG
#  define ensure(...) ((void)0)  ///< No-op in release builds; condition not evaluated.
#else

/**
 * @brief Assert that a runtime condition is true; terminate on failure.
 *
 * Tests @p condition at runtime. If it is false, prints a diagnostic message
 * to `stderr` (including file name, line number, and the optional formatted
 * message) and immediately terminates the process via `_exit(EXIT_FAILURE)`.
 * Destructors and `atexit` handlers are **not** called.
 *
 * If `ENSURE_WITH_BACKTRACE` is defined and the platform is POSIX, a stack
 * trace is also written to `stderr` before termination.
 *
 * Compiles to `((void)0)` when `NDEBUG` is defined; the condition expression
 * is not evaluated.
 *
 * @param condition  Expression that must evaluate to `true` under normal
 *                   circumstances.
 * @param fmt        Optional `std::format`-style format string describing the
 *                   failure (default: `"No details provided."`).
 * @param ...        Optional arguments for @p fmt.
 *
 * @note Unlike `throw_if`, this macro cannot be caught — it terminates the
 *       process. Use it for invariants that indicate programming errors.
 */
#define ensure(condition, ...)  ((void)detail::ensure_impl(__FILE__, __LINE__, (condition) __VA_OPT__(,) __VA_ARGS__))

/// @cond INTERNAL
namespace detail {

template<class... Args>
void ensure_impl(const char *file, int line, bool condition,
                 std::format_string<Args...> fmt = "No details provided.",
                 Args&&... args) {
    if (!condition) {
        std::cerr << "Assertion failure in " << file << " line " << line << ": "
                  << std::vformat(fmt.get(), std::make_format_args(args...))
                  << std::endl;

#if defined(ENSURE_WITH_BACKTRACE) && !defined(__EMSCRIPTEN__) && !defined(_WIN32)
        const int maxPointers = 100;
        void *stackPointers[maxPointers];
        auto pointerCount = backtrace(stackPointers, maxPointers);
        backtrace_symbols_fd(stackPointers, pointerCount, STDERR_FILENO);
#endif
        _exit(EXIT_FAILURE);
    }
}

} // namespace detail
/// @endcond

#endif // NDEBUG

/**
 * @brief Throw an exception of type @p T if @p condition is true.
 *
 * Constructs and throws an exception of type @p T, forwarding @p args to its
 * constructor, when @p condition evaluates to `true`. Has no effect when
 * @p condition is `false`.
 *
 * Unlike `ensure`, this function is **always active** regardless of `NDEBUG`,
 * making it suitable for validating inputs at API boundaries where callers
 * expect recoverable error handling via exceptions.
 *
 * @tparam T     The exception type to throw. Must be constructible from
 *               @p args.
 * @tparam Args  Constructor argument types for @p T (deduced).
 *
 * @param condition  If `true`, an exception of type @p T is thrown.
 * @param args       Arguments forwarded to the constructor of @p T.
 *
 * @throws T When @p condition is `true`.
 */
template<class T, class... Args>
constexpr inline void throw_if(bool condition, Args&&... args) {
    if (condition)
        throw T(std::forward<Args>(args)...);
}
