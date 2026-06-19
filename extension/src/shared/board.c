#include "board.h"
#include "log.h"
#include <stdlib.h>

/*
Shared logic for validing ship placements, testing collisions
*/

struct Board {
    ShipDefs ships;
    CellState cells[BOARD_SIZE][BOARD_SIZE];
};

#define check_pos_in_bounds(pos)                                               \
    (pos.x >= 0 && pos.x < BOARD_SIZE && pos.y >= 0 && pos.y < BOARD_SIZE)
#define mask_bit_for_ship(ship) (1 << ship)

int ship_length(ShipType ship) {
    switch (ship) {
    case SHIP_BATTLESHIP:
        return 5;
    case SHIP_CARRIER:
        return 4;
    case SHIP_CRUISER:
    case SHIP_SUBMARINE:
        return 3;
    case SHIP_DESTROYER:
        return 2;
    default:
        return -1;
    }
}

static void next_position(PositionWithDirection *pwd) {
    if (pwd->horizontal) {
        pwd->pos.x++;
    } else {
        pwd->pos.y++;
    }
}

static bool place_ship(ShipState sl, Board board) {
    int len = ship_length(sl.ship);
    if (len == -1) {
        // Invalid ship type passed
        return false;
    }

    PositionWithDirection pwd = sl.pwd;

    for (int i = 0; i < len; i++) {
        Position pos = pwd.pos;
        if (!check_pos_in_bounds(pos)) {
            // Illegal position - the placement must not be valid
            return false;
        }

        if (board->cells[pos.x][pos.y] != CELL_WATER) {
            // Ships overlap
            return false;
        }

        board->cells[pos.x][pos.y] = CELL_SHIP;
        next_position(&pwd);
    }

    return true;
}

bool board_add_single_ship(InitialShipState isl, Board board) {
    ShipState sl =
        (ShipState){.ship = isl.ship, .pwd = isl.pwd, .destroyed = false};

    return place_ship(sl, board);
}

ShipState board_get_ship(Board board, int index) {
    if (index >= 0 && index < NUM_SHIPS) {
        return board->ships[index];
    }

    LOG_ERROR("Invalid index (%d) provided", index);
    exit(EXIT_FAILURE);
}

/*
Add the following ships at given the board is empty.
It is the caller's responsibility to keep track of how many ships are already on
the board. This should only be used for initialisation and never after gameplay
has started. If you are adding multiple ships, see board_is_valid_placement_set.
This function does not clear the board if it is invalid. The caller must provide
a valid placement set before any of the game code has defined behaviour,
*/
bool board_add_placement_set(InitialShipDefs ship_defs, Board board) {
    // Bit n in declared_types being set to 1 corresponds to having a ship of
    // that type already added
    int declared_types = 0;

    for (int i = 0; i < NUM_SHIPS; i++) {
        // Initialise internal struct
        InitialShipState isl = ship_defs[i];
        ShipState sl =
            (ShipState){.ship = isl.ship, .pwd = isl.pwd, .destroyed = false};

        int mask = mask_bit_for_ship(sl.ship);
        if ((declared_types & mask) != 0) {
            LOG_ERROR("%s", "Already ship of same type");
            return false;
        }

        // No ship of this type declared yet
        if (!place_ship(sl, board)) {
            // Ship position was invalid or ship type invalid
            return false;
        }

        board->ships[i] = sl;
        declared_types = declared_types | mask;
    }

    return true;
}

Board create_empty_board(void) {
    Board b = malloc(sizeof(struct Board));
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            b->cells[i][j] = CELL_WATER;
        }
    }

    return b;
}

void free_board(Board b) { free(b); }

struct PosShipPair {
    Position pos;
    ShipType *ship;
};

void board_mark_sunk_ship(Board board, ShipType type,
                          PositionWithDirection pwd) {
    int len = ship_length(type);
    for (int i = 0; i < len; i++) {
        board->cells[pwd.pos.x][pwd.pos.y] = CELL_SUNK;
        next_position(&pwd);
    }
}

static void check_destroyed(Board board, ShipState *s, void *data) {
    if (s->destroyed) {
        return;
    }

    int len = ship_length(s->ship);
    struct PosShipPair *psp = (struct PosShipPair *)data;
    ShipType *sunk_ship = psp->ship;
    PositionWithDirection pwd = s->pwd;

    s->destroyed = true;

    for (int i = 0; i < len; i++) {
        Position pos = pwd.pos;
        if (board->cells[pos.x][pos.y] != CELL_HIT) {
            s->destroyed = false;
            break;
        }
        next_position(&pwd);
    }

    if (s->destroyed) {
        *sunk_ship = s->ship;
        board_mark_sunk_ship(board, s->ship, s->pwd);
    }
}

static void for_each_ship(Board board,
                          void (*cb)(Board board, ShipState *s, void *data),
                          void *data) {
    for (int i = 0; i < NUM_SHIPS; i++) {
        (*cb)(board, &(board->ships[i]), data);
    }
}

/*
Attempts to hit at pos on opponent_ships_board. If this succeeds,
opponent_ships_board has pos updated to a hit and player_target_board has pos
updated to a hit. If it misses, the both boards have pos updated to a miss.

Returns true if the shot was successfully processed. Returns false if the
position is invalid or if an attack has already been launched on that position
*/
bool board_try_hit(Board opponent_ships_board, Position pos, bool *was_hit,
                   ShipType *sunk) {
    if (!check_pos_in_bounds(pos)) {
        return false;
    }

    CellState target = opponent_ships_board->cells[pos.x][pos.y];

    switch (target) {
    case CELL_WATER:
        opponent_ships_board->cells[pos.x][pos.y] = CELL_MISS;
        *was_hit = false;
        break;
    case CELL_SHIP:
        opponent_ships_board->cells[pos.x][pos.y] = CELL_HIT;
        *was_hit = true;

        // Update the relevant ship definitions on the opponent's board
        *sunk = -1;
        struct PosShipPair psp = {.pos = pos, .ship = sunk};
        for_each_ship(opponent_ships_board, *check_destroyed, &psp);
        break;
    case CELL_HIT:
    case CELL_SUNK:
    case CELL_MISS:
        return false;
    }

    return true;
}

void board_mark_strike(Board board, Position pos, bool success) {
    if (!check_pos_in_bounds(pos)) {
        return;
    }

    CellState new_state;

    if (success) {
        new_state = CELL_HIT;
    } else {
        new_state = CELL_MISS;
    }

    board->cells[pos.x][pos.y] = new_state;
}

bool valid_attack_pos(Board board, Position pos) {
    if (!check_pos_in_bounds(pos)) {
        return false;
    }

    return board->cells[pos.x][pos.y] == CELL_WATER;
}

bool all_ships_destroyed(Board board) {
    for (int i = 0; i < NUM_SHIPS; i++) {
        if (!board->ships[i].destroyed) {
            return false;
        }
    }

    return true;
}

static char cell_to_char(CellState cell) {
    switch (cell) {
    case CELL_HIT:
        return 'x';
    case CELL_SUNK:
        return 'X';
    case CELL_MISS:
        return 'O';
    case CELL_SHIP:
        return '*';
    default:
        return '-';
    }
}

void print_board(Board board, FILE *out) {
    fprintf(out, " ");
    for (int i = 0; i < BOARD_SIZE; i++) {
        fprintf(out, " %d", i);
    }
    fprintf(out, "\n");

    for (int i = 0; i < BOARD_SIZE; i++) {
        fprintf(out, "%d", i);
        for (int j = 0; j < BOARD_SIZE; j++) {
            fprintf(out, " %c", cell_to_char(board->cells[j][i]));
        }
        fprintf(out, "\n");
    }
}

CellState get_cell(Board b, int x, int y) {
    Position p = {.x = x, .y = y};
    if (!check_pos_in_bounds(p)) {
        LOG_ERROR("Position (%d, %d) not in bounds", x, y);
        exit(1);
    }
    return b->cells[x][y];
}
