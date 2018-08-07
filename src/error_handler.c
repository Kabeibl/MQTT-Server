#include "error_handler.h"
#include <unistd.h>
#include <stdlib.h>

/* Error handler */
void handle_error(int error, int type, WINDOW** original_win) {

    print_error(error, type, original_win);   // Print error
}