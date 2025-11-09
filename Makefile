CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
TARGET = compiler
SRC = main.c $(wildcard src/*.c)

# Default target: show help
.DEFAULT_GOAL := help

# Show available commands
help:
	@echo "=== Compiler Makefile Commands ==="
	@echo ""
	@echo "Build:"
	@echo "  make build          - Compile the compiler"
	@echo ""
	@echo "Run:"
	@echo "  make run            - Run compiler with first test file"
	@echo ""
	@echo "Testing:"
	@echo "  make list-tests     - List all available test files with numbers"
	@echo "  make test FILE=N    - Test specific file (e.g., make test FILE=2)"
	@echo "  make test-all       - Run all tests and compare outputs"
	@echo ""
	@echo "Memory Check:"
	@echo "  make valgrind FILE=N    - Check memory leaks for specific file"
	@echo "  make valgrind-all       - Check memory leaks for all files"
	@echo ""
	@echo "Cleanup:"
	@echo "  make clean          - Remove compiled files and outputs"
	@echo ""
	@echo "Examples:"
	@echo "  make build && make test FILE=2"
	@echo "  make valgrind FILE=1"
	@echo ""

# files handling
build: $(TARGET)
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET) $(shell ls test/test_files/src/*.wren | head -n 1)

# tests 
# List available test files with numbers
list-tests:
	@echo "Available test files:"
	@i=1; for file in test/test_files/src/*.wren; do \
		echo "  $$i: $$(basename $$file)"; \
		i=$$((i + 1)); \
	done

# Test a specific file by number: make test FILE=1
test: $(TARGET)
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make test FILE=<number>"; \
		echo "Use 'make list-tests' to see available files"; \
		exit 1; \
	fi
	@mkdir -p test/test_files/output
	@file=$$(ls test/test_files/src/*.wren | sed -n '$(FILE)p'); \
	if [ -z "$$file" ]; then \
		echo "Error: File number $(FILE) not found"; \
		exit 1; \
	fi; \
	base=$$(basename $$file .wren); \
	echo "Testing $$base (file $(FILE))..."; \
	./$(TARGET) $$file > test/test_files/output/$$base.myout 2>&1 || true; \
	test/test_files/compilers/wren_cli-x86_64-linux $$file > test/test_files/output/$$base.expected 2>&1 || true; \
	if diff -u test/test_files/output/$$base.expected test/test_files/output/$$base.myout > test/test_files/output/$$base.diff; then \
		echo "✓ $$base PASSED"; \
	else \
		echo "✗ $$base FAILED"; \
		echo "Diff:"; \
		cat test/test_files/output/$$base.diff; \
	fi

# Run all tests and compare
test-all: $(TARGET)
	@mkdir -p test/test_files/output
	@echo "Running tests..."
	@for file in test/test_files/src/*.wren; do \
		base=$$(basename $$file .wren); \
		echo "Testing $$base..."; \
		./$(TARGET) $$file > test/test_files/output/$$base.myout 2>&1 || true; \
		test/test_files/compilers/wren_cli-x86_64-linux $$file > test/test_files/output/$$base.expected 2>&1 || true; \
		if diff -u test/test_files/output/$$base.expected test/test_files/output/$$base.myout > test/test_files/output/$$base.diff; then \
			echo "  ✓ $$base PASSED"; \
		else \
			echo "  ✗ $$base FAILED (see output/$$base.diff)"; \
		fi; \
	done

# test-ifjcode:
# 	test/test_files/compilers/ic25int-linux-x86_64 materials/IFJcode25_examples/example_demo.ifjcode

# Memory leak testing
VALGRIND = valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose

# Run valgrind on a specific test file: make valgrind FILE=2
valgrind: $(TARGET)
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make valgrind FILE=<number>"; \
		echo "Use 'make list-tests' to see available files"; \
		exit 1; \
	fi
	@file=$$(ls test/test_files/src/*.wren | sed -n '$(FILE)p'); \
	if [ -z "$$file" ]; then \
		echo "Error: File number $(FILE) not found"; \
		exit 1; \
	fi; \
	base=$$(basename $$file .wren); \
	echo "Running valgrind on $$base..."; \
	$(VALGRIND) ./$(TARGET) $$file

# Run valgrind on all test files
valgrind-all: $(TARGET)
	@echo "Running valgrind on all tests..."
	@for file in test/test_files/src/*.wren; do \
		base=$$(basename $$file .wren); \
		echo "=== Valgrind: $$base ==="; \
		$(VALGRIND) ./$(TARGET) $$file 2>&1 | grep -E "(LEAK SUMMARY|ERROR SUMMARY|definitely lost|indirectly lost|possibly lost)"; \
		echo ""; \
	done

# clenup
clean:
	rm -f $(TARGET)
	rm -f test/test_files/output/*

.PHONY: help build all run list-tests test test-all valgrind valgrind-all clean

