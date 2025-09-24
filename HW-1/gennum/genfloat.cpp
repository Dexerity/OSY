#include "gennum.h"


void generateDecimal(int m, int n)
{
    srand(time(NULL));

    for (int i = 0; i < n; i++)
    {
        int numCount = rand() % m + 1;
        float sum = 0;
        for (int i = 0; i < numCount; i++)
        {
            float num = (rand() % 10000) / 100.0;
            fprintf(stdout, "%.2f ", num);
            sum += num;
        }
        fprintf(stdout, "%.2f\n", sum);
    }
}