#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <uv.h>

#define BUF_SIZE 1024

// 1. Forward declarations of callback functions
static void on_new_connection(uv_stream_t *server, int status);
static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);
static void on_client_closed(uv_handle_t *handle);
static void on_write_done(uv_write_t *req, int status);

// 2. Main entry point (replaces select()-based event_loop)
//
//    - Initializes libuv
//    - Creates/binds a TCP server
//    - Listens for new connections
//    - Runs the libuv loop (blocking)
int main()
{
    // Disable output buffering (as in your original code)
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    // Create a libuv event loop
    uv_loop_t *loop = uv_default_loop();

    // Create a TCP server handle
    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    // Bind to 0.0.0.0:6379
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", 6379, &addr);
    uv_tcp_bind(&server, (const struct sockaddr *)&addr, 0);

    // Start listening; on_new_connection is called when clients connect
    int r = uv_listen((uv_stream_t *)&server, 128, on_new_connection);
    if (r)
    {
        fprintf(stderr, "Listen error: %s\n", uv_strerror(r));
        return 1;
    }

    printf("Server listening on port 6379...\n");

    // Run the libuv loop (blocks until the server is stopped)
    uv_run(loop, UV_RUN_DEFAULT);

    // Cleanup once the loop exits
    uv_loop_close(loop);
    return 0;
}


// 3. Callback: Called when a new client connects
//
//    - Allocate a uv_tcp_t for the client
//    - Accept the connection
//    - Start reading data (on_read will be called upon incoming data)
static void on_new_connection(uv_stream_t *server, int status)
{
    if (status < 0)
    {
        fprintf(stderr, "New connection error: %s\n", uv_strerror(status));
        return;
    }

    // Allocate a new uv_tcp_t for the incoming client
    uv_tcp_t *client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(server->loop, client);

    // Accept the connection
    if (uv_accept(server, (uv_stream_t *)client) == 0)
    {
        printf("Client connected!\n");
        // Begin reading from this client
        uv_read_start((uv_stream_t *)client, alloc_buffer, on_read);
    }
    else
    {
        // If we failed to accept, close and free the client object
        uv_close((uv_handle_t *)client, on_client_closed);
    }
}

// 4. Callback: libuv calls this to allocate a buffer before reading data
static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    // Allocate a buffer for reading
    buf->base = (char *)malloc(suggested_size);
    buf->len = suggested_size;
}

// 5. Callback: Called when data is available on the client
//    - nread < 0 => Error or EOF
//    - nread > 0 => We have data in buf->base
//    - We replicate your handle_connection logic here
static void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)
{
    if (nread < 0)
    {
        // Error or EOF
        if (nread != UV_EOF)
        {
            fprintf(stderr, "Read error: %s\n", uv_strerror(nread));
        }
        uv_close((uv_handle_t *)client, on_client_closed);
    }
    else if (nread > 0)
    {
        // We have data in buf->base, length = nread
        // ---------------------------------------------------------------------
        // This is the logic from your handle_connection():
        //   1. Parse the buffer
        //   2. Send "+PONG\r\n"
        // ---------------------------------------------------------------------

        // Simple pointer arithmetic from your code:
        unsigned char *p = (unsigned char *)buf->base;
        p++; // skip the first byte
        int len = 0;

        if (*p != '\r')
        {
            len = (len * 10) + (*p - '0');
            p++;
            printf("Parsed length: %d\n", len);
            printf("Remaining data: %s\n", p);
        }

        // Build the response
        const char *response = "+PONG\r\n";
        size_t resp_len = strlen(response);

        // We must keep the response data alive until the write completes,
        // so we allocate it dynamically:
        char *resp_copy = (char *)malloc(resp_len + 1);
        memcpy(resp_copy, response, resp_len + 1);

        uv_write_t *write_req = (uv_write_t *)malloc(sizeof(uv_write_t));
        // We'll store the resp_copy pointer in write_req->data so we can free it later
        write_req->data = resp_copy;

        uv_buf_t wrbuf = uv_buf_init(resp_copy, resp_len);

        // Write the data back to the client
        int ret = uv_write(write_req, client, &wrbuf, 1, on_write_done);
        if (ret < 0)
        {
            fprintf(stderr, "Write error: %s\n", uv_strerror(ret));
            uv_close((uv_handle_t *)client, on_client_closed);
        }
    }

    // Free the read buffer allocated in alloc_buffer()
    if (buf->base)
    {
        free(buf->base);
    }
}

// 6. Callback: Called after uv_write() finishes sending data
//   - We free the write buffer here
static void on_write_done(uv_write_t *req, int status)
{
    if (status < 0)
    {
        fprintf(stderr, "Write error: %s\n", uv_strerror(status));
    }

    // Free the response buffer
    if (req->data)
    {
        free(req->data); // this was resp_copy
    }

    // Free the write request itself
    free(req);
}

// 7. Callback: Called when a client handle is closed (cleanup)
static void on_client_closed(uv_handle_t *handle)
{
    printf("Client disconnected or closed.\n");
    free(handle); // free the uv_tcp_t
}
