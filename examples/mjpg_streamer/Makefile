FLAGS = ../../smart.c -I../..
PROGS = device_side cloud_side

all: $(PROGS)

device_side: Makefile device_side.c ../../smart.c
	$(CC) device_side.c $(FLAGS) -o $@ -W -Wall -pthread $(CFLAGS_EXTRA)

cloud_side: Makefile cloud_side.c ../../smart.c
	$(CC) cloud_side.c $(FLAGS) -o $@ -W -Wall -pthread $(CFLAGS_EXTRA)

device_side.exe: Makefile device_side.c $(NS)/net_skeleton.c
	cl device_side.c $(FLAGS) /MD /Fe$@

clean:
	rm -rf *.gc* *.dSYM *.exe *.obj *.o a.out $(PROGS)
