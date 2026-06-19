#include "test_board.h"
#include "shared/log.h"

int main(void) { 
    init_logger(NULL);
    test_board(); 
    close_logger();
}
