/*
 * msg_process.cpp
 *
 *  Created on: Jan 22, 2018
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

//#include "button.h"
//#include "weight.h"
#include "lock.h"
#include "gprs.h"
#include "json/json.h"
#include "hodgepodge.h"
#include "weiding.h"
#include "msqueue.h"
#include "surfPictureMatch.h"

char lock_dev[] = "PB08";
lock 	ele_lock;

struct buy_record buy_rec;

void *msg_process_thread(void * queue_args)
{

	DBG( "\n\n******msg process thread create success******"
			"pthread_id: %lu \n\n\n", (unsigned long)pthread_self() );

	struct thread_arg *args = (struct thread_arg *)queue_args;
	DBG( "Msg process thread received queue ID: %d\n", args->queue_id );
//	pthread_mutex_t * mute = args->p_socket_mutex;
	struct mymesg rec_buf;
//	char time_buf[20];
	struct opendoor_store_msg open_buf;
	int err;

	open_buf.dev_id.assign(args->dev_id);

	buy_rec.door_open = false;
	buy_rec.ele_lock = true;
//	bool door_open = false;

	if( ele_lock.init( lock_dev ) != 0){
		err_sys( "%s init error\n",  lock_dev);
		pthread_exit((void *)0);
	}
#if 0
	int camera_num = 0;
	if( !pic_match_init(camera_num) ){
		err_sys(  "Init camera: /dev/video%d failed\n", camera_num );
		pthread_exit((void *)0);
	}
#endif

#if 0
	DBG( "\n\n*****************door test open*****************************\n\n" );
	ele_lock.dev_unlock();
	sleep_us(1000000);
	ele_lock.dev_lock();
	DBG( "\n\n*****************door test close*****************************\n\n" );
	/** init timer **/
	init_sigaction();
//	init_time();
#endif
	while( 1 ){

		if ( -1 == (err = msgrcv(args->queue_id, &rec_buf, sizeof(struct mymesg), -IMAGE_TYPE, 0)) ){
			err_sys( " msg process received message error!!!Recevied message: type-%ld, text:%s.", rec_buf.mtype, rec_buf.mtext.c_str());
			continue;
		}
		DBG( "Recevied message: type-%ld, text:%s\n", rec_buf.mtype, rec_buf.mtext.c_str() );
		int cmd_type = analysis_type(rec_buf.mtext);
		if( (OPEN_DOOR_TYPE == cmd_type) && (!buy_rec.door_open) ){
			if( opendoor_cmd_confirm( rec_buf.mtext, &open_buf ) ){
				DBG( "\n\n*****************Received msg to open the door*****************************\n\n" );
				ele_lock.dev_unlock();
//				sleep_us(100000);
				buy_rec.door_open = true;
//				init_time();
				pthread_mutex_lock(args->p_socket_mutex);
				door_open_respond( args->p_clientsocket, true, &open_buf );
				pthread_mutex_unlock(args->p_socket_mutex);
				sleep(1);
				ele_lock.dev_lock();
				buy_rec.door_open = false;
				continue;
			}else{
				fprintf( stderr, "open door command error or has opened\n" );
				continue;
			}
		}
#if 0
		if( buy_rec.door_open && ( MSG_TYPE == cmd_type) ){
			DBG( "Information exchange \n" );

			pthread_mutex_lock(args->p_socket_mutex);
			weight_change_respond( args->p_clientsocket, rec_buf.mtext, &open_buf );
			pthread_mutex_unlock(args->p_socket_mutex);
			continue;
		}

		if( buy_rec.door_open && ( IMAGE_TYPE == cmd_type) ){
			DBG( "\n\n*****Image info received*****\n\n" );
			continue;
		}
#endif
		if( (CLOSE_DOOR_TYPE == cmd_type)){
			buy_rec.door_open = false;
			pthread_mutex_lock(args->p_socket_mutex);
			door_close_respond( args->p_clientsocket, &open_buf );
//			door_open_respond( args->p_clientsocket, true, &open_buf );
			pthread_mutex_unlock(args->p_socket_mutex);
			DBG( "Send door closed msg to server\n" );
		}
//		pthread_mutex_lock(args->p_socket_mutex);
////		if( 0 != heart_respond(args->p_clientsocket) )
////			fprintf( stderr, "heart reply failed\n\n" );
//		pthread_mutex_unlock(args->p_socket_mutex);

	}//end of while(1)

	pthread_exit(NULL);
}



bool create_msg_process_thread(struct thread_arg *arg )
{
	pthread_t msg_process_tid;
	pthread_attr_t  msg_process_attr;

	pthread_attr_init(&msg_process_attr);
	pthread_attr_setdetachstate(&msg_process_attr, PTHREAD_CREATE_DETACHED);
	if( pthread_create(&msg_process_tid, &msg_process_attr, msg_process_thread, (void *)(arg) ) != 0) {
		err_sys("create msg process thread FAIL!!!\n");
		return false;
	}
	pthread_attr_destroy(&msg_process_attr);
	return true;
}



	/* signal process */
void timeout_info(int signo)
{
	ele_lock.dev_lock();
	DBG("Powered on, waiting for lock door.\n");
}

 	/* init sigaction */
void init_sigaction(void)
{
	struct sigaction act;

	act.sa_handler = timeout_info;
	act.sa_flags   = 0;
	sigemptyset(&act.sa_mask);
	sigaction(SIGPROF, &act, NULL);
}

	/* init */
void init_time(void)
{
	struct itimerval val;

	val.it_value.tv_sec = 30;
	val.it_value.tv_usec = 0;
	val.it_interval.tv_sec = 0;
	val.it_interval.tv_usec = 0;
	setitimer(ITIMER_PROF, &val, NULL);
}


int door_open_respond( GPRS *respond_socket, bool open, struct opendoor_store_msg *open_door_msg )
{
	using namespace std;

	Json::Value root;

	root["device_id"] = open_door_msg->dev_id.c_str();
//	root["open_id"] = open_door_msg->open_id.c_str();
//	root["user_id"] = open_door_msg->user_id.c_str();

	if( open ){
		root["status"] = "0";
		root["errno"] = "0";
	}else{
		root["status"] = "-1";
		root["errno"] = "-1";
	}
	std::string out = root.toStyledString();
	out.append( "\x1A" );
	DBG( "%s\n", out.c_str() );

	int times = 10;
	while( times-- ){	//send_tcp_data_no_chk
		if( respond_socket->send_tcp_data( (unsigned char *)out.c_str(), out.length() ) ){
			DBG("Respond door open command success\n");
			return 0;
		}
	}

	DBG("Try to send door open message 10 times failed\n");
	return -1;
}



int door_close_respond( GPRS *respond_socket, struct opendoor_store_msg *open_door_msg )
{
	using namespace std;

	std::string object;
	int chance = 0;
	while( ++chance < 10 ){
		if( !get_good_num( object ) ){
			cout << "failed-----chance: " << chance << endl;
			continue;
		}else{
			if( !object.empty() )
				break;
		}
	}

	Json::Value item;
	for( int i = 0; i < (int)object.length(); i++ ){
		if( object[i] != '0' ){
			char temp[10];
			int n = 4;
			sprintf( temp, "wd%0*d", n, i+1 );
			item[temp] = object.substr( i, 1);
		}
	}
	std::string good;
	Json::Value root;
	Json::Value arrayObj;
//	Json::Value item;

//	item["wd0001"] = "1";
//	item["wd0003"] = "1";

	arrayObj.append(item);

	root["dev_id"] = open_door_msg->dev_id.c_str();
//	root["sp_begin"] = "yes";
	root["good"] = arrayObj;
//	root["sp_end"] = "yes";


//	root.toStyledString();
	good = root.toStyledString();
	good.append( "\x1A" );
	DBG( "length: %d--%s\n", good.length(), good.c_str() );

	int times = 1;
	while( times-- ){		//end_tcp_data_no_chk
		if( 0 == respond_socket->send_tcp_data( (unsigned char *)good.c_str(), good.length() ) ){
			DBG("Send close door command success\n");
			return 0;
		}
	}

	DBG("Try to send close door message 1 times failed\n");
	return -1;
}



int weight_change_respond( GPRS *respond_socket, const std::string &msg, struct opendoor_store_msg *close_door_msg )
{
	using namespace std;

	std::string action, action_times, weight;
	Json::Reader reader;
	Json::Value value;

	if (reader.parse(msg, value)){
		action = value["action"].asString();
		action_times = value["action_times"].asString();
		weight = value["weight"].asString();
	}else
		return -1;

	Json::Value root;
	root["device_id"] = close_door_msg->dev_id.c_str();
	root["order_id"] = close_door_msg->timestamp.c_str();
	root["user_id"] = close_door_msg->user_id.c_str();
	root["action"] = action.c_str();
	root["action_times"] = action_times.c_str();
	root["weight"] = weight.c_str();

	std::string out = root.toStyledString();
	DBG( "%s\n", out.c_str() );

	int times = 10;
	while( times-- ){
		if( 0 == respond_socket->send_tcp_data( (unsigned char *)out.c_str(), out.length() ) ){
			DBG("Send close door command success\n");
			return 0;
		}
	}

	DBG("Try to send door open message 10 times failed\n");
	return -1;

	return false;
}
