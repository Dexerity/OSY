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
#include <pthread.h>

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

int g_clients[10];
int g_client_count = 0;

void add_client(int sock) 
{
    if (g_client_count < 10) 
    {
        g_clients[g_client_count++] = sock;
    }
    else
    {
        log_msg( LOG_ERROR, "Maximum client limit reached!" );
        close(sock);
    }
}

void remove_client(int sock) 
{
    for (int i = 0; i < g_client_count; i++) 
    {
        if (g_clients[i] == sock) 
        {
            g_clients[i] = g_clients[--g_client_count];
            return;
        }
    }
}

void broadcast_message(const char* msg, int len, int sender_sock) 
{
    for (int i = 0; i < g_client_count; i++) 
    {
        if (g_clients[i] != sender_sock) 
        {
            int w = write(g_clients[i], msg, len);
            if (w < 0) 
            {
                log_msg( LOG_ERROR, "Unable to send data to client %d.", g_clients[i] );
            } 
        }
    }
}

//***************************************************************************


void* client_thread( void* arg )
{
    int sock = *(int*)arg;
    free(arg);
    char nickname[32];

    log_msg( LOG_INFO, "Client thread started.");

    add_client(sock);

    char buffer[256];

    const char* prompt = "Enter Nickname: ";
    if (write(sock, prompt, strlen(prompt)) < 0) 
    {
        log_msg(LOG_ERROR, "Failed to send nickname prompt to client.");
        remove_client(sock);
        close(sock);
        return nullptr;
    }

    int nlen = read(sock, nickname, sizeof(nickname) - 1);
    if (nlen <= 0) 
    {
        log_msg(LOG_ERROR, "Failed to read nickname from client.");
        remove_client(sock);
        close(sock);
        return nullptr;
    }
    nickname[nlen - 1] = '\0';

    while (1)
    {
        int len = read(sock, buffer, sizeof(buffer));
        if (len == 0)
        {
            log_msg( LOG_INFO, "Client closed socket.");
            break;
        }
        else if (len < 0)
        {
            log_msg( LOG_ERROR, "Unable to read data from client." );
            break;
        }

        buffer[len] = '\0';
        char msg_with_nick[300];
        snprintf(msg_with_nick, sizeof(msg_with_nick), "%s: %s", nickname, buffer);

        int w = write( STDOUT_FILENO, msg_with_nick, strlen(msg_with_nick) );
        if ( w < 0 )
        {
            log_msg( LOG_ERROR, "Unable to write data to stdout." );
            break;
        }

        if (!strncasecmp(buffer, STR_CLOSE, strlen(STR_CLOSE)))
        {
            log_msg( LOG_INFO, "Closing connection.");
            break;
        }

        

        broadcast_message(msg_with_nick, strlen(msg_with_nick), sock);
    }

    remove_client(sock);
    close(sock);
    log_msg( LOG_INFO, "Client thread finished.");
    return nullptr;
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

    pollfd l_read_poll[ 2 ];

    l_read_poll[ 0 ].fd = STDIN_FILENO;
    l_read_poll[ 0 ].events = POLLIN;
    l_read_poll[ 1 ].fd = l_sock_listen;
    l_read_poll[ 1 ].events = POLLIN;

    while ( 1 ) // wait for new client
    {
        int l_poll = poll( l_read_poll, 2, -1 );

        if ( l_poll < 0 )
        {
            log_msg( LOG_ERROR, "Function poll failed!" );
            exit( 1 );
        }

        if ( l_read_poll[ 0 ].revents & POLLIN )
        { // data on stdin
            char buf[ 128 ];
            
            int l_len = read( STDIN_FILENO, buf, sizeof( buf) );
            if ( l_len == 0 )
            {
                log_msg( LOG_DEBUG, "Stdin closed." );
                exit( 0 );
            }
            if ( l_len < 0 )
            {
                log_msg( LOG_DEBUG, "Unable to read from stdin!" );
                exit( 1 );
            }

            log_msg( LOG_DEBUG, "Read %d bytes from stdin", l_len );

            // request to quit?
            if ( !strncmp( buf, STR_QUIT, strlen( STR_QUIT ) ) )
            {
                log_msg( LOG_INFO, "Request to 'quit' entered.");
                close( l_sock_listen );
                exit( 0 );
            }

            buf[l_len] = 0;
            char message[256];
            snprintf(message, sizeof(message), "SERVER MSG: %s", buf);

            for (int i = 0; i < g_client_count; i++) 
            {
                int w = write(g_clients[i], message, strlen(message));
                if (w < 0) 
                {
                    log_msg( LOG_ERROR, "Unable to send data to client %d.", g_clients[i] );
                } 
            }
        }

        if ( l_read_poll[ 1 ].revents & POLLIN )
        { // new client?
            sockaddr_in l_rsa;
            int l_rsa_size = sizeof( l_rsa );
            // new connection
            int l_sock_client = accept( l_sock_listen, ( sockaddr * ) &l_rsa, ( socklen_t * ) &l_rsa_size );
            if ( l_sock_client == -1 )
            {
                log_msg( LOG_ERROR, "Unable to accept new client." );
                continue;
            }

            uint l_lsa = sizeof( l_srv_addr );
            // my IP
            getsockname( l_sock_client, ( sockaddr * ) &l_srv_addr, &l_lsa );
            log_msg( LOG_INFO, "My IP: '%s'  port: %d",
                                inet_ntoa( l_srv_addr.sin_addr ), ntohs( l_srv_addr.sin_port ) );
            // client IP
            getpeername( l_sock_client, ( sockaddr * ) &l_srv_addr, &l_lsa );
            log_msg( LOG_INFO, "Client IP: '%s'  port: %d",
                                inet_ntoa( l_srv_addr.sin_addr ), ntohs( l_srv_addr.sin_port ) );


            int* cs = (int*)malloc(sizeof(int));
            *cs = l_sock_client;

            pthread_t th;
            int ret = pthread_create(&th, nullptr, client_thread, cs);
            if (ret != 0)
            {
                log_msg(LOG_ERROR, "Unable to create thread for new client.");
                close(l_sock_client);
                free(cs);
                continue;
            }
            pthread_detach(th);
        }
    }  

    for (int i = 0; i < g_client_count; i++) 
    {
        close(g_clients[i]);
    }
    close(l_sock_listen);


    return 0;
}
