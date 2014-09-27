FLAGS = ../../smart.c -I../..

all: device_side cloud_side
	./device_side /tmp/mjpg_stream 100 echo.websocket.org:80

device_side: Makefile device_side.c $(COMMON)
	$(CC) device_side.c $(FLAGS) -o $@ -W -Wall -pthread $(CFLAGS_EXTRA)

cloud_side:
	true

device_side.exe: Makefile device_side.c $(NS)/net_skeleton.c
	cl device_side.c $(FLAGS) /MD /Fe$@

clean:
	rm -rf *.gc* *.dSYM *.exe *.obj *.o a.out $(PROG)
