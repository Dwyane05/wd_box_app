/*
 * main_thread.cpp
 *
 *  Created on: Jan 13, 2018
 *      Author: cui
 */

#include <pthread.h>
#include <string>
#include <iostream>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "weiding.h"
#include "config.h"

#define ID  0xFF
//#define ID1 0xFE
//#define MSG_MODE IPC_CREAT|IPC_EXCL|0666     //创建消息队列时的权限信息
#define MSG_MODE IPC_CREAT | 0666     //创建消息队列时的权限信息

pthread_mutex_t socket_mutex;
pthread_mutex_t	lcd_mutex;

struct mymesg{
	long mtype;
	std::string mtext;
};

struct thread_arg{
	int queue_id;
};

void *heart_thread(void * queue_args)
{
	int err;
	struct mymesg rec_buf;

	DBG( "\n\n******heart thread create success******\n\n\n" );

	struct thread_arg args = *(struct thread_arg *)queue_args;
	DBG( "Heart thread received queue ID: %d\n", args.queue_id );

	while( 1 ){
		if (-1 == (err = msgrcv(args.queue_id, &rec_buf, sizeof(struct mymesg), 1, 0)) ){
			perror("thr1: recv msg error!");
			printf("strerror = %s\n", strerror(errno));
			continue;
			//pthread_exit((void *)-1);
		}
		DBG( "Recevied message: type-%ld, text:%s\n", rec_buf.mtype, rec_buf.mtext.c_str() );
	}//end of while(1)

	pthread_exit(NULL);
}

int main( int argc, char *argv[] )
{
	int ret;

	struct mymesg mes_buf;
	struct thread_arg queue_args;
	key_t key;
	/*************** create queue  *****************/
	if ( -1 == (key = ftok(".", ID)) )
		 err_sys("Create key error!\n");


	if ( -1 == (queue_args.queue_id = msgget(key, MSG_MODE)) )  // | IPC_EXCL
		err_sys("message queue already exitsted!\n");
	DBG( "Create queue success, ID: %d\n\n", queue_args.queue_id );

	/*************   create threads    ********************/
	pthread_t heart_tid, worker_tid, uart_tid, lcd_tid;
	pthread_attr_t  heart_attr, worker_attr, uart_attr, lcd_attr;

	if (pthread_mutex_init(&socket_mutex, NULL) != 0)  {
		printf("Thread mutex init error\n");
	}

	pthread_attr_init(&heart_attr);
	pthread_attr_setdetachstate(&heart_attr, PTHREAD_CREATE_DETACHED);
	if( pthread_create(&heart_tid, &heart_attr, heart_thread, (void *)(&queue_args) ) != 0) {
		printf("create heart thread FAIL!!!\n");
	}
	pthread_attr_destroy(&heart_attr);

	mes_buf.mtype = 1;
	mes_buf.mtext = "test message!";

	while(1){
		sleep(1);
		if( -1 == (msgsnd(queue_args.queue_id, &mes_buf, sizeof(struct mymesg), 0)) )
		{
			perror("msg closed! quit the system!\n");
			break;
		}
		printf("Write msg success!\n");
//		mes_buf.mtype++;
		mes_buf.mtype %= 5;
		mes_buf.mtype += 1;

		char str[10];
		sprintf( str, "%d",mes_buf.mtype);
		mes_buf.mtext = "test message!";
		mes_buf.mtext.append(str);
		DBG("Send message: type-%ld, text: %s\n", mes_buf.mtype, mes_buf.mtext.c_str() );
	}

	exit(0);

}

