#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <list>

std::list<int> listuser;

void error(const char *msg)
{
    perror(msg);
    exit(-1);
}

int Accept(int socket, struct sockaddr *addr, socklen_t *addrlen)
{
    int msg;

    for(;;) {
        msg = accept(socket, addr, addrlen);
        if(msg != -1) break;
        if(errno == EINTR || errno == ECONNABORTED) continue;
        error("accept()");
    }

    return msg;
}

size_t Read(int fd, void *buf, size_t count)
{
    ssize_t msg;

    for(;;) {
        msg = read(fd, buf, count);
        if(msg != -1) break;
        if(errno == EINTR) continue;
        error("read()");
    }

    return static_cast<size_t>(msg);
}

size_t Write(int fd, const void *buf, size_t count)
{
    ssize_t msg;

    for(;;) {
        for (int n : listuser) {
            if(n != fd){
        msg = write(n, buf, count);}
        }
        if(msg != -1) break;
        if(errno == EINTR) continue;
        error("write()");
    }

    return static_cast<size_t>(msg);
}

void *Malloc(size_t size)
{
    void *rc;

    rc = malloc(size);
    if(rc == nullptr) error("malloc()");

    return rc;
}

void Pthread_create(pthread_t *thread, pthread_attr_t *attr,
                    void *(*start_routine)(void *), void *arg)
{
    int msg;

    msg = pthread_create(thread, attr, start_routine, arg);
    if(msg) {
        errno = msg;
        error("pthread_create()");
    }
}


size_t reads(int socket, char *s, size_t size)
{
    char *p;
    size_t n, msg;

    if(s == nullptr) {
        errno = EFAULT;
        error("reads()");
    }
    if(!size) return 0;

    p = s;
    size--;
    n = 0;
    while(n < size) {
        msg = Read(socket, p, 1);
        if(msg == 0) break;
        if(*p == '\n') {
            p++;
            n++;
            break;
        }
        p++;
        n++;
    }
    *p = 0;

    return n;
}

/*
 * Запись count байтов в сокет.
 */
size_t writen(int socket, const char *buf, size_t count)
{
    const char *p;
    size_t n, msg;

    if(buf == nullptr) {
        errno = EFAULT;
        error("writen()");
    }

    p = buf;
    n = count;
    while(n) {
        msg = Write(socket, p, n);
        n -= msg;
        p += msg;
    }

    return count;
}

void *serve_client(void *arg)
{
    int socket;
    char s[256];
    ssize_t msg;

    pthread_detach(pthread_self());

    socket = *((int *) arg);
    free(arg);
    while((msg = reads(socket, s, 256)) > 0) {
        if(writen(socket, s, msg) == -1) break;
    }
    puts("[-]Left client");
    listuser.remove(socket);
    std::cout << "socket: " << socket <<std::endl << "list_user: ["<<std::endl;
    for(int n : listuser){
        std::cout << "   " << n <<std::endl;
    }
    std::cout << "     ]" <<std::endl;
    close(socket);

    return nullptr;
}

struct sockaddr_in servaddr;

int main()
{
    int lsocket;    /* Дескриптор прослушиваемого сокета. */
    int csocket;    /* Дескриптор присоединенного сокета. */

    int *arg;
    pthread_t thread;

    /* Создать сокет. */
    lsocket = socket(PF_INET, SOCK_STREAM, 0);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(1027);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int num = bind(lsocket, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if(num == -1) {
        error("bind()");
    }

    listen(lsocket, 5);

    puts("[+]TCP Server Start!");

    while(true) {
        csocket = Accept(lsocket, nullptr, 0);
        arg = new int;
        *arg = csocket;
        puts("[+]New Client");
        std::cout << "socket: " << csocket <<std::endl << "list_user: ["<<std::endl;
        listuser.push_back(csocket);
        for(int n : listuser){
            std::cout << "   " <<n <<std::endl;
        }
        std::cout << "      ]" <<std::endl;
        Pthread_create(&thread, nullptr, serve_client, arg);
    }

    return 0;
}