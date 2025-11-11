.PHONY: compile link run clean

all: run
	@echo "program ran"

compile:
	@echo "Compiling main.cpp..."
	g++-11 -std=c++20 src/main.cpp -o src/main.o

link: src/main.o src/test.gaz
	@echo "Running compiler on test.hy..."
	./src/main.o src/test.gaz

run: compile link
	@echo "Running assembled binary..."
	@bash -c './test; echo "Exit status: $$?"'
	@$(MAKE) clean

comp_link: compile link

clean:
	rm test test.o src/main.o
