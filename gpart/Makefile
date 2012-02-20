#
# gpart library makefile.
#

AR := ar
CC := gcc
LD := gcc

CFLAGS := -g -Wall -std=c99 -O2
LDFLAGS := -lm

lib := libgpart.a
lib_objs := gpart.o csrgraph.o coarsen.o refine_fm.o

example_generate := example_generate
example_generate_objs := example_generate.o

example_part := example_part
example_part_objs := example_part.o

.PHONY: all clean

all: $(lib) $(example_generate) $(example_part)

$(lib): $(lib_objs)
	$(AR) rcs $@ $^

$(example_generate): $(example_generate_objs)
	$(LD) $(LDFLAGS) $< -o $@ -L. -lgpart

$(example_part): $(example_part_objs)
	$(LD) $(LDFLAGS) $< -o $@ -L. -lgpart

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

gpart.o: gpart.c gpart.h
coarsen.o: coarsen.c coarsen.h
refine_fm.o: refine_fm.c refine_fm.h
csrgraph.o: csrgraph.c gpart.h

example_generate.o: example_generate.c

clean:
	@rm -rf *.o $(lib) $(example_generate) $(example_part) 

