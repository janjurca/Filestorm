CMAKELISTS_FILE := CMakeLists.txt

srpm:
	dnf -y install cmake gcc-c++ make spdlog-devel fmt-devel cxxopts-devel json-devel git
	mkdir -p build
	make
	@VERSION_STRING=$$(cat build/.version); \
	sed -i 's/\$${PROJECT_VERSION}/'$$VERSION_STRING'/g' $(CMAKELISTS_FILE); \
	sed -i '/include(cmake\/DynamicVersion\.cmake)/d' $(CMAKELISTS_FILE); \
	sed -i '/dynamic_version(PROJECT_PREFIX filestorm_)/d' $(CMAKELISTS_FILE); \
	sed -i 's/cmake_minimum_required(VERSION 3.25)/cmake_minimum_required(VERSION 3.20)/g' $(CMAKELISTS_FILE)
	make clean
	mkdir -p build
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	cpack -G RPM --config build/CPackSourceConfig.cmake
	echo `ls`
	for rpmfile in `ls filestorm-*.src.rpm`; do \
		echo $$rpmfile; \
		cp $$rpmfile $(outdir); \
	done
