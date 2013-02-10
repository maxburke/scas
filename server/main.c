#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include "scas_base.h"
#include "scas_connection.h"
#include "scas_net.h"

static int done;

#define MAX_NUM_EVENTS 128

static void
scas_add_to_epoll_list(int epoll_fd, int fd)
{
    struct epoll_event event;

    memset(&event, 0, sizeof event);
    fcntl(fd, F_SETFD, O_NONBLOCK);
    event.events = EPOLLIN | EPOLLOUT;
    event.data.fd = fd;
    VERIFY(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) != -1);
}

static void
scas_remove_from_epoll_list(int epoll_fd, int fd)
{
    VERIFY(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) != -1);
}

int
main(void)
{
    int socket_fd;
    int epoll_fd;
    struct epoll_event event;
    struct epoll_event events[MAX_NUM_EVENTS];

    scas_connection_initialize();

    socket_fd = scas_listen();
    epoll_fd = epoll_create1(0);
    VERIFY(epoll_fd >= 0);

    event.events = EPOLLIN;
    event.data.fd = socket_fd;
    VERIFY(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == 0);

    while (!done)
    {
        int num_ready_fds;
        int i;

        num_ready_fds = epoll_wait(epoll_fd, events, MAX_NUM_EVENTS, -1);
        VERIFY(num_ready_fds != -1);

        for (i = 0; i < num_ready_fds; ++i)
        {
            int fd;

            fd = events[i].data.fd;

            if (fd == socket_fd)
            {
                int new_socket;

                new_socket = scas_accept(fd);
                scas_add_to_epoll_list(epoll_fd, new_socket);
            }
            else if (scas_handle_connection(fd))
            {
                scas_remove_from_epoll_list(epoll_fd, fd);
                close(fd);
            }
        }
    }

    close(socket_fd);
    close(epoll_fd);

    return 0;
}

