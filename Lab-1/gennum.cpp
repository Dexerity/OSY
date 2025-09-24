#include <stdlib.h>
#include <cstdio>
#include <cstring>

int main(int argc, char *argv[])
{
    long s;
    int n;
    bool binary = false;

    if(argc == 2)
    {
        s = atol(argv[1]);
        n = 1000;
    }
    else if(argc == 3)
    {
        s = atol(argv[1]);
        
        if(strcmp("-b", argv[2]) == 0)
            binary = true;

        if(!binary)
            n = atol(argv[2]);

    }
    else if(argc == 4)
    {
        s = atol(argv[1]);

        for(int i = 2; i < 4; i++)
        {
            if(strcmp("-b", argv[i]) == 0)
            {
                binary = true;
                if(i == 2)
                    n = atol(argv[3]);
                else
                    n = atol(argv[2]);
            }
        }
    }

    for(int i = 0; i < n; i++)
    {
        if(binary)
        {
            fwrite(&s, sizeof(long), 1, stdout);
        }
        else
        {
            fprintf(stdout, "%ld\n", s);
        }
        s++;
    }



    return 0;
}