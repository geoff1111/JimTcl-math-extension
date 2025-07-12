JIMDIR    = ../jimtcl/

CPPFLAGS += -I${JIMDIR}
CFLAGS   += -g -Os -fPIC -shared -Wall -Wextra

all: apex.so

apex.so: apex.c
	cc ${CFLAGS} ${CPPFLAGS} -o $@ $^

.PHONY: clean 

clean:
	rm -f apex.so
