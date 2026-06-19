#include "client/input.h"
#include "shared/log.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#define BUFFER_SIZE 20

// Staging for placing the ships
static int staged_count = 0;
static InitialShipState staged_ships[NUM_SHIPS];

int get_staged_ship_count(void) { return staged_count; }
InitialShipState *get_staged_ships(void) { return staged_ships; }
void reset_staged_ships(void) { staged_count = 0; }

static int input_available(void) {
    struct timeval tv = {0, 0}; // zero timeout = don't wait at all
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

InputData get_user_input(void) {
    InputData data;

    data.type = INPUT_NONE;
    data.grid_pos.x = 0;
    data.grid_pos.y = 0;

    if (!input_available()) {
        return data; // INPUT_NONE
    }

    char input[BUFFER_SIZE];
    if (fgets(input, BUFFER_SIZE, stdin) != NULL) {
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "quit") == 0) {
            data.type = INPUT_QUIT;
            return data;
        }
        // Try parsing ship placement
        char orientation;
        int x, y;
        if (sscanf(input, "%d %d %c", &x, &y, &orientation) == 3) {
            // Save ths ship to local staging array
            staged_ships[staged_count].ship = (ShipType)staged_count;
            staged_ships[staged_count].pwd.pos.x = x;
            staged_ships[staged_count].pwd.pos.y = y;
            staged_ships[staged_count].pwd.horizontal =
                (orientation == 'h' || orientation == 'H');

            staged_count++;

            // If it was the fifth ship then send everything to main game loop
            if (staged_count == NUM_SHIPS) {
                data.type = INPUT_PLACED_SHIPS;
                for (int i = 0; i < NUM_SHIPS; i++) {
                    data.ships[i] = staged_ships[i];
                }

                staged_count = 0; // Reset for future
                return data;
            }
            LOG_DEBUG("%s", "Parsed ship input!");
            return data; // Return INPUT_NONE; we are still building the list.
        } else if (sscanf(input, "%d %d", &x, &y) == 2) {
            // expect grid coords
            data.type = INPUT_FIRE;
            data.grid_pos.x = x;
            data.grid_pos.y = y;

            LOG_DEBUG("%s", "Parsed fire input!");
            return data;
        }

        else {
            LOG_DEBUG("Couldn't parse input: '%s'", input);
        }
    }

    return data;
}
