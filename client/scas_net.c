#include <netdb.h>
#include <stddef.h>

#include "scas_base.h"
#include "scas_net.h"

int
scas_connect(const char *server_name)
{
    struct addrinfo *addrinfo, *p;
    int sockfd;
    int success = 0;

    if (getaddrinfo(server_name, NULL, NULL, &addrinfo))
    {
        scas_log("Could not look up server %s", server_name);
        return -1;
    }

    for (p = addrinfo; p; p = p->ai_next)
    {
        if (p->ai_socktype != SOCK_STREAM)
            continue;

        sockfd = connect(p->ai_family, p->ai_addr, p->ai_protocol);

        if (sockfd < 0)
            continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0)
        {
            success = 1;
            break;
        }
    }

    freeaddrinfo(addrinfo);

    if (!success)
    {
        scas_log("Unable to open connection to server %s.", server_name);
        return -1;
    }

    return sockfd;
}
