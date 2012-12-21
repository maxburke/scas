CC = gcc
CFLAGS = -g -std=c99 -Wall -Wextra -Werror -pedantic -g -I/usr/local/include -L/usr/local/lib -D_FILE_OFFSET_BITS=64 
LIBS = -lfuse4x
OBJS = main.o scas_mount.o scas_base.o scas_create.o scas_list.o scas_net.o
HEADERS = scas_base.h scas_mount.h

%.o : %.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

scas_client : $(OBJS)
	gcc -o $@ $(LIBS) $^ $(CFLAGS)

clean :
	rm *.o
	rm scas_client
