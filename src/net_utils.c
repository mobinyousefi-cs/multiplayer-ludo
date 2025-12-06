/*
===========================================================
 Project:    Multiplayer Ludo Game
 File:       net_utils.c
 Author:     Mobin Yousefi (GitHub: github.com/mobinyousefi-cs)
 Created:    2025-12-06
 Updated:    2025-12-06
 License:    MIT License (see LICENSE file for details)
===========================================================

 Description:
    Implementation of small networking helpers for sending
    and receiving line-oriented messages over TCP sockets.

 Usage:
    - Used by both server.c and client.c to interact with
      the simple text protocol of the Ludo game.

 Notes:
    - This module uses blocking I/O and is intended only for
      small demo/teaching projects.
===========================================================
*/

#include "net_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

int send_all(int fd, const char *buf, size_t len)
{
    size_t total = 0;
    while (total < len) {
        ssize_t n = send(fd, buf + total, len - total, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return -1;
        }
        total += (size_t)n;
    }
    return 0;
}

int send_line(int fd, const char *line)
{
    size_t len = strlen(line);
    if (send_all(fd, line, len) < 0) {
        return -1;
    }
    if (send_all(fd, "\n", 1) < 0) {
        return -1;
    }
    return 0;
}

int recv_line(int fd, char *buf, size_t maxlen)
{
    if (maxlen == 0) {
        return -1;
    }

    size_t pos = 0;
    while (pos < maxlen - 1) {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            /* connection closed */
            if (pos == 0) {
                return 0;
            }
            break;
        }

        if (c == '\n') {
            break;
        }

        buf[pos++] = c;
    }
    buf[pos] = '\0';
    return (int)pos;
}

void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}
