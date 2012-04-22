include ../Makefile.inc
lib=$(topdir)/src/libmpiprof.a
obj_profgen = wrappers_profgen.o reqlist.o communication.o profgenmode.o log.o
obj_profuse = mapping.o profusemode.o subsystem.o algo.o log.o
obj = control_monitoring.o wrappers_f.o

$(lib): ${obj_profgen} ${obj_profuse} ${obj}
	ar cr $(lib) ${obj_profgen} ${obj_profuse} ${obj}
	cp -f $(lib) $(BUILDDIR)/lib

%.o:
	${COMPILPATH}${MPICC} -g -Wall -std=c99  -c $< 

wrappers_profgen.o:     wrappers_profgen.c
reqlist.o:              reqlist.c
communication.o:        communication.c
nodes.o:                nodes.c
profgenmode.o:          profgenmode.c
profusemode.o:		profusemode.c
mapping.o:		mapping.c
subsystem.o:            subsystem.c
algo.o:                 algo.c
log.o:					log.c
control_monitoring.o: control_monitoring.c
wrappers_f.o:			wrappers_f.c

clean:
	rm -f *.o
	rm -f *.a