/***********************************************************************
 * scas, Copyright (c) 2012-2013, Maximilian Burke
 * This file is distributed under the FreeBSD license. 
 * See LICENSE for details.
 ***********************************************************************/

#ifndef SCAS_CONNECTION_H
#define SCAS_CONNECTION_H

void
scas_connection_initialize(void);

int
scas_handle_connection(int fd);

#endif
