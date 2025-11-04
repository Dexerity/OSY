//***************************************************************************
//
// Program example for subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2021
//
// Example of socket server/client.
//
// This program is example of socket client.
// The mandatory arguments of program is IP adress or name of server and
// a port number.
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
#include <vector>
#include <string>
#include <netdb.h>
#include <pthread.h>

#define STR_CLOSE               "close"

int producing_speed = 60;

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
            "  Socket client example.\n"
            "\n"
            "  Use: %s [-h -d] ip_or_name port_number\n"
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

void* producer_thread(void* arg)
{
    int sock = *((int*)arg);
    delete (int*)arg;
    char names[201][20];
    char* nameFile = "jmena.txt";

    FILE* file = fopen(nameFile, "r");
    if (file == nullptr)
    {
        log_msg( LOG_ERROR, "Unable to open file %s.", nameFile );
        close(sock);
        return nullptr;
    }

    int count = 0;
    while (fgets(names[count], sizeof(names[count]), file) != nullptr && count < 200)
    {
        names[count][strcspn(names[count], "\n")] = '\0';
        count++;
    }
    fclose(file);

    for(int i = 0; i < count; i++)
    {
        usleep((60.0 / producing_speed) * 1000000);

        write(sock, names[i], strlen(names[i]));

        char buf[256];
        ssize_t len = read(sock, buf, sizeof(buf) - 1);
        if (len <= 0)
            break;

        buf[len] = '\0';

        if (strncmp(buf, "OK\n", 3) != 0)
            break;
    }

    fprintf(stdout, "[CL] Producer thread finished.\n");

    close(sock);
    return nullptr;
}

void* consumer_thread(void* arg)
{
    int sock = *((int*)arg);
    delete (int*)arg;

    while (true)
    {
        char buf[256];
        ssize_t len = read(sock, buf, sizeof(buf) - 1);
        if (len <= 0)
            break;

        buf[len] = '\0';

        fprintf(stdout, "%s", buf);

        write(sock, "OK\n", 3);
    }

    fprintf(stdout, "[CL] Consumer thread finished.\n");

    close(sock);

    return nullptr;
}


static int connect_and_send_task(sockaddr_in server_addr, const char* task, ssize_t task_len)
{
    int sock = socket( AF_INET, SOCK_STREAM, 0 );
    if ( sock == -1 )
    {
        log_msg( LOG_ERROR, "Unable to create socket." );
        return -1;
    }

    if ( connect( sock, ( sockaddr * ) &server_addr, sizeof( server_addr ) ) < 0 )
    {
        log_msg( LOG_ERROR, "Unable to connect server." );
        close(sock);
        return -1;
    }

    char l_buf[256];
    ssize_t l_len = read(sock, l_buf, sizeof(l_buf) - 1);
    if (l_len > 0)
    {
        l_buf[l_len] = '\0';
        //write(STDOUT_FILENO, l_buf, l_len);
    }

    if ( write(sock, task, task_len) < 0 )
    {
        log_msg( LOG_ERROR, "Unable to write task to server." );
        close(sock);
        return -1;
    }

    return sock;
}


//***************************************************************************

int main( int t_narg, char **t_args )
{

    if ( t_narg <= 2 ) help( t_narg, t_args );

    int l_port = 0;
    char *l_host = nullptr;

    // parsing arguments
    for ( int i = 1; i < t_narg; i++ )
    {
        if ( !strcmp( t_args[ i ], "-d" ) )
            g_debug = LOG_DEBUG;

        if ( !strcmp( t_args[ i ], "-h" ) )
            help( t_narg, t_args );

        if ( *t_args[ i ] != '-' )
        {
            if ( !l_host )
                l_host = t_args[ i ];
            else if ( !l_port )
                l_port = atoi( t_args[ i ] );
        }
    }

    if ( !l_host || !l_port )
    {
        log_msg( LOG_INFO, "Host or port is missing!" );
        help( t_narg, t_args );
        exit( 1 );
    }

    log_msg( LOG_INFO, "Connection to '%s':%d.", l_host, l_port );

    addrinfo l_ai_req, *l_ai_ans;
    bzero( &l_ai_req, sizeof( l_ai_req ) );
    l_ai_req.ai_family = AF_INET;
    l_ai_req.ai_socktype = SOCK_STREAM;

    int l_get_ai = getaddrinfo( l_host, nullptr, &l_ai_req, &l_ai_ans );
    if ( l_get_ai )
    {
        log_msg( LOG_ERROR, "Unknown host name!" );
        exit( 1 );
    }

    sockaddr_in l_cl_addr =  *( sockaddr_in * ) l_ai_ans->ai_addr;
    l_cl_addr.sin_port = htons( l_port );
    freeaddrinfo( l_ai_ans );

    // socket creation
    int l_sock_server = socket( AF_INET, SOCK_STREAM, 0 );
    if ( l_sock_server == -1 )
    {
        log_msg( LOG_ERROR, "Unable to create socket.");
        exit( 1 );
    }

    // connect to server
    if ( connect( l_sock_server, ( sockaddr * ) &l_cl_addr, sizeof( l_cl_addr ) ) < 0 )
    {
        log_msg( LOG_ERROR, "Unable to connect server." );
        exit( 1 );
    }

    uint l_lsa = sizeof( l_cl_addr );
    // my IP
    getsockname( l_sock_server, ( sockaddr * ) &l_cl_addr, &l_lsa );
    log_msg( LOG_INFO, "My IP: '%s'  port: %d",
             inet_ntoa( l_cl_addr.sin_addr ), ntohs( l_cl_addr.sin_port ) );
    // server IP
    getpeername( l_sock_server, ( sockaddr * ) &l_cl_addr, &l_lsa );
    log_msg( LOG_INFO, "Server IP: '%s'  port: %d",
             inet_ntoa( l_cl_addr.sin_addr ), ntohs( l_cl_addr.sin_port ) );

    log_msg( LOG_INFO, "Enter 'close' to close application." );

    // list of fd sources
    pollfd l_read_poll[ 2 ];

    l_read_poll[ 0 ].fd = STDIN_FILENO;
    l_read_poll[ 0 ].events = POLLIN;
    l_read_poll[ 1 ].fd = l_sock_server;
    l_read_poll[ 1 ].events = POLLIN;

    // go!
    while ( 1 )
    {
        char l_buf[ 128 ];

        // select from fds
        if ( poll( l_read_poll, 2, -1 ) < 0 ) break;

        // data on stdin?
        if ( l_read_poll[ 0 ].revents & POLLIN )
        {
            //  read from stdin
            int l_len = read( STDIN_FILENO, l_buf, sizeof( l_buf ) );
            if ( l_len == 0 )
            {
                log_msg( LOG_DEBUG, "Stdin closed." );
                break;
            }
            if ( l_len < 0 )
            {
                log_msg( LOG_ERROR, "Unable to read from stdin." );
                break;
            }
            else
                log_msg( LOG_DEBUG, "Read %d bytes from stdin.", l_len );

            if(strncmp(l_buf, "producer\n", 9) == 0)
            {
                int new_sock = connect_and_send_task(l_cl_addr, l_buf, l_len);
                if (new_sock < 0) continue;

                pthread_t prod_thread;
                pthread_create(&prod_thread, nullptr, producer_thread, new int(new_sock));
                pthread_detach(prod_thread);

                continue;
            }
            else if(strncmp(l_buf, "consumer\n", 9) == 0)
            {
                int new_sock = connect_and_send_task(l_cl_addr, l_buf, l_len);
                if (new_sock < 0) continue;

                pthread_t cons_thread;
                pthread_create(&cons_thread, nullptr, consumer_thread, new int(new_sock));
                pthread_detach(cons_thread);

                continue;
            }

            producing_speed = atoi(l_buf);
        }

        // data from server?
        if ( l_read_poll[ 1 ].revents & POLLIN )
        {
            // read data from server
            int l_len = read( l_sock_server, l_buf, sizeof( l_buf ) );
            if ( l_len == 0 )
            {
                log_msg( LOG_DEBUG, "Server closed socket." );
                break;
            }
            else if ( l_len < 0 )
            {
                log_msg( LOG_ERROR, "Unable to read data from server." );
                break;
            }
            else
                log_msg( LOG_DEBUG, "Read %d bytes from server.", l_len );

            // display on stdout
            l_len = write( STDOUT_FILENO, l_buf, l_len );
            if ( l_len < 0 )
            {
                log_msg( LOG_ERROR, "Unable to write to stdout." );
                break;
            }

            // request to close?
            if ( !strncasecmp( l_buf, STR_CLOSE, strlen( STR_CLOSE ) ) )
            {
                log_msg( LOG_INFO, "Connection will be closed..." );
                break;
            }
        }
    }

    // close socket
    close( l_sock_server );

    return 0;
  }
