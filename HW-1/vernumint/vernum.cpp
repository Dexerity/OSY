#include "vernum.h"

void vernum()
{
    char* line;
    size_t len;
    while(getline(&line, &len, stdin) != -1)
    {
        int num, size, sum = 0;
        for(int i = 0; i < strlen(line) - 1; i += size)
        {
            sscanf(&(line[i]), "%d%n", &num, &size);
            sum += num;        
        }

        if(sum - num == num)
            fprintf(stdout, "Sum Valid\n");
        else
            fprintf(stdout, "Sum Invalid\n");
    }
}