//
// Created by 杨君鹏 on 2022/5/26.
//

#include <sys/types.h>
#include "../include/csapp.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
/*
 *  tiny.c - A simple,iterative HTTP/1.0 Web server that uses the
 *  GET method to serve static and dynamic content
 * */


void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri,char *filename,char *cgiargs);
void serve_static(int fd,char *filename,int filesize);
void get_filetype(char *filename,char *cgiargs);
void serve_dynamic(int fd,char *filename,char *cgiargs);
void clienterror(int fd,char *caue,char *errnum,char *shortmsg,char *longmsg);

int main(int argc,char **argv){
    int listenfd,connfd;
    char hostname[500],port[500];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    printf("%d",argc);

    /*check command-line args*/
    if(argc != 2){
        fprintf(stderr,"usage: %s <port>\n",argv[0]);
        exit(1);
    }

    listenfd = open_listenfd(argv[1]);
    while(1){
        clientlen = sizeof (clientaddr);
        connfd = accept(listenfd,(SA *)&clientaddr,&clientlen);
        getnameinfo((SA *)&clientaddr,clientlen,hostname,5000,port,5000,0);
        printf("Accepted connection from (%s,%s)\n",hostname,port);
        doit(connfd);
        close(connfd);
    }
}


void doit(int fd){
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE];
    char filename[MAXLINE],cgiargs[MAXLINE];
    rio_t rio;

    /* Read request line and headers*/
    rio_readinitb(&rio,fd);
    rio_readlineb(&rio,buf,MAXLINE);
    printf("Request headers:\n");
    printf("%s",buf);
    sscanf(buf,"%s %s %s",method,uri,version);
    if (strcasecmp(method,"GET")){
        clienterror(fd,method,"501","Not implemented",
                    "Tiny does not implement this method");
        return;
    }
    read_requesthdrs(&rio);

    /* Parse URI from GET request */
    is_static = parse_uri(uri,filename,cgiargs);
    if (stat(filename,&sbuf) < 0){
        clienterror(fd,filename,"404","Not found",
                    "Tiny couldn't find this file");
        return;
    }

    /* Server static content */
    if(is_static){
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){
            clienterror(fd,filename,"403","Forbidden",
                        "Tiny couldn't read the file");
            return;
        }
        serve_static(fd,filename,sbuf.st_size);
    }else{          /* Serve dynamic content*/
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
            clienterror(fd,filename,"403","Forbidden",
                        "Tiny couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd,filename,cgiargs);
    }
}

void clienterror(int fd,char *cause,char *errnum,char *shortmsg,char *longmsg){
    char buf[MAXLINE],body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body,"<html><title>Tiny Errir</title>");
    sprintf(body,"%s<body bgcolor=""ffffff"">\r\n",body);
    sprintf(body,"%s%s: %s\r\n",body,errnum,shortmsg);
    sprintf(body,"%s<p>%s: %s\r\n",body,longmsg,cause);
    sprintf(body,"%s<hr><em>The Tiny Web server</em>\r\n",body);

    /* Print the HTTP response */
    sprintf(buf,"HTTP/1.0 %s %s\r\n",errnum,shortmsg);
    rio_writen(fd,buf,strlen(buf));
    sprintf(buf,"Content-Type: text/html\r\n");
    rio_writen(fd,buf,strlen(buf));
    sprintf(buf,"Content-length: %d\r\n\r\n",(int) strlen(body));
    rio_writen(fd,buf,strlen(buf));
    rio_writen(fd,buf,strlen(body));
}

void read_requesthdrs(rio_t *rp){
    char buf[MAXLINE];

    rio_readlineb(rp,buf,MAXLINE);
    while(strcmp(buf,"\r\n")){
        rio_readlineb(rp,buf,MAXLINE);
        printf("%s",buf);
    }
    return;
}

int parse_uri(char *uri,char *filename,char *cgiargs){

    char *ptr;

    if(!strstr(uri,"cgi-bin")){     /* Static content */
        strcpy(cgiargs,"");
        strcpy(filename,".");
        strcat(filename,uri);
        if (uri[strlen(uri-1)] == '/')
            strcat(filename,"home.html");
        return -1;
    }else{
        /* Dynamic content */
        ptr = index(uri,'?');
        if (ptr){
            strcpy(cgiargs,ptr+1);
            *ptr = '\0';
        }else{
            strcpy(cgiargs,"");
        }
        strcpy(filename,".");
        printf(filename);
        strcat(filename,uri);
        return 0;
    }
}


void serve_static(int fd,char *filename,int filesize){
    int srcfd;
    char *srcp,filetype[MAXLINE],buf[MAXBUF];

    /* Send response headers to client */
    get_filetype(filename,filetype);
    sprintf(buf,"HTTP/1.0 200 OK\r\n");
    sprintf(buf,"%sServer: Tiny Web Server \r\n",buf);
    sprintf(buf,"%sConnection: close\r\n",buf);
    sprintf(buf,"%sContent-length: %d\r\n",buf,filesize);
    sprintf(buf,"%sContent-type: %s\r\n\r\n",buf,filetype);
    rio_writen(fd,buf,strlen(buf));
    printf("Response headers:\n");
    printf("%s",buf);

    /* Send response body to client */
    srcfd=open(filename,O_RDONLY,0);
    srcp= (mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0));
    close(srcfd);
    rio_writen(fd,srcp,filesize);
    munmap(srcp,filesize);
}

/*
 *  get_filetype -Derive file type from filename
 * */
void get_filetype(char *filename,char *filetype){
    if(strstr(filename,".html"))
        strcpy(filetype,"text/html");
    else if (strstr(filename,".gif"))
        strcpy(filetype,"image/gif");
    else if (strstr(filename,".png"))
        strcpy(filetype,"image/png");
    else if (strstr(filename,".jpg"))
        strcpy(filetype,"image/jpeg");
    else
        strcpy(filetype,"text/plain");
}

void serve_dynamic(int fd,char *filename,char *cgiargs){
    char buf[MAXLINE],*emptylist[]={NULL};

    /* Return first part of HTTP response */
    sprintf(buf,"HTTP/1.0 200 OK\r\n");
    rio_writen(fd,buf, strlen(buf));
    sprintf(buf,"Server: Tiny Web Server\r\n");
    rio_writen(fd,buf, strlen(buf));

    if (fork() == 0){ /* Child */
        /* Real server would set all CGI vars here */
        setenv("QUERY_STRING",cgiargs,1);
        dup2(fd,STDERR_FILENO);        /* Redirect stdout to client */
        execve(filename,emptylist,environ);        /* Run CGI program*/
    }
    wait(NULL);        /* Parent waits for and reaps child */
}

