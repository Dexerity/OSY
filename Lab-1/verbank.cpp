#include <stdlib.h>
#include <cstring>
#include <cstdio>


int main(int argc, char *argv[])
{
    bool verOnly = false;
    bool binary = false;

    if(argc == 2)
    {
        if(strcmp("-v", argv[1]) == 0)
        {
            verOnly = true;
        }
        else if(strcmp("-b", argv[1]) == 0)
        {
            binary = true;
        }
    }
    else if(argc == 3)
    {
        if(strcmp("-v", argv[1]) == 0 || strcmp("-v", argv[2]) == 0)
            verOnly = true;
        if(strcmp("-b", argv[1]) == 0 || strcmp("-b", argv[2]) == 0)
            binary = true;
    }

    long num;

    int vahy[] = { 1, 2, 4, 8, 5, 10, 9, 7, 3, 6 };
    int weightPos = 0;

    if(binary)
    {
        while(fread(&num, sizeof(long), 1, stdin)) 
        {
            long numCpy = num;
            int sum = 0;
            while(num > 0)
            {
                sum += num % 10 * vahy[weightPos];
                num /= 10;

                weightPos++;
            }

            if(sum % 11 == 0)
            {
                printf("%ld - Valid\n", numCpy);
            }
            else if(!verOnly)
            {
                printf("%ld - Invalid\n", numCpy);
            }

            weightPos = 0;
        }
    }
    else
    {
        while (fscanf(stdin, "%ld", &num) != EOF) 
        {
            long numCpy = num;
            int sum = 0;
            while(num > 0)
            {
                sum += num % 10 * vahy[weightPos];
                num /= 10;

                weightPos++;
            }

            if(sum % 11 == 0)
            {
                printf("%ld - Valid\n", numCpy);
            }
            else if(!verOnly)
            {
                printf("%ld - Invalid\n", numCpy);
            }

            weightPos = 0;
        }
    }





    return 0;
}