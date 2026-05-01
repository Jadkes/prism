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
