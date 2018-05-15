/*
 * capture_process.cpp
 *
 *  Created on: May 15, 2018
 *      Author: cui
 */



#include "msg_process.h"

#include <pthread.h>
#include <string>
#include <iostream>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <syslog.h>
#include <linux/input.h>
#include <signal.h>
#include <sys/time.h>

#include "surfPictureMatch.h"
#include "weiding.h"

void *capture_process_thread(void * queue_args)
{

	DBG( "\n\n******capture process thread create success******"
			"pthread_id: %lu \n\n\n", (unsigned long)pthread_self() );

	struct thread_arg *args = (struct thread_arg *)queue_args;
	DBG( "capture process thread received queue ID: %d\n", args->queue_id );

	int camera_num = 0;
	if( !pic_match_init(camera_num) ){
		err_sys(  "Init camera: /dev/video%d failed\n", camera_num );
		pthread_exit((void *)0);
	}


	bool newest = false;
	while( 1 ){
		grab_capture( newest );
		sleep_us(1000000/25);
	}
	pthread_exit(NULL);
}



bool create_capture_process_thread(struct thread_arg *arg )
{
	pthread_t capture_process_tid;
	pthread_attr_t  capture_process_attr;

	pthread_attr_init(&capture_process_attr);
	pthread_attr_setdetachstate(&capture_process_attr, PTHREAD_CREATE_DETACHED);
	if( pthread_create(&capture_process_tid, &capture_process_attr, capture_process_thread, (void *)(arg) ) != 0) {
		err_sys("create msg process thread FAIL!!!\n");
		return false;
	}
	pthread_attr_destroy(&capture_process_attr);
	return true;
}



