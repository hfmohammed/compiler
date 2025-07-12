.PHONY: compile link run clean

compile:
	@echo "Compiling main.cpp..."
	g++ -std=c++17 src/main.cpp -o main

link: main src/test.hy
	@echo "Running compiler on test.hy..."
	./main src/test.hy

run: compile link
	@echo "Running assembled binary..."
	@bash -c './test; echo "Exit status: $$?"'
	@$(MAKE) clean

clean:
	rm test test.o main
