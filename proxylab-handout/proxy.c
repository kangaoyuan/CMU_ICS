#include "csapp.h" // IWYU pragma: keep
#include "cache.h"
#include <assert.h>
#include <stdbool.h>
#pragma GCC diagnostic ignored "-Wunused-variable"

/* Recommended max cache and object sizes */
//#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define ARRSIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define URILEN 1024
struct uri_t{
    char host[URILEN];
    char port[URILEN];
    char path[URILEN];
};

struct queue_t{
    pthread_cond_t cond;
    pthread_mutex_t mutex;

    int *buf;
    int front, rear, cap, size;
};

static struct queue_t* queue;

struct queue_t* queue_init(int cap){
    queue = Malloc(sizeof(struct queue_t));
    memset(queue, 0, sizeof(struct queue_t));

    queue->cap = cap;
    queue->buf = Malloc(sizeof(int) * cap);

    pthread_cond_init(&queue->cond, NULL);
    pthread_mutex_init(&queue->mutex, NULL);

    return queue;
}

void queue_destory(struct queue_t* queue){
    free(queue->buf);
    free(queue);
}

void queue_push(struct queue_t* queue, uint connect_fd){
    pthread_mutex_lock(&queue->mutex);
    while(queue->size >= queue->cap)
        pthread_cond_wait(&queue->cond, &queue->mutex);
    assert(queue->size < queue->cap);
    queue->buf[(queue->rear) % (queue->cap)] = connect_fd;
    queue->size++;
    queue->rear = (queue->rear+1) % queue->cap;
    pthread_cond_broadcast(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}
uint queue_pop(struct queue_t* queue){
    pthread_mutex_lock(&queue->mutex);
    while(queue->size <= 0)
        pthread_cond_wait(&queue->cond, &queue->mutex);
    assert(queue->size > 0);
    uint connect_fd = queue->buf[(queue->front) % (queue->cap)];
    queue->size--;
    queue->front = (queue->front+1) % queue->cap;
    pthread_cond_broadcast(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
    return connect_fd;
}

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void* client_func(void* arg);
void do_request(int clientfd);
void parse_uri(char *uri, struct uri_t *uri_data);
void build_headers(char* header, struct uri_t* uri_data, rio_t* rp);

void clienterror(int fd, char* cause, char* errnum, char* shortmsg,
                 char* longmsg) {
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}

int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN);

    uint  listen_fd, connect_fd;
    char host_name[MAXLINE], port[MAXLINE];

    socklen_t               client_len;
    struct sockaddr_storage client_addr;

    /* Check command line args */
    if(argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    /* get listen_fd on the specific port. */

    /* this statement can do the below get listen_fd on sepcific port
     * action. */
    // listen_fd = Open_listenfd(argv[1]);

    //If hints is not NULL it points to an addrinfo structure whose ai_family, ai_socktype, and ai_protocol specify criteria that limit the set of socket addresses returned by getaddrinfo() through list.
    struct addrinfo hints, *list_ptr, *ptr;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; // AF stands for address family.
    hints.ai_socktype = SOCK_STREAM; // SOCK_STREAM for TCP, SOCK_DGRAM for UDP
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_flags |= AI_NUMERICSERV;

    /* Get a list of potential server addresses */
    // If the AI_PASSIVE flag is specified in hints.ai_flags, and node is NULL, then the returned socket addresses will be suitable for bind(2)ing a socket that will accept(2) connections.
    Getaddrinfo(NULL, argv[1], &hints, &list_ptr);

    //the network host is multihomed, accessible  over  multiple protocols (e.g., both AF_INET and AF_INET6); or the same service is available from multiple	socket types (one SOCK_STREAM address and another SOCK_DGRAM address, for example). Normally, the application should try using the addresses in the order in which they are returned.
    for(ptr = list_ptr; ptr ; ptr = ptr->ai_next) {
        listen_fd =
            socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if(listen_fd  < 0)
            continue;

        /* Eliminates "Address already in use" error from bind */
        int optavl = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optavl, sizeof(int));

        if(bind(listen_fd, ptr->ai_addr, ptr->ai_addrlen) == 0)
            break;

        // RAII's intent to avoid resource leak.
        close(listen_fd);
    }

    freeaddrinfo(list_ptr);
    if(!ptr)
        exit(1);

    if(listen(listen_fd, LISTENQ) < 0){
        // RAII's intent to avoid resource leak.
        close(listen_fd);
        fprintf(stderr, "listen failed (port %s)\n", argv[1]);
        exit(1);
    }

    queue = queue_init(9);
    pthread_t threads[7];
    for(uint i = 0; i < ARRSIZE(threads); ++i){
        Pthread_create(threads+i, NULL, client_func, NULL);
    }

    cache_init();

    while(true){
        int rc;
        client_len = sizeof(struct sockaddr_storage);
        connect_fd = Accept(listen_fd, (SA*)&client_addr, &client_len);

        /* Below (do_request) can be wrapped into the thread control flow to support concurrency. */
        // We can't use the wrapper function Getnameinfo() because it will exit when the function failure.
        //Getnameinfo((SA*)&client_addr, client_len, host_name, MAXLINE, port, MAXLINE, 0);
        if ((rc = getnameinfo((SA*)&client_addr, client_len, host_name,
                               MAXLINE, port, MAXLINE, 0)) != 0) {
            fprintf(stderr, "getnameinfo failed, %s\n", gai_strerror(rc));
            continue;
        }
        printf("Accept connection from (%s, %s)\n", host_name, port);

        queue_push(queue, connect_fd);
/*
 *        do_request(connect_fd);
 *
 *        close(connect_fd);
 */
    }

    queue_destory(queue);
    return 0;
}

void* client_func([[maybe_unused]]void* arg){
    Pthread_detach(Pthread_self());

    while(true){
        uint connect_fd = queue_pop(queue);
        do_request(connect_fd);
        close(connect_fd);
    }
}

// handle one HTTP request/response transaction
// It reads the client's request, then parses out the URL in it, then sends the request to the server, and finally forwards the server's response to the client.

void do_request(int connect_fd) {
    char  buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];

    rio_t client_io;
    Rio_readinitb(&client_io, connect_fd);
    if (!Rio_readlineb(&client_io, buf, MAXLINE))
        return;
    printf("In proxy, client request header: %s", buf);

    if(sscanf(buf, "%s %s %s", method, uri, version) != 3){
        fprintf(stderr, "Parse request line error: %s\n", strerror(errno));
        return;
    }
    if (strcasecmp(method, "GET")) {
        clienterror(connect_fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }

    if(cache_src(&client_io, uri))
        return;

    char proxy_headers[MAXLINE];
    struct uri_t *uri_data = (struct uri_t *)malloc(sizeof(struct uri_t));
    memset(uri_data, 0, sizeof(struct uri_t));
    parse_uri(uri, uri_data);
    build_headers(proxy_headers, uri_data, &client_io);
    printf("proxy_headers:\n %s\n", proxy_headers);

    int             server_fd, rc;
    struct addrinfo hints, *list_ptr, *ptr;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV;

    if( (rc = getaddrinfo(uri_data->host, uri_data->port, &hints, &list_ptr)) != 0) {
        fprintf(stderr, "getaddrinfo failed (%s:%s): %s\n", uri_data->host, uri_data->port, gai_strerror(rc));
        return;
    }

    for(ptr = list_ptr; ptr; ptr = list_ptr->ai_next){
        // It's bad practice to use below assignment, easy to bug.
        if( (server_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) < 0)
           continue;

        if(connect(server_fd, ptr->ai_addr, ptr->ai_addrlen) != -1)
            break;
        close(server_fd);
    }

    freeaddrinfo(list_ptr);
    if(!ptr)
        return;

    rio_t server_io;
    Rio_readinitb(&server_io, server_fd);
    // Attention the lenth
    Rio_writen(server_fd, proxy_headers, strlen(proxy_headers));

    char cache[MAX_OBJECT_SIZE];
    size_t read_n, cache_n = 0;
    while((read_n = Rio_readlineb(&server_io, buf, MAXLINE)) > 0){
        // Attention the lenth
        if(cache_n + read_n < MAX_OBJECT_SIZE)
            memcpy(cache+cache_n, buf, read_n);
        cache_n += read_n;
        Rio_writen(connect_fd, buf, read_n);
    }

    if(cache_n < MAX_OBJECT_SIZE){
        cache_add(uri, cache, cache_n);
    }
    close(server_fd);
    return;
}

// example: http://www.cmu.edu:8080/home/index.html
void parse_uri(char *uri, struct uri_t *uri_data) {
    char* host_ptr = strstr(uri, "//");
    if(host_ptr)
        host_ptr += strlen("//");
    else
        host_ptr = uri;

    char* port_ptr = strstr(host_ptr, ":");
    if(port_ptr){
        *port_ptr = '\0';
        sscanf(host_ptr, "%s", uri_data->host);
        *port_ptr = ':';
        port_ptr += strlen(":");
        char* path_ptr = strstr(port_ptr, "/");
        if(path_ptr){
            *path_ptr = '\0';
            sscanf(port_ptr, "%s", uri_data->port);
            *path_ptr = '/';
            sscanf(path_ptr, "%s", uri_data->path);
        } else {
            sscanf(port_ptr, "%s", uri_data->port);
            strcpy(uri_data->path, "/home.html");
        }
    } else{
        strcpy(uri_data->port, "80");
        char* path_ptr = strstr(host_ptr, "/");
        if(path_ptr){
            *path_ptr = '\0';
            sscanf(host_ptr, "%s", uri_data->host);
            *path_ptr = '/';
            sscanf(path_ptr, "%s", uri_data->path);
        } else {
            sscanf(host_ptr, "%s", uri_data->host);
            strcpy(uri_data->path, "/home.html");
        }
    }
}

void build_headers(char* header, struct uri_t* uri_data, rio_t* rp) {
    char buf[MAXLINE], request_hdr[MAXLINE], host_hdr[MAXLINE], other_hdr[MAXLINE];
    sprintf(request_hdr, "GET %s HTTP/1.0\r\n", uri_data->path);
    sprintf(host_hdr, "HOST: %s\r\n", uri_data->host);

     while(Rio_readlineb(rp, buf, MAXLINE)){
        if(!strcmp(buf, "\r\n"))
            break;
        else if (strncmp(buf, "HOST", 4))
            strcpy(host_hdr, buf);
        else if (!(strncasecmp(buf, "User-Agent", 10)) ||
                 !(strncasecmp(buf, "Proxy-Connection", 16)) ||
                 !(strncasecmp(buf, "Connection", 10))) /* headers should not be changed */
            continue;
        else
            strcat(other_hdr, buf);
     }

     static const char* end_hdr = "\r\n";
     static const char* conn_hdr = "Connection: close\r\n";
     static const char* proxy_conn_hdr = "Proxy-Connection: close\r\n";
     sprintf(header, "%s%s%s%s%s%s%s", request_hdr, host_hdr, conn_hdr,
             proxy_conn_hdr, user_agent_hdr, other_hdr, end_hdr);
}
