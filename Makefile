
# either "curl" or "socket". the former requires libcurl.
HTTP_BACKEND = curl

CC ?= gcc
CFLAGS ?= -O3 -Wall -Wextra -std=gnu89 -pedantic -Wno-long-long -Wformat-security
#CFLAGS += -Weverything -Wno-padded
LDLIBS = `sdl2-config --libs` -lz

# do not forget to link with libcurl if curl backend is to be used
ifeq ($(HTTP_BACKEND), curl)
LDLIBS += `curl-config --libs`
endif

all: simplesok

simplesok: simplesok.o crc32.o data.o gra.o gz.o net-$(HTTP_BACKEND).o save.o skin.o sok_core.o

clean:
	rm -f *.o simplesok file2c

data: file2c assets/img/*.bmp.gz skins/yoshi.bmp.gz assets/font/*.bmp.gz assets/levels/*.xsb.gz assets/icon.bmp.gz
	echo "/* This file is part of the simplesok project. */" > data.c
	echo '#include <stddef.h> /* size_t */' >> data.c
	echo '#include "data.h"' >> data.c
	for x in assets/img/*.bmp.gz ; do ./file2c $$x >> data.c ; done
	./file2c skins/yoshi.bmp.gz >> data.c
	for x in assets/levels/*.xsb.gz ; do ./file2c $$x >> data.c ; done
	for x in assets/font/*.bmp.gz ; do ./file2c $$x >> data.c ; done
	./file2c assets/icon.bmp.gz >> data.c
	grep -o '^unsigned char.*gz\[\]' data.c | sed 's/^/extern /g' | sed 's/$$/;/g' > data.h
	grep -o '^.*gz_len' data.c | sed 's/^/extern /g' | sed 's/$$/;/g' >> data.h

file2c: file2c.c
