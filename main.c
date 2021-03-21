#include "csapp.h"

void process(connectfd);
void read_requesthdrs(rio_t *rp);
int parse_url(char *url, char *filename, char *cgiargs);
void serve_static(int connectfd, char *filename, size_t size);
void client_error(int connectfd, char *errCode, char *errMsg);
void read_post_headers(rio_t *rp);
void serve_post(int connectfd, char *buf, size_t size);
void receive(rio_t *rp, char *buf);

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
    char filename[MAXLINE], cgiargs[MAXLINE];
    int is_static;

    Rio_readinitb(&rio, connectfd);
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, url, version);
    is_static = parse_url(url, filename, cgiargs);
    if(is_static) {
        if(!strcasecmp(method, "GET")) {
            read_requesthdrs(&rio);                 //read requested head;

            if(stat(filename, &sbuf)<0) {                   //将filename所指向的文件读入sbuf
                printf("%s", filename);
                printf("testclient\n");

                client_error(connectfd, "404", "File not found.");
                printf("test client\r\n");
                return;
            }
            printf("Ssss\n");
            if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
                //clienterror(connectfd, filename, "403", "Tiny couldn't read the file");
                return;
            }
            serve_static(connectfd, filename, sbuf.st_size);
            return;
        }
        else if(!strcasecmp(method, "POST")) {
            char buf[MAXBUF];
            read_post_headers(&rio);
            receive(&rio, buf);
            serve_post(connectfd, buf, sizeof(buf));
        }
        else {
            client_error(connectfd, "501", "Method not supported.");
        }
    }
}

void read_post_headers(rio_t *rp) {
    printf("Method: POST\r\n");
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")){
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }

    printf("test");

    Rio_readlineb(rp, buf, MAXLINE);
    //while(strcmp(buf, "\r\n")) {
    //    Rio_readlineb(rp, buf, MAXLINE);
    //    printf("%s", buf);
    //}
    //strcpy(buf, "ssss");
    //Rio_writen(connectfd, buf, strlen(buf));
    return;
}

void receive(rio_t *rp, char *buf) {
    char body[MAXLINE];
    Rio_readlineb(rp, body, MAXLINE);
    sprintf(buf, "%s", body);
    //while(strcmp(body, "\r\n")) {
    //    Rio_readlineb(rp, body, MAXLINE);
    //    sprintf(buf, "%s%s", buf, body);
    //}
    strcpy(buf, "ssss");
    return;
}

void serve_post(int connectfd, char *buf, size_t size) {
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, size);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf);
    Rio_writen(connectfd, buf, strlen(buf));

    Rio_writen(connectfd, buf, size);
    return;
}

void read_requesthdrs(rio_t *rp) {
    printf("Method: GET");
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")){
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}

int parse_url(char *url, char *filename, char *cgiargs) {
    char *p;
    memset(filename, '\0', sizeof(filename));
    if (!strstr(url, "cgi-bin")) {
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, url);
        if(url[strlen(url)-1] == "/") {
            strcat(filename, "index.html");
        }
        return 1;
    }
    else {
        p = index(url, '?');          //找到？首次出现
        if (p) {
            strcpy(cgiargs, p+1);
            *p = '\0';
        }
        else                            //如果找不到？,则将其url按照静态处理
            strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, url);
        return 0;
    }
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

void get_filetype(char *filename, char *filetype) {
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/plain");
}

void client_error(int connectfd, char *errCode, char *errMsg) {
    printf("step in client\r\n");
    char buf[MAXBUF], body[MAXBUF];
    sprintf(body, "<html><title>Error</title></head>" );
    printf("1\r\n");
    sprintf(body, "%s\n%s", errCode, errMsg);
    printf("2\r\n");
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errCode, errMsg);
    printf("3\r\n");
    Rio_writen(connectfd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(connectfd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(connectfd, buf, strlen(buf));
    Rio_writen(connectfd, body, (int)strlen(body));
}