.PHONY: compile link run clean

all: run
	@echo "program ran"

compile:
	@echo "Compiling main.cpp..."
	g++-11 -std=c++20 src/main.cpp -o src/main.o

link: src/main.o src/test.hy
	@echo "Running compiler on test.hy..."
	./src/main.o src/test.hy

run: compile link
	@echo "Running assembled binary..."
	@bash -c './test; echo "Exit status: $$?"'
	@$(MAKE) clean

clean:
	rm test test.o src/main.o
