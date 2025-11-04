#include <stdio.h>
#include <stdlib.h>
#include <cstring>

int main(int argc, char *argv[]) {
    char options[3] = {0}; // Initialize options array
    char* directory = "."; // Default to current directory

    if (argc == 2) 
    {
        directory = argv[1];
    }
    else if (argc > 2) 
    {
        for (int i = 1; i < argc - 1; ++i) 
        {
            if (strcmp(argv[i], "-s") == 0) 
            {
                options[i - 1] = 's';
            } 
            else if (strcmp(argv[i], "-t") == 0) 
            {
                options[i - 1] = 't';
            } 
            else if (strcmp(argv[i], "-r") == 0) 
            {
                options[i - 1] = 'r';
            }
        }
        directory = argv[argc - 1];
    }


    

    return 0;
}