srpm:
	dnf -y install cmake gcc-c++ make cxxopts-devel spdlog-devel git
	mkdir  build
	cd build && cmake .. && make
	cpack -G RPM --config build/CPackSourceConfig.cmake
	rpmfiles=`ls filestorm*.rpm`
	echo $$rpmfiles
	for rpmfile in $$rpmfiles; do \
		echo $$rpmfile; \
		cp $$rpmfile $(outdir); \
	done
