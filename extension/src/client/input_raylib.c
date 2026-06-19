#include "input.h"

InputData input_state;

InputData get_user_input(void) {
    InputData current = input_state;

    input_state.type = INPUT_NONE;

    return current;
}

extern void reset_ui_ships(void);

void reset_staged_ships(void) {
    reset_ui_ships();

    input_state.type = INPUT_NONE;
}
