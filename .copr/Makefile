CMAKELISTS_FILE := CMakeLists.txt

srpm:
	dnf -y install cmake gcc-c++ make cxxopts-devel spdlog-devel git
	mkdir -p build
	make
	@VERSION_STRING=$$(cat build/.version); \
	sed -i 's/\$${PROJECT_VERSION}/'$$VERSION_STRING'/g' $(CMAKELISTS_FILE); \
	sed -i '/include(cmake\/DynamicVersion\.cmake)/d' $(CMAKELISTS_FILE); \
	sed -i '/dynamic_version(PROJECT_PREFIX filestorm_)/d' $(CMAKELISTS_FILE)
	make clean
	mkdir -p build
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	cpack -G RPM --config build/CPackSourceConfig.cmake
	echo `ls`
	for rpmfile in `ls filestorm-*.src.rpm`; do \
		echo $$rpmfile; \
		cp $$rpmfile $(outdir); \
	done
