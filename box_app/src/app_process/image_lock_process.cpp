/*
 * weight_lock.cpp
 *
 *  Created on: Jan 23, 2018
 *      Author: cui
 */

#include "image_lock_process.h"

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

#include "button.h"
//#include "weight.h"
//#include "lock.h"
#include "hodgepodge.h"
#include "weiding.h"


//char weight_dev[] = "/dev/ttyS1";

char lock_event[] = "/dev/input/event1";

button lock_detect;

struct mymesg msg_buf;

void *image_lock_process_thread(void * queue_args)
{
	DBG( "\n\n******image_lock_process_thread create success******"
			"pthread_id: %lu \n\n\n", (unsigned long)pthread_self());
	struct thread_arg *args = (struct thread_arg *)queue_args;
	DBG( "image_lock_process_thread received queue ID: %d\n", args->queue_id );


	int key_status;
	int ret;
//	struct mymesg msg1_buf;


	lock_detect.print_all_events();
	if( !lock_detect.open_device( lock_event ) ){
		err_sys( "open device error" );
		DBG( "\n\nxxxxxxxxxxxxxxxxxxxxxxxx image lock thread exitxxxxxxxxxxxxxxxxxxxxxxxxx\n\n" );
		pthread_exit((void *)0);
	}
	DBG( "open /dev/input/event1 success\n" );
//	lock_detect.printf_detail_info(lock_event);

//	memset( back, 0, sizeof(back) );
//	wg_detect.get_curr_weight( 31, &last_weight, back );
//	DBG( "last_weight %d\n", last_weight );

	while(1){
		ret = lock_detect.get_key( &key_status, 1000);
		if( ret < 0 ){
			err_sys( "Get key error\n" );
			break;
		}else if( ret == KEY_TIMEOUT ){
//			DBG( "get key time out \n" );
			continue;
		}else if( ret == 0 ){
			switch( key_status ){
			case MENUKEY_PRESS_EVENT:
				DBG( "\n\n\n\n**************event checked ----- door opened***************\n\n\n\n" );
				break;
#if 1
			case MENUKEY_RELEASE_EVENT:
				DBG( "\n\n\n\n**********event checked ----- door closed*******\n\n\n\n" );
				msg_buf.mtype = CLOSE_DOOR_TYPE;
//				close_door_msg_creat_p( msg_buf.mtext, args->dev_id );
				close_door_creat_type( msg_buf.mtext, args->dev_id );
//				= "{\"category\":\"4\",\"door_status\":\"1\",\"version\":\"1.0.0\"}";
				DBG( "thread_args.queue_id:%d\n", args->queue_id );
				send_queue_msg( &args->queue_id, &msg_buf, 0 );
				DBG( "Product parameter interaction, received message:\n" );
				break;
#endif
			case ENTERKEY_PRESS_EVENT:
				DBG( "enter key pressed\n" );

				break;
			case ENTERKEY_RELEASE_EVENT:
				DBG( "enter key released\n" );
				break;
			default:
				err_sys( "key type error\n" );
			}
		}
	}

	lock_detect.close_device();
	pthread_exit(NULL);
}

bool create_image_lock_process_thread(struct thread_arg *arg )
{
	pthread_t w_l_process_tid;
	pthread_attr_t  w_l_process_attr;

	pthread_attr_init(&w_l_process_attr);
	pthread_attr_setdetachstate(&w_l_process_attr, PTHREAD_CREATE_DETACHED);
	if( pthread_create(&w_l_process_tid, &w_l_process_attr, image_lock_process_thread, (void *)(arg) ) != 0) {
		err_sys("create image lock process thread FAIL!!!\n");
		return false;
	}
	pthread_attr_destroy(&w_l_process_attr);
	return true;
}
