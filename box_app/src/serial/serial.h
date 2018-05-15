/*
 * serial.h
 *
 *  Created on: Oct 26, 2016
 *      Author: cui
 */

#ifndef SERIAL_H_
#define SERIAL_H_



class serial
{
private:
	int fd;

public:
	serial();

	void msleep(int ms);
	int dev_open(char const *file_name);
	int dev_open(char const *file_name, bool block);
	void dev_close(void);

	int setopt(int nSpeed, int flow_ctrl, int nBits, int nStop, char nEvent);

	int send_data(unsigned char const *send_buf, int data_len);

	int recv_data(unsigned char *recv_buf,
			int data_len,
			unsigned int wait_ms);

	int read_data(unsigned char * const rcv_buf,
		unsigned int data_len,
		unsigned int wait_ms);

	void flush_recv();
	void flush_send();
	void flush_all();
};





#endif /* SERIAL_H_ */
