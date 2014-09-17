PROG = unit_test
PROF = -fprofile-arcs -ftest-coverage -g -O0
NS = ../../net_skeleton
SFLAGS = -I.. -I$(NS) -DNS_ENABLE_SSL $(CFLAGS_EXTRA)
CFLAGS = -W -Wall -pthread $(PROF) $(SFLAGS)

all: clean $(PROG)

$(PROG):
	g++ $(PROG).c $(NS)/net_skeleton.c -o $(PROG) $(CFLAGS) -lssl && ./$(PROG)
	gcov -b $(PROG).c

$(PROG).exe:
	wine cl $(PROG).c /MD $(SFLAGS) && wine $(PROG).exe

clean:
	rm -rf *.gc* *.dSYM $(PROG) *.txt *.exe *.obj *.o a.out
