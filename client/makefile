CC = gcc
CFLAGS = -g -std=c99 -Wall -Wextra -Werror -pedantic -I/usr/local/include -L/usr/local/lib -D_FILE_OFFSET_BITS=64  -I../common
LDFLAGS = -g -L/usr/local/lib -L../common
LIBS = -lfuse4x -lscas_common
OBJS = main.o scas_mount.o scas_create.o scas_list.o scas_net.o
HEADERS = scas_mount.h

%.o : %.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

scas_client : $(OBJS)
	gcc -o $@ $(LIBS) $^ $(LDFLAGS)

.PHONY: clean
clean :
	rm *.o
	rm scas_client

.PHONY: all
all : scas_client

