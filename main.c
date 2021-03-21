#include "csapp.h"

void process(int connectfd);
void read_requesthdrs(rio_t *rp);
int parse_url(char *url, char *filename, char *cgiargs);
void serve_static(int connectfd, char *filename, size_t size);
void client_error(int connectfd, char *errCode, char *errMsg);
void read_requestbody(rio_t *rp, char *buf);
void serve_dynamic(int connectfd, char *filename, char *cgiargs);
void serve_post(int connectfd);
void get_filetype(char *filename, char *filetype);
void serve_get(int connectfd, char *filename, int is_static, char *cgiargs);

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

void process(int connectfd) {
    rio_t rio;
    char buf[MAXLINE];
    char method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    int is_static;

    Rio_readinitb(&rio, connectfd);
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, url, version);
    if(strcasecmp(method, "GET")!=0 && strcasecmp(method, "POST")!=0) {             //仅支持POST和GET请求
        client_error(connectfd, "501", "Request method is not supported.");
        return;
    }
    printf("method:%s\r\n", method);

    is_static = parse_url(url, filename, cgiargs);
    read_requesthdrs(&rio);
    printf("%s \r\n", filename);

    if(strcasecmp(method, "POST")==0){
        printf("method:%s\r\n", method);
        read_requesthdrs(&rio);
        serve_post(connectfd);
        return;
    }
    else{
        serve_get(connectfd, filename, is_static, cgiargs);
        return;
    }
}

void serve_get(int connectfd, char *filename, int is_static, char *cgiargs) {               //处理GET请求
    struct stat sbuf;
    if(stat(filename, &sbuf)<0) {                   //将filename所指向的文件读入sbuf
        client_error(connectfd, "404", "File not found.");
        return;
    }

    if(is_static) {
        printf("static\r\n");
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            client_error(connectfd, "403", "Tiny couldn't read the file");
            return;
        }
        serve_static(connectfd, filename, sbuf.st_size);
        return;
    }
    else {
        printf("not static\r\n");
        serve_dynamic(connectfd, filename, cgiargs);
    }
}

void serve_post(int connectfd) {
    char buf[MAXBUF], body[MAXBUF];
    sprintf(body, "<html><title>POST</title></head>" );
    sprintf(body, "Upload Successfully");
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(connectfd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(connectfd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(connectfd, buf, strlen(buf));
    Rio_writen(connectfd, body, (int)strlen(body));
}

void read_requestbody(rio_t *rp, char *cgiargs){
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    strcat(cgiargs, buf);
    while(strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        strcat(cgiargs, buf);
    }
    return;
}

void read_requesthdrs(rio_t *rp) {              //读取请求头，在后台输出
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
        printf("%s\r\n", url);
        printf("%d", strlen(url)-1);
        if(url[strlen(url)-1] == '/') {
            strcat(filename, "index.html");
        }
        printf("%s \r\n", filename);
        return 1;
    }
    else {
        p = index(url, '?');          //找到？首次出现的位置
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
    get_filetype(filename, filetype);

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

void client_error(int connectfd, char *errCode, char *errMsg) {             //向用户输出错误信息
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

void serve_dynamic(int connectfd, char *filename, char *cgiargs) {          //处理动态请求
    char buf[MAXLINE], *emptylist[] = { NULL };

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(connectfd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(connectfd, buf, strlen(buf));

    if (Fork() == 0) {
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(connectfd, STDOUT_FILENO);
        Execve(filename, emptylist, environ);
    }
    Wait(NULL);
}