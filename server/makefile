CC = gcc
CFLAGS = -g -std=c99 -Wall -Wextra -Werror -pedantic -g 
OBJS = $(patsubst %.c,%.o,$(wildcard *.c))
HEADERS = $(wildcard *.h)
INCLUDEDIRS = /usr/local/include ../common

%.o : %.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(addprefix -I, $(INCLUDEDIRS)) $(addprefix -D, $(DEFINES))

scas_server : $(OBJS) ../common/libscas_common.a
	gcc -o $@ $(LIBS) $^ $(CFLAGS)

.PHONY : clean
clean :
	rm *.o
	rm scas_server

.PHONY : all
all : scas_server

