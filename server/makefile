CC = gcc
CFLAGS = -g -std=c99 -Wall -Wextra -Werror -pedantic -g 
OBJS = $(patsubst %.c,%.o,$(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o : %.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

scas_server : $(OBJS)
	gcc -o $@ $(LIBS) $^ $(CFLAGS)

.PHONY : clean
clean :
	rm *.o
	rm scas_server

.PHONY : all
all : scas_server

