#
# Makefile for complement_critical_Pfree_graphs
#

SHELL = /bin/sh

# Compiling executing program with DWORDSIZE=32 is slightly faster, 
# but limits the order of the graphs to 32.
CC32 = gcc -DWORDSIZE=32 -DMAXN=WORDSIZE
CC64 = gcc -DWORDSIZE=64 -DMAXN=WORDSIZE
CFLAGS = -O4

all : clean 32bit

32bit :
	${CC32} $(CFLAGS) complement_critical_Pfree_graphs.c planarity.c nautyW1.a -o complement_critical_Pfree_graphs

64bit :
	${CC64} $(CFLAGS) complement_critical_Pfree_graphs.c planarity.c nautyL1.a -o complement_critical_Pfree_graphs-64

profile :
	${CC32} -pg -g complement_critical_Pfree_graphs.c planarity.c nautyW1.a -o complement_critical_Pfree_graphs-profile

clean :
	rm -f complement_critical_Pfree_graphs
