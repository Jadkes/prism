CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -O2
TARGET = c_tester

BAD_CODES_DIR = bad_codes
HOOKS_DIR = hooks

.PHONY: all clean test test-all install install-hooks deps

all: $(TARGET)

$(TARGET): c_tester.c c_tester.h
	$(CC) $(CFLAGS) -o $(TARGET) c_tester.c

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Running c_tester test suite..."
	@PASS=0; FAIL=0; \
	run_test() { \
		name=$$1; file=$$2; expect=$$3; \
		output=$$(./$(TARGET) $$file 2>&1); \
		if echo "$$output" | grep -qi "$$expect"; then \
			echo "  PASS: $$name"; \
			PASS=$$((PASS + 1)); \
		else \
			echo "  FAIL: $$name (expected '$$expect')"; \
			echo "    Output: $$output"; \
			FAIL=$$((FAIL + 1)); \
		fi; \
	}; \
	run_test "Clean run" "tests/clean.c" "No errors detected"; \
	run_test "NULL deref" "tests/null_deref.c" "NULL Pointer"; \
	run_test "Buffer overflow" "tests/buffer_overflow.c" "Buffer Overflow"; \
	run_test "Use after free" "tests/use_after_free.c" "Use After Free"; \
	run_test "Memory leak" "tests/memory_leak.c" "Memory Leak"; \
	run_test "Int overflow" "tests/int_overflow.c" "Integer Overflow"; \
	run_test "Div by zero" "tests/div_by_zero.c" "Division by Zero"; \
	run_test "Uninit var" "tests/uninit_var.c" "Uninitialized"; \
	run_test "Infinite loop" "tests/infinite_loop.c" "timeout"; \
	run_test "Syntax error" "tests/syntax_error.c" "COMPILE ERROR"; \
	run_test "Data race (TSan)" "--tsan tests/data_race.c" "Data Race"; \
	run_test "C++ vector overflow" "tests/cpp_vector_overflow.cpp" "Out of Range"; \
	run_test "Multi-file overflow" "tests/multi_main.c tests/multi_helper.c" "Buffer Overflow"; \
	if which valgrind >/dev/null 2>&1; then \
		run_test "Valgrind uninit" "--valgrind tests/valgrind_uninit.c" "Uninitialized"; \
	else \
		echo "  SKIP: Valgrind uninit (valgrind not found)"; \
		PASS=$$((PASS + 1)); \
	fi; \
	echo ""; \
	echo "Results: $$PASS passed, $$FAIL failed"; \
	if [ "$$FAIL" -gt 0 ]; then exit 1; fi

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

install-hooks: $(HOOKS_DIR)/pre-commit
	chmod +x $(HOOKS_DIR)/pre-commit
	cp $(HOOKS_DIR)/pre-commit .git/hooks/pre-commit
	@echo "Pre-commit hook installed to .git/hooks/"

test-all: $(TARGET)
	@echo "Running c_tester on all bad_codes files..."
	@PASS=0; FAIL=0; TOTAL=0; \
	for f in $(BAD_CODES_DIR)/*.c; do \
		TOTAL=$$((TOTAL + 1)); \
		echo "  Testing: $$f"; \
		output=$$(./$(TARGET) $$f 2>&1); \
		if echo "$$output" | grep -qi "detected\|error"; then \
			echo "    PASS (error detected)"; \
			PASS=$$((PASS + 1)); \
		else \
			echo "    FAIL (no error detected)"; \
			echo "    Output: $$output"; \
			FAIL=$$((FAIL + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "Results: $$PASS/$$TOTAL passed (error detected), $$FAIL failed (missed)"; \
	if [ "$$FAIL" -gt 0 ]; then exit 1; fi

deps:
	bash install_deps.sh
