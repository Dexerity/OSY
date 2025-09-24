#include "gennum.h"


void generateBinary(int m, int n)
{
    srand(time(NULL));

    for (int i = 0; i < n; i++)
    {
        int numCount = rand() % m + 1;
        int sum = 0;
        for (int i = 0; i < numCount; i++)
        {
            int num = (rand() % 10000) / 100.0;
            fprintf(stdout, "0b%b ", num);
            sum += num;
        }
        fprintf(stdout, "%b\n", sum);
    }
}