/*
 * main.cpp
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
#include <syslog.h>

#include "app_process/image_lock_process.h"
#include "msg_process.h"
#include "heart.h"
#include "json/json.h"
#include "client_socket.h"
#include "hodgepodge.h"
#include "weiding.h"
#include "config.h"

std::string online = "{\"device_id\":\"00000001\",\"switch\":\"1\",\"version\":\"1.0.0\"}";

ClientSocket client_socket;
pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t	lcd_mutex;


int main ( int argc, char* argv[] )
{
	struct mymesg mes_buf;
	struct thread_arg thread_args;
	bool non_bck = true;
	std::string reply;

    openlog("wd_app ", LOG_PID | LOG_CONS | LOG_PERROR, LOG_USER);
    syslog(LOG_INFO, "wd_app starting......");

	if( client_socket.client_socket_init(SERVER_IP, PORT_NUM, non_bck) < 0){
		err_sys( "client socket init error\n" );
	}


	if( queue_init(&thread_args.queue_id) ){

		/***************** 线程传递参数初始化  ***********************/
		thread_args.p_clientsocket = &client_socket;
		thread_args.p_socket_mutex = &socket_mutex;
		thread_args.dev_id = "00000001";
		DBG( "queue init OK\n" );
	}
	query_queue_status(thread_args.queue_id);

    if( !create_heart_thread( &thread_args ) )
    	err_sys("Create_heart_thread failed\n");
    DBG( "Create_heart_thread success\n" );

    if( !create_msg_process_thread( &thread_args ) )
    	err_sys("Create_msg_process_thread failed\n");
    DBG( "Create_msg_process_thread success\n" );

    if( !create_weight_lock_process_thread( &thread_args ) )
    	err_sys("create_weight_lock_process_thread failed\n");
    DBG( "create_weight_lock_process_thread success\n" );

    pthread_mutex_lock( &socket_mutex );
	if( client_socket.send_tcpinfo( online.c_str() ) < 0 ){
		err_sys( "send_tcpinfo error\n" );
	}
	pthread_mutex_unlock(&socket_mutex );

	while( 1 ){
		pthread_mutex_lock( &socket_mutex );
		if(0 == client_socket.get_tcpinfo(reply) ){
			pthread_mutex_unlock(&socket_mutex );
//			DBG( "Receive message success: %s\n", reply.c_str()  );
			if( 0 == reply.compare("Heart") ){
				mes_buf.mtype = HEART_TYPE;
				mes_buf.mtext = reply;
//				DBG( "thread_args.queue_id:%d\n", thread_args.queue_id );
				send_queue_msg(&thread_args.queue_id, &mes_buf, 0 );
//				DBG( "heart end\n");
				continue;
			}else{
				int cmd_type = analysis_type(reply);
				switch( cmd_type ){
				case DEV_REG_TYPE:
					device_register_confirm(reply);
					DBG( "Device registered, received message:\n" );
					break;
				case OPEN_DOOR_TYPE:
					mes_buf.mtype = OPEN_DOOR_TYPE;
					mes_buf.mtext = reply;
					DBG( "thread_args.queue_id:%d\n", thread_args.queue_id );
					send_queue_msg(&thread_args.queue_id, &mes_buf, 0 );
					DBG( "Open the door command:\n" );
					break;
				case MSG_TYPE:
					mes_buf.mtype = MSG_TYPE;
					mes_buf.mtext = reply;
					DBG( "thread_args.queue_id:%d\n", thread_args.queue_id );
//					send_queue_msg(&thread_args.queue_id, &mes_buf, 0 );

					DBG( "Product parameter interaction, received message from server: %s\n",  reply.c_str() );
					break;
				case CLOSE_DOOR_TYPE:
					mes_buf.mtype = CLOSE_DOOR_TYPE;
					mes_buf.mtext = reply;
					DBG( "thread_args.queue_id:%d\n", thread_args.queue_id );
//					send_queue_msg(&thread_args.queue_id, &mes_buf, 0 );
					DBG( "Closed the door, received message from server:" );
					std::cout << reply << std::endl;
					break;
				default:
					fprintf( stderr, "Received message from server type error!\n" );
				}
			}

		}else{
			pthread_mutex_unlock(&socket_mutex );
			if( !client_socket.test_connected() ){
				err_sys( "Server is disconnected" );
				break;
			}
			continue;
		}

	}//end of while(1)

	queue_delete(thread_args.queue_id);
	return 0;
}
