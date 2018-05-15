/*
 * heart.h
 *
 *  Created on: Jan 15, 2018
 *      Author: cui
 */

#ifndef SRC_HEART_HEART_H_
#define SRC_HEART_HEART_H_

//#include <pthread.h>
//#include "client_socket.h"
#include "msqueue.h"
//
//struct mymesg{
//	long mtype;
//	std::string mtext;
//	pthread_mutex_t	que_mutex;
//};
//
//struct thread_arg{
//	int queue_id;
//	pthread_mutex_t *p_mutex;
//	ClientSocket *p_clientsocket;
//	bool is_send;
//
//};

bool create_heart_thread(struct thread_arg *arg );
int heart_respond( GPRS *respond_socket );

#endif /* SRC_HEART_HEART_H_ */
