include ../Makefile.inc
lib=$(topdir)/src/libmpiprof.a
obj_profgen = wrappers_profgen.o reqlist.o communication.o profgenmode.o log.o
obj_profuse = mapping.o profusemode.o wrappers_profuse.o subsystem.o algo.o log.o

$(lib): ${obj_profgen} ${obj_profuse}
	ar cr $(lib) ${obj_profgen}
	cp -f $(lib) $(BUILDDIR)/lib

%.o:
	${COMPILPATH}${MPICC} -g -Wall -std=c99  -c $< 

wrappers_profgen.o:     wrappers_profgen.c
reqlist.o:              reqlist.c
communication.o:        communication.c
nodes.o:                nodes.c
profgenmode.o:          profgenmode.c
profusemode.o:		profusemode.c
wrappers_profuse.o:	wrappers_profuse.c
mapping.o:		mapping.c
subsystem.o:            subsystem.c
algo.o:                 algo.c
log.o:					log.c

clean:
	rm -f *.o
	rm -f *.a