#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stdio.h>


#define BOARD_SIZE 10
#define NUM_SHIPS 5

typedef enum {
    SHIP_BATTLESHIP,
    SHIP_CARRIER,
    SHIP_CRUISER,
    SHIP_SUBMARINE,
    SHIP_DESTROYER
} ShipType;

typedef enum {
    CELL_WATER,
    CELL_SHIP,
    CELL_MISS,
    CELL_HIT,
    CELL_SUNK
} CellState;

// 0, 0 is the top left
struct Board;
typedef struct Board *Board;

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    Position pos;
    bool horizontal;
} PositionWithDirection;

typedef struct {
    ShipType ship;
    PositionWithDirection pwd;
    bool destroyed;
} ShipState;

typedef struct {
    ShipType ship;
    PositionWithDirection pwd;
} InitialShipState;

typedef ShipState ShipDefs[NUM_SHIPS];

typedef InitialShipState InitialShipDefs[NUM_SHIPS];

typedef InitialShipState InitialShipDefs[NUM_SHIPS];

/*
Returns true if the ship placements form a valid board. If it is a valid
placement, populate the board
*/
extern bool board_add_placement_set(InitialShipDefs ship_defs, Board board);

/*
Attempts to place a single ship onto the board.
Used for building temporary UI boards or step-by-step validation.
Returns true if it fits within bounds and doesn't overlap existing ships.
*/
extern bool board_add_single_ship(InitialShipState isl, Board board);

extern Board create_empty_board(void);
extern void free_board(Board board);

extern void board_mark_strike(Board board, Position pos, bool success);

extern void board_mark_sunk_ship(Board board, ShipType type,
                                 PositionWithDirection pwd);

extern ShipState board_get_ship(Board board, int index);

extern bool valid_attack_pos(Board board, Position pos);

extern bool board_try_hit(Board opponent_ships_board, Position pos,
                          bool *was_hit, ShipType *sunk);
extern bool all_ships_destroyed(Board board);

extern void print_board(Board b, FILE *out);

extern int ship_length(ShipType ship);

extern CellState get_cell(Board b, int x, int y);

#endif
