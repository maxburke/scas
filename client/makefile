CC = gcc
DEFINES = _POSIX_C_SOURCE=200112L _XOPEN_SOURCE _FILE_OFFSET_BITS=64
INCLUDEDIRS = /usr/local/include ../common

ifeq ($(OS), Linux)
    LIBS = fuse scas_common
    LIBDIRS = ../common
else
    LIBS = fuse4x scas_common
    LIBDIRS = /usr/local/lib ../common
endif

CFLAGS = -g -std=c99 -Wall -Wextra -Werror -pedantic
LDFLAGS = -g -L/usr/local/lib -L../common

OBJS = main.o scas_mount.o scas_create.o scas_list.o
HEADERS = scas_mount.h

%.o : %.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(addprefix -I, $(INCLUDEDIRS)) $(addprefix -D, $(DEFINES))

scas_client : $(OBJS)
	gcc -o $@ $^ $(LDFLAGS) $(addprefix -L, $(LIBDIRS)) $(addprefix -l, $(LIBS))

.PHONY: clean
clean :
	rm *.o
	rm scas_client

.PHONY: all
all : scas_client

