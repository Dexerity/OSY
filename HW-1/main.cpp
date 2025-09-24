#include <stdio.h>
#include <stdlib.h>
#include <cstring>

void generateInt(int m, int n);
void generateDecimal(int m, int n);

int main(int argc, char *argv[])
{
    int n, m;
    bool decimal = false;

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
    }

    if(decimal)
        generateDecimal(m, n);
    else
        generateInt(m, n);
    return 0;
}