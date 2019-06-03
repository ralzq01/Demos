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

#include "sock.h"
#include "base.h"

#if HAVE_CONFIG_H
#include <config.h>
#endif

int main(int argc, char *argv[])
{
	struct resources res;
	int test_result = 1;
	char str[100] = "Hello I sent the Message";
	//memset(str, 0, 100);

	/* 显示配置列表 */
	print_config();

	/* 初始化资源 */
	if (resources_init(&res)){
		fprintf(stderr, "failed to init resources\n");
		CleanUp(&res);
		return 1;
	}

	/*  连接 QPs */
	if (connect_qp(&res)) {
		fprintf(stderr, "failed to connect QPs\n");
		CleanUp(&res);
		return 1;
	}

	while(1){
		int msgsize;
		fgets(str, 100, stdin);	//内含回车
		msgsize = strlen(str);

		/* 发送数据 */
		if (post_send(&res, str, msgsize)) {
			fprintf(stderr, "failed to post SR\n");
			CleanUp(&res);
			return 1;
		}

		/* 轮询 cq */
		if (poll_completion(res.sendcq) < 0) {
			fprintf(stderr, "poll completion failed\n");
			CleanUp(&res);
			return 1;
		}

		if(str[0] == 'q') break;
	}
	/* 资源销毁 */
	if(!CleanUp(&res)){
		return 1;
	}
	return 0;
}