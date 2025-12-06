#===========================================================
# Project:    Multiplayer Ludo Game
# File:       Makefile
# Author:     Mobin Yousefi (GitHub: github.com/mobinyousefi-cs)
# Created:    2025-12-06
# Updated:    2025-12-06
# License:    MIT License (see LICENSE file for details)
#===========================================================
#
# Description:
#    Build configuration for the Multiplayer Ludo Game project.
#
# Usage:
#    make            # Build both server and client
#    make ludo_server
#    make ludo_client
#    make clean      # Remove build artifacts
#
# Notes:
#    - Produces binaries in the bin/ directory.
#    - Uses a simple dependency scheme suitable for small C projects.
#===========================================================

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -pedantic -O2
INCLUDES = -Iinclude
LDFLAGS = 

SRC_DIR = src
BIN_DIR = bin

SERVER_OBJS = $(SRC_DIR)/server.o $(SRC_DIR)/game.o $(SRC_DIR)/net_utils.o
CLIENT_OBJS = $(SRC_DIR)/client.o $(SRC_DIR)/net_utils.o

.PHONY: all clean dirs

all: dirs ludo_server ludo_client

dirs:
	mkdir -p $(BIN_DIR)

ludo_server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(BIN_DIR)/ludo_server $(SERVER_OBJS) $(LDFLAGS)

ludo_client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(BIN_DIR)/ludo_client $(CLIENT_OBJS) $(LDFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c include/%.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(SRC_DIR)/server.o: $(SRC_DIR)/server.c include/game.h include/net_utils.h
$(SRC_DIR)/client.o: $(SRC_DIR)/client.c include/net_utils.h
$(SRC_DIR)/game.o:   $(SRC_DIR)/game.c include/game.h
$(SRC_DIR)/net_utils.o: $(SRC_DIR)/net_utils.c include/net_utils.h

clean:
	rm -f $(SRC_DIR)/*.o
	rm -rf $(BIN_DIR)
