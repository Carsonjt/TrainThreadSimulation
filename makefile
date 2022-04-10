.phony all:
all: mts

mts: trains.c
	gcc -pthread trains.c -o mts -g

.PHONY clean:
clean:
	-rm -rf *.o *.exe