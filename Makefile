# Build using meson/ninja
MESON_BUILD_DIR = build

all: $(MESON_BUILD_DIR)/build.ninja
	ninja -C $(MESON_BUILD_DIR)

$(MESON_BUILD_DIR)/build.ninja:
	meson $(MESON_BUILD_DIR)

clean:
	rm -rf $(MESON_BUILD_DIR) test_output_*

install: $(MESON_BUILD_DIR)/build.ninja
	ninja -C $(MESON_BUILD_DIR) install

test: $(MESON_BUILD_DIR)/build.ninja
	ninja -C $(MESON_BUILD_DIR)
	@echo "Running tests with module files..."
	./$(MESON_BUILD_DIR)/test/test_main ./test/cndmcrrp.mod
	./$(MESON_BUILD_DIR)/test/test_main ./test/nova.s3m
	./$(MESON_BUILD_DIR)/test/test_main ./test/zalza-karate_muffins.xm
	@echo "Tests completed!"

format:
	@echo "Formatting source code with clang-format..."
	clang-format -i untracker.cpp
	@echo "Code formatting completed."

lint:
	@echo "Running cppcheck for static analysis..."
	cppcheck --enable=style,performance,portability,warning --std=c++17 --verbose untracker.cpp
	@echo "Linting completed."

rebuild: clean all

.PHONY: all clean install test rebuild format lint