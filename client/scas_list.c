#include "scas_base.h"
#include "scas_list.h"
#include "scas_net.h"

int
scas_list_snapshots(const char *snapshot_server)
{
    int sockfd;

    UNUSED(snapshot_server);

    sockfd = scas_connect(snapshot_server);
    
    if (sockfd < 0)
    {
        return -ENOCONN;
    }
    
    return 0;
}

