
# Makefile

# Set up the default target - all, and specify what it should do
all: build
	
srpm:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUSE_CPM=1
	cd build; cpack -G RPM --config CPackSourceConfig.cmake

.PHONY: all srpm