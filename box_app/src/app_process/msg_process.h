/*
 * msg_process.h
 *
 *  Created on: Jan 22, 2018
 *      Author: cui
 */

#ifndef SRC_APP_PROCESS_MSG_PROCESS_H_
#define SRC_APP_PROCESS_MSG_PROCESS_H_

#include "msqueue.h"

struct opendoor_store_msg{
	std::string user_id;
	std::string open_id;
	std::string dev_id;
	std::string timestamp;
};

struct buy_record {
	bool door_open;
	bool ele_lock;

};

bool create_msg_process_thread(struct thread_arg *arg );

void timeout_info(int signo);
void init_sigaction(void);
void init_time(void);

int door_open_respond( GPRS *respond_socket, bool open, struct opendoor_store_msg *open_door_msg );
int door_close_respond( GPRS *respond_socket, struct opendoor_store_msg *open_door_msg );

int weight_change_respond( GPRS *respond_socket, const std::string &msg, struct opendoor_store_msg *close_door_msg );
#endif /* SRC_APP_PROCESS_MSG_PROCESS_H_ */
