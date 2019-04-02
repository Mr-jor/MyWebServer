#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>

#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/epoll.h>
#include<unistd.h>
#include<time.h>
#include<sys/types.h>

#define IPADDRESS	"127.0.0.1"
#define SERV_PORT		6666
#define MAXSIZE		1024
#define FDSIZE		1024
#define EPOLLEVENTS	20

/* 函数声明 */

/* 创建套接字并进行绑定 */
int handle_connection(int sockfd) ;
/* 事件处理函数 */
void handle_events(int epollfd ,struct epoll_event *events,int num,int sockfd,char *buf) ;
/* 读处理 */
void do_read(int epollfd, int fd, int sockfd, char *buf) ;
/* 写处理 */
void do_write(int epollfd, int fd, int sockfd, char *buf) ;
/* 添加事件 */
void add_event(int epollfd, int fd ,int state) ;
/* 修改事件 */
void modify_event(int epollfd, int fd ,int state) ;
/* 删除事件 */
void delete_event(int epollfd, int fd ,int state) ;

int count = 0 ;

int main(int argc,char *argv[]){
	int sockfd ;
	struct sockaddr_in servaddr ;
	sockfd = socket(AF_INET,SOCK_STREAM,0) ;
	bzero(&servaddr,sizeof(servaddr)) ;
	servaddr.sin_family = AF_INET ;
	inet_pton(AF_INET,IPADDRESS,&servaddr.sin_addr) ;
	servaddr.sin_port = htons(SERV_PORT) ;
	connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) ;
	/*处理连接*/
	handle_connection(sockfd) ;
	close(sockfd) ;
	return 0 ;
}


int handle_connection(int sockfd) {
	int epollfd ;
	struct epoll_event events[EPOLLEVENTS] ;
	char buf[MAXSIZE] ;
	int ret ;
	epollfd = epoll_create(FDSIZE) ;
	add_event(epollfd,STDIN_FILENO,EPOLLIN) ;
	while(1){
		ret = epoll_wait(epollfd,events,EPOLLEVENTS,-1) ;
		handle_events(epollfd,events,ret,sockfd,buf) ;
	}
	close(epollfd) ;
}

void handle_events(int epollfd ,struct epoll_event *events,int num,int sockfd,char *buf) {
	int fd ;
	int i ;
	for(i = 0 ;i < num ;++i){
		fd = events[i].data.fd ;
		if(events[i].events & EPOLLIN)
			do_read(epollfd,fd,sockfd,buf) ;
		else if(events[i].events & EPOLLOUT)
			do_write(epollfd,fd,sockfd,buf) ;
	}
}

void do_read(int epollfd, int fd, int sockfd, char *buf) {
	int nread ;
	nread = read(fd,buf,MAXSIZE) ;
	if(nread == -1){
		perror("read error :") ;
		close(fd) ;
	}
	else if(nread == 0){
		fprintf(stderr,"server close.\n") ;
		close(fd) ;
	}
	else {
		if(fd == STDIN_FILENO)
			add_event(epollfd,sockfd,EPOLLOUT) ;
		else{
			delete_event(epollfd,sockfd,EPOLLIN) ;
			add_event(epollfd,STDOUT_FILENO,EPOLLOUT) ;
		}
	}
}

void do_write(int epollfd, int fd, int sockfd, char *buf) {
	int nwrite ;
	char temp[100] ;
	buf[strlen(buf)-1] = '\0' ;
	snprintf(temp,sizeof(temp),"%s_%02d\n",buf,count++) ;
	nwrite = write(fd,temp,strlen(temp)) ;
	if(nwrite == -1){
		perror("write error :") ;
		close(fd) ;
	}
	else{
		if(fd == STDOUT_FILENO)
			delete_event(epollfd,fd,EPOLLOUT) ;
		else
			modify_event(epollfd,fd,EPOLLIN) ;
	}
	memset(buf,0,MAXSIZE) ;
}

void add_event(int epollfd, int fd ,int state) {
	struct epoll_event ev ;
	ev.events = state ;
	ev.data.fd = fd ;
	epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev) ;
}

void modify_event(int epollfd, int fd ,int state) {
	struct epoll_event ev ;
	ev.events = state ;
	ev.data.fd = fd ;
	epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,&ev) ;
}

void delete_event(int epollfd, int fd ,int state) {
	struct epoll_event ev ;
	ev.events = state ;
	ev.data.fd = fd ;
	epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev) ;
}

