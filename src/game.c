/*
===========================================================
 Project:    Multiplayer Ludo Game
 File:       game.c
 Author:     Mobin Yousefi (GitHub: github.com/mobinyousefi-cs)
 Created:    2025-12-06
 Updated:    2025-12-06
 License:    MIT License (see LICENSE file for details)
===========================================================

 Description:
    Implementation of the core game logic for a simplified
    Multiplayer Ludo game. Manages players, tokens, dice
    rolls, turn transitions, and winner detection.

 Usage:
    - Linked into the server binary.
    - The server drives the game by calling
      game_roll_and_move() for the current player and
      broadcasting updates to all clients.

 Notes:
    - The RNG is seeded once (per process) using time(NULL).
    - The board is modeled as a linear track of length
      TRACK_LENGTH.
    - This implementation is intentionally minimal and can
      be extended with real Ludo rules (captures, safe
      cells, home stretch, etc.).
===========================================================
*/

#include "game.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

void game_init(GameState *game)
{
    if (!game) {
        return;
    }
    memset(game, 0, sizeof(*game));
    game->num_players    = 0;
    game->current_player = 0;
    game->started        = 0;
    game->winner         = -1;

    for (int i = 0; i < MAX_PLAYERS; ++i) {
        game->players[i].id         = i;
        game->players[i].connected  = 0;
        game->players[i].tokens_finished = 0;
        for (int t = 0; t < TOKENS_PER_PLAYER; ++t) {
            game->players[i].tokens[t].position = 0;
            game->players[i].tokens[t].state    = TOKEN_STATE_HOME;
        }
    }

    /* Seed RNG once per process */
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }
}

int game_add_player(GameState *game, const char *name)
{
    if (!game || !name) {
        return -1;
    }
    if (game->num_players >= MAX_PLAYERS) {
        return -1;
    }

    int id = game->num_players;
    Player *p = &game->players[id];

    snprintf(p->name, sizeof(p->name), "%s", name);
    p->id        = id;
    p->connected = 1;
    p->tokens_finished = 0;

    for (int t = 0; t < TOKENS_PER_PLAYER; ++t) {
        p->tokens[t].position = 0;
        p->tokens[t].state    = TOKEN_STATE_HOME;
    }

    game->num_players++;
    return id;
}

int game_can_start(const GameState *game, int required_players)
{
    if (!game) {
        return 0;
    }
    if (required_players < 2 || required_players > MAX_PLAYERS) {
        return 0;
    }
    return (game->num_players == required_players);
}

void game_prepare(GameState *game)
{
    if (!game) {
        return;
    }
    game->current_player = 0;
    game->started        = 1;
    game->winner         = -1;
}

/* Helper: roll dice [1..6] */
static int roll_dice(void)
{
    return (rand() % 6) + 1;
}

static int all_tokens_finished(const Player *p)
{
    return (p->tokens_finished == TOKENS_PER_PLAYER);
}

/*
 * Find the first token that can move given dice roll.
 * Return token index or -1 if none.
 */
static int find_movable_token(const Player *p, int dice)
{
    /* Rule:
     * - To leave HOME, must roll 6 -> move to position 0 (ACTIVE).
     * - ACTIVE token can move if position + dice <= TRACK_LENGTH.
     */

    /* First, prefer finishing moves (ACTIVE tokens near end) */
    for (int i = 0; i < TOKENS_PER_PLAYER; ++i) {
        const Token *tk = &p->tokens[i];
        if (tk->state == TOKEN_STATE_ACTIVE) {
            if (tk->position + dice == TRACK_LENGTH) {
                return i;
            }
        }
    }

    /* Then, tokens on board that can move */
    for (int i = 0; i < TOKENS_PER_PLAYER; ++i) {
        const Token *tk = &p->tokens[i];
        if (tk->state == TOKEN_STATE_ACTIVE) {
            if (tk->position + dice <= TRACK_LENGTH) {
                return i;
            }
        }
    }

    /* Finally, from home, only if dice == 6 */
    if (dice == 6) {
        for (int i = 0; i < TOKENS_PER_PLAYER; ++i) {
            const Token *tk = &p->tokens[i];
            if (tk->state == TOKEN_STATE_HOME) {
                return i;
            }
        }
    }

    return -1;
}

int game_roll_and_move(GameState *game,
                       int player_id,
                       int *rolled_value,
                       int *moved_token_index)
{
    if (!game || player_id < 0 || player_id >= game->num_players) {
        return 0;
    }

    Player *p = &game->players[player_id];

    int dice = roll_dice();
    if (rolled_value) {
        *rolled_value = dice;
    }

    int token_idx = find_movable_token(p, dice);
    if (moved_token_index) {
        *moved_token_index = token_idx;
    }

    if (token_idx >= 0) {
        Token *tk = &p->tokens[token_idx];

        if (tk->state == TOKEN_STATE_HOME && dice == 6) {
            tk->state    = TOKEN_STATE_ACTIVE;
            tk->position = 0;
        } else if (tk->state == TOKEN_STATE_ACTIVE) {
            int new_pos = tk->position + dice;
            if (new_pos > TRACK_LENGTH) {
                /* Illegal: don't move */
            } else if (new_pos == TRACK_LENGTH) {
                tk->position = new_pos;
                tk->state    = TOKEN_STATE_FINISHED;
                p->tokens_finished++;
            } else {
                tk->position = new_pos;
            }
        }
    }

    /* Check winner */
    if (all_tokens_finished(p)) {
        game->winner = p->id;
        return 1;
    }

    /* Turn logic */
    if (!(dice == 6 && token_idx >= 0)) {
        /* Advance to next player */
        int next = game->current_player;
        int steps = 0;
        do {
            next = (next + 1) % game->num_players;
            steps++;
            /* Avoid infinite loop if all disconnected (should not happen) */
            if (steps > game->num_players) {
                break;
            }
        } while (!game->players[next].connected);

        game->current_player = next;
    }

    return 0;
}

void game_state_summary(const GameState *game, char *buf, size_t buflen)
{
    if (!game || !buf || buflen == 0) {
        return;
    }

    int written = 0;
    written += snprintf(buf + written, buflen - (size_t)written,
                        "Players=%d, Current=%d, Winner=%d | ",
                        game->num_players,
                        game->current_player,
                        game->winner);

    for (int i = 0; i < game->num_players; ++i) {
        const Player *p = &game->players[i];
        if (written >= (int)buflen) {
            break;
        }
        written += snprintf(buf + written, buflen - (size_t)written,
                            "[P%d:%s finished=%d ",
                            p->id, p->name, p->tokens_finished);
        for (int t = 0; t < TOKENS_PER_PLAYER; ++t) {
            const Token *tk = &p->tokens[t];
            char s = '?';
            if (tk->state == TOKEN_STATE_HOME)     s = 'H';
            else if (tk->state == TOKEN_STATE_ACTIVE)   s = 'A';
            else if (tk->state == TOKEN_STATE_FINISHED) s = 'F';

            written += snprintf(buf + written, buflen - (size_t)written,
                                "T%d=%c@%d ",
                                t, s, tk->position);
        }
        written += snprintf(buf + written, buflen - (size_t)written, "] ");
    }

    if (written >= (int)buflen) {
        buf[buflen - 1] = '\0';
    }
}
