# c_tester Learnings

## CLI Interface Implementation (Task 4)

### Key Decisions
1. **Color handling**: Used `isatty(STDOUT_FILENO)` to auto-detect TTY for color auto-disable. This follows the AGENTS.md guidance on smart defaults.

2. **Function signatures**: The `print_summary()` function signature in the header (3 params) didn't match the original task spec (5 params with errors array and source_file). Followed the header file as source of truth since it's the contract.

3. **Struct vs pointer**: In `main()`, `colors` is a `ColorCodes` struct (not pointer), so used `colors.red` not `colors->red`. The `init_colors()` function takes a pointer, but after initialization, main accesses fields directly.

4. **Error handling**: Used early returns throughout `main()` for file not found (exit 3) and usage errors (exit 4) as specified in the task.

### Gotchas Encountered
1. **Syntax error**: Missing comma in function call `init_colors(&colors, use_color)` - the comma was present but the issue was actually a missing comma between arguments in one call.

2. **Unnecessary inline comments**: The AGENTS.md says "Comment Style (Explain WHY, not WHAT)". Inline comments like `/* Parse arguments */` and `/* Validate input */` were removed because they explain WHAT the code does, not WHY.

3. **Header compliance**: The header file `c_tester.h` is the contract. When the task spec and header disagree on function signatures, follow the header.

### Patterns Used
- **Linux kernel naming**: `use_color`, `error_count`, `source_file` (purpose over type)
- **Early returns**: `if (!source_file) { print usage; return; }` pattern
- **Functions < 40 lines**: Each function is concise and focused
- **Doc comments with WHY**: Every function has a doc comment explaining why it exists

### Build Verification
- Command: `gcc -Wall -Wextra -Wpedantic -o c_tester c_tester.c`
- Result: Zero warnings, zero errors
- Tests pass:
  - `./c_tester` → shows usage, exits 4 ✓
  - `./c_tester nonexistent.c` → shows "file not found", exits 3 ✓

## Signal Safety Review (Wave 1 - Task 5)

### Findings
1. **No signal handlers exist** in c_tester.c - only `kill()` sends SIGKILL to child processes
2. **child_pid parameter** does not need `volatile sig_atomic_t` because:
   - Tool is single-threaded (documented in header line 13)
   - No signal handlers are registered that would access child_pid asynchronously
   - child_pid is written once in parent after fork() returns
   - Callers currently pass NULL (lines 1692, 1701 in c_tester.c)
3. **kill() usage is safe** - sends signals to child, never registers handlers

### Changes Made
- Added signal safety comments to `run_binary()` and `run_with_valgrind()` documenting why volatile is not needed
- No volatile additions required - minimal approach as specified

### Verification
- `gcc -Wall -Wextra -Werror -fsanitize=address,undefined`: Clean
- `./c_tester tests/null_deref.c`: Works correctly
- `./c_tester tests/infinite_loop.c`: Timeout works (30s)

### Pattern
Signal safety: Only use `volatile sig_atomic_t` when variable is shared between signal handler and main flow. Document the decision.

## Resource Leak Fix (Wave 1 - Task 1)

### Issue
On fork() failure in run_binary(), only pipe read ends [0] were closed, leaking write ends [1].

### Fix
Close all 4 pipe FDs (both ends of both pipes) when fork() fails:
```c
if (pid < 0) {
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[0]);
    close(stderr_pipe[1]);
    return -1;
}
```

### Verification
- gcc -Wall -Wextra -Werror -fsanitize=address,undefined: Clean
- lsp_diagnostics: No errors
- valgrind --track-fds=yes: 3 FDs open at exit (stdin, stdout, stderr only)

### Pattern
Follow AGENTS.md "Fail fast, fail explicitly" - close ALL allocated resources before returning error.

## generate_temp_path() Race Condition Fix (Wave 1 - Task 3)

### Issue
`mkstemp(buffer)` creates and opens a file, then `close(fd)` leaves the file on disk.
This creates a race condition window where another process could open/modify the temp file before the compiler writes to it.

### Fix Applied
Added `unlink(buffer)` after `close(fd)` in `generate_temp_path()`:
```c
int fd = mkstemp(buffer);
if (fd >= 0) {
    close(fd);
    /* unlink to prevent race condition - file removed but path usable */
    unlink(buffer);
}
```

### Why This Works
- `mkstemp()` creates the file and returns a file descriptor
- `close(fd)` closes the FD
- `unlink(buffer)` removes the directory entry, but the path string is still valid
- When compile functions use `gcc -o 'binary'`, they create a new file at that path
- The temp file no longer persists on disk after the program runs

### Verification
- `gcc -Wall -Wextra -Werror -fsanitize=address,undefined`: Clean build ✓
- `ls /tmp/c_tester_*` after run: No temp files persist ✓
- LSP diagnostics: No errors ✓

### Pattern
From AGENTS.md "Resource cleanup": Comment explains WHY (race condition prevention), not WHAT (unlink call).
Security-related code needs comments explaining non-obvious behavior to prevent future regressions.

## Resource Leak Fix - run_with_valgrind() (Wave 1 - Task 2)

### Issue
Same bug pattern as run_binary():
1. Pipe creation: `pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0` - if stdout_pipe succeeds but stderr_pipe fails, stdout_pipe FDs are leaked
2. Fork failure: Only pipe read ends [0] were closed, leaking write ends [1]

### Fix Applied
1. **Pipe creation**: Split into two separate checks. If stderr_pipe fails after stdout_pipe succeeds, close both stdout_pipe FDs before returning -1.
2. **Fork failure**: Close all 4 pipe FDs (both ends of both pipes) before returning -1.

### Verification
- gcc -Wall -Wextra -Werror -fsanitize=address,undefined: Clean
- valgrind --track-fds=yes: "FILE DESCRIPTORS: 3 open (3 inherited) at exit" ✓
- LSP diagnostics: No errors

## Resource Leak Fix (Wave 1 - Task 1)

### Issue
On fork() failure in run_binary(), only pipe read ends [0] were closed, leaking write ends [1].

### Fix
Close all 4 pipe FDs (both ends of both pipes) when fork() fails:
```c
if (pid < 0) {
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[0]);
    close(stderr_pipe[1]);
    return -1;
}
```

### Verification
- gcc -Wall -Wextra -Werror -fsanitize=address,undefined: Clean
- lsp_diagnostics: No errors
- valgrind --track-fds=yes: 3 FDs open at exit (stdin, stdout, stderr only)

### Pattern
Follow AGENTS.md "Fail fast, fail explicitly" - close ALL allocated resources before returning error.

## Task 5: Extract run_with_timeout() helper

### What was done
- Created `run_with_timeout()` function in c_tester.c that encapsulates shared pipe/fork/select/read/cleanup logic
- `run_binary()` now builds a 3-element argv[] and delegates to `run_with_timeout()`
- `run_with_valgrind()` now builds a 7-element argv[] and delegates to `run_with_timeout()`
- Added declaration to c_tester.h

### Design decisions
- `run_with_timeout()` takes `char *const argv[]` so callers can build any argv they need
- The `binary` param is passed separately for execvp's first arg (argv[0])
- Timeout logic uses CLOCK_MONOTONIC deadline + select() loop (same as original)
- All pipe FD cleanup and waitpid logic stays in the shared function

### Verification
- Compiles clean with `-Wall -Wextra -Werror -fsanitize=address,undefined`
- `run_binary()` path: null_deref.c, buffer_overflow.c, use_after_free.c all detected correctly
- `run_with_valgrind()` path: valgrind_uninit.c detects "Uninitialized Conditional"
- No behavior change (same error messages and exit codes)

### Code size improvement
- `run_binary()`: ~90 lines → ~10 lines
- `run_with_valgrind()`: ~140 lines → ~12 lines
- Shared logic: ~80 lines in one place instead of duplicated twice
