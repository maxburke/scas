CC = gcc
CFLAGS = -g -std=c99 -Wall -Wextra -Werror -pedantic -I/usr/local/include -L/usr/local/lib -D_FILE_OFFSET_BITS=64 
LIBS = -lfuse4x
OBJS = scas_base.o
HEADERS = scas_base.h

%.o : %.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

libscas_common.a : $(OBJS)
	if test -f $@; then rm $@; fi
	ar -r $@ $(OBJS)

.PHONY : clean
clean :
	rm *.o
	rm libscas_common.a

.PHONY : all
all: libscas_common.a

