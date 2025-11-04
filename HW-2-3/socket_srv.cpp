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
#include <semaphore.h>
#include <vector>
#include <string>
#include <pthread.h>

#define STR_CLOSE   "close"
#define STR_QUIT    "quit"

#define BUFFER_SIZE 100

std::vector<std::string> buffer;

sem_t *g_sem_empty = nullptr;
sem_t *g_sem_full = nullptr;
sem_t *g_sem_mutex = nullptr;

//***************************************************************************

void producer(std::string *item)
{
    sem_wait(g_sem_empty);
    sem_wait(g_sem_mutex);

    buffer.push_back(*item);
    printf("[SRV] Produced: %s\n", item->c_str());

    sem_post(g_sem_mutex);
    sem_post(g_sem_full);
}

void consumer(std::string *item)
{
    sem_wait(g_sem_full);
    sem_wait(g_sem_mutex);

    *item = buffer.front();
    buffer.erase(buffer.begin());
    printf("[SRV] Consumed: %s\n", item->c_str());

    sem_post(g_sem_mutex);
    sem_post(g_sem_empty);
}

//****************************************************************************

void* producer_client(void* arg)
{
    int sock = *((int*)arg);
    delete (int*)arg;
    char buf[256];

    fprintf(stdout, "[SRV] Producer client started.\n");

    while (1)
    {
        int len = read(sock, buf, sizeof(buf) - 1);
        if (len <= 0)
            break; 

        buf[len] = '\0';


        std::string item(buf);
        item.erase(item.find_last_not_of(" \n\r\t") + 1);

        if (item == STR_CLOSE || item == STR_QUIT)
            break;


        producer(&item);
        write(sock, "OK\n", 3);
    }

    close(sock);
    return nullptr;
}

void* consumer_client(void* arg)
{
    int sock = *((int*)arg);
    delete (int*)arg;
    char buf[256];

    fprintf(stdout, "[SRV] Consumer client started.\n");

    while (1)
    {
        if(buffer.empty())
            break;

        std::string item;
        consumer(&item);
        item += "\n";
        write(sock, item.c_str(), item.size());

        int len = read(sock, buf, sizeof(buf) - 1);
        if (len <= 0)
            break;
        buf[len] = '\0';
        std::string response(buf);
        if (strncmp(response.c_str(), "OK", 2) != 0)
            break;
    }

    close(sock);
    return nullptr;
}

//***************************************************************************

void* handle_client(void* arg)
{
    int sock = *((int*)arg);
    write(sock, "Task?\n", 6);

    fprintf(stdout, "[SRV] Waiting for client task...\n");

    char buf[256];
    int len = read(sock, buf, sizeof(buf) - 1);
    if (len <= 0)
    {
        close(sock);
        return nullptr;
    }

    buf[len] = '\0';
    std::string task(buf);
    task.erase(task.find_last_not_of(" \n\r\t") + 1);

    fprintf(stdout, "[SRV] Client task: %s\n", task.c_str());

    if (task == "producer")
        producer_client(arg);
    else if (task == "consumer")
        consumer_client(arg);
    else
        close(sock);

    return nullptr;
}

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

    sem_unlink("/sem_empty");
    sem_unlink("/sem_full");
    sem_unlink("/sem_mutex");

    g_sem_empty = sem_open("/sem_empty", O_CREAT, 0644, BUFFER_SIZE);
    if (g_sem_empty == SEM_FAILED) {
        log_msg(LOG_ERROR, "sem_open /sem_empty failed");
        exit(1);
    }
    g_sem_full = sem_open("/sem_full", O_CREAT, 0644, 0);
    if (g_sem_full == SEM_FAILED) {
        log_msg(LOG_ERROR, "sem_open /sem_full failed");
        exit(1);
    }
    g_sem_mutex = sem_open("/sem_mutex", O_CREAT, 0644, 1);
    if (g_sem_mutex == SEM_FAILED) {
        log_msg(LOG_ERROR, "sem_open /sem_mutex failed");
        exit(1);
    }
    g_sem_mutex = sem_open("/sem_mutex", O_CREAT, 0644, 1);




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
        int l_sock_client = -1;

        // list of fd sources
        pollfd l_read_poll[ 2 ];

        l_read_poll[ 0 ].fd = STDIN_FILENO;
        l_read_poll[ 0 ].events = POLLIN;
        l_read_poll[ 1 ].fd = l_sock_listen;
        l_read_poll[ 1 ].events = POLLIN;

        while ( 1 ) // wait for new client
        {
            // select from fds
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
            }

            if ( l_read_poll[ 1 ].revents & POLLIN )
            { // new client?
                sockaddr_in l_rsa;
                int l_rsa_size = sizeof( l_rsa );
                // new connection
                l_sock_client = accept( l_sock_listen, ( sockaddr * ) &l_rsa, ( socklen_t * ) &l_rsa_size );
                if ( l_sock_client == -1 )
                {
                    log_msg( LOG_ERROR, "Unable to accept new client." );
                    close( l_sock_listen );
                    exit( 1 );
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

                int* client_sock = new int(l_sock_client);

                fprintf(stdout, "[SRV] Client connected, creating thread.\n");

                pthread_t client_thread;
                pthread_create( &client_thread, nullptr, handle_client, ( void* ) client_sock );
                pthread_detach( client_thread );

                fprintf(stdout, "[SRV] Thread created.\n");

                break;
            }

        } // while wait for client
    } // while ( 1 )

    close( l_sock_listen );

    sem_close(g_sem_empty);
    sem_close(g_sem_full);
    sem_close(g_sem_mutex);

    sem_unlink("/sem_empty");
    sem_unlink("/sem_full");
    sem_unlink("/sem_mutex");

    return 0;
}
