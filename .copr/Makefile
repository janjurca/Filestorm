srpm:
	dnf -y install cmake gcc-c++ make cxxopts-devel spdlog-devel git
	mkdir -p build
	make
	cp build/.version .version
	make clean
	mkdir -p build
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	cpack -G RPM --config build/CPackSourceConfig.cmake
	echo `ls`
	for rpmfile in `ls filestorm-*.src.rpm`; do \
		echo $$rpmfile; \
		cp $$rpmfile $(outdir); \
	done
