.PHONY: compile link run clean

all: run
	@echo "program ran"

compile:
	@echo "Compiling main.cpp..."
	g++-11 -std=c++20 src/main.cpp -o src/main.o

link: src/main.o src/example.gaz
	@echo "Running compiler on example.gaz..."
	./src/main.o src/example.gaz

run: compile link
	@echo "Running assembled binary..."
	@clang output.s -o output
	@./output; echo "Exit status: $$?"
	@$(MAKE) clean

comp_link: compile link

run_asm:
	@echo "Running assembled binary..."
	@clang output.s -o output
	@./output; echo "Exit status: $$?"
	@$(MAKE) clean

clean:
	@rm output
