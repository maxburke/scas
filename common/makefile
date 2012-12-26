CC = gcc
DEFINES = _POSIX_C_SOURCE=200112L _XOPEN_SOURCE _FILE_OFFSET_BITS=64
INCLUDEDIRS = /usr/local/include

CFLAGS = -g -std=c99 -Wall -Wextra -Werror -pedantic
OBJS = $(patsubst %.c,%.o,$(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o : %.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(addprefix -I, $(INCLUDEDIRS)) $(addprefix -D, $(DEFINES))

libscas_common.a : $(OBJS)
	if test -f $@; then rm $@; fi
	ar -r $@ $(OBJS)

.PHONY : clean
clean :
	rm *.o
	rm libscas_common.a

.PHONY : all
all: libscas_common.a

