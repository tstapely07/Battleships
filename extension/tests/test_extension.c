#include "shared/log.h"
#include "test_board.h"

int main(void) {
    init_logger(NULL);
    test_board();
    close_logger();
}
