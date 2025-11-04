//***************************************************************************
//
// Program example for labs in subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2017
//
// Example of socket server.
//
// This program is example of socket server and it allows to connect and serve
// the only one client.
// The mandatory argument of program is port number for listening.
//
//***************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>

#define STR_CLOSE   "close"
#define STR_QUIT    "quit"

//***************************************************************************
// log messages

#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages

// debug flag
int g_debug = LOG_INFO;

void log_msg( int t_log_level, const char *t_form, ... )
{
    const char *out_fmt[] = {
            "ERR: (%d-%s) %s\n",
            "INF: %s\n",
            "DEB: %s\n" };

    if ( t_log_level && t_log_level > g_debug ) return;

    char l_buf[ 1024 ];
    va_list l_arg;
    va_start( l_arg, t_form );
    vsprintf( l_buf, t_form, l_arg );
    va_end( l_arg );

    switch ( t_log_level )
    {
    case LOG_INFO:
    case LOG_DEBUG:
        fprintf( stdout, out_fmt[ t_log_level ], l_buf );
        break;

    case LOG_ERROR:
        fprintf( stderr, out_fmt[ t_log_level ], errno, strerror( errno ), l_buf );
        break;
    }
}

//***************************************************************************
// help

void help( int t_narg, char **t_args )
{
    if ( t_narg <= 1 || !strcmp( t_args[ 1 ], "-h" ) )
    {
        printf(
            "\n"
            "  Socket server example.\n"
            "\n"
            "  Use: %s [-h -d] port_number\n"
            "\n"
            "    -d  debug mode \n"
            "    -h  this help\n"
            "\n", t_args[ 0 ] );

        exit( 0 );
    }

    if ( !strcmp( t_args[ 1 ], "-d" ) )
        g_debug = LOG_DEBUG;
}

//***************************************************************************
// Client handling

void serve_client(int client_sock)
{
    log_msg(LOG_INFO, "Child %d: handling new client", getpid());

    char buffer[256];
    char expressions[10][256];
    int expr_count = 0;

    while (1)
    {
        int len = read(client_sock, buffer, sizeof(buffer));
        if (len == 0)
        {
            log_msg(LOG_INFO, "Client closed connection.");
            break;
        }
        else if (len < 0)
        {
            log_msg(LOG_ERROR, "Unable to read from client.");
            break;
        }

        buffer[len] = 0;
    
        char* line = strtok(buffer, "\n\r");
        while (line)
        {
            if(strlen(line) > 0)
            {
                if (!strncasecmp(buffer, STR_CLOSE, strlen(STR_CLOSE)))
                {
                    log_msg(LOG_INFO, "Client requested to close connection.");
                    break;
                }

                strncpy(expressions[expr_count++], line, sizeof(expressions[0]) - 1);
                expressions[expr_count - 1][sizeof(expressions[0]) - 1] = 0;
            }

            line = strtok(nullptr, "\n\r");
        }

        if(expr_count < 3)
            continue;

        char pyExpr[2000];
        pyExpr[0] = 0;

        for(int i = 0; i < expr_count; i++)
        {
            char line[200];
            snprintf(line, sizeof(line), "print(\"%s =\", %s)\n", expressions[i], expressions[i]);
            strncat(pyExpr, line, sizeof(pyExpr) - strlen(pyExpr) - 1);
        }

        int pipeIN[2];
        int pipeOUT[2];
        pipe(pipeIN);
        pipe(pipeOUT);

        pid_t pid = fork();
        if (pid == 0)
        {
            close(pipeIN[1]);
            close(pipeOUT[0]);

            dup2(pipeIN[0], STDIN_FILENO);
            dup2(pipeOUT[1], STDOUT_FILENO);

            close(pipeIN[0]);
            close(pipeOUT[1]);

            execlp("python3", "python3", (char*)NULL);

            exit(1);
        }
        else if (pid > 0)
        {
            close(pipeIN[0]);
            close(pipeOUT[1]);

            write(pipeIN[1], pyExpr, strlen(pyExpr));
            close(pipeIN[1]);

            char resultBuffer[1000];
            int totalRead = 0;
            
            while (1)
            {
                int rlen = read(pipeOUT[0], resultBuffer + totalRead, sizeof(resultBuffer) - totalRead - 1);
                
                if (rlen <= 0) 
                    break;

                totalRead += rlen;
            }

            if (totalRead > 0)
            {
                resultBuffer[totalRead] = 0;
                write(client_sock, resultBuffer, totalRead);
            }

            close(pipeOUT[0]);
            expr_count = 0;

            wait(NULL);
        }

    }

    close(client_sock);
    log_msg(LOG_INFO, "Child %d closing.", getpid());
    exit(0);
}


//***************************************************************************

int main( int t_narg, char **t_args )
{
    if ( t_narg <= 1 ) help( t_narg, t_args );

    int l_port = 0;

    // parsing arguments
    for ( int i = 1; i < t_narg; i++ )
    {
        if ( !strcmp( t_args[ i ], "-d" ) )
            g_debug = LOG_DEBUG;

        if ( !strcmp( t_args[ i ], "-h" ) )
            help( t_narg, t_args );

        if ( *t_args[ i ] != '-' && !l_port )
        {
            l_port = atoi( t_args[ i ] );
            break;
        }
    }

    if ( l_port <= 0 )
    {
        log_msg( LOG_INFO, "Bad or missing port number %d!", l_port );
        help( t_narg, t_args );
    }

    log_msg( LOG_INFO, "Server will listen on port: %d.", l_port );

    // socket creation
    int l_sock_listen = socket( AF_INET, SOCK_STREAM, 0 );
    if ( l_sock_listen == -1 )
    {
        log_msg( LOG_ERROR, "Unable to create socket.");
        exit( 1 );
    }

    in_addr l_addr_any = { INADDR_ANY };
    sockaddr_in l_srv_addr;
    l_srv_addr.sin_family = AF_INET;
    l_srv_addr.sin_port = htons( l_port );
    l_srv_addr.sin_addr = l_addr_any;

    // Enable the port number reusing
    int l_opt = 1;
    if ( setsockopt( l_sock_listen, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof( l_opt ) ) < 0 )
      log_msg( LOG_ERROR, "Unable to set socket option!" );

    // assign port number to socket
    if ( bind( l_sock_listen, (const sockaddr * ) &l_srv_addr, sizeof( l_srv_addr ) ) < 0 )
    {
        log_msg( LOG_ERROR, "Bind failed!" );
        close( l_sock_listen );
        exit( 1 );
    }

    // listenig on set port
    if ( listen( l_sock_listen, 1 ) < 0 )
    {
        log_msg( LOG_ERROR, "Unable to listen on given port!" );
        close( l_sock_listen );
        exit( 1 );
    }

    log_msg( LOG_INFO, "Enter 'quit' to quit server." );

    // go!
    while ( 1 )
    {
        sockaddr_in l_cli_addr{};
        socklen_t l_cli_len = sizeof( l_cli_addr );

        int l_sock_client = accept(l_sock_listen, (sockaddr *) &l_cli_addr, &l_cli_len);
        if (l_sock_client < 0)
        {
            log_msg(LOG_ERROR, "Unable to accept new connection.");
            continue;
        }

        log_msg(LOG_INFO, "New connection from %s:%d",
                inet_ntoa(l_cli_addr.sin_addr), ntohs(l_cli_addr.sin_port));

        pid_t pid = fork();
        if (pid < 0)
        {
            log_msg(LOG_ERROR, "Fork failed!");
            close(l_sock_client);
            continue;
        }

        if (pid == 0)
        {
            close(l_sock_listen);
            serve_client(l_sock_client);
        }
        else
        {
            close(l_sock_client);
        }
    } // while ( 1 )
    close( l_sock_listen );

    return 0;
}
