#ifndef _BASE_H_
#define _BASE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <fcntl.h>
#include <poll.h>
#include "sock.h"

/* 网络 大尾端、小尾端处理 */
#if __BYTE_ORDER == __LITTLE_ENDIAN
static inline uint64_t htonll(uint64_t x) { return bswap_64(x); }
static inline uint64_t ntohll(uint64_t x) { return bswap_64(x); }
#elif __BYTE_ORDER == __BIG_ENDIAN
static inline uint64_t htonll(uint64_t x) { return x; }
static inline uint64_t ntohll(uint64_t x) { return x; }
#else
#error __BYTE_ORDER is neither __LITTLE_ENDIAN nor __BIG_ENDIAN
#endif

/*  Structure of test parameters */
struct config_t{
    char        *dev_name;             // IB device name
    char        *server_name;          // daemon host name
    u_int32_t   tcp_port;              // daemon TCP port
    int         ib_port;               // local IB port to work with
    int         cq_size;               // cq max size
    int         sr_max;                // send request max times in one time
    int         rr_max;                // recv request max times in one time
    int         send_sge_max;
    int         recv_sge_max;
};

struct config_t config = {
    NULL,       // device name
    NULL,       // server name
    18515,      // tcp_port
    1,          // ib_port
    10,         // cq size
    5,          // sr_max
    5,          // rr_max
    5,          // send_sge_max
    5           // send_sge_max
};

/*  Structure of exchanging data which is needed to connect the QPs 
    ------------------------------------------------------------------------------------------------------------
    __atribute__((packed)) 作用是告诉编译器取消结构在编译过程中的优化对齐，按照实际
    占用字节数进行对齐，GCC 特有语法
    ------------------------------------------------------------------------------------------------------------
*/
struct cm_con_data_t{
    uint64_t        addr;           // Buffer Address
    uint32_t        rkey;           // Remote Key
    uint32_t        qp_num;         // QP Number
    uint16_t        lid;            // LID of the IB port
}__attribute__((packed));

/*  Structure of needed test resources */
struct resources{
    struct ibv_device_attr      device_attr;        // device attributes
    struct ibv_port_attr        port_attr;          // IB port attributes
    struct cm_con_data_t        remote_props;       // value to connect to remote side
    struct ibv_device           **dev_list;         // devie list
    struct ibv_context          *ib_ctx;            // device handle
    struct ibv_comp_channel     *channel;           // event channel
    struct ibv_pd               *pd;                // PD handle
    struct ibv_cq               *sendcq;            // Send CQ handle
    struct ibv_cq               *recvcq;            // Receive CQ handle 
    struct ibv_qp               *qp;                // QP handle
    struct ibv_mr               *sendmr;            // Send MR handle
    struct ibv_mr               *recvmr;            // Recv MR handle
    void                        *sendbuf;           // send buff pointer
    void                        *recvbuf;           // recv buff pointer
    int                         sock;               // TCP socket file descriptor
};

/* Poll CQ timeout in milisec */
#define MAX_POLL_CQ_TIMEOUT 2000

/* Functions */

/* 打印 config 内容 */
static void print_config(){
    fprintf(stdout, " ------------------------------------------------\n");
	if (config.dev_name) fprintf(stdout, " Device name                  : \"%s\"\n", config.dev_name);
	else fprintf(stdout, " Device name                  : No Default Device\n");
	fprintf(stdout, " IB port                      : %u\n", config.ib_port);
	if (config.server_name)
		fprintf(stdout, " IP                           : %s\n", config.server_name);
    fprintf(stdout, " TCP port                     : %u\n", config.tcp_port);
    fprintf(stdout, " CQ size                      : %u\n", config.cq_size);
    fprintf(stdout, " Send Requests Max:           : %u\n", config.sr_max);
    fprintf(stdout, " Recv Requests Max:           : %u\n", config.rr_max);
    fprintf(stdout, " Send Sge Max:                : %u\n", config.send_sge_max);
    fprintf(stdout, " Recv Sge Max:                : %u\n", config.recv_sge_max);
	fprintf(stdout, " ------------------------------------------------\n\n");
}
/*  proll_completion
    ------------------------------------------------------------------------------------------------------------
    int gettimeofday(struct timeval *tv, struct timezone *tz);
    获得当前时间
    ------------------------------------------------------------------------------------------------------------
    struct ibv_wc describes the Work Completion attributes
    Work Completion means that the corresponding Work Request is ended and the buffer can be reused for read,
    write or free. 
    ------------------------------------------------------------------------------------------------------------
    int ibv_poll_cq(struct ibv_cq *cq, int num_entries, struct ibv_wc *wc);
    Function: check if Work Completions are present in a CQ and pop them from the head of the CQ in the order
    they entered it (FIFO). After a Work Completion was popped from a CQ, it can't be returned to it.
    num_entries: Maximum number of Work Completions to read from the CQ.
    wc: direction: out. Array of size num_entries of the Work Completion that will be read from the CQ.
    return: Positive: Number of Work Completion taht were read from CQ.
            0: The CQ is empty
            Negative: Poll Failure
    ------------------------------------------------------------------------------------------------------------
*/
static unsigned long poll_completion(struct ibv_cq *cq){
    struct ibv_wc wc;
    unsigned long start_time_msec, cur_time_msec;
    struct timeval cur_time;
    int rc;

    gettimeofday(&cur_time, NULL);
    start_time_msec = (cur_time.tv_sec * 1000) + (cur_time.tv_usec / 1000);

    do{
        rc = ibv_poll_cq(cq, 1, &wc);
        if(rc < 0){
            fprintf(stderr, "Poll CQ failed\n");
            return -1;
        }
        gettimeofday(&cur_time, NULL);
        cur_time_msec = (cur_time.tv_sec * 1000) + (cur_time.tv_usec / 1000);
    }while((rc == 0) && ((cur_time_msec - start_time_msec) < MAX_POLL_CQ_TIMEOUT));

    if(rc == 0){
        fprintf(stderr, "Completion wasn't found in the CQ after timeout.\n");
        return -1;
    }

    if(wc.status != IBV_WC_SUCCESS){
        fprintf(stderr, "got bad completion with status: 0x%x, vendor syndrome: 0x%x\n",
                wc.status, wc.vendor_err);
        return -1;
    }

    #ifdef DEBUG
	fprintf(stdout, "poll_completion() executes successfully.\n");
	#endif

    return wc.imm_data;
}
/*  post_send & post_recv 网络 I/O 通信组
    ------------------------------------------------------------------------------------------------------------
    struct ibv_send_wr
        describes the Work Request to the Send Queue of the QP.
    struct ibv_sge{
        uint64_t        addr;       The address of the buffer to read from write to
        uint32_t        length;     The length of the buffer in bytes.
        uint32_t        lkey        The Local key of the Memory Region that this memory buffer was registered with
    };
        describes a scatter / gather entry 
    ------------------------------------------------------------------------------------------------------------
    int ibv_post_send(struct ibv_qp *qp, struct ibv_send_wr *sr,
                      struct ibv_send_wr **bad_sr);
    posts a linked list of Work Requests to the Send Queue of a QP
    wr: linked list of Work Request to be posted to the Send Queue of the Queue Pair
    bad_wr: a pointer to that will be filled with the first Work Request that its processing failed.
    Return: 0: On success.
    ------------------------------------------------------------------------------------------------------------
*/
static int post_send(struct resources *res, void *msg, size_t msgsize){
    
    struct ibv_send_wr sr;
    struct ibv_sge sge;
    struct ibv_send_wr *bad_sr;
    int rc;
    const bufsize = 4096;

    // 将数据填入缓冲区 格式： (void*)msg, imm_data 中为数据长度
    memcpy(res->sendbuf, msg, msgsize);

    #ifdef DEBUG
    fprintf(stdout, "%lu bytes: ", msgsize);
    fprintf(stdout, "%s\n", msg);
    #endif

    memset(&sge, 0, sizeof sge);
    sge.addr = (uintptr_t)res->sendbuf;
    sge.length = bufsize;
    sge.lkey = res-> sendmr->lkey;

    memset(&sr, 0, sizeof sr);
    sr.next = NULL;
    sr.wr_id = 0;
    sr.sg_list = &sge;
    sr.num_sge = 1;
    sr.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;     
    sr.send_flags = IBV_SEND_SOLICITED; 
    sr.imm_data = msgsize;                             // 将数据大小放入 imm_data 中接收       
    sr.wr.rdma.remote_addr = res->remote_props.addr;
    sr.wr.rdma.rkey = res->remote_props.rkey; 

    rc = ibv_post_send(res->qp, &sr, &bad_sr);
    if(rc){
        fprintf(stderr, "failed to post SR\n");
        return 1;
    }

    #ifdef DEBUG
	fprintf(stdout, "post_send() executes successfully.\n");
	#endif

    return 0;
}

static int post_recv(struct resources *res){
    struct ibv_recv_wr wr;
    struct ibv_sge sge;
    struct ibv_recv_wr *bad_wr;
    int rc;
    const size_t bufsize = 4096;

    memset(&sge, 0, sizeof sge);
    sge.addr = (uintptr_t)res->recvbuf;         
    sge.length = bufsize;
    sge.lkey = res->recvmr->lkey;

    memset(&wr, 0, sizeof wr);
    wr.next = NULL;
    wr.wr_id = 0;
    wr.sg_list = &sge;
    wr.num_sge = 1;

    rc = ibv_post_recv(res->qp, &wr, &bad_wr);
    if(rc){
        fprintf(stderr, "failed to post RR\n");
        return 1;
    }

    #ifdef DEBUG
	fprintf(stdout, "post_recv() executes successfully.\n");
	#endif

    return 0;
}

/*  resource_init 资源初始化 
    获取 IB 设备名单
    查找特定 IB 设备
    获取 IB 设备句柄
    获取 IB 设备属性
    分配 Protection Domain
    创建 cq
    创建 Memory Buffer
    注册 Memory Buffer
    创建 QP
*/
static int resources_init(struct resources *res){
    int i, num_devices;
    struct ibv_device *ib_dev = NULL;

    memset(res, 0, sizeof(*res));

    res->dev_list = ibv_get_device_list(&num_devices);
    /* 错误处理： 获取列表错误 || 不存在 IB 设备*/
    if(!res->dev_list){
        fprintf(stderr, "failed to get IB devices list.\n");
        return 1;
    }
    if(!num_devices){
        fprintf(stderr, "found %d IB device.\n", num_devices);
        return 1;
    }

    if(!config.dev_name){
        ib_dev = *res->dev_list;
        fprintf(stdout, "IB device name: %s\n",ibv_get_device_name(res->dev_list[0]));
        if(!ib_dev){
            fprintf(stderr, "No IB devices found.\n");
            return 1;
        }
    }
    else{
        // 查找特定的 IB 设备
        for(i = 0; i < num_devices; ++i){
            if(!strcmp(ibv_get_device_name(res->dev_list[i]), config.dev_name)){
                ib_dev = res->dev_list[i];
                break;
            }
        }   
        // 当在宿主机上不到对应设备时报错 
        if(ib_dev){
            fprintf(stderr, "IB device %s wasn't found\n", config.dev_name);
            return 1;
        }
    }

    // 获取设备句柄
    res->ib_ctx = ibv_open_device(ib_dev);
    if(!res->ib_ctx){
        fprintf(stderr, "failed to open device %s\n", config.dev_name);
        return 1;
    }

    // 获取设备属性
    if(ibv_query_port(res->ib_ctx, config.ib_port, &res->port_attr)){
        fprintf(stderr, "ibv_query_port on port %u failed\n", config.ib_port);
        return 1;
    }

    // 分配 Protection Domain
    res->pd = ibv_alloc_pd(res->ib_ctx);
    if(!res->pd){
        fprintf(stderr, "ibv_alloc_pd failed\n");
        return 1;
    }

    // 创建事件 channel
    res->channel = ibv_create_comp_channel(res->ib_ctx);
    if (!res->channel) {
	    fprintf(stderr, "Error, ibv_create_comp_channel() failed\n");
	    return 1;
    }

    // 创建 CQ
    res->sendcq = ibv_create_cq(res->ib_ctx, config.cq_size, NULL, res->channel, 0);
    res->recvcq = ibv_create_cq(res->ib_ctx, config.cq_size, NULL, res->channel, 0);
    if(!res->sendcq || !res->recvcq){
        fprintf(stderr, "failed to create CQ with %u entries.\n", config.cq_size);
        return 1;
    }

    // 分配内存给 Recv Memory buffer
    size_t bufsize = 4096;     // 4KB
    res->recvbuf = malloc(bufsize);
    res->sendbuf = malloc(bufsize);
    memset(res->sendbuf, 0, bufsize);
    memset(res->recvbuf, 0, bufsize);
    if(!res->recvbuf || !res->sendbuf){
        fprintf(stderr, "failed to mafailed to malloc %Zu bytes to memory buffer.\n", bufsize);
        return 1;
    }

    // 注册 Memory Buffer
    // 只有客户端会收到 RDMA Write 操作
    int mr_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE;
    res->recvmr = ibv_reg_mr(res->pd, res->recvbuf, bufsize, mr_flags);
    res->sendmr = ibv_reg_mr(res->pd, res->sendbuf, bufsize, mr_flags);
    if(!res->recvmr || !res->sendmr){
        fprintf(stderr, "ibv_reg_mr failed with mr_flags=0x%x\n", mr_flags);
        return 1;
    }

    #ifdef DEBUG
    fprintf(stdout, "Recv MR was registered with \n\taddr=%p, lkey=0x%x, rkey=0x%x, flags=0x%x\n",
            res->recvbuf, res->recvmr->lkey, res->recvmr->rkey, mr_flags);
    fprintf(stdout, "Send MR was registered with \n\taddr=%p, lkey=0x%x, rkey=0x%x, flags=0x%x\n",
            res->sendbuf, res->sendmr->lkey, res->sendmr->rkey, mr_flags);
    #endif

    // 创建 QP
    struct ibv_qp_init_attr qp_init_attr;
    memset(&qp_init_attr, 0, sizeof qp_init_attr);
    qp_init_attr.qp_type = IBV_QPT_RC;
    qp_init_attr.sq_sig_all = 1;
    qp_init_attr.send_cq = res->sendcq;     // 将 qp 维护的两个队列 send_cq 和 recv_cq 绑定到两个 cq 上
    qp_init_attr.recv_cq = res->recvcq;
    qp_init_attr.cap.max_send_wr = config.sr_max;       
    qp_init_attr.cap.max_recv_wr = config.rr_max;
    qp_init_attr.cap.max_send_sge = config.send_sge_max;      // 每个 SGE 都会指向一个内存中的 buffer 用于读写
    qp_init_attr.cap.max_recv_sge = config.recv_sge_max;
    res->qp = ibv_create_qp(res->pd, &qp_init_attr);
    if(!res->qp){
        fprintf(stderr, "failed to create QP.\n");
        return 1;
    }

    // 初始化套接字
    res->sock = -1;

    // 请求事件非阻塞
    if(notify_event(res)){
        fprintf(stderr, "notify_event() fails\n");
        return 1;
    }

    #ifdef DEBUG
    fprintf(stdout, "Resouces_init() executes successfully.\n");
    #endif
    
    return 0;
}

// QP 状态机转换： RESET -> INIT
static int modify_qp_to_init(struct ibv_qp *qp){
    struct ibv_qp_attr attr;
    int flags;
    int rc;

    memset(&attr, 0, sizeof attr);
    attr.qp_state = IBV_QPS_INIT;
    attr.port_num = config.ib_port;
    attr.pkey_index = 0;
    // 只有客户端会收到 RDMA Write 操作
    attr.qp_access_flags = (config.server_name) ? IBV_ACCESS_REMOTE_WRITE : 0;

    flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;

    rc = ibv_modify_qp(qp, &attr, flags);
    if(rc){
        fprintf(stderr, "failed to modify QP state to INIT\n");
        return rc;
    }

    return 0;
}

// QP 状态机转换： INIT -> RTR
static int modify_qp_to_rtr(struct ibv_qp *qp, uint32_t remote_qpn, uint16_t dlid){
	struct ibv_qp_attr attr;
	int flags;
	int rc;

	memset(&attr, 0, sizeof(attr));

	attr.qp_state = IBV_QPS_RTR;
	attr.path_mtu = IBV_MTU_256;
	attr.dest_qp_num = remote_qpn;
	attr.rq_psn = 0;
	attr.max_dest_rd_atomic = 0;
	attr.min_rnr_timer = 0x12;
	attr.ah_attr.is_global = 0;
	attr.ah_attr.dlid = dlid;
	attr.ah_attr.sl = 0;
	attr.ah_attr.src_path_bits = 0;
	attr.ah_attr.port_num = config.ib_port;

	flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | 
		IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;

	rc = ibv_modify_qp(qp, &attr, flags);
	if (rc) {
		fprintf(stderr, "failed to modify QP state to RTR\n");
		return rc;
	}

	return 0;
}

// QP 状态机转换： RTR -> RTS
static int modify_qp_to_rts(struct ibv_qp *qp){
	struct ibv_qp_attr attr;
	int flags;
	int rc;


	/* do the following QP transition: RTR -> RTS */
	memset(&attr, 0, sizeof(attr));

	attr.qp_state = IBV_QPS_RTS;
	attr.timeout = 0x12;
	attr.retry_cnt = 6;
	attr.rnr_retry = 0;
	attr.sq_psn = 0;
	attr.max_rd_atomic = 0;

 	flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | 
		IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;

	rc = ibv_modify_qp(qp, &attr, flags);
	if (rc) {
		fprintf(stderr, "failed to modify QP state to RTS\n");
		return rc;
	}

	return 0;
}

// 销毁资源
static int resources_destroy(struct resources *res){
	int test_result = 0;
	if (res->qp) {
		if (ibv_destroy_qp(res->qp)) {
			fprintf(stderr, "failed to destroy QP\n");
			test_result = 1;
		}
	}

    if (res->recvmr) {
		if (ibv_dereg_mr(res->recvmr)) {
			fprintf(stderr, "failed to deregister Send Recv MR\n");
			test_result = 1;
		}
    }
    
    if (res->sendmr) {
		if (ibv_dereg_mr(res->sendmr)) {
			fprintf(stderr, "failed to deregister Send Recv MR\n");
			test_result = 1;
		}
	}
    
    if(res->recvbuf)
        free(res->recvbuf);
    
    if(res->sendbuf)
        free(res->sendbuf);

	if (res->sendcq) {
		if (ibv_destroy_cq(res->sendcq)) {
			fprintf(stderr, "failed to destroy CQ\n");
			test_result = 1;
		}
    }
    
    if (res->recvcq) {
		if (ibv_destroy_cq(res->recvcq)) {
			fprintf(stderr, "failed to destroy CQ\n");
			test_result = 1;
		}
	}

	if (res->pd) {
		if (ibv_dealloc_pd(res->pd)) {
			fprintf(stderr, "failed to deallocate PD\n");
			test_result = 1;
		}
	}

	if (res->ib_ctx) {
		if (ibv_close_device(res->ib_ctx)) {
			fprintf(stderr, "failed to close device context\n");
			test_result = 1;
		}
	}

	if (res->dev_list)
		ibv_free_device_list(res->dev_list);

	if (res->sock >= 0) {
		if (close(res->sock)) {
			fprintf(stderr, "failed to close socket\n");
			test_result = 1;
		}
	}

    if(res->channel){
        if (ibv_destroy_comp_channel(res->channel)) {
	        fprintf(stderr, "Error, ibv_destroy_comp_channel() failed\n");
	        test_result = 1;
        }
    }

	return test_result;
}
/*  连接 QP
    TCP 连接交换双方数据
*/
static int connect_qp(struct resources *res){
    // TCP scoket 连接
    if(config.server_name){
        res->sock = sock_client_connect(config.server_name, config.tcp_port);
        if(res->sock < 0){
            fprintf(stderr, "failed to eatablish TCP connection to server %s, port %d\n", config.server_name, config.tcp_port);
            return -1;
        }
    }
    else{
        fprintf(stdout, "Waiting on port %d for TCP connection\n", config.tcp_port);
        res->sock = sock_daemon_connect(config.tcp_port);
        if(res->sock < 0){
            fprintf(stderr, "failed to establish TCP connection with client on port %d\n", config.tcp_port);
            return -1;
        }
    }
    fprintf(stdout, "TCP connection was established.\n");
    // 本地数据填入
    struct cm_con_data_t local_con_data, remote_con_data, tmp_con_data;
    int rc;

    if(rc = modify_qp_to_init(res->qp)){
        fprintf(stderr, "change QP state to INT failed\n");
        return rc;
    }
    local_con_data.addr   = htonll((uintptr_t)res->recvbuf);            // 对面得到的地址为本地的接收地址
	local_con_data.rkey   = htonl(res->recvmr->rkey);
	local_con_data.qp_num = htonl(res->qp->qp_num);
	local_con_data.lid    = htons(res->port_attr.lid);

	fprintf(stdout, "\nLocal LID        = 0x%x\n", res->port_attr.lid);
    // 交换数据
	if (sock_sync_data(res->sock, !config.server_name, sizeof(struct cm_con_data_t), &local_con_data, &tmp_con_data) < 0) {
		fprintf(stderr, "failed to exchange connection data between sides\n");
		return 1;
	}

	remote_con_data.addr   = ntohll(tmp_con_data.addr);
	remote_con_data.rkey   = ntohl(tmp_con_data.rkey);
	remote_con_data.qp_num = ntohl(tmp_con_data.qp_num);
	remote_con_data.lid    = ntohs(tmp_con_data.lid);

	/* save the remote side attributes, we will need it for the post SR */
	res->remote_props = remote_con_data;

	fprintf(stdout, "Remote address   = 0x%"PRIx64"\n", remote_con_data.addr);
	fprintf(stdout, "Remote rkey      = 0x%x\n", remote_con_data.rkey);
	fprintf(stdout, "Remote QP number = 0x%x\n", remote_con_data.qp_num);
	fprintf(stdout, "Remote LID       = 0x%x\n", remote_con_data.lid);

	/* QP 状态调整为 RTR，只具备接受能力 */
	rc = modify_qp_to_rtr(res->qp, remote_con_data.qp_num, remote_con_data.lid);
	if (rc) {
		fprintf(stderr, "failed to modify QP state from RESET to RTS\n");
		return rc;
	}

    /* QP 状态调整为 RTS，也具备接收能力 */
	rc = modify_qp_to_rts(res->qp);
	if (rc) {
		fprintf(stderr, "failed to modify QP state from RESET to RTS\n");
		return rc;
	}

	fprintf(stdout, "QP state was change to RTS\n");

	/* sync to make sure that both sides are in states that they can connect to prevent packet loose */
	if (sock_sync_ready(res->sock, !config.server_name)) {
		fprintf(stderr, "sync after QPs are were moved to RTS\n");
		return 1;
	}

    #ifdef DEBUG
	fprintf(stdout, "connect_qp() executes successfully.\n");
	#endif

	return 0;
}

/*  请求事件  --只有 Receiver Side 可以调用, 只需调用一次
    Send 使用 IMM 和 Solicited 事件通知模式，
    Receiver Side 需要用 get_event 和 notify_event 非阻塞调用写入
*/
int notify_event(struct resources *res){
    if(ibv_req_notify_cq(res->recvcq, 1)){
		fprintf(stderr, "Coudn't request CQ notification\n");
		CleanUp(res);
		return 1;
	}

	#ifdef DEBUG
	fprintf(stdout, "ibv_req_notify_cq() executes successfully.\n");
	#endif

	/*  配置句柄使其非阻塞 */
	int flags = fcntl(res->channel->fd, F_GETFL);
	int rc = fcntl(res->channel->fd, F_SETFL, flags | O_NONBLOCK);
	if(rc < 0){
		fprintf(stderr, "Failed to change file descriptor of Completion Event Channel\n");
		CleanUp((res));
		return 1;
	}

    #ifdef DEBUG
	fprintf(stdout, "fcntl() executes successfully.\n");
	#endif

    return 0;
}

/*  响应事件  --只有 Receiver Side 可以调用，每次读取时均需调用
    Send 使用 IMM 和 Solicited 事件通知模式，
    Receiver Side 需要用 get_event 和 notify_event 配合非阻塞写入
    最后再次调用 notify 等待下次事件到来
*/
int get_event(struct resources *res){
    /* 配置句柄使其非阻塞 */
	struct pollfd my_pollfd;
	int ms_timeout = 10;
	my_pollfd.fd      = res->channel->fd;
	my_pollfd.events  = POLLIN;
	my_pollfd.revents = 0;
    int rc;
	do {
        rc = poll(&my_pollfd, 1, ms_timeout);
	}while (rc == 0);
	if (rc < 0) {
        fprintf(stderr, "poll failed\n");
		CleanUp(res);
        return 1;
	}

	#ifdef DEBUG
	fprintf(stdout, "poll() executes successfully.\n");
	#endif

	/*  事件处理  */
	struct ibv_cq *ev_cq;
	void *ev_ctx;
    ev_cq = res->recvcq;
	if(ibv_get_cq_event(res->channel, &ev_cq, &ev_ctx)){
		fprintf(stderr, "Failed to get cq_event\n");
		return 1;
	}
	ibv_ack_cq_events(ev_cq, 1);

    #ifdef DEBUG
	fprintf(stdout, "ibv_ack_cq_events() executes successfully.\n");
	#endif

    /* 等待下次完成事件 */
    if (ibv_req_notify_cq(res->recvcq, 0)) {
        fprintf(stderr, "Couldn't request CQ notification\n");
        return 1;
    }

    return 0;
}

int CleanUp(struct resources *res){
	if (resources_destroy(res)) {
		fprintf(stderr, "failed to destroy resources\n");
		return 1;
	}
	free(config.server_name);
	fprintf(stdout, "Clean Up the init resources.\n");
	return 0;	// Success
}

#endif