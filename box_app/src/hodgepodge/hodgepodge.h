/*
 * hodgepodge.h
 *
 *  Created on: Jan 11, 2018
 *      Author: cui
 */

#ifndef SRC_HODGEPODGE_HODGEPODGE_H_
#define SRC_HODGEPODGE_HODGEPODGE_H_



void get_time(char *tm );

int analysis_type(std::string &reply_info );

bool device_register_confirm( std::string &reply_info, char *version );


bool opendoor_cmd_confirm( std::string &reply_info, struct opendoor_store_msg *open_door_msg );

void close_door_creat_type( std::string &msg_buf, std::string &id );

void close_door_msg_creat_p( std::string &msg_buf, std::string &id );
#if 0
void wg_change_msg_creat_p( std::string &msg_buf, struct food_weight *fd_wg );
#endif

#endif /* SRC_HODGEPODGE_HODGEPODGE_H_ */
