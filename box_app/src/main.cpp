/*
 * main.cpp
 *
 *  Created on: Apr 9, 2018
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

#include "heart.h"
#include "image_lock_process.h"
#include "msg_process.h"
#include "capture_process.h"
#include "json/json.h"
#include "hodgepodge.h"
#include "gprs.h"
#include "weiding.h"
#include "config.h"

unsigned char string_TCP[] = "AT+CIPSTART=\"TCP\",\"112.74.59.162\",54989\r\n"; //weiding服务器
char ver[] = "1.0.0";

using namespace std;

char gprs_dev[] = "/dev/ttyS0";

GPRS SIM800;
pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;

int main( int argc, char *argv[] )
{

	char gprs_imei[16];
	int nodata_times = 0;

    openlog("box_app ", LOG_PID | LOG_CONS | LOG_PERROR, LOG_USER);
    syslog(LOG_INFO, "box_app starting......");

	if( !SIM800.gprs_init(gprs_dev) )
		err_quit( "gprs SIM800 init error\n" );

	DBG( "SIM800 init success\n" );
	sleep(1);
	if( !SIM800.check_test() ){
		err_sys( "gprs SIM800 AT test error\n" );
		return -1;
	}
	DBG( "SIM800 AT test success\n" );
	sleep(1);
	int retry = 5;
	while( retry--){
		if( !SIM800.disable_echo() ){
			err_msg( "SIM800C disable echo failed\n" );
			sleep(1);
			continue;
		}else
			break;
	}				//关闭模块回显,如失败
	if( retry <= 0 )
		err_quit( "SIM800C disable echo failed\n" );
	sleep(1);

	int signal = 0;
	int times = 10;
	while( times-- ){
		if( SIM800.get_sigquality( &signal ) != 0 ){
			sleep(2);
			continue;
		}else if( signal < 3 ){
			sleep(2);
			continue;
		}else{
			printf( "signal intensity : %d \n", signal );
			break;
		}
	}
	if( times <= 0)
		err_quit( "get signal error\n" );

	sleep(1);
	retry = 0;
	while(!SIM800.check_regnet() ){
		sleep(1);
		if(retry++ >= 20)
			err_quit("check register net fail\n");
	}
	sleep(1);
	memset( gprs_imei, 0, sizeof(gprs_imei) );
	while( true ){
		memset( gprs_imei, 0, sizeof(gprs_imei) );
		if ( SIM800.get_imei( gprs_imei )  ){
			DBG( "gprs_imei len-%d : %s\n", sizeof(gprs_imei), gprs_imei );
			break;
		}else{
			sleep(1);
			continue;
		}
	}
	DBG( "IMEI:%s\n", gprs_imei );
	sleep(2);
	if( !SIM800.connect_server( string_TCP, sizeof(string_TCP) ))
		err_quit("Connect server failed\n");

	struct mymesg mes_buf;
	struct thread_arg thread_args;
	string reply;
	int length;

	if( queue_init(&thread_args.queue_id) ){

		/***************** 线程传递参数初始化  ***********************/
		thread_args.p_clientsocket = &SIM800;
		thread_args.p_socket_mutex = &socket_mutex;
		thread_args.dev_id = gprs_imei;
		DBG( "queue init OK\n" );
	}
	query_queue_status(thread_args.queue_id);


    if( !create_heart_thread( &thread_args ) ){
    	err_sys("Create_heart_thread failed\n");
    	exit(-1);
    }
    DBG( "Create_heart_thread success\n" );

    if( !create_msg_process_thread( &thread_args ) ){
    	err_sys("Create_msg_process_thread failed\n");
    	exit(-1);
    }
    DBG( "Create_msg_process_thread success\n" );

    if( !create_image_lock_process_thread( &thread_args ) ){
       	err_sys("create_weight_lock_process_thread failed\n");
       	exit(-1);
    }
   DBG( "create_weight_lock_process_thread success\n" );

   sleep(1);
   if( !create_capture_process_thread( &thread_args ) ){
		err_sys("create_capture_process_thread failed\n");
		exit(-1);
   }
   DBG( "create_capture_process_thread success\n" );
   sleep(1);

//	std::string online = "{\"dev_id\":\"866710036780986\",\"switch\":\"1\",\"version\":\"1.0.0\"}\x1A";
	std::string online = "{\"dev_id\":\"";
	online += gprs_imei;
	online += "\",\"switch\":\"1\",\"version\":\"";
	online += ver;
	online += "\"}\x1A";

	DBG( "online data:%s\n length: %d\n", online.c_str(), online.length() );
	pthread_mutex_lock( &socket_mutex );
//	SIM800.send_tcp_data( (unsigned char *)online.c_str(), online.length());

	times = 0;
	while( !SIM800.send_tcp_data( (unsigned char *)online.c_str(), online.length()) && (times++ < 5) ) {
		fprintf( stderr, "Send register message failed %d times\n", times );
		sleep(2);
	}
	if( times == 5 )
		err_quit( "Send register failed, so exit\n" );

	sleep(2);

	bool inc_heart;
	SIM800.rec_tcp_data(reply,length, inc_heart);
	DBG( "string data:%s\n length: %d\n", reply.c_str(), length );
	pthread_mutex_unlock(&socket_mutex );

	if( times >= 9 )
		err_quit( "Register device error\n" );

	if ( analysis_type(reply) == 1){
		DBG( "analysis type == 1\n" );
	}
	if(!device_register_confirm(reply, ver)){
		//		system( "reboot" );
		DBG( "device register error\n" );
	}
	DBG( "device register confirmed right,please continue.....\n" );

	string::size_type bj;
	string::size_type ej;
	while( 1 ){
		pthread_mutex_lock( &socket_mutex );
		if( SIM800.rec_tcp_data(reply,length, inc_heart) ){
			pthread_mutex_unlock(&socket_mutex );
			nodata_times = 0;
//			DBG( "Unlocked**************Receive message success: %s\n", reply.c_str()  );
			if( (reply.npos != (bj = reply.find("{"))) && (reply.npos != ( ej = reply.find("}"))) ){
//				bj = reply.find("{");
//				ej = reply.find("}");
//				string sub_str = reply.substr( bj, ej-bj+1 );
				reply = reply.substr( bj, ej-bj+1 );
				DBG( "\n\n\n*****bj=%d, ej=%d; msg-len=%d,msg:%s\n***************\n\n\n",
						bj, ej, reply.length(), reply.c_str() );

				int cmd_type = analysis_type(reply);
				switch( cmd_type ){
				case OPEN_DOOR_TYPE:
					mes_buf.mtype = OPEN_DOOR_TYPE;
					mes_buf.mtext = reply;
					DBG( "thread_args.queue_id:%d\n", thread_args.queue_id );
					send_queue_msg(&thread_args.queue_id, &mes_buf, 0 );
					DBG( "Open the door command:\n" );
					sleep_us(50000);
					break;
				case MSG_TYPE:
					mes_buf.mtype = MSG_TYPE;
					mes_buf.mtext = reply;
					DBG( "thread_args.queue_id:%d\n", thread_args.queue_id );
//					send_queue_msg(&thread_args.queue_id, &mes_buf, 0 );
					sleep_us(50000);
					DBG( "Product parameter interaction, received message from server: %s\n",  reply.c_str() );
					break;
#if 0
				case CLOSE_DOOR_TYPE:
					mes_buf.mtype = CLOSE_DOOR_TYPE;
					mes_buf.mtext = reply;
					DBG( "thread_args.queue_id:%d\n", thread_args.queue_id );
//					send_queue_msg(&thread_args.queue_id, &mes_buf, 0 );
					DBG( "Closed the door, received message from server:" );
					std::cout << reply << std::endl;
					break;
#endif
				default:
					fprintf( stderr, "Received message from server type error!\n" );
				}
			}	//接受消息中有{}

			if( (reply.npos != reply.find("Heart")) || inc_heart ){
				mes_buf.mtype = HEART_TYPE;
				mes_buf.mtext = reply;

//				DBG( "thread_args.queue_id:%d\n", thread_args.queue_id );
				send_queue_msg(&thread_args.queue_id, &mes_buf, 0 );
//				DBG( "heart end\n");
				sleep_us(50000);			//如果不延时，会导致heart线程不能加锁，出现while一直加锁解锁的情况
				continue;
			}

		}else{
			pthread_mutex_unlock(&socket_mutex );

//			DBG( "mutex unlocked, nodata_times: %d\n", nodata_times );
			if( nodata_times++ > 1000){
				if( !SIM800.tcp_status_check() ){
					err_sys( "Server is disconnected" );
					break;
				}else{
					nodata_times = 0;
				}
			}
			continue;
		}

	}//end of while(1)

	queue_delete(thread_args.queue_id);

	exit(0);

}

