/*
===========================================================
 Project:    Multiplayer Ludo Game
 File:       client.c
 Author:     Mobin Yousefi (GitHub: github.com/mobinyousefi-cs)
 Created:    2025-12-06
 Updated:    2025-12-06
 License:    MIT License (see LICENSE file for details)
===========================================================

 Description:
    TCP client for the Multiplayer Ludo game. Connects to
    the server, participates in the text-based protocol,
    and provides a simple CLI for the human player.

 Usage:
    Compile and run, for example:
        ./bin/ludo_client 127.0.0.1 4000 Alice

    Where 127.0.0.1 is the server IP, 4000 is the TCP port,
    and "Alice" is the player name.

 Notes:
    - Interaction is line-based; when it is the player's
      turn, the client waits for ENTER to send a ROLL
      command.
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
#include <netinet/in.h>
#include <arpa/inet.h>

/* Client-side protocol:
 *  - Connects to server <host>:<port>.
 *  - Sends: HELLO <name>.
 *  - Then reacts to server messages and, when receiving
 *    YOUR_TURN, asks user to press Enter and sends ROLL.
 */

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <server_ip> <port> <player_name>\n", prog);
}

static int connect_to_server(const char *ip, int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket");
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((uint16_t)port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IP address: %s\n", ip);
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        die("connect");
    }

    return fd;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip   = argv[1];
    int         port        = atoi(argv[2]);
    const char *player_name = argv[3];

    int fd = connect_to_server(server_ip, port);
    printf("Connected to server %s:%d\n", server_ip, port);

    char hello[128];
    snprintf(hello, sizeof(hello), "HELLO %s", player_name);
    if (send_line(fd, hello) < 0) {
        die("send HELLO");
    }

    char line[512];
    int my_id = -1;
    int expected_players = 0;

    /* Initial handshake */
    int n = recv_line(fd, line, sizeof(line));
    if (n <= 0) {
        fprintf(stderr, "Server closed connection.\n");
        close(fd);
        return EXIT_FAILURE;
    }

    char *cmd = strtok(line, " ");
    if (!cmd || strcmp(cmd, "WELCOME") != 0) {
        fprintf(stderr, "Unexpected server response: %s\n", line);
        close(fd);
        return EXIT_FAILURE;
    }

    char *id_str = strtok(NULL, " ");
    char *exp_str = strtok(NULL, " ");
    if (!id_str || !exp_str) {
        fprintf(stderr, "Malformed WELCOME.\n");
        close(fd);
        return EXIT_FAILURE;
    }
    my_id = atoi(id_str);
    expected_players = atoi(exp_str);

    printf("WELCOME: You are Player %d (%s). Waiting for %d players total...\n",
           my_id, player_name, expected_players);

    /* Main loop: read messages and act */
    while ((n = recv_line(fd, line, sizeof(line))) > 0) {
        if (n <= 0) {
            break;
        }

        char *msg = line;
        char *tok = strtok(msg, " ");

        if (!tok) {
            continue;
        }

        if (strcmp(tok, "PLAYER_JOINED") == 0) {
            char *id  = strtok(NULL, " ");
            char *name = strtok(NULL, "");
            if (!id || !name) {
                continue;
            }
            printf("[INFO] Player joined: id=%s, name=%s\n", id, name);

        } else if (strcmp(tok, "START") == 0) {
            char *cur = strtok(NULL, " ");
            printf("[GAME] Game started. First player: %s\n", cur ? cur : "?");

        } else if (strcmp(tok, "STATE") == 0) {
            char *rest = strtok(NULL, "");
            printf("[STATE] %s\n", rest ? rest : "");

        } else if (strcmp(tok, "TURN") == 0) {
            char *id = strtok(NULL, " ");
            if (!id) id = "?";
            int turn_id = atoi(id);
            if (turn_id == my_id) {
                printf("[TURN] It is your turn!\n");
            } else {
                printf("[TURN] It is Player %d's turn.\n", turn_id);
            }

        } else if (strcmp(tok, "YOUR_TURN") == 0) {
            printf(">> Your turn. Press ENTER to roll the dice...");
            fflush(stdout);

            /* Wait for user input */
            char dummy[4];
            (void)fgets(dummy, sizeof(dummy), stdin);

            if (send_line(fd, "ROLL") < 0) {
                fprintf(stderr, "Failed to send ROLL.\n");
                break;
            }

        } else if (strcmp(tok, "ROLLED") == 0) {
            char *id  = strtok(NULL, " ");
            char *val = strtok(NULL, " ");
            if (id && val) {
                printf("[ROLL] Player %s rolled %s.\n", id, val);
            }

        } else if (strcmp(tok, "MOVED") == 0) {
            char *id   = strtok(NULL, " ");
            char *idx  = strtok(NULL, " ");
            char *pos  = strtok(NULL, " ");
            char *stat = strtok(NULL, " ");
            if (id && idx && pos && stat) {
                printf("[MOVE] Player %s moved token %s to pos %s (state=%s).\n",
                       id, idx, pos, stat);
            }

        } else if (strcmp(tok, "WINNER") == 0) {
            char *id   = strtok(NULL, " ");
            char *name = strtok(NULL, "");
            if (id && name) {
                printf("\n*** GAME OVER ***\n");
                printf("Winner: Player %s (%s)\n", id, name);
            } else {
                printf("\n*** GAME OVER (winner unknown) ***\n");
            }
            break;

        } else if (strcmp(tok, "ERROR") == 0) {
            char *rest = strtok(NULL, "");
            fprintf(stderr, "[ERROR] %s\n", rest ? rest : "");
            break;

        } else {
            /* Unknown message, just print */
            char *rest = strtok(NULL, "");
            printf("[SERVER] %s %s\n", tok, rest ? rest : "");
        }
    }

    if (n == 0) {
        printf("Server closed the connection.\n");
    } else if (n < 0) {
        perror("recv_line");
    }

    close(fd);
    return EXIT_SUCCESS;
}
