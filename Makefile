
# Makefile

# Set up the default target - all, and specify what it should do
all:
	cmake -S . -B build
	cmake --build build


run:
	./build/standalone/Filestorm

clean:
	rm -rf build

.PHONY: all clean
