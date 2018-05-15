/*
 * heart.cpp
 *
 *  Created on: Jan 15, 2018
 *      Author: cui
 */

#include <pthread.h>
#include <string>
#include <iostream>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
 #include <syslog.h>

#include "weiding.h"
#include "msqueue.h"
#include "heart.h"

void *heart_thread(void * queue_args)
{

	DBG( "\n\n******heart thread create success******\n\n\n" );
	//打印调用此函数的线程ID，便于调试分析
	pid_t		pid;
	pthread_t	tid;

	pid = getpid();
	tid = pthread_self();
	DBG("heart thread pid %lu tid %lu (0x%lx)\n", (unsigned long)pid,
	  (unsigned long)tid, (unsigned long)tid);

	struct thread_arg *args = (struct thread_arg *)queue_args;
	DBG( "Heart thread received queue ID: %d\n", args->queue_id );
//	pthread_mutex_t * mute = args->p_socket_mutex;
	struct mymesg rec_buf;
	int err;

	while( 1 ){

		if ( -1 == (err = msgrcv(args->queue_id, &rec_buf, sizeof(struct mymesg), HEART_TYPE, 0)) ){
			err_msg( " heart received message error!!!Recevied message: type-%ld, text:%s.", rec_buf.mtype, rec_buf.mtext.c_str());
			continue;
		}

		pthread_mutex_lock(args->p_socket_mutex);
//		DBG( "Recevied message: type-%ld, text:%s\n", rec_buf.mtype, rec_buf.mtext.c_str() );
//		DBG( "heart mutex locked\n" );
		if( 0 != heart_respond(args->p_clientsocket) )
			fprintf( stderr, "heart reply failed\n\n" );
		pthread_mutex_unlock(args->p_socket_mutex);
//		DBG( "heart mutex unlocked\n" );

	}//end of while(1)

	pthread_exit(NULL);
}


bool create_heart_thread(struct thread_arg *arg )
{
	pthread_t heart_tid;
	pthread_attr_t  heart_attr;

	pthread_attr_init(&heart_attr);
	pthread_attr_setdetachstate(&heart_attr, PTHREAD_CREATE_DETACHED);
	if( pthread_create(&heart_tid, &heart_attr, heart_thread, (void *)(arg) ) != 0) {
		err_sys("create heart thread FAIL!!!\n");
		return false;
	}
	pthread_attr_destroy(&heart_attr);
	return true;
}


/*********************************************************************************
*Function:  heart_respond
*Input:  socket节点
*Output:  无
*Return:  0 -1
**********************************************************************************/
int heart_respond( GPRS *respond_socket )
{
	int times = 10;
	unsigned char alive[] = "alive\x1A";
	while( times-- ){
		if( respond_socket->send_tcp_data(alive, sizeof(alive)) ){
			DBG("Send alive success\n\n");
			return 0;
		}
	}

	DBG("Try to send alive 10 times failed\n");
	return -1;
}
