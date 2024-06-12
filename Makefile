
# Makefile

# Set up the default target - all, and specify what it should do
all: build
	
cmake:
	cmake -S . -B build

build: cmake
	cmake --build build

run:
	./build/filestorm

tests:
	cmake -Stest -Bbuild -DENABLE_TEST_COVERAGE=1 -DCMAKE_BUILD_TYPE=Debug
	cmake --build build -j4
	cd build; ctest --build-config Debug

rpm:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	cd build; cpack -G RPM

srpm:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	cd build; cpack -G RPM --config CPackSourceConfig.cmake

clean:
	rm -rf build

build-with-cpm:
	cmake -S . -B build -DUSE_CPM=1
	cmake --build build

.PHONY: all clean tests rpm run cmake build srpm
