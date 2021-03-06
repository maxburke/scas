/***********************************************************************
 * scas, Copyright (c) 2012-2013, Maximilian Burke
 * This file is distributed under the FreeBSD license. 
 * See LICENSE for details.
 ***********************************************************************/

#include "scas_base.h"
#include "scas_net.h"
#include "scas_mount.h"

int
scas_mount_snapshot(const char *server_name, const char *snapshot_name)
{
    int sockfd;

    UNUSED(snapshot_name);

    sockfd = scas_connect(server_name);

    if (sockfd < 0)
    {
        return -ENOCONN;
    }

    return 1;
}

