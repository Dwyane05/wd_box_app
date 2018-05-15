/*
 * gprs.h
 *
 *  Created on: Apr 2, 2018
 *      Author: cui
 */

#ifndef GPRS_GPRS_H_
#define GPRS_GPRS_H_


#include "serial.h"

class GPRS : public serial{
private:
	int fd;
	bool echo_enable;
	bool nochk;
public:
	GPRS();

	bool gprs_init( char * dev_name );
	bool check_test();

	bool send_command_check( unsigned char *cmd, int cmd_len, unsigned char *check);
	bool send_command_nocheck( unsigned char *cmd, int cmd_len );
	bool disable_echo( void );
	int get_sigquality( int * sig );
	bool check_regnet();
	bool get_imei( char *imei  );
	bool connect_server( unsigned char * ip_server, int ip_len );

	bool tcp_status_check();
	bool send_tcp_data( unsigned char * tcp_data, int data_len );
	bool send_tcp_data_no_chk( unsigned char * tcp_data, int data_len );
	bool rec_tcp_data( std::string &data_buf, int &data_len, bool &has_heart );
};


#endif /* GPRS_GPRS_H_ */
