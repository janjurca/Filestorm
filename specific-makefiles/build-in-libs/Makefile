
# Makefile

# Set up the default target - all, and specify what it should do
all: 
	touch ../../USE_CPM
	

srpm:
	pwd
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUSE_CPM=1
	cd build; cpack -G RPM --config CPackSourceConfig.cmake

.PHONY: all srpm