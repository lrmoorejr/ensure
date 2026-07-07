# Ensure.hpp

[API docs](https://lrmoorejr.github.io/ensure/)

All I really wanted was an assertion function that included a filename, line number, and an optional formatted messsage.  What C++ offered was a
circa 1969 C assert() macro whose main feature is to crash.  So, I made this little header library-- maybe it will be useful for you as well.

Ensure.hpp is a single-header C++20 library providing assertion, caution, and conditional-throw utilities with `std::format`-style messages.

```cpp
#include "Ensure.hpp"

ensure(ptr != nullptr, "expected non-null pointer, got index {}", i);
caution("cache miss rate is high: {:.1f}%", rate * 100);
caution_if(elapsed > budget, "frame over budget by {}ms", elapsed - budget);
throw_if<std::invalid_argument>(value < 0, "value must be non-negative");
```

## Requirements

- C++20 or later
- Header-only — copy `Ensure.hpp` into your project and `#include` it
- Stack traces (`ENSURE_WITH_BACKTRACE`) require a POSIX platform (Linux, macOS); no effect on Windows or Emscripten

## Facilities

| Facility           | Behaviour on trigger                              | Active under NDEBUG? |
|--------------------|---------------------------------------------------|----------------------|
| `ensure()`         | Prints to stderr, calls `_exit(EXIT_FAILURE)`     | No — no-op, condition not evaluated |
| `caution()`        | Always writes to stderr; execution continues      | No — silent, but args still evaluated |
| `caution_if()`     | Writes to stderr if condition is true; continues  | No — silent, but args still evaluated |
| `throw_if<T>()`    | Throws exception of type T                        | **Yes — always active** |

## Usage

### ensure(condition [, fmt, args...])

Use for invariants that indicate a programming error. On failure, prints a diagnostic and terminates immediately via `_exit` — no destructors run, no exceptions to catch.

```cpp
ensure(index < size);
ensure(result == expected, "got {} but expected {}", result, expected);
```

Under `NDEBUG`, `ensure` compiles to `((void)0)` — the condition expression itself is not evaluated.

### caution(fmt [, args...])

Unconditionally prints a message to stderr and continues. The call-site file and line number are captured automatically.

```cpp
caution("unexpected state, recovering");
caution("retry {}/{}", attempt, max_retries);
```

### caution_if(condition, fmt [, args...])

Prints a message only when the condition is true.

```cpp
caution_if(buffer.size() > threshold, "buffer pressure: {} bytes", buffer.size());
```

### throw_if<ExceptionType>(condition, args...)

Throws an exception when the condition is true. The exception is constructed by forwarding `args` directly to its constructor. Unlike the other facilities, `throw_if` is always active regardless of `NDEBUG`, making it suitable for input validation at API boundaries.

```cpp
throw_if<std::out_of_range>(index >= size, "index out of range");
throw_if<std::runtime_error>(fd < 0, "failed to open file");
```

## NDEBUG behaviour

`ensure`, `caution`, and `caution_if` are designed for use during development and testing. When `NDEBUG` is defined (typical in release builds):

- `ensure` becomes a true no-op: the condition expression is **not evaluated**, matching the behaviour of the standard `assert` macro.
- `caution` and `caution_if` produce no output. A good optimiser will eliminate pure-expression arguments entirely.
- `throw_if` is unaffected and always active.

## Stack traces

Define `ENSURE_WITH_BACKTRACE` before including the header to print a stack trace to stderr when `ensure` triggers:

```cpp
#define ENSURE_WITH_BACKTRACE
#include "Ensure.hpp"
```

This uses `backtrace` / `backtrace_symbols_fd` and is only available on POSIX platforms. It has no effect on Windows or Emscripten.

## License

Apache License 2.0 — see [LICENSE](https://github.com/lrmoorejr/ensure/blob/main/LICENSE).
