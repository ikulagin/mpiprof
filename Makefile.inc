#
# MPIPerf top-level makefile.
#

topdir := $(PWD)

AR := ar
CC := gcc
LD := gcc

#
# Base MPI library for MPIPerf.
#
COMPILPATH=~/mpich2-install/bin/
MPICC := mpicc
MPICXX := mpicxx
MPIF77:= mpif77
MPIF90 := mpif90

CFLAGS := -Wall -std=c99 -O2 -g -DDEBUG
LDFLAGS := -lm
