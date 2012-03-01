#
# MPIPerf makefile.
#

include Makefile.inc

builddir=$(topdir)/build
mpiprof_dir=$(topdir)/src
gpart_dir=$(topdir)/src/gpart

all: mpiprof

mpiprof: extlibdir builddir gpart
	echo "[*] Building mpiprof $(mpiprof_dir)"
	$(MAKE) -C $(mpiprof_dir) BUILDDIR=$(builddir)
	./tools/generate-compiler-wrapper $(COMPILPATH)$(MPICC) $(builddir)/lib > $(builddir)/bin/mpiprofcc
	chmod +x $(builddir)/bin/mpiprofcc
	./tools/generate-compiler-wrapper $(COMPILPATH)$(MPICXX) $(builddir)/lib > $(builddir)/bin/mpiprofxx
	chmod +x $(builddir)/bin/mpiprofxx
	./tools/generate-compiler-wrapper $(COMPILPATH)$(MPIF77) $(builddir)/lib > $(builddir)/bin/mpiproff77
	chmod +x $(builddir)/bin/mpiproff77
	./tools/generate-compiler-wrapper $(COMPILPATH)$(MPIF90) $(builddir)/lib > $(builddir)/bin/mpiproff90
	chmod +x $(builddir)/bin/mpiproff90

gpart:
	echo "[*] Building gpart $(gpart_dir)"
	$(MAKE) -C $(gpart_dir) LIBDIR=$(topdir)/extlib

builddir:
	@mkdir -p $(builddir)
	@mkdir -p $(builddir)/bin
	@mkdir -p $(builddir)/lib

extlibdir:
	@mkdir -p $(topdir)/extlib

clean:
	@echo "[*] Cleaning $(builddir)"
	@rm -rf $(builddir)
	@echo "[*] Cleaning $(mpiprof_dir)"
	@rm -f $(mpiprof_dir)/*.o
	@rm -f $(mpiprof_dir)/*.a
	@echo "[*] Cleaning $(gpart_dir)"
	@rm -f $(gpart_dir)/*.o
	@rm -f $(gpart_dir)/*.a
	@echo "[*] Cleaning $(topdir)/extlib"
	@rm -f $(topdir)/extlib/*.a