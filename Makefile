#adapted from http://stackoverflow.com/questions/31421616/c-creating-static-library-and-linking-using-makefile
CFLAGS="-DMSPACES" "-DONLY_MSPACES" "-DUSE_LOCKS" "-DDEBUG" "-DFOOTERS"
all: master slave

master: master.o libcold.a
	gcc -g -O0 -lm -o master master.o -L. -lcold

slave: slave.o libcold.a
	gcc -g -O0 -lm -o slave slave.o -L. -lcold

master.o: master.c alloc.h
	gcc $(CFLAGS)  -g -O0 -c master.c

slave.o: slave.c alloc.h
	gcc $(CFLAGS)  -g -O0 -c slave.c

alloc.o: alloc.c alloc.h
	gcc $(CFLAGS) -O0 -g -c  alloc.c

getb.o: getb.c getb.h alloc.h
	gcc $(CFLAGS)  -O0 -g -c  getb.c

bus.o: bus.c cool_bus.h
	gcc -O0 -g -c bus.c

mut.o: mut.c om_mutex.h
		gcc $(CFLAGS) -O0 -g -c mut.c

m.o: m.c m.h
	gcc -O0 -g -c m.c
libcold.a: alloc.o getb.o bus.o m.o mut.o
	ar rcs libcold.a alloc.o getb.o bus.o m.o mut.o

libs: libcold.a

clean:
	rm -f stub *.o *.a *.gch
