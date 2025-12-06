<!--
===========================================================
 Project:    Multiplayer Ludo Game
 File:       README.md
 Author:     Mobin Yousefi (GitHub: github.com/mobinyousefi-cs)
 Created:    2025-12-06
 Updated:    2025-12-06
 License:    MIT License (see LICENSE file for details)
===========================================================

 Description:
    Project documentation and usage guide for the
    Multiplayer Ludo Game implemented in C.

 Usage:
    View this file on GitHub or in a text editor to learn
    how to build and run the project.

 Notes:
    - This header is provided in a Markdown-friendly
      comment block.
===========================================================
-->

# Multiplayer Ludo Game (C, TCP Networked)

A **Multiplayer Ludo Game** implemented in C, using a simple **TCP client–server architecture** and a **line-based text protocol**. The goal of this project is to demonstrate:

* Network programming with BSD sockets in C
* Turn-based multiplayer game coordination on a server
* Clean modular design (game logic separated from networking)
* Professional code organization suitable for a GitHub portfolio

> Author: **Mobin Yousefi**
> GitHub: [github.com/mobinyousefi-cs](https://github.com/mobinyousefi-cs)

---

## Features

* Up to **4 players** (configurable on the server side)
* **TCP server** that:

  * Accepts player connections
  * Handles a minimal text protocol
  * Manages all game state and turns centrally
* **TCP client** that:

  * Connects to the server
  * Provides a simple command-line interface
  * Lets the player roll the dice and follow the game state
* **Simplified Ludo rules**:

  * Linear track of length `TRACK_LENGTH` (default: 40)
  * 4 tokens per player
  * Roll **6** to move a token from HOME to the board
  * Tokens move along the track and finish at the end
  * First player to finish all tokens wins

This project is intentionally minimal and is an excellent base for adding:

* Real Ludo board geometry and paths
* Capture rules and safe cells
* Graphical UI (SDL, ncurses, or external front-ends)

---

## Project Structure

```text
multiplayer-ludo/
├─ Makefile
├─ include/
│  ├─ game.h
│  └─ net_utils.h
└─ src/
   ├─ game.c
   ├─ net_utils.c
   ├─ server.c
   └─ client.c
```

### Modules

* **game.h / game.c**

  * Core game data structures and logic (players, tokens, turns, dice).
  * No networking dependencies.

* **net_utils.h / net_utils.c**

  * Small helpers for sending/receiving line-based messages over TCP.

* **server.c**

  * Listens on a TCP port.
  * Accepts a fixed number of players.
  * Drives the game loop and broadcasts updates.

* **client.c**

  * Connects to the server.
  * Displays game state and turn information.
  * Allows the human player to roll the dice.

---

## Build Instructions

### Requirements

* POSIX-like system (Linux, macOS, WSL, etc.)
* GCC or a compatible C compiler
* Standard BSD sockets (already available on most Unix-like systems)

### Build

From the project root:

```bash
make
```

This will create the following binaries:

* `bin/ludo_server`
* `bin/ludo_client`

To clean build artifacts:

```bash
make clean
```

---

## Run Instructions

### 1. Start the Server

Example: start a game with **2 players** on port **4000**:

```bash
./bin/ludo_server 4000 2
```

Arguments:

* `4000` – TCP port to listen on
* `2` – number of players required to start the game (2–4)

The server will:

* Initialize an empty game state
* Wait until the specified number of players have joined
* Start the game and orchestrate turns

### 2. Start Clients

Each player runs a **client** and connects to the server.

On the same machine (localhost) for two players:

```bash
./bin/ludo_client 127.0.0.1 4000 Alice
./bin/ludo_client 127.0.0.1 4000 Bob
```

Arguments:

* `127.0.0.1` – IP address of the server
* `4000` – server port
* `Alice` / `Bob` – player names

The client will:

1. Connect to the server.
2. Send a `HELLO <name>` message.
3. Receive a `WELCOME` message with its player id.
4. Wait until the server announces `START`.
5. Participate in the turn-based game.

### 3. Playing the Game

When it is your turn, you will see:

```text
[TURN] It is your turn!
>> Your turn. Press ENTER to roll the dice...
```

Press ENTER to roll the dice. The server will:

* Roll the dice
* Move one of your tokens if a legal move exists
* Broadcast messages such as:

  * `ROLLED <id> <value>`
  * `MOVED <id> <token_index> <position> <state>`
  * Updated `STATE ...`

The game continues until one player finishes all their tokens. The server will then send:

```text
WINNER <id> <name>
```

and the clients will show a **GAME OVER** message.

---

## Protocol Overview

The server and clients communicate with **newline-terminated** ASCII messages.

### Client → Server

* `HELLO <name>`
  Sent once after connection to register the player.

* `ROLL`
  Sent when the client receives `YOUR_TURN` and the user presses ENTER.

### Server → Client

* `WELCOME <player_id> <required_players>`
* `PLAYER_JOINED <id> <name>`
* `START <current_player>`
* `STATE <summary>`
* `TURN <current_player>`
* `YOUR_TURN`
* `ROLLED <id> <value>`
* `MOVED <id> <token_index> <position> <state_char>`
* `WINNER <id> <name>`
* `ERROR <reason>`

The `STATE` message contains a compact textual snapshot of the game, generated by `game_state_summary()`.

---

## Simplified Ludo Rules (Current Version)

* Board modeled as a **linear track** of `TRACK_LENGTH` cells (default: 40).
* Each player has **4 tokens**:

  * Initially at **HOME**.
  * To leave HOME: must roll a **6**, token placed at position 0 (ACTIVE).
* On each roll:

  * If moving a token would exceed `TRACK_LENGTH`, that move is skipped.
  * If a token lands exactly on `TRACK_LENGTH`, it becomes **FINISHED**.
* A player wins when **all 4 tokens are FINISHED**.
* Turn logic:

  * If dice != 6, the turn passes to the next connected player.
  * If dice == 6 and a token was moved, the player gets another turn.
  * If dice == 6 but no move is possible, the turn still passes.

---

## Possible Extensions

This codebase is intentionally structured to allow easy evolution. Some directions:

1. **Real Ludo board geometry**

   * Implement the full board with home columns and safe cells.
   * Map each player's path correctly.

2. **Capture and safety rules**

   * When a token lands on an opponent's token, send it back HOME.
   * Protect tokens on safe cells.

3. **Non-blocking / multiplexed I/O**

   * Replace blocking calls with `select()`, `poll()`, or `epoll()`.
   * Support chat or more complex commands.

4. **GUI or TUI front-end**

   * ncurses-based board visualization.
   * SDL/SDL2 or another graphical library.

5. **Persistence / replay**

   * Log game states and moves to a file.
   * Implement replay or analysis tools.

---

## License

This project is licensed under the **MIT License**. See the `LICENSE` file for full details.

---

## Author

* **Mobin Yousefi**
  GitHub: [github.com/mobinyousefi-cs](https://github.com/mobinyousefi-cs)

If you find this project useful or educational, consider starring the repository on GitHub or using the architecture as a template for other multiplayer/networked C projects.
