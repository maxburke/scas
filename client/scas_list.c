/***********************************************************************
 * scas, Copyright (c) 2012-2013, Maximilian Burke
 * This file is distributed under the FreeBSD license. 
 * See LICENSE for details.
 ***********************************************************************/

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "scas_base.h"
#include "scas_list.h"
#include "scas_net.h"

struct scas_snapshot_t
{
    uint64_t creation_time;
    uint32_t name_length;
    char name[1];
};

#define next_snapshot(x) (x = ((struct scas_snapshot_t *)(((char *)x) + sizeof(struct scas_snapshot_t) + (x->name_length - 1))))

int
scas_list_snapshots(const char *snapshot_server)
{
    int sockfd;
    struct scas_snapshot_t *snapshots;

    sockfd = scas_connect(snapshot_server);
    
    if (sockfd < 0)
    {
        return -ENOCONN;
    }

    scas_write(sockfd, CMD_SNAPSHOT_LIST, NULL, 0);

    snapshots = scas_read(sockfd);

    if (!snapshots)
    {
        printf("No snapshots on server!\n");
        return 0;
    }

    printf("Creation date               Snapshot Name\n");
    printf("--------------------------------------------------------------------------\n");

    for (; snapshots->name_length > 0; next_snapshot(snapshots))
    {
        char time_buf[64];
        uint32_t i;
        time_t creation_time;

        creation_time = (time_t)snapshots->creation_time;

        ctime_r(&creation_time, time_buf);
        time_buf[63] = 0;

        printf("%s ", time_buf);

        for (i = 0; i < snapshots->name_length; ++i)
        {
            putchar(snapshots->name[i]);
        }

        printf("\n");
    }

    free(snapshots);
    
    return 0;
}

