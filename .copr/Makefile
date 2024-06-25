
# Determine if we are in the specific subdirectory
ROOT_DIR := $(shell git rev-parse --show-toplevel)
CURRENT_DIR := $(shell pwd)
SUBDIR := specific-makefiles/build-in-libs

# Path to the build directory should always be in the root
BUILD_DIR := $(ROOT_DIR)/build
CMAKELISTS_FILE := $(ROOT_DIR)/CMakeLists.txt

# Command to use for cmake based on current directory
CMAKE_COMMAND := cmake -S $(ROOT_DIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
ifneq ($(findstring $(SUBDIR), $(CURRENT_DIR)),)
    CMAKE_COMMAND := $(CMAKE_COMMAND) -DUSE_CPM=1
endif

srpm:
	dnf -y install cmake gcc-c++ make spdlog-devel fmt-devel cxxopts-devel json-devel git
	mkdir -p $(BUILD_DIR)
	cd $(ROOT_DIR); make
	@VERSION_STRING=$$(cat $(BUILD_DIR)/.version); \
	sed -i 's/\$${PROJECT_VERSION}/'$$VERSION_STRING'/g' $(CMAKELISTS_FILE); \
	sed -i '/include(cmake\/DynamicVersion\.cmake)/d' $(CMAKELISTS_FILE); \
	sed -i '/dynamic_version(PROJECT_PREFIX filestorm_)/d' $(CMAKELISTS_FILE); \
	sed -i 's/cmake_minimum_required(VERSION 3.25)/cmake_minimum_required(VERSION 3.20)/g' $(CMAKELISTS_FILE)
	cd $(ROOT_DIR); make clean
	mkdir -p $(BUILD_DIR)
	$(CMAKE_COMMAND)
	cpack -G RPM --config $(BUILD_DIR)/CPackSourceConfig.cmake
	echo `ls $(BUILD_DIR)`
	for rpmfile in `ls $(CURRENT_DIR)/filestorm-*.src.rpm`; do \
		echo $$rpmfile; \
		cp $$rpmfile $(outdir); \
	done
