CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
TARGET = compiler
SRC = main.c $(wildcard src/*.c)

# files handling
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

# clenup
clean:
	rm -f $(TARGET)
	rm -f test/test_files/output/*

.PHONY: all run list-tests test test-wren test-all clean

