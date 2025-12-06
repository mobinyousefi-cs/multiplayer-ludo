/*
===========================================================
 Project:    Multiplayer Ludo Game
 File:       net_utils.h
 Author:     Mobin Yousefi (GitHub: github.com/mobinyousefi-cs)
 Created:    2025-12-06
 Updated:    2025-12-06
 License:    MIT License (see LICENSE file for details)
===========================================================

 Description:
    Small networking utility API used by both server and
    client for sending and receiving line-oriented text
    messages over TCP sockets.

 Usage:
    - Include this header in any translation unit that
      needs to send/receive protocol messages.
    - Implementations are provided in net_utils.c.

 Notes:
    - The API is intentionally minimal and blocking. It is
      sufficient for a small teaching/demo project.
===========================================================
*/

#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <stddef.h>

/*
 * Send all bytes in 'buf' to socket 'fd'.
 * Returns 0 on success, -1 on error.
 */
int send_all(int fd, const char *buf, size_t len);

/*
 * Send a null-terminated line with '\n' appended.
 * Returns 0 on success, -1 on error.
 */
int send_line(int fd, const char *line);

/*
 * Receive a line (up to maxlen-1 chars) from 'fd'.
 * Line is terminated by '\n'. The newline is stripped.
 * Returns:
 *   >0 : number of bytes read (excluding newline)
 *    0 : connection closed gracefully
 *   -1 : error
 */
int recv_line(int fd, char *buf, size_t maxlen);

/* Utility to print an error using perror and terminate. */
void die(const char *msg);

#endif /* NET_UTILS_H */
