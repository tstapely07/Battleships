#include "test_board.h"
#include "shared/board.h"
#include <assert.h>

void test_invalid_placement(void) {
    Board b = create_empty_board();
    InitialShipState defs[5];
    Position pos = {.x = 0, .y = 0};
    PositionWithDirection pwd = {.pos = pos, .horizontal = true};

    InitialShipState sl0 = {.ship = SHIP_CARRIER, .pwd = pwd};

    pwd.pos.x = 1;
    pwd.pos.y = 0;
    InitialShipState sl1 = {.ship = SHIP_BATTLESHIP, .pwd = pwd};

    pwd.pos.x = 5;
    pwd.pos.y = 0;
    InitialShipState sl2 = {.ship = SHIP_CRUISER, .pwd = pwd};

    pwd.pos.x = 0;
    pwd.pos.y = 2;
    pwd.horizontal = false;
    InitialShipState sl3 = {.ship = SHIP_SUBMARINE, .pwd = pwd};

    pwd.pos.x = 1;
    pwd.pos.y = 5;
    InitialShipState sl4 = {.ship = SHIP_DESTROYER, .pwd = pwd};

    defs[0] = sl0;
    defs[1] = sl1;
    defs[2] = sl2;
    defs[3] = sl3;
    defs[4] = sl4;
    assert(!board_add_placement_set(defs, b));

    sl1.pwd.pos.y = 7;
    defs[1] = sl1;

    sl4.pwd.pos.y = 8;
    defs[4] = sl4;
    assert(!board_add_placement_set(defs, b));

    free_board(b);
    printf("Check invalid placement: OK\n");
}

void test_valid_placement(Board b) {
    InitialShipState defs[5];
    Position pos = {.x = 0, .y = 0};
    PositionWithDirection pwd = {.pos = pos, .horizontal = true};

    InitialShipState sl0 = {.ship = SHIP_CARRIER, .pwd = pwd};

    pwd.pos.x = 0;
    pwd.pos.y = 1;
    InitialShipState sl1 = {.ship = SHIP_BATTLESHIP, .pwd = pwd};

    pwd.pos.x = 5;
    pwd.pos.y = 0;
    InitialShipState sl2 = {.ship = SHIP_CRUISER, .pwd = pwd};

    pwd.pos.x = 0;
    pwd.pos.y = 2;
    pwd.horizontal = false;
    InitialShipState sl3 = {.ship = SHIP_SUBMARINE, .pwd = pwd};

    pwd.pos.x = 1;
    pwd.pos.y = 5;
    InitialShipState sl4 = {.ship = SHIP_DESTROYER, .pwd = pwd};

    defs[0] = sl0;
    defs[1] = sl1;
    defs[2] = sl2;
    defs[3] = sl3;
    defs[4] = sl4;
    assert(board_add_placement_set(defs, b));
    printf("Check valid placement: OK\n");
}

void test_board() {
    printf("===Testing Board===\n");

    Board b = create_empty_board();
    assert(b != NULL);
    printf("Create board: OK\n");

    test_invalid_placement();
    print_board(b, stdout);
    test_valid_placement(b);
    print_board(b, stdout);

    Position p = {
        .x = 1,
        .y = 5,
    };

    bool was_hit;
    ShipType sunk;
    assert(board_try_hit(b, p, &was_hit, &sunk));
    assert(was_hit);

    p.y = 6;
    assert(board_try_hit(b, p, &was_hit, &sunk));
    assert(was_hit);
    assert(sunk == SHIP_DESTROYER);

    p.x = 5;
    assert(board_try_hit(b, p, &was_hit, &sunk));

    assert(!all_ships_destroyed(b));

    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            p.x = i;
            p.y = j;
            board_try_hit(b, p, &was_hit, &sunk);
        }
    }

    assert(all_ships_destroyed(b));
    free_board(b);

    printf("Board gameplay logic: OK\n");
    printf("===BOARD TESTS PASSED===\n");
}
