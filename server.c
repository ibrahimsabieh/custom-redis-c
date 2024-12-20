#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#define BUF_SIZE 1024

void parser(char clientRead) {
    switch(clientRead) {
        case '+':
            //Simple strings
            break;

        case '-':
            // Simple Errors
            break;

        case ':':
            // Integers :[<+|->]<value>\r\n
            // [<+|->] is an optional plus or minus
            break;

        case '$':
            // Bulk strings $<length>\r\n<data>\r\n
            break;

        case '*':
            // Arrays *<number-of-elements>\r\n<element-1>...<element-n>
            break;

        case '_':
            // Nulls
            break;

        case '#':
            // Booleans
            break;

        case ',':
            // Doubles
            break;

        case '(':
            // Big numbers
            break;

        case '!':
            // Bulk errors
            break;

        case '=':
            // Verbatim strings
            break;

        case '%':
            // Maps
            break;

        case '`':
            // Attributes
            break;

        case '~':
            //Sets
            break;

        case '>':
            // Pushes
            break;
        }
}

int create_socket()
{
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    int server_fd;
    struct sockaddr_in client_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Socket creation failed: %s...\n", strerror(errno));
        return 1;
    }

    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        printf("SO_REUSEADDR failed: %s \n", strerror(errno));
        return 1;
    }

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(6379),
        .sin_addr = {htonl(INADDR_ANY)},
    };

    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
        printf("Bind failed: %s \n", strerror(errno));
        return 1;
    }

    int connection_backlog = 5;

    if (listen(server_fd, connection_backlog) != 0) {
        printf("Listen failed: %s \n", strerror(errno));
        return 1;
    }

    printf("Socket Created! \n");

    return server_fd;
}

int accpet_connect(int server_fd)
{

    struct sockaddr_in client_addr;
    socklen_t client_addr_len;

    printf("Waiting for a client to connect...\n");
    client_addr_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    if (client_fd < 0) {
        printf("Accept failed: %s \n", strerror(errno));
        return -1;
    }

    printf("Client connected! \n");

    return client_fd;
}

int handle_connection(int client_fd)
{
    char buffer[BUF_SIZE];
    ssize_t bytesReadFromClient = read(client_fd, buffer, BUF_SIZE);
    unsigned char *p = (void *)buffer;
    // printf("%s\n", p);  // PING
    // printf("%c\n", *p);
    // printf("%s\n", buffer);

    int len = 0;
    p++;
    printf("%p\n", p);
    

    if (*p != '\r')
    {
        len = (len * 10) + (*p - '0');
        p++;
        printf("%d\n", len);
        printf("%s\n", p);
        // printf("%c\n", *p);
        // printf("BYE");
    }
    

    /* Now p points at '\r', and the length is in len. */
    // printf("%d\n", len);

    if (bytesReadFromClient < 0) {
        perror("Read Error, bytes ln 0");
        close(client_fd);
        return -1;
    }

    if (bytesReadFromClient == 0) {
        printf("Client Disconnected FD: %d \n", client_fd);
        close(client_fd);
        return -1;
    }

    char *response = "+PONG\r\n";
    // printf("%s\n", buffer);

    if (send(client_fd, response, strlen(response), 0) < 0) {
        perror("Send error");
        close(client_fd);
        return -1;
    }
    
    return 0;
}

void event_loop(int server_fd)
{

    fd_set current_sockets, ready_sockets;

    FD_ZERO(&current_sockets);
    FD_SET(server_fd, &current_sockets);

    while (1) {
        
        ready_sockets = current_sockets;

        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &ready_sockets)) {
                if (i == server_fd) {
                    int client_fd;
                    if ((client_fd = accpet_connect(server_fd)) < 0) {
                        continue;
                    }
                    FD_SET(client_fd, &current_sockets);
                }
                else 
                {
                    if (handle_connection(i) < 0) {
                        FD_CLR(i, &current_sockets);
                    }
                }
            }
        }
    }
}

int main()
{
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    printf("Logs from your program will appear here!\n");

    // Uncomment this block to pass the first stage
    int server_fd = create_socket();
    event_loop(server_fd);
    close(server_fd);
    return 0;
}