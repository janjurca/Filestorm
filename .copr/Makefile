srpm:
    dnf -y install cmake gcc-c++ make cxxopts-devel spdlog-devel git
	mkdir  build
	cd build && cmake .. && make
	cpack -G SRPM
	rpmfiles=`ls *.rpm`
	for rpmfile in $$rpmfiles; do \
		echo $$rpmfile; \
		cp $$rpmfile $(outdir); \
	done
