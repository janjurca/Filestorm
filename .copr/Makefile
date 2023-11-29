srpm:
	dnf -y install cmake gcc-c++ make cxxopts-devel spdlog-devel git
	mkdir -p build
	cd build && cmake ..
	ls -l build
	ls -l
	pwd
	cp .version build/.version
	cpack -G RPM --config build/CPackSourceConfig.cmake
	echo `ls`
	for rpmfile in `ls filestorm-*.src.rpm`; do \
		echo $$rpmfile; \
		cp $$rpmfile $(outdir); \
	done
