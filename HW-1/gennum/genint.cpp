#include "gennum.h"

void generateInt(int m, int n)
{
    srand(time(NULL));

    for (int i = 0; i < n; i++)
    {
        int numCount = rand() % m + 1;
        int sum = 0;
        for (int i = 0; i < numCount; i++)
        {
            int num = rand() % 10000;
            fprintf(stdout, "%d ", num);
            sum += num;
        }
        fprintf(stdout, "%d\n", sum);
    }
}