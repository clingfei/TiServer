#include "csapp.h"

void process(connectfd);
void read_requesthdrs(rio_t *rp);
void parse_url(char *url, char *filename);
void serve_static(int connectfd, char *filename, size_t size);
void client_error(int connectfd, char *cause, char *errCode, char *errMsg);

int main(int argc, char **argv) {
    int listenfd, connectfd;
    int client_len;
    struct sockaddr_in clientaddr;
    if(argc!=2){
        printf("Incorrect parameter numbers");
        exit(1);
    }
    int port = atoi(argv[1]);

    listenfd = Open_listenfd(port);
    while(1) {
        client_len = sizeof (clientaddr);
        connectfd = Accept(listenfd, (SA *)&clientaddr, &client_len);
        process(connectfd);
        Close(connectfd);
    }
}

void process(connectfd) {
    struct stat sbuf;
    rio_t rio;
    char buf[MAXLINE];
    char method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char filename[MAXLINE];

    Rio_readinitb(&rio, connectfd);
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, url, version);

    read_requesthdrs(&rio);                 //read requested head;
    parse_url(url, filename);               //get requested filename;
    printf("the url is %s \n", url);
    printf("Requested filename is %s", filename);

    if(stat(filename, &sbuf)<0) {                   //将filename所指向的文件读入sbuf
        client_error(connectfd, filename, "404", "File not found.");
        return;
    }

    if((S_ISREG(sbuf.st_mode)&&(sbuf.st_mode & S_IXUSR))) {
        serve_static(connectfd, filename, sbuf.st_size);
    }
    else {
        client_error(connectfd, filename, "403", "Permission not allowed.");
        return;
    }
}

void read_requesthdrs(rio_t *rp) {
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")){
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}

void parse_url(char *url, char *filename) {
    strcpy(filename, ".");
    strcat(filename, url);
    if (url[strlen(url)-1]=="/"){               //如果Url
        strcat(filename, "index.html");
    }
    return;
}

void serve_static(int connectfd, char *filename, size_t size) {
    int srcfd;
    char *srcp, buf[MAXBUF], filetype[MAXLINE];

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, size);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(connectfd, buf, strlen(buf));

    srcfd = Open(filename, O_RDONLY, 0);                //打开文件
    srcp = Mmap(0, size, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(connectfd, srcp, size);
    Munmap(srcp, size);
}

void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/plain");
}

void client_error(int connectfd, char *cause, char *errCode, char *errMsg) {
    char buf[MAXBUF], body[MAXBUF];
    sprintf(body, "<html><title>Error</title></head>" );
    sprintf(body, "%s\n%s", errCode, errMsg);
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errno, errMsg);
    Rio_writen(connectfd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(connectfd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(connectfd, buf, strlen(buf));
    Rio_writen(connectfd, body, (int)strlen(body));
}