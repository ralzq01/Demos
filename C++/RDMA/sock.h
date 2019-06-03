#ifndef _SOCK_H_
#define _SOCK_H_
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <infiniband/verbs.h>
#include <netdb.h>
#include "sock.h"

int sock_daemon_connect(int port);

int sock_client_connect(const char *server_name, int port);

int sock_sync_data(int sock_fd, int is_daemon, size_t size, const void *out_buf, void *in_buf);

int sock_sync_ready( int sock_fd, int is_daemon);

/*  Function: sock_daemon_connect	server
    ------------------------------------------------------------------------------------------------------------
    struct addrinfo：
        int ai_flags;               -> AI_PASSIVE: socket 套接字地址用于绑定
        int ai_family;              -> 地址族： AF_INET: IPv4, 2; AF_INET6: IPv6, 23; AF_UNSPEC: 协议无关, 0
        int ai_socktype;            -> 套接字类型： SOCK_STREAM: 流，1，默认 TCP； SOCK_DGRAM： 数据报，2，默认 UDP
        size_t ai_addrlen;          -> 字节长度
        char *ai_canonname;         -> 主机名称
        struct sockaddr *ai_addr;   -> sockaddr 地址
        struct addrinfo *ai_next;   -> 下一个清单节点的指针
    ------------------------------------------------------------------------------------------------------------
    int getaddrinfo(const char *hostname,
                    const char *service,
                    const struct addrinfo *hint,
                    struct addrinfo **result);
    处理名字到地址以及服务到端口这两种转换，返回的是一个 sockaddr 结构的链而不是一个地址清单，具有协议无关性。
    头文件： netdb.h
    hostname: 一个主机名或者地址串(IPv4 的点分十进制或者 IPv6 的16进制串)
    service: 一个服务名或者10进制端口号数串
    hints: 可以是一个空指针，也可以是一个指向 addrinfo 结构的指针，调用者在这个结构中填入关于期望返回的信息类型暗示
    result: 用于返回得到的信息
    返回结果 int: 0 -> 成功， 非0 -> 失败
    ------------------------------------------------------------------------------------------------------------
    void freeaddrinfo(struct addrinfo *ai);
    由于 getaddrinfo 返回的所有存储空间都是动态获取的，这些存储空间必须通过调用 freeaddrinfo 返回给系统
    ------------------------------------------------------------------------------------------------------------
    int setsockopt(int sock, int level, int optname, const void *optval, socklen_t optlen);
    获得或者设置某个与套接字关联的选项。该选项可能存在于多层协议中，它们总会出现在最上面的套接字层。为了操作套接字层的选项，应将层的
    值指定为 SOL_SOCKRT. 
    sock: 将要被设置或者获取选项的套接字
    level: 选项所在的协议层
    optname： 需要访问的选项名
    optval：指向包含新选项值得缓冲
    optlen：现选项的长度
    执行成功：返回0
    ------------------------------------------------------------------------------------------------------------
*/
int sock_daemon_connect(int port){
	struct addrinfo *res, *t;
	struct addrinfo hints = {
		.ai_flags    = AI_PASSIVE,
		.ai_family   = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM
	};
	char *service;
	int n;
	int sockfd = -1, connfd;

	if (asprintf(&service, "%d", port) < 0) {   // asprintf 将 port 数字以字符串写入 service 中
		fprintf(stderr, "asprintf failed\n");
		return -1;
	}

	n = getaddrinfo(NULL, service, &hints, &res);
	if (n < 0) {
		fprintf(stderr, "%s for port %d\n", gai_strerror(n), port);
		free(service);
		return -1;
	}

	for (t = res; t; t = t->ai_next) {
		sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
		if (sockfd >= 0) {
			n = 1;

			setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof n);

			if (!bind(sockfd, t->ai_addr, t->ai_addrlen))
				break;
			close(sockfd);
			sockfd = -1;
		}
	}

	freeaddrinfo(res);
	free(service);

	if (sockfd < 0) {
		fprintf(stderr, "couldn't listen to port %d\n", port);
		return -1;
	}

	listen(sockfd, 1);
	connfd = accept(sockfd, NULL, 0);
	close(sockfd);
	if (connfd < 0) {
		fprintf(stderr, "accept() failed\n");
		return -1;
	}

	return connfd;
}

/*****************************************
* Function: sock_client_connect
*****************************************/
int sock_client_connect(const char *server_name, int port){
	struct addrinfo *res, *t;
	struct addrinfo hints = {
		.ai_family   = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM
	};
	char *service;
	int n;
	int sockfd = -1;


	if (asprintf(&service, "%d", port) < 0) {
		fprintf(stderr, "asprintf failed\n");
		return -1;
	}

	n = getaddrinfo(server_name, service, &hints, &res);
	if (n < 0) {
		fprintf(stderr, "%s for %s:%d\n", gai_strerror(n), server_name, port);
		free(service);
		return -1;
	}

	for (t = res; t; t = t->ai_next) {
		sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
		if (sockfd >= 0) {
			if (!connect(sockfd, t->ai_addr, t->ai_addrlen))
				break;
			close(sockfd);
			sockfd = -1;
		}
	}
	freeaddrinfo(res);
	free(service);

	if (sockfd < 0) {
		fprintf(stderr, "couldn't connect to %s:%d\n", server_name, port);
		return -1;
	}

	return sockfd;
}

/*****************************************
* Function: sock_recv
*****************************************/
static int sock_recv(int sock_fd, size_t size, void *buf){
	int rc;

retry_after_signal:
	rc = recv(sock_fd, buf, size, MSG_WAITALL);
	if (rc != size) {
		fprintf(stderr, "recv failed: %s, rc=%d\n", strerror(errno), rc);

		if ((errno == EINTR) && (rc != 0))
			goto retry_after_signal;    /* Interrupted system call */
		if (rc)
			return rc;
		else
			return -1;
	}

	return 0;
}

/*****************************************
* Function: sock_send
*****************************************/
static int sock_send(int sock_fd, size_t size, const void *buf){
	int rc;

retry_after_signal:
	rc = send(sock_fd, buf, size, 0);

	if (rc != size) {
		fprintf(stderr, "send failed: %s, rc=%d\n", strerror(errno), rc);

		if ((errno == EINTR) && (rc != 0))
			goto retry_after_signal;    /* Interrupted system call */
		if (rc)
			return rc;
		else
			return -1;
	}

	return 0;
}

/*****************************************
* Function: sock_sync_data
*****************************************/
int sock_sync_data(int sock_fd, int is_daemon, size_t size, 
                    const void *out_buf, void *in_buf){
	int rc;

	if (is_daemon) {
		rc = sock_send(sock_fd, size, out_buf);
		if (rc)
			return rc;

		rc = sock_recv(sock_fd, size, in_buf);
		if (rc)
			return rc;
	}
	else {
		rc = sock_recv(sock_fd, size, in_buf);
		if (rc)
			return rc;

		rc = sock_send(sock_fd, size, out_buf);
		if (rc)
			return rc;
	}

	return 0;
}

/*****************************************
* Function: sock_sync_ready
*****************************************/
int sock_sync_ready(int sock_fd, int is_daemon){
	char cm_buf = 'a';
	return sock_sync_data(sock_fd, is_daemon, sizeof(cm_buf), &cm_buf, &cm_buf);
}

#endif