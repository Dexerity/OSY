#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <cstring>

using namespace std;

#define MAX_NAME_LENGTH 21

void readNames(const char* filename, char names[200][MAX_NAME_LENGTH]) {
    FILE *file = fopen(filename, "r");
    char line[MAX_NAME_LENGTH];
    int count = 0;
    while (fgets(line, sizeof(line), file)) 
    {
        strcpy(names[count++], line);
    }
}


int main(int argc, char *argv[]) {
    char names[200][MAX_NAME_LENGTH];
    readNames(argv[1], names);

    // for (int i = 0; i < 200; i++)
    // {
    //     printf("%s", names[i]);
    // }

    srand(time(nullptr));

    int pipe1[2], pipe2[2], pipe3[2];
    pipe(pipe1);
    pipe(pipe2);
    pipe(pipe3);

    //CHILD 1
    if (fork() == 0)
    {
        close(pipe1[0]);
        close(pipe2[0]);
        close(pipe2[1]);
        close(pipe3[0]);
        close(pipe3[1]);

        int count = (getpid() % 100) + 10;

        for (int i = 0; i < count; i++)
        {
            int index = rand() % 200;
            write(pipe1[1], names[index], MAX_NAME_LENGTH);
            usleep(75000);
        }
        
        close(pipe1[1]);
        exit(0);
    }

    //CHILD 2
    if (fork() == 0)
    {
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe3[0]);
        close(pipe3[1]);

        
        char buffer[MAX_NAME_LENGTH];
        size_t bytes;
        int count = 0;

        while ((bytes = read(pipe1[0], buffer, MAX_NAME_LENGTH)) > 0)
        {
            char modified[MAX_NAME_LENGTH + 10];
            sprintf(modified, "%d: %s", count++, buffer);
            modified[strcspn(modified, "\n")] = '\0';
            write(pipe2[1], modified, strlen(modified) + 1);
        }
        
        close(pipe1[0]);
        close(pipe2[1]);
        exit(0);
    }
    
    //CHILD 3
    if (fork() == 0)
    {   
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[1]);
        close(pipe3[0]);

        char buffer[MAX_NAME_LENGTH + 10];
        size_t bytes;

        while ((bytes = read(pipe2[0], buffer, sizeof(buffer))) > 0)
        {
            char* name = strchr(buffer, ' ');
            int nameLength = strlen(name + 1);
            char modified[MAX_NAME_LENGTH + 20];
            sprintf(modified, "%s (%d)", buffer, nameLength);

            write(pipe3[1], modified, strlen(modified) + 1);
        }


        
        close(pipe2[0]);
        close(pipe3[1]);
        exit(0);
    }

    //PARENT
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);
    close(pipe3[1]);

    char buffer[MAX_NAME_LENGTH + 20];
    size_t bytes;
    while ((bytes = read(pipe3[0], buffer, sizeof(buffer))) > 0)
    {
        char nameTmp[MAX_NAME_LENGTH + 20];
        strcpy(nameTmp, buffer);
        char* token = strtok(nameTmp, " ");
        token = strtok(nullptr, " ");
        char name[MAX_NAME_LENGTH];
        strcpy(name, token);


        for (int i = 0; i < 200; i++)
        {
            char* newNames = names[i];
            newNames[strcspn(newNames, "\n")] = '\0';
            if(strcmp(name, newNames) == 0)
            {
                buffer[strcspn(buffer, "\n")] = '\0';
                fprintf(stdout, "%s - %d\n", buffer, i + 1);
            }
        }
        

    }

    close(pipe3[0]);

    while (wait(nullptr) > 0);
    
    return 0;
}
