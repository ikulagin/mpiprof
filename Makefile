COMPILPATH=~/mpich2-install/bin/
CC=mpicc
obj_profgen = wrappers_profgen.o reqlist.o communication.o profgenmode.o

obj_profuse = mapping.o profusemode.o wrappers_profuse.o subsystem.o algo.o

Wrappers_profgen_.a: ${obj_profgen} ${obj_profuse}
	ar cr libWrappers_profgen_.a ${obj_profgen}
	ar cr libWrappers_profuse_.a ${obj_profuse}

%.o:
	${COMPILPATH}${CC} -g -Wall -std=c99  -c $< 

gpart/libgpart.a:
	make -C gpart

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

clean:
	rm -f *.o
	rm -f *.a