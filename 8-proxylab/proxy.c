#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE  1049000
#define MAX_OBJECT_SIZE 102400

struct URI_INFO
{
    char hostname[MAXLINE];
    char port[MAXLINE];
    char filename[MAXLINE];
};

static const char *EOL = "\r\n";
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_conn_hdr = "Proxy-Connection: close\r\n";

void doit(int fd);
int connnect_server(char *uri, struct URI_INFO *);
void forward_requesthdrs(rio_t *client_rio, rio_t *server_rio);
void forward_response(rio_t *client_rio, rio_t *server_rio);

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd    = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port,
                    MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);
        Close(connfd);
    }
}

void doit(int fd)
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    rio_t client_rio, server_rio;

    /* Read request line and headers */
    Rio_readinitb(&client_rio, fd);
    if (!Rio_readlineb(&client_rio, buf, MAXLINE))
        return;
    sscanf(buf, "%s %s %s", method, uri, version);
    printf("%s %s %s\n", method, uri, version);
    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not Implemented",
                    "proxy does not implement this method");
        return;
    }
    /* Open Server Connection */
    struct URI_INFO uri_info;
    int server_fd = connnect_server(uri, &uri_info);
    if (server_fd < 0)
        return;
    Rio_readinitb(&server_rio, server_fd);

    /* requst line */
    sprintf(buf, "GET /%s HT\rTP/1.0\r\n", uri_info.filename);
    Rio_writen(server_fd, buf, strlen(buf));
    /* Host */
    sprintf(buf, "Host: %s\r\n", uri_info.hostname);
    Rio_writen(server_fd, buf, strlen(buf));
    /* request header */
    forward_requesthdrs(&client_rio, &server_rio);
    puts("");
    /* response */
    forward_response(&client_rio, &server_rio);

    Close(server_fd);
}

int connnect_server(char *uri, struct URI_INFO *uri_info)
{
    char *filename = NULL;
    char *hostname = NULL, *port = NULL;

    char *p  = strstr(uri, "//");
    hostname = p ? p + 2 : uri;
    p        = index(hostname, ':');
    if (p) {
        *p   = '\0';
        port = p + 1;
        if ((p = index(port, '/')))
            *p = '\0';
        filename = port + strlen(port) + 1;
    } else {
        port     = "80";
        p        = index(hostname, '/');
        *p       = '\0';
        filename = p + 1;
    }

    strcpy(uri_info->filename, filename);
    strcpy(uri_info->port, port);
    strcpy(uri_info->filename, filename);

    printf("hostname: %s, port: %s, filename: %s\n", hostname, port, filename);
    return open_clientfd(hostname, port);
}

void forward_requesthdrs(rio_t *client_rio, rio_t *server_rio)
{
    char buf[MAXLINE];

    for (Rio_readlineb(client_rio, buf, MAXLINE); strcmp(buf, EOL);
         Rio_readlineb(client_rio, buf, MAXLINE)) {
        Rio_writen(server_rio->rio_fd, buf, strlen(buf));
    }

    Rio_writen(server_rio->rio_fd, user_agent_hdr, strlen(user_agent_hdr));
    Rio_writen(server_rio->rio_fd, connection_hdr, strlen(connection_hdr));
    Rio_writen(server_rio->rio_fd, proxy_conn_hdr, strlen(proxy_conn_hdr));
    Rio_writen(server_rio->rio_fd, EOL, 2);
    return;
}
void forward_response(rio_t *client_rio, rio_t *server_rio)
{
    size_t len;
    char buf[MAXLINE];

    while ((len = Rio_readlineb(server_rio, buf, MAXLINE))) {
        printf("%s", buf);
        Rio_writen(client_rio->rio_fd, buf, len);
    }
    printf("Connection closed by foreign host.\n");
    return;
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg)
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=ffffff>\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
