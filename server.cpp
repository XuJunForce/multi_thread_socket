#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include<pthread.h>
#include<string.h>

#define THREADNUM 128

pthread_t tid[THREADNUM];
struct INFO{
    int fd;     //文件传输符
    char ip[INET_ADDRSTRLEN];  //ip地址
    int port;   //端口号
    long tid;    //当前线程池的ID
};


long find_empty(pthread_t* tid){
    for(long i = 0;i<THREADNUM;i++){
        if(tid[i]==0){
            return i;
        }
    }
    return -1;

}

void* worker(void* args){
    struct INFO* info = (struct INFO*)args; 
    char buff[1024];
    memset(buff, 0, sizeof(buff));
    while(1){
        int len = read(info->fd, buff, sizeof(buff));
        if(len > 0){
            if(strcmp(buff,"exit")==0){
                break;
            }
            printf("client says:%s\n",buff);
            write(info->fd, buff, len);
            memset(buff, 0, sizeof(buff));
        }
        else if(len == 0){
            printf("client disconnected\n");
            break;
        }
        else if(len < 0){
            perror("read");
            break;
        }
    }
    tid[info->tid] = 0;
    close(info->fd);
    pthread_exit(NULL);

}


int main(){

    
    struct INFO info[THREADNUM]; 
    memset(tid, 0, sizeof(tid));
    int lfd = socket(AF_INET,SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8888);

    //端口复用
    int flag = 1;
    setsockopt(lfd, SOL_SOCKET,SO_REUSEADDR, &flag, sizeof(flag));

    

    int ret = bind(lfd, (struct sockaddr*)& addr, sizeof(addr));
    if (ret == -1){
        perror("bind");
        return -1;
    }


    ret = listen(lfd, 128);
    if(ret == -1){
        perror("listen");
        return -1;
    }

    while(1){
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);
        int cfd = accept(lfd, (struct sockaddr*)&caddr, &clen);
        if(cfd == -1){
            perror("accpet");
            break;
        }
        long empty = find_empty(tid);


        if(empty == -1){
            printf("线程已满\n");
            close(cfd);
            continue;
        }

        
        info[empty].tid = empty;
        info[empty].fd = cfd;
        
        inet_ntop(AF_INET,&caddr.sin_addr.s_addr,info[empty].ip,INET_ADDRSTRLEN); //对info[empty].ip进行赋值
        info[empty].port = ntohs(caddr.sin_port);
        pthread_create(&tid[empty], NULL, worker, &info[empty]);
        pthread_detach(tid[empty]);
        
    }
    close(lfd);

    return 0;
}
