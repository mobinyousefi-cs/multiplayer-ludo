/*
===========================================================
 Project:    Multiplayer Ludo Game
 File:       game.h
 Author:     Mobin Yousefi (GitHub: github.com/mobinyousefi-cs)
 Created:    2025-12-06
 Updated:    2025-12-06
 License:    MIT License (see LICENSE file for details)
===========================================================

 Description:
    Core data structures and public API for the simplified
    Multiplayer Ludo game logic.

 Usage:
    - Include this header in server-side translation units
      that need to manage game state and turns.
    - Implementations are provided in game.c.

 Notes:
    - The board is modeled as a single linear track with
      length TRACK_LENGTH.
    - Rules are intentionally simplified (no captures, safe
      cells, or complex home paths) to keep the focus on
      networking and state management.
===========================================================
*/

#ifndef GAME_H
#define GAME_H

#include <stddef.h>

#define MAX_PLAYERS        4
#define TOKENS_PER_PLAYER  4
#define TRACK_LENGTH       40  /* Linear track length */

typedef enum {
    TOKEN_STATE_HOME = 0,
    TOKEN_STATE_ACTIVE,
    TOKEN_STATE_FINISHED
} TokenState;

typedef struct {
    int         position;   /* 0..TRACK_LENGTH, valid only if ACTIVE/FINISHED */
    TokenState  state;
} Token;

typedef struct {
    char  name[32];
    int   id;               /* 0..MAX_PLAYERS-1 */
    int   connected;        /* bool: 1 = active, 0 = disconnected */
    Token tokens[TOKENS_PER_PLAYER];
    int   tokens_finished;
} Player;

typedef struct {
    Player players[MAX_PLAYERS];
    int    num_players;
    int    current_player;  /* index of current player */
    int    started;         /* bool */
    int    winner;          /* -1 if none, otherwise player id */
} GameState;

/* Initialize an empty game state */
void game_init(GameState *game);

/* Add a new player; returns player id on success, or -1 if full */
int game_add_player(GameState *game, const char *name);

/* Returns non-zero if the game can start with 'required_players' */
int game_can_start(const GameState *game, int required_players);

/* Prepare game state for play (current_player=0, winner=-1, etc.) */
void game_prepare(GameState *game);

/*
 * Roll dice for player_id, apply a legal move (if any), and update state.
 * - rolled_value: output dice value (1..6)
 * - moved_token_index: index (0..TOKENS_PER_PLAYER-1) or -1 if no token moves
 * Returns:
 *  - 1 if the game ended and winner is set
 *  - 0 otherwise
 *
 * Turn rule:
 *  - If dice != 6, turn passes to next player.
 *  - If dice == 6 and a move was possible, player gets another turn.
 *  - If dice == 6 but no move possible, turn still passes.
 */
int game_roll_and_move(GameState *game,
                       int player_id,
                       int *rolled_value,
                       int *moved_token_index);

/*
 * Create a human-readable single-line summary of the board into 'buf'.
 * Buffer is always null-terminated.
 */
void game_state_summary(const GameState *game, char *buf, size_t buflen);

#endif /* GAME_H */
