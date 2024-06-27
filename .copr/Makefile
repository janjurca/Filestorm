CMAKELISTS_FILE := CMakeLists.txt

srpm:
	@if ! command -v git &> /dev/null; then \
		dnf -y install git; \
	fi
	@ROOT_DIR=$$(git rev-parse --show-toplevel); \
	CURRENT_DIR=$$(pwd); \
	SUBDIR=specific-makefiles/build-in-libs; \
	BUILD_DIR=$$ROOT_DIR/build; \
	if echo $$CURRENT_DIR | grep -q $$SUBDIR; then \
		CMAKE_COMMAND="cmake -S $$ROOT_DIR -B $$BUILD_DIR -DCMAKE_BUILD_TYPE=Release -DUSE_CPM=1"; \
	else \
		CMAKE_COMMAND="cmake -S $$ROOT_DIR -B $$BUILD_DIR -DCMAKE_BUILD_TYPE=Release"; \
	fi; \
	echo "Root directory: $$ROOT_DIR"; \
	echo "Build directory: $$BUILD_DIR"; \
	echo "CMake command: $$CMAKE_COMMAND"; \
	dnf -y install cmake gcc-c++ make spdlog-devel fmt-devel cxxopts-devel json-devel; \
	mkdir -p $$BUILD_DIR; \
	make -C $$ROOT_DIR; \
	VERSION_STRING=$$(cat $$BUILD_DIR/.version); \
	sed -i 's/\$${PROJECT_VERSION}/'$$VERSION_STRING'/g' $$ROOT_DIR/$(CMAKELISTS_FILE); \
	sed -i '/include(cmake\/DynamicVersion\.cmake)/d' $$ROOT_DIR/$(CMAKELISTS_FILE); \
	sed -i '/dynamic_version(PROJECT_PREFIX filestorm_)/d' $$ROOT_DIR/$(CMAKELISTS_FILE); \
	sed -i 's/cmake_minimum_required(VERSION 3.25)/cmake_minimum_required(VERSION 3.20)/g' $$ROOT_DIR/$(CMAKELISTS_FILE); \
	make -C $$ROOT_DIR clean; \
	mkdir -p $$BUILD_DIR; \
	$$CMAKE_COMMAND; \
	cpack -G RPM --config $$BUILD_DIR/CPackSourceConfig.cmake; \
	echo `ls $$BUILD_DIR`; \
	for rpmfile in `ls $$CURRENT_DIR/filestorm-*.src.rpm`; do \
		echo $$rpmfile; \
		cp $$rpmfile $(outdir); \
	done

.PHONY: clean
clean:
	rm -rf build
