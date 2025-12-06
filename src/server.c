/*
===========================================================
 Project:    Multiplayer Ludo Game
 File:       server.c
 Author:     Mobin Yousefi (GitHub: github.com/mobinyousefi-cs)
 Created:    2025-12-06
 Updated:    2025-12-06
 License:    MIT License (see LICENSE file for details)
===========================================================

 Description:
    TCP server for the Multiplayer Ludo game. Manages client
    connections, drives the game loop, broadcasts state and
    turn updates, and enforces a minimal text-based protocol
    between server and clients.

 Usage:
    Compile and run, for example:
        ./bin/ludo_server 4000 2

    Where 4000 is the TCP port and 2 is the number of
    players required to start the game.

 Notes:
    - The server uses blocking I/O and a simple protocol.
    - For simplicity, if a player disconnects mid-game,
      the server currently aborts the game.
===========================================================
*/

#include "game.h"
#include "net_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Simple protocol commands:
 * Client -> Server:
 *   HELLO <name>
 *   ROLL
 *
 * Server -> Client:
 *   WELCOME <player_id> <required_players>
 *   PLAYER_JOINED <id> <name>
 *   START <current_player>
 *   STATE <summary>
 *   TURN <current_player>
 *   YOUR_TURN
 *   ROLLED <id> <value>
 *   MOVED <id> <token_index> <position> <state_char>
 *   WINNER <id> <name>
 */

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <port> <num_players>\n", prog);
    fprintf(stderr, "  num_players between 2 and %d\n", MAX_PLAYERS);
}

static int create_listen_socket(int port)
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        die("socket");
    }

    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        die("setsockopt");
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons((uint16_t)port);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        die("bind");
    }

    if (listen(listen_fd, 8) < 0) {
        die("listen");
    }

    return listen_fd;
}

static void broadcast_line_to_all(int *fds, int count, const char *line)
{
    for (int i = 0; i < count; ++i) {
        if (fds[i] >= 0) {
            if (send_line(fds[i], line) < 0) {
                /* Ignore errors: client may have disconnected */
            }
        }
    }
}

static void broadcast_state(GameState *game, int *fds, int count)
{
    char summary[512];
    game_state_summary(game, summary, sizeof(summary));

    char line[600];
    snprintf(line, sizeof(line), "STATE %s", summary);
    broadcast_line_to_all(fds, count, line);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    int required_players = atoi(argv[2]);
    if (required_players < 2 || required_players > MAX_PLAYERS) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    int listen_fd = create_listen_socket(port);
    printf("Ludo server listening on port %d, expecting %d players...\n",
           port, required_players);

    GameState game;
    game_init(&game);

    int client_fds[MAX_PLAYERS];
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        client_fds[i] = -1;
    }

    /* Phase 1: Accept connections until required_players joined */
    while (!game_can_start(&game, required_players)) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &cli_len);
        if (fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            die("accept");
        }

        printf("Incoming connection from %s:%d\n",
               inet_ntoa(cli_addr.sin_addr),
               ntohs(cli_addr.sin_port));

        char line[256];
        int n = recv_line(fd, line, sizeof(line));
        if (n <= 0) {
            close(fd);
            continue;
        }

        /* Expect: HELLO <name> */
        const char *cmd = strtok(line, " ");
        const char *name = strtok(NULL, "");
        if (!cmd || strcmp(cmd, "HELLO") != 0 || !name) {
            fprintf(stderr, "Invalid HELLO from client.\n");
            send_line(fd, "ERROR InvalidHELLO");
            close(fd);
            continue;
        }

        /* Trim leading spaces from name */
        while (*name && isspace((unsigned char)*name)) {
            name++;
        }

        int player_id = game_add_player(&game, name);
        if (player_id < 0) {
            fprintf(stderr, "Game full; rejecting client.\n");
            send_line(fd, "ERROR GameFull");
            close(fd);
            continue;
        }

        client_fds[player_id] = fd;

        /* Send WELCOME */
        char welcome[128];
        snprintf(welcome, sizeof(welcome),
                 "WELCOME %d %d", player_id, required_players);
        send_line(fd, welcome);

        /* Notify others */
        char joined[128];
        snprintf(joined, sizeof(joined),
                 "PLAYER_JOINED %d %s", player_id, game.players[player_id].name);
        broadcast_line_to_all(client_fds, required_players, joined);

        printf("Player %d (%s) joined. total=%d/%d\n",
               player_id, game.players[player_id].name,
               game.num_players, required_players);
    }

    /* Phase 2: Start game */
    game_prepare(&game);

    char start_line[64];
    snprintf(start_line, sizeof(start_line),
             "START %d", game.current_player);
    broadcast_line_to_all(client_fds, required_players, start_line);

    /* Main game loop */
    while (game.winner < 0) {
        broadcast_state(&game, client_fds, required_players);

        int current = game.current_player;
        char turn_line[64];
        snprintf(turn_line, sizeof(turn_line),
                 "TURN %d", current);
        broadcast_line_to_all(client_fds, required_players, turn_line);

        int cur_fd = client_fds[current];
        if (cur_fd < 0) {
            fprintf(stderr, "Current player %d has no socket. Aborting.\n", current);
            break;
        }

        /* Ask current player to roll */
        send_line(cur_fd, "YOUR_TURN");

        char line[256];
        int n = recv_line(cur_fd, line, sizeof(line));
        if (n <= 0) {
            fprintf(stderr, "Player %d disconnected during turn.\n", current);
            game.players[current].connected = 0;
            client_fds[current] = -1;
            /* For simplicity, abort game */
            break;
        }

        if (strcmp(line, "ROLL") != 0) {
            fprintf(stderr, "Unexpected command from player %d: %s\n", current, line);
            send_line(cur_fd, "ERROR ExpectedROLL");
            continue; /* ask again */
        }

        int rolled = 0;
        int moved_token = -1;
        int finished = game_roll_and_move(&game, current, &rolled, &moved_token);

        char rolled_line[64];
        snprintf(rolled_line, sizeof(rolled_line),
                 "ROLLED %d %d", current, rolled);
        broadcast_line_to_all(client_fds, required_players, rolled_line);

        if (moved_token >= 0) {
            Token *tk = &game.players[current].tokens[moved_token];
            char s = '?';
            if (tk->state == TOKEN_STATE_HOME)     s = 'H';
            else if (tk->state == TOKEN_STATE_ACTIVE)   s = 'A';
            else if (tk->state == TOKEN_STATE_FINISHED) s = 'F';

            char moved_line[64];
            snprintf(moved_line, sizeof(moved_line),
                     "MOVED %d %d %d %c",
                     current, moved_token, tk->position, s);
            broadcast_line_to_all(client_fds, required_players, moved_line);
        }

        if (finished) {
            broadcast_state(&game, client_fds, required_players);
            int w = game.winner;
            char win_line[128];
            snprintf(win_line, sizeof(win_line),
                     "WINNER %d %s", w, game.players[w].name);
            broadcast_line_to_all(client_fds, required_players, win_line);
            printf("Game finished. Winner: P%d (%s)\n",
                   w, game.players[w].name);
            break;
        }
    }

    for (int i = 0; i < required_players; ++i) {
        if (client_fds[i] >= 0) {
            close(client_fds[i]);
        }
    }
    close(listen_fd);

    return EXIT_SUCCESS;
}
