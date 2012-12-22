#ifndef SCAS_NET_H
#define SCAS_NET_H

/*
 * Connects to the specified server. On success, returns a socket that can be 
 * read from/written to. On failure, returns < 0.
 */
int
scas_connect(const char *server_name);

#endif
