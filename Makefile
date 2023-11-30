
# Makefile

# Set up the default target - all, and specify what it should do
all:
	cmake -S . -B build
	cmake --build build


run:
	./build/filestorm

tests:
	cmake -Stest -Bbuild -DENABLE_TEST_COVERAGE=1 -DCMAKE_BUILD_TYPE=Debug
	cmake --build build -j4
	cd build; ctest --build-config Debug


clean:
	rm -rf build

.PHONY: all clean tests
