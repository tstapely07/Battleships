CC      ?= gcc
CFLAGS  ?= -std=c17 -g \
        -D_POSIX_SOURCE -D_DEFAULT_SOURCE \
        -Wall -Werror -pedantic -Isrc

# HOW TO COMPILE RAYLIB
# https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux
RAYLIB_INCLUDE = ../raylib/src
RAYLIB_LIB = ../raylib/src
RAYGUI_INCLUDE = ../raygui/src

CFLAGS += -I$(RAYLIB_INCLUDE) -I$(RAYGUI_INCLUDE)
LDLIBS = -L$(RAYLIB_LIB) -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

$(shell mkdir -p bin)

# SOURCE FILES
SHARED_SRCS = \
	src/shared/board.c

SERVER_SRCS = \
	src/server/main.c \
	src/server/game.c \
	src/server/network.c \
	src/shared/log.c \
	src/shared/network.c

CLIENT_CORE_SRCS = \
	src/client/main.c \
	src/client/state.c \
	src/client/network.c \
	src/client/game.c \
	src/shared/log.c \
	src/shared/network.c

CLIENT_GUI_SRCS = \
	src/client/render_raylib.c \
	src/client/input_raylib.c \

CLIENT_CLI_SRCS = \
	src/client/render_text.c \
	src/client/input_text.c

TEST_SRCS = \
	tests/test_extension.c \
	tests/test_board.c \
	src/shared/log.c \
	src/shared/board.c

TEST_CLIENT_SRCS = \
	tests/test_client.c \
	src/client/network.c \
	src/shared/log.c \
	src/shared/network.c

TEST_SERVER_SRCS = \
	tests/test_server.c \
	src/server/network.c \
	src/shared/log.c \
	src/shared/network.c

# OBJECT FILES
SHARED_OBJS = $(SHARED_SRCS:.c=.o)
SERVER_OBJS = $(SERVER_SRCS:.c=.o)
CLIENT_CORE_OBJS = $(CLIENT_CORE_SRCS:.c=.o)
CLIENT_GUI_OBJS = $(CLIENT_GUI_SRCS:.c=.o)
CLIENT_CLI_OBJS = $(CLIENT_CLI_SRCS:.c=.o)
TEST_OBJS = $(TEST_SRCS:.c=.o)
TEST_CLIENT_OBJS = $(TEST_CLIENT_SRCS:.c=.o)
TEST_SERVER_OBJS = $(TEST_SERVER_SRCS:.c=.o)

# BUILD TARGETS AND RULES
.PHONY: all clean format prepare test test-client test-server server client-gui client-cli

all: prepare bin/server bin/client-gui bin/client-cli

bin/server: $(SHARED_OBJS) $(SERVER_OBJS)
	$(CC) $(CFLAGS) $^ -o $@
server: bin/server

bin/client-gui: $(SHARED_OBJS) $(CLIENT_CORE_OBJS) $(CLIENT_GUI_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
client-gui: bin/client-gui

bin/client-cli: $(SHARED_OBJS) $(CLIENT_CORE_OBJS) $(CLIENT_CLI_OBJS)
	$(CC) $(CFLAGS) $^ -o $@
client-cli: bin/client-cli

bin/test: $(TEST_OBJS)
	$(CC) $(CFLAGS) $^ -o $@
test: bin/test

bin/test-client: $(TEST_CLIENT_OBJS)
	$(CC) $(CFLAGS) $^ -o $@
test-client: bin/test-client

bin/test-server: $(TEST_SERVER_OBJS)
	$(CC) $(CFLAGS) $^ -o $@
test-server: bin/test-server

prepare:
	mkdir -p logs

# Compiles any missing .o file, and places next to its .c counterpart
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# FORMAT
format:
	git ls-files '*.c' '*.h' | xargs clang-format -i

# CLEAN
clean:
	-$(RM) -r bin/*
	-$(RM) $(SHARED_OBJS) $(SERVER_OBJS) $(CLIENT_CORE_OBJS) $(CLIENT_GUI_OBJS) $(CLIENT_CLI_OBJS) main.o
