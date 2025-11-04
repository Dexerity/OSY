#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <cstring>
#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char** argv) {
    char nameFile[20];
    char outFile[20] = "out2.txt";

    strcpy(nameFile, argv[1]);

    if(argc == 3)
        strcpy(outFile, argv[2]);

    int pipe1[2], pipe2[2];
    pipe(pipe1);
    pipe(pipe2);
    

    if(fork() == 0)
    {
        close(pipe1[0]);
        close(pipe2[0]);
        close(pipe2[1]);

        int x = open(nameFile, O_RDONLY);

        dup2(x, STDIN_FILENO);
        dup2(pipe1[1], STDOUT_FILENO);

        close(pipe1[1]);

        execlp("sort", "sort", nullptr);

        
    }

    if(fork() == 0)
    {
        close(pipe1[1]);
        close(pipe2[0]);

        dup2(pipe1[0], STDIN_FILENO);
        dup2(pipe2[1], STDOUT_FILENO);

        close(pipe1[0]);
        close(pipe2[1]);

        execlp("nl", "nl", "-s", ". ", nullptr);

        
    }

    if(fork() == 0)
    {
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[1]);

        dup2(pipe2[0], STDIN_FILENO);

        int outF = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        dup2(outF, STDOUT_FILENO);

        close(pipe2[0]);
        close(outF);

        execlp("tr", "tr", "a-z", "A-Z", nullptr);

        
    }


    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);









    while (wait(nullptr) > 0);


    return 0;
}