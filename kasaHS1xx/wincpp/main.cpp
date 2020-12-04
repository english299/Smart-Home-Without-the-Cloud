#include "kasaSocketWin.h"

#include <stdio.h>


int main(int argc, char* argv[])
    {
    kasaSocketWin k;
    k.kasaControl(argv[1], argv[2]);
    printf("%s\n", k.response);
    return 0;
    }

