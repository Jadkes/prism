# C Tester v2 - Medium Features

## TODOs

- [ ] T1: Thread sanitizer support - Add `--tsan` flag, compile with `-fsanitize=thread`, detect data race patterns
- [ ] T2: C++ support - Accept `.cpp`/`.cxx`/`.cc` files, use `g++` with sanitizers, add C++ error patterns
- [ ] T3: Multi-file project support - Accept multiple source files, compile together, track per-file errors
- [ ] T4: Valgrind fallback - Detect valgrind availability, run as alternative to sanitizers, parse valgrind output
- [ ] T5: HTML reports - Add `--html=output.html` flag, generate styled HTML report with error details
- [ ] T6: Build system integration - Add CTest support, `make test-all` target, pre-commit hook script

## Dependencies

- T1, T2: Independent, can parallel
- T3: Depends on T2 (multi-file needs C++ support for .cpp files)
- T4: Independent
- T5: Independent (only adds output mode)
- T6: Depends on T1-T5 (tests all features)

## Final Verification Wave

- [ ] F1: Code Quality Review - Oracle reviews all changes for architecture, naming, function size
- [ ] F2: Security Review - Oracle checks for injection vulnerabilities in shell commands
- [ ] F3: QA Testing - Run all existing tests + new feature tests, verify 0 warnings
- [ ] F4: Context Mining - Check git history for any missed conventions
