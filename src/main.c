#include <stdio.h>

#include "overlay.h"
#include "uiaccess.h"

int main(void) {
    if (PrepareForUIAccess() != ERROR_SUCCESS) {
        printf("There was an error trying to run the program: %lu", GetLastError());
        return 1;
    }

    OV_Render();
    return 0;
}