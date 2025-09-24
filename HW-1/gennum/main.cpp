#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include "gennum.h"

int main(int argc, char *argv[]) 
{
    int n, m;
    bool decimal = false, binary = false;

    if (argc == 3)
    {
        m = atoi(argv[1]);
        n = atoi(argv[2]);
    }
    else if (argc == 4)
    {
        m = atoi(argv[1]);
        n = atoi(argv[2]);
        if(strcmp(argv[3], "-f") == 0) 
        {
            decimal = true;
        }
        else if(strcmp(argv[3], "-b") == 0)
        {
            binary = true;
        }
    }

    if(decimal)
        generateDecimal(m, n);
    else if(binary)
        generateBinary(m, n);
    else
        generateInt(m, n);


    return 0;
}
