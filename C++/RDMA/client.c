#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <endian.h>
#include <byteswap.h>
#include <getopt.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include "base.h"


int main(int argc, char *argv[])
{
	struct resources res;
	int test_result = 1;
	char str[100];
	memset(str, 0, (size_t)100);

	if(argc == 2){
        config.server_name = strdup(argv[1]);
    }
    else{
        fprintf(stderr, "Can't find the correct server.\n");
        return -1;
    }

	/* print the used parameters for info*/
	print_config();

	/* 初始化资源 */
	if (resources_init(&res))
	{
		fprintf(stderr, "failed to init resources\n");
		CleanUp(&res);
		return 1;
	}

	/* 连接 QPs */
	if (connect_qp(&res)) {
		fprintf(stderr, "failed to connect QPs\n");
		CleanUp(&res);
		return 1;
	}

	/* 请求队列部署 */
	int i = 0;
	while(i++ < config.rr_max){
		if (post_recv(&res)) {
			fprintf(stderr, "failed to post RR\n");
			CleanUp(&res);
			return 1;
		}
	}

	while(1){
		unsigned long msgsize;

		/* 事件处理 */
		if(get_event(&res)){
			fprintf(stderr, "get_event fails.\n");
			CleanUp(&res);
			return 1;
		}

		msgsize = poll_completion(res.recvcq);
		if(msgsize < 0){
			fprintf(stderr, "poll_completion time out.\n");
			CleanUp(&res);
			return 1;
		}

		/* 请求接收 */
		if (post_recv(&res)) {
			fprintf(stderr, "failed to post RR\n");
			CleanUp(&res);
			return 1;
		}

		memcpy(str, res.recvbuf, msgsize);
		str[msgsize -1] = '\0';
		fprintf(stdout, "size: %lu\t message: %s\n", msgsize, str);

		if(str[0] == 'q' && msgsize == 2) break;

	}

	if(!CleanUp(&res)){
		return 1;
	}
	return 0;
}