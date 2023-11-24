srpm:
	dnf -y install cmake gcc-c++ make cxxopts-devel spdlog-devel git
	mkdir  build
	cd build && cmake .. && make && cpack -G RPM 
	rpmfiles=`ls filestorm*.rpm`
	echo $$rpmfiles
	if [ -z "$$rpmfiles" ]; then \
		echo "No RPM files found"; \
		exit 1; \
	fi
	for rpmfile in $$rpmfiles; do \
		echo $$rpmfile; \
		cp $$rpmfile $(outdir); \
	done
