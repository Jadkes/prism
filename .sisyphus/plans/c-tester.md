# C Tester - Sanitizer Wrapper Tool

## TL;DR

> **Quick Summary**: Build `c_tester`, a single-command CLI tool that wraps GCC compilation with ASan/UBSan sanitizers, runs the program, parses error output, and provides human-readable fix suggestions. Simpler than GDB, catches more than raw compilation.
>
> **Deliverables**:
> - `c_tester` binary - the main tool (`c_tester file.c`)
> - `tests/` directory - 11 test files covering all error types
> - `Makefile` - build, test, install targets
> - `install_deps.sh` - one-liner to install libasan/libubsan/valgrind
>
> **Estimated Effort**: Short
> **Parallel Execution**: YES - 2 waves
> **Critical Path**: T1 -> T2 -> T3 -> T4 -> T6 / T1 -> T5 -> T6

---

## Context

### Original Request
"Make a plan and execute it to make a simple C tester for memory leaks, simple and easier debugging than the complicated classic GDB, catch errors more than the compiler, many times tell you the fix if your error is common."

### Interview Summary
**Key Discussions**:
- **Approach**: Sanitizer wrapper around GCC's ASan/UBSan (not custom runtime, not GDB wrapper)
- **CLI**: Single command - `c_tester file.c` builds, runs, reports all in one shot
- **Fix suggestions**: Human-readable explanations via pattern matching (not LLM API)
- **Dependencies**: Install libasan, libubsan, valgrind from dnf
- **Code style**: Linux kernel naming (AGENTS.md), early returns, functions < 40 lines, proper headers

**Research Findings**:
- GCC 15.2.1 installed but libasan/libubsan NOT installed -> `install_deps.sh` needed
- Valgrind available as supplementary leak checker
- No external libraries needed for the tool itself (standard C only)

---

## Work Objectives

### Core Objective
Build a single-command C error detection tool that catches memory leaks, buffer overflows, null dereferences, use-after-free, and more - with plain-English fix suggestions.

### Concrete Deliverables
- `c_tester` executable (single C file)
- `Makefile` with build/test/install targets
- `install_deps.sh` for dependency setup
- `tests/` directory with test cases for each error type

### Definition of Done
- [ ] `make && ./c_tester tests/null_deref.c` produces readable error + fix suggestion
- [ ] `make && ./c_tester tests/clean.c` produces "No errors detected" message
- [ ] `make test` runs all test cases and passes
- [ ] `gcc -Wall -Wextra -Wpedantic -o c_tester c_tester.c` produces zero warnings

### Must Have
- ASan + UBSan detection for: null deref, buffer overflow, use-after-free, memory leak, integer overflow, division by zero, uninitialized variable, stack overflow
- Signal-based fallback (SIGSEGV, SIGABRT) if sanitizers unavailable
- Human-readable error titles + fix suggestions
- Color-coded output (disabled when piped)
- Execution timeout (30s default)
- Auto-cleanup of compiled binaries (with `--keep` flag)
- Distinct exit codes: 0=clean, 1=errors, 2=compile fail, 3=file not found

### Must NOT Have (Guardrails)
- No external libraries (standard C only for the tool itself)
- No LLM/API calls for fix suggestions
- No source file modification
- No auto-fixing of code
- No config files (CLI flags only)
- No multi-file project support (single .c file only)
- No code modification whatsoever

---

## Verification Strategy

### Test Decision
- **Infrastructure exists**: NO (tool is in C, no test framework needed)
- **Automated tests**: Tests-after (test C files as inputs, verified by running c_tester)
- **Framework**: Shell scripts in Makefile test target
- **Agent-Executed QA**: ALWAYS mandatory

### QA Policy
Every task MUST include agent-executed QA scenarios.
- **CLI tool**: Use interactive_bash (tmux) - run commands, validate output
- **Evidence saved to**: `.sisyphus/evidence/task-{N}-{scenario-slug}.txt`

---

## Execution Strategy

### Parallel Execution Waves

```
Wave 1 (Start Immediately - foundation):
+-- Task 1: Install dependencies + project scaffolding [quick]
+-- Task 2: Core engine - compile, run, capture pipeline [deep]
+-- Task 3: Error pattern matching + fix suggestion engine [deep]

Wave 2 (After Wave 1 - integration):
+-- Task 4: CLI interface + output formatting + colors [quick]
+-- Task 5: Test suite - create test C files for all error types [quick]
+-- Task 6: Makefile + integration test + final verification [quick]

Critical Path: T1 -> T2 -> T3 -> T4 -> T6
               T1 -> T5 -> T6
Parallel Speedup: ~40% faster than sequential
Max Concurrent: 3 (Wave 1)
```

### Agent Dispatch Summary
- **Wave 1**: T1 -> `quick`, T2 -> `deep`, T3 -> `deep`
- **Wave 2**: T4 -> `quick`, T5 -> `quick`, T6 -> `quick`

---

## TODOs

 - [x] 1. Install Dependencies + Project Scaffolding

  **What to do**:
  - Create `install_deps.sh` that runs: `sudo dnf install -y gcc libasan libubsan valgrind`
  - Run it to install all dependencies
  - Create `c_tester.h` header file with all structs, enums, constants, function prototypes
  - Create `Makefile` with targets: `all`, `clean`, `test`, `install`, `deps`
  - Create `tests/` directory
  - Verify: `gcc --version` shows GCC 15, `ld -lasan` finds libasan

  **Must NOT do**:
  - Implement any logic beyond scaffolding
  - Modify any files outside this project

  **Recommended Agent Profile**:
  - **Category**: `quick`
  - **Skills**: `gcc`
    - `gcc`: Needed for verifying compiler setup and sanitizer availability
  - **Skills Evaluated but Omitted**:
    - `cmake`: Overkill for single-file project, Makefile is simpler

  **Parallelization**:
  - **Can Run In Parallel**: NO (must complete first - others depend on it)
  - **Blocks**: T2, T3, T4, T5, T6
  - **Blocked By**: None

  **References**:
  - `/home/jadkeskes/AGENTS.md` - Code style rules (kernel naming, headers, early returns)
  - `/home/jadkeskes/c_tester/` - Empty project directory

  **Acceptance Criteria**:
  - [ ] `install_deps.sh` exists and runs successfully
  - [ ] `libasan.so` and `libubsan.so` findable by linker
  - [ ] `c_tester.h` exists with all type definitions and prototypes
  - [ ] `Makefile` exists with all, clean, test targets
  - [ ] `tests/` directory exists

  **QA Scenarios**:

  ```
  Scenario: Dependencies install successfully
    Tool: Bash
    Steps:
      1. Run: bash /home/jadkeskes/c_tester/install_deps.sh
      2. Run: ld -lasan 2>&1
      3. Assert: output does NOT contain "cannot find"
    Expected Result: libasan linked successfully
    Evidence: .sisyphus/evidence/task-1-deps-install.txt

  Scenario: Header file compiles cleanly
    Tool: Bash
    Steps:
      1. Run: gcc -Wall -Wextra -fsyntax-only -c /home/jadkeskes/c_tester/c_tester.h
      2. Assert: exit code 0, no warnings
    Expected Result: Header has no syntax errors or warnings
    Evidence: .sisyphus/evidence/task-1-header-check.txt
  ```

  **Commit**: YES
  - Message: `feat(c-tester): install dependencies and scaffold project`
  - Files: `install_deps.sh`, `c_tester.h`, `Makefile`
  - Pre-commit: `gcc -Wall -Wextra -fsyntax-only c_tester.h`

- [x] 2. Core Engine - Compile, Run, Capture Pipeline

  **What to do**:
  - Implement `compile_with_sanitizers()`: GCC compilation with `-fsanitize=address,undefined -g -fno-omit-frame-pointer`
  - Implement `compile_fallback()`: Compile with `-g` only when sanitizers unavailable, rely on signal detection
  - Implement `run_binary()`: Fork/exec the compiled binary, capture stdout and stderr separately via pipes, enforce 30s timeout with `alarm()` or `select()`
  - Implement `generate_temp_path()`: Create temp binary name like `/tmp/c_tester_XXXXXX`
  - Implement `cleanup_binary()`: Remove compiled binary after run
  - Implement `file_exists()`, `is_c_file()`, `has_main_function()` for input validation
  - Capture compilation output, runtime output, and sanitizer output separately into `TestResult` struct
  - Handle child process timeouts: kill with SIGKILL after 30s, report "execution timeout"

  **Must NOT do**:
  - Implement error pattern matching (that's Task 3)
  - Implement output formatting (that's Task 4)
  - Use any libraries beyond stdlib

  **Recommended Agent Profile**:
  - **Category**: `deep`
  - **Skills**: `gcc`
    - `gcc`: Need to know exact sanitizer flags and compilation behavior
  - **Skills Evaluated but Omitted**:
    - `gdb`: Not needed for this task, only signal detection via waitpid
    - `valgrind`: Used later for supplementary checking, not core pipeline

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on T1 header)
  - **Blocks**: T3, T4, T6
  - **Blocked By**: T1

  **References**:
  - `c_tester.h` - TestResult struct, function prototypes, constants
  - GCC docs: `-fsanitize=address,undefined`, `-fno-omit-frame-pointer`, `-g`
  - `fork()`, `pipe()`, `dup2()`, `execvp()`, `waitpid()` man pages

  **Acceptance Criteria**:
  - [ ] Compiles a valid C file with sanitizers and captures output
  - [ ] Falls back to non-sanitizer compile when libasan missing
  - [ ] Kills hung processes after 30s timeout
  - [ ] Returns correct exit codes for compile failure vs runtime errors
  - [ ] Cleans up temp binary after run

  **QA Scenarios**:

  ```
  Scenario: Compile and run a clean program
    Tool: Bash
    Preconditions: Create tests/clean.c with simple hello world
    Steps:
      1. Create a temp test file: printf '#include <stdio.h>\nint main() { printf("hello\\n"); return 0; }' > /tmp/test_clean.c
      2. Compile with: gcc -fsanitize=address,undefined -g -o /tmp/test_out /tmp/test_clean.c
      3. Run: /tmp/test_out
      4. Assert: stdout contains "hello", no stderr output
      5. Cleanup: rm /tmp/test_out /tmp/test_clean.c
    Expected Result: Program runs cleanly with no sanitizer errors
    Evidence: .sisyphus/evidence/task-2-clean-run.txt

  Scenario: Timeout kills hung process
    Tool: Bash
    Preconditions: Create infinite loop test file
    Steps:
      1. Create: printf 'int main() { while(1); return 0; }' > /tmp/test_loop.c
      2. Compile: gcc -g -o /tmp/test_loop_bin /tmp/test_loop.c
      3. Run with timeout: timeout 5 /tmp/test_loop_bin 2>&1; echo "exit=$?"
      4. Assert: process killed within 5 seconds, exit code 124 (timeout)
      5. Cleanup: rm /tmp/test_loop_bin /tmp/test_loop.c
    Expected Result: Hung process killed, timeout reported
    Evidence: .sisyphus/evidence/task-2-timeout.txt

  Scenario: Detects compilation failure
    Tool: Bash
    Preconditions: Create file with syntax error
    Steps:
      1. Create: printf 'int main() { missing_semicolon }' > /tmp/test_syntax.c
      2. Compile: gcc -fsanitize=address,undefined -g -o /tmp/test_syntax_out /tmp/test_syntax.c 2>&1; echo "exit=$?"
      3. Assert: exit code non-zero, stderr contains error message
      4. Cleanup: rm -f /tmp/test_syntax.c /tmp/test_syntax_out
    Expected Result: Compilation fails with clear error message
    Evidence: .sisyphus/evidence/task-2-compile-fail.txt
  ```

  **Commit**: YES (with T1)
  - Message: `feat(c-tester): implement compile/run/capture pipeline`
  - Files: `c_tester.c` (compile/run functions)

- [x] 3. Error Pattern Matching + Fix Suggestion Engine

  **What to do**:
  - Implement `ErrorPattern` table with 8+ entries mapping sanitizer output patterns to error types and fix suggestions:
    - `ERROR: AddressSanitizer: heap-buffer-overflow` -> "Buffer overflow detected" -> "Check array bounds. Buffer size is N, but you accessed index M."
    - `ERROR: AddressSanitizer: stack-buffer-overflow` -> "Stack buffer overflow" -> "Use dynamic allocation (malloc) for large arrays, or increase buffer size."
    - `ERROR: AddressSanitizer: heap-use-after-free` -> "Use after free" -> "Don't access pointer after calling free(). Set pointer to NULL after free()."
    - `ERROR: AddressSanitizer: detected memory leaks` -> "Memory leak detected" -> "Add free() for memory allocated at FILE:LINE."
    - `ERROR: AddressSanitizer: SEGV on unknown address` -> "NULL pointer dereference" -> "Add NULL check before accessing this pointer."
    - `runtime error: signed integer overflow` -> "Integer overflow" -> "Use int64_t for large values, or add overflow checks before arithmetic."
    - `runtime error: division by zero` -> "Division by zero" -> "Check that divisor is non-zero before performing division."
    - `runtime error: load of uninitialized value` -> "Uninitialized variable" -> "Initialize variable before using it. Assign a default value at declaration."
  - Implement `classify_error()`: match error line against patterns, return ErrorType
  - Implement `parse_sanitizer_errors()`: scan full error output, find all error blocks, extract file:line info
  - Implement `parse_signal_errors()`: handle SIGSEGV (exit 139) and SIGABRT (exit 134) when sanitizers unavailable
  - Implement `generate_fix_suggestion()`: produce human-readable fix text with source context
  - Implement `get_source_line()`: read and return a specific line from source file for context
  - Implement `get_error_name()`: return string name for ErrorType enum
  - Implement `string_contains()`, `string_starts_with()`, `trim_whitespace()` helpers

  **Must NOT do**:
  - Implement compilation or execution (that's Task 2)
  - Implement CLI interface or colors (that's Task 4)
  - Use regex or external parsing libraries

  **Recommended Agent Profile**:
  - **Category**: `deep`
  - **Skills**: `gcc`
    - `gcc`: Need to understand exact sanitizer output format for pattern matching
  - **Skills Evaluated but Omitted**:
    - `static-analysis`: This is runtime error detection, not static analysis
    - `gdb`: Not needed, we're parsing sanitizer text output

  **Parallelization**:
  - **Can Run In Parallel**: YES (with T2 after T1)
  - **Parallel Group**: Wave 1 (with T2)
  - **Blocks**: T4, T6
  - **Blocked By**: T1

  **References**:
  - `c_tester.h` - ErrorType enum, DetectedError struct, ErrorPattern struct
  - GCC ASan output format: look at actual sanitizer output from test programs
  - AGENTS.md - Naming conventions, function size limits, comment style

  **Acceptance Criteria**:
  - [ ] Correctly classifies all 8 error types from sanitizer output
  - [ ] Extracts file:line numbers from error messages
  - [ ] Generates fix suggestions with source code context
  - [ ] Handles SIGSEGV and SIGABRT signals without sanitizers
  - [ ] Parses multiple errors from a single run

  **QA Scenarios**:

  ```
  Scenario: Detects null pointer dereference
    Tool: Bash
    Preconditions: Compile and run null deref test
    Steps:
      1. Create: printf '#include <stdlib.h>\nint main() { int *p = NULL; *p = 42; return 0; }' > /tmp/test_null.c
      2. Compile: gcc -fsanitize=address,undefined -g -o /tmp/test_null_bin /tmp/test_null.c 2>&1
      3. Run: /tmp/test_null_bin 2>&1
      4. Assert: stderr contains "SEGV" or "null" or "dereference"
      5. Cleanup: rm /tmp/test_null_bin /tmp/test_null.c
    Expected Result: Sanitizer output contains null dereference information
    Evidence: .sisyphus/evidence/task-3-null-deref.txt

  Scenario: Detects memory leak
    Tool: Bash
    Preconditions: Compile and run leak test
    Steps:
      1. Create: printf '#include <stdlib.h>\nint main() { malloc(100); return 0; }' > /tmp/test_leak.c
      2. Compile: gcc -fsanitize=address -g -o /tmp/test_leak_bin /tmp/test_leak.c 2>&1
      3. Run: ASAN_OPTIONS=detect_leaks=1 /tmp/test_leak_bin 2>&1
      4. Assert: stderr contains "detected memory leaks" or "LeakSanitizer"
      5. Cleanup: rm /tmp/test_leak_bin /tmp/test_leak.c
    Expected Result: LeakSanitizer reports memory leak
    Evidence: .sisyphus/evidence/task-3-memory-leak.txt

  Scenario: Detects buffer overflow
    Tool: Bash
    Preconditions: Compile and run overflow test
    Steps:
      1. Create: printf '#include <string.h>\nint main() { char buf[5]; strcpy(buf, "this is way too long"); return 0; }' > /tmp/test_overflow.c
      2. Compile: gcc -fsanitize=address -g -o /tmp/test_overflow_bin /tmp/test_overflow.c 2>&1
      3. Run: /tmp/test_overflow_bin 2>&1
      4. Assert: stderr contains "heap-buffer-overflow" or "stack-buffer-overflow"
      5. Cleanup: rm /tmp/test_overflow_bin /tmp/test_overflow.c
    Expected Result: ASan reports buffer overflow with location
    Evidence: .sisyphus/evidence/task-3-buffer-overflow.txt

  Scenario: Pattern matching extracts fix suggestion
    Tool: Bash
    Steps:
      1. Feed actual sanitizer output (from scenarios above) into pattern matching logic
      2. Assert: ErrorType is correctly classified
      3. Assert: Fix suggestion is generated with source context
    Expected Result: Pattern matcher produces correct error type and human-readable fix
    Evidence: .sisyphus/evidence/task-3-pattern-match.txt
  ```

  **Commit**: YES (with T2)
  - Message: `feat(c-tester): implement error pattern matching and fix suggestions`
  - Files: `c_tester.c` (pattern matching functions)

- [x] 4. CLI Interface + Output Formatting + Colors

  **What to do**:
  - Implement `main()`: parse arguments (`c_tester [options] file.c`), support `--keep` flag, `--timeout=N` flag, `--no-color` flag
  - Implement `init_colors()`: detect TTY with `isatty(STDOUT_FILENO)`, set ANSI codes or empty strings
  - Implement `print_banner()`: display tool name, version, source file being tested
  - Implement `print_colored()`: variadic color printing
  - Implement `print_summary()`: display compilation status, execution time, error count, all detected errors with titles, file:line, fix suggestions
  - Main flow: validate input -> compile -> run -> parse errors -> print report -> cleanup -> return exit code
  - Error display format for each error:
    ```
    [ERROR] Buffer Overflow (tests/overflow.c:15)
      Fix: Check array bounds. Buffer size is 5, but you accessed index 20.
      -> 15 | strcpy(buf, "this is way too long");
    ```
  - Success display:
    ```
    [OK] No errors detected - clean run in 12ms
    ```

  **Must NOT do**:
  - Modify source files being tested
  - Implement compilation logic (that's Task 2)
  - Implement pattern matching (that's Task 3)

  **Recommended Agent Profile**:
  - **Category**: `quick`
  - **Skills**: `gcc`
    - `gcc`: Verify final build with -Wall -Wextra -Wpedantic produces zero warnings
  - **Skills Evaluated but Omitted**:
    - `cmake`: Single Makefile project, no CMake needed

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on T2 and T3)
  - **Blocks**: T6
  - **Blocked By**: T2, T3

  **References**:
  - `c_tester.h` - ColorCodes struct, function prototypes
  - AGENTS.md - Code style, early returns, function size limits
  - ANSI color codes: \033[0m, \033[1m, \033[31m, \033[32m, \033[33m, \033[34m

  **Acceptance Criteria**:
  - [ ] `c_tester file.c` runs full pipeline and displays readable report
  - [ ] Colors disabled when output piped (`c_tester file.c | cat`)
  - [ ] `--keep` flag preserves compiled binary
  - [ ] `--timeout=N` sets custom timeout
  - [ ] Exit code 0 for clean runs, 1 for errors, 2 for compile fail, 3 for missing file
  - [ ] `gcc -Wall -Wextra -Wpedantic -o c_tester c_tester.c` zero warnings

  **QA Scenarios**:

  ```
  Scenario: Full pipeline with error detection
    Tool: interactive_bash (tmux)
    Preconditions: c_tester built, test files exist
    Steps:
      1. Run: ./c_tester tests/null_deref.c
      2. Assert: output contains "NULL pointer dereference" in red
      3. Assert: output contains "Fix:" with suggestion
      4. Assert: output contains source line context
      5. Assert: exit code is 1
    Expected Result: Error displayed with color, fix suggestion, source context
    Evidence: .sisyphus/evidence/task-4-error-report.txt

  Scenario: Clean run displays success message
    Tool: interactive_bash (tmux)
    Preconditions: c_tester built, tests/clean.c exists
    Steps:
      1. Run: ./c_tester tests/clean.c
      2. Assert: output contains "No errors detected" in green
      3. Assert: exit code is 0
    Expected Result: Success message displayed
    Evidence: .sisyphus/evidence/task-4-clean-run.txt

  Scenario: Colors disabled when piped
    Tool: Bash
    Steps:
      1. Run: ./c_tester tests/clean.c | cat -v
      2. Assert: output does NOT contain escape sequences (^[[32m etc.)
    Expected Result: No ANSI escape codes in piped output
    Evidence: .sisyphus/evidence/task-4-no-color.txt

  Scenario: Missing file returns proper error
    Tool: interactive_bash (tmux)
    Steps:
      1. Run: ./c_tester nonexistent.c
      2. Assert: output contains "not found" or similar
      3. Assert: exit code is 3
    Expected Result: Clear error message, correct exit code
    Evidence: .sisyphus/evidence/task-4-missing-file.txt
  ```

  **Commit**: YES (with T2, T3)
  - Message: `feat(c-tester): implement CLI interface and formatted output`
  - Files: `c_tester.c` (main function, output functions)

- [x] 5. Test Suite - Create Test C Files

  **What to do**:
  - Create `tests/clean.c` - Valid program, no errors, prints "hello" and exits cleanly
  - Create `tests/null_deref.c` - NULL pointer dereference: `int *p = NULL; *p = 42;`
  - Create `tests/buffer_overflow.c` - Stack buffer overflow: `char buf[5]; strcpy(buf, "overflow")`
  - Create `tests/use_after_free.c` - Use after free: `int *p = malloc(4); free(p); *p = 1;`
  - Create `tests/memory_leak.c` - Memory leak: `malloc(100);` without free
  - Create `tests/int_overflow.c` - Integer overflow: `int x = INT_MAX; x += 1;`
  - Create `tests/div_by_zero.c` - Division by zero: `int x = 1 / 0;`
  - Create `tests/uninit_var.c` - Uninitialized variable: `int x; printf("%d", x);`
  - Create `tests/infinite_loop.c` - Infinite loop: `while(1);`
  - Create `tests/syntax_error.c` - Syntax error: `int main() { missing }`
  - Create `tests/no_main.c` - No main function: `void helper() {}`
  - Each test file has header comment explaining what error it triggers

  **Must NOT do**:
  - Implement any logic beyond test files
  - Modify the tool itself

  **Recommended Agent Profile**:
  - **Category**: `quick`
  - **Skills**: `gcc`
    - `gcc`: Verify each test file compiles (or fails compilation as expected)
  - **Skills Evaluated but Omitted**:
    - `static-analysis`: These are runtime test inputs, not analysis targets

  **Parallelization**:
  - **Can Run In Parallel**: YES (with T4, after T1)
  - **Parallel Group**: Wave 2 (with T4)
  - **Blocks**: T6
  - **Blocked By**: T1

  **References**:
  - AGENTS.md - Header comments, function documentation, clean variable names
  - GCC sanitizer documentation - What triggers each error type

  **Acceptance Criteria**:
  - [ ] All 11 test files created in `tests/` directory
  - [ ] `tests/clean.c` compiles and runs without errors
  - [ ] Error test files compile with sanitizers and trigger expected errors
  - [ ] `tests/syntax_error.c` fails compilation
  - [ ] `tests/infinite_loop.c` runs forever (needs timeout to stop)

  **QA Scenarios**:

  ```
  Scenario: All test files compile or fail as expected
    Tool: Bash
    Steps:
      1. For each test in tests/*.c:
         - Compile: gcc -fsanitize=address,undefined -g -o /tmp/test_out tests/<file>.c 2>&1
         - Record: compilation success/failure
      2. Assert: clean.c compiles, syntax_error.c fails, others compile with sanitizers
    Expected Result: Compilation behavior matches expectations for each test file
    Evidence: .sisyphus/evidence/task-5-test-compilation.txt

  Scenario: Error test files trigger sanitizer errors
    Tool: Bash
    Steps:
      1. Compile tests/null_deref.c with ASan, run, capture stderr
      2. Assert: stderr contains error indicator (SEGV, null, etc.)
      3. Compile tests/buffer_overflow.c with ASan, run, capture stderr
      4. Assert: stderr contains "buffer-overflow"
      5. Compile tests/memory_leak.c with ASan, run with ASAN_OPTIONS=detect_leaks=1
      6. Assert: stderr contains "memory leak"
    Expected Result: Each error test triggers its expected sanitizer error
    Evidence: .sisyphus/evidence/task-5-error-trigger.txt
  ```

  **Commit**: YES
  - Message: `feat(c-tester): add test files for all error types`
  - Files: `tests/*.c`
  - Pre-commit: `ls tests/*.c | wc -l` should be 11

- [x] 6. Makefile + Integration Test + Final Verification

  **What to do**:
  - Write `Makefile` with targets:
    - `all` / `c_tester`: Build with `gcc -Wall -Wextra -Wpedantic -O2 -o c_tester c_tester.c`
    - `test`: Run c_tester against each test file, verify expected output
    - `clean`: Remove c_tester binary and temp files
    - `install`: Copy c_tester to `/usr/local/bin/` (requires sudo)
    - `deps`: Run `install_deps.sh`
  - `test` target runs each test and verifies:
    - `tests/clean.c` -> output contains "No errors"
    - `tests/null_deref.c` -> output contains "NULL pointer dereference"
    - `tests/buffer_overflow.c` -> output contains "Buffer overflow" or "buffer overflow"
    - `tests/use_after_free.c` -> output contains "Use after free"
    - `tests/memory_leak.c` -> output contains "Memory leak"
    - `tests/int_overflow.c` -> output contains "Integer overflow"
    - `tests/div_by_zero.c` -> output contains "Division by zero"
    - `tests/uninit_var.c` -> output contains "Uninitialized"
    - `tests/infinite_loop.c` -> exits within timeout, contains "timeout"
    - `tests/syntax_error.c` -> output contains compilation error
  - Run `make clean && make && make test` end-to-end
  - Fix any warnings from `gcc -Wall -Wextra -Wpedantic`
  - Verify final build is warning-free

  **Must NOT do**:
  - Add new features beyond what's specified
  - Modify test files

  **Recommended Agent Profile**:
  - **Category**: `quick`
  - **Skills**: `make`, `gcc`
    - `make`: Write proper Makefile with phony targets, dependencies
    - `gcc`: Ensure final build is warning-free with strict flags
  - **Skills Evaluated but Omitted**:
    - `cmake`: Makefile is sufficient for this project

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on all other tasks)
  - **Blocks**: None (final task)
  - **Blocked By**: T2, T3, T4, T5

  **References**:
  - AGENTS.md - Build commands, test execution policy
  - `c_tester.h` - Final API surface
  - All test files in `tests/`

  **Acceptance Criteria**:
  - [ ] `make clean && make` builds without warnings
  - [ ] `make test` passes all 11 test cases
  - [ ] `c_tester --help` or `c_tester` with no args shows usage
  - [ ] Binary can be installed with `sudo make install`

  **QA Scenarios**:

  ```
  Scenario: Full build and test pipeline
    Tool: interactive_bash (tmux)
    Steps:
      1. Run: make clean
      2. Run: make
      3. Assert: build completes, zero warnings from gcc -Wall -Wextra -Wpedantic
      4. Run: make test
      5. Assert: all tests pass, summary shows results
    Expected Result: Clean build, all tests pass
    Evidence: .sisyphus/evidence/task-6-full-build.txt

  Scenario: Clean build with zero warnings
    Tool: Bash
    Steps:
      1. Run: gcc -Wall -Wextra -Wpedantic -Wshadow -o c_tester c_tester.c 2>&1
      2. Assert: no warnings in output
    Expected Result: Zero compiler warnings
    Evidence: .sisyphus/evidence/task-6-zero-warnings.txt

  Scenario: Install and run from PATH
    Tool: interactive_bash (tmux)
    Steps:
      1. Run: sudo make install
      2. Run: c_tester tests/clean.c
      3. Assert: runs correctly from PATH
      4. Cleanup: sudo rm /usr/local/bin/c_tester
    Expected Result: c_tester works when installed to /usr/local/bin
    Evidence: .sisyphus/evidence/task-6-install.txt
  ```

  **Commit**: YES
  - Message: `feat(c-tester): add Makefile and integration tests`
  - Files: `Makefile`
  - Pre-commit: `make clean && make && make test`

---

## Final Verification Wave (MANDATORY - after ALL implementation tasks)

> 4 review agents run in PARALLEL. ALL must APPROVE.

- [x] F1. **Plan Compliance Audit** - `oracle`
  Verify all "Must Have" features implemented. Check all 11 test files exist. Verify evidence files in `.sisyphus/evidence/`.
  Output: `Must Have [8/8] | Tests [11/11] | VERDICT: APPROVE/REJECT`

- [x] F2. **Code Quality Review** - `unspecified-high`
  Run `gcc -Wall -Wextra -Wpedantic -o c_tester c_tester.c`. Check for: `as` casts, empty catches, console prints, unused imports, functions > 40 lines, Hungarian notation, vague names.
  Output: `Build [PASS/FAIL] | Warnings [0] | Functions [all < 40 lines] | VERDICT`

- [x] F3. **Real Manual QA** - `unspecified-high`
  Run `make clean && make && make test`. Execute each test file through c_tester. Verify error detection, fix suggestions, clean run message, timeout behavior, exit codes.
  Output: `Tests [11/11 pass] | Exit Codes [correct] | VERDICT`

- [x] F4. **Scope Fidelity Check** - `deep`
  Verify no external libraries used, no LLM calls, no source modification, no config files, no multi-file support creep. Check no unaccounted changes.
  Output: `Guardrails [CLEAN] | Scope [N/N compliant] | VERDICT`

---

## Commit Strategy

- **1+2+3+4**: `feat(c-tester): implement sanitizer wrapper with error detection` - c_tester.c, c_tester.h
- **5**: `feat(c-tester): add test files for all error types` - tests/*.c
- **6**: `feat(c-tester): add Makefile and integration tests` - Makefile

---

## Success Criteria

### Verification Commands
```bash
make clean && make               # Expected: zero warnings
./c_tester tests/clean.c          # Expected: "No errors detected" in green
./c_tester tests/null_deref.c     # Expected: error + fix suggestion in red
./c_tester tests/infinite_loop.c  # Expected: timeout message within 35s
./c_tester nonexistent.c          # Expected: "file not found", exit code 3
make test                         # Expected: 11/11 tests pass
```

### Final Checklist
- [ ] All "Must Have" features implemented (8 error types, fallback, colors, timeout, cleanup, exit codes)
- [ ] All "Must NOT Have" absent (no external libs, no LLM, no source mod, no config files)
- [ ] `gcc -Wall -Wextra -Wpedantic` produces zero warnings
- [ ] `make test` passes all 11 test cases
- [ ] `install_deps.sh` installs libasan, libubsan, valgrind
