#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "board.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MSG_JOIN,
    MSG_REQ_BOARD,
    MSG_INIT_BOARD_LAYOUT,
    MSG_GAME_START,
    MSG_FIRE,
    MSG_ATTACK_RESULT,
    MSG_GAME_OVER,
    MSG_INVALID_BOARD
} MessageType;

// Universal header for every packet
typedef struct {
    MessageType type;
    uint32_t payload_length;
} PacketHeader;

// Payloads for specific packets
typedef struct {
    InitialShipDefs defs;
} InitBoardPayload;

typedef struct {
    int x;
    int y;
} FirePayload;

typedef struct {
    FirePayload shot;
    bool success;
    ShipType ship; // -1 if no ship destroyed
    PositionWithDirection
        sunk_pwd; // the root position and orientation of the sunk ship
} AttackResultPayload;

typedef struct {
    bool your_turn;
} GameStartPayload;

typedef struct {
    bool you_won;
} GameOverPayload;

#endif
