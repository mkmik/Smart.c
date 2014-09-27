NS = ../../net_skeleton
HEADERS = $(NS)/net_skeleton.h sha1.h util.h http.h
SOURCES = $(NS)/net_skeleton.c http.c sha1.c util.c

all: ../smart.c ../smart.h

../smart.h: Makefile $(HEADERS)
	cat $(HEADERS) > $@

../smart.c: Makefile $(SOURCES)
	(echo '#include "smart.h"'; (cat $(SOURCES) | sed '/^#include "/d')) > $@
