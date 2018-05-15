/*
 * msqueue.h
 *
 *  Created on: Jan 16, 2018
 *      Author: cui
 */

#ifndef SRC_MSQUEUE_MSQUEUE_H_
#define SRC_MSQUEUE_MSQUEUE_H_

#include <pthread.h>
#include <iostream>
#include "gprs.h"

#define FILEPATH "."
#define ID  0xFE
//#define MSG_MODE IPC_CREAT|IPC_EXCL|0666     //创建消息队列时的权限信息
#define MSG_MODE IPC_CREAT | 0666     //创建消息队列时的权限信息

#define HEART_TYPE 		11
#define DEV_REG_TYPE	1
#define OPEN_DOOR_TYPE	2
#define MSG_TYPE		3
#define CLOSE_DOOR_TYPE	4
#define IMAGE_TYPE	5

struct mymesg{
	long mtype;
	std::string mtext;
};

struct thread_arg{
	int queue_id;
	pthread_mutex_t *p_socket_mutex;
	GPRS 	*p_clientsocket;
	std::string 	dev_id;

};

bool queue_init( int *msqid );
void query_queue_status( int msqid );
bool send_queue_msg(int *msqid, struct mymesg *send_buf, int flag );
bool rcv_queuq_msg( int msqid, struct mymesg *rcv_buf, long type, int flag);
bool queue_delete( int msqid );

#endif /* SRC_MSQUEUE_MSQUEUE_H_ */
