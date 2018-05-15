/*
 * button.h
 *
 *  Created on: Jan 20, 2018
 *      Author: cui
 */

#ifndef BUTTON_H_
#define BUTTON_H_

class button
{
private:
	int 	fd;

public:
	button();

	bool open_device( const char *device );
	void close_device( void );

	void print_all_events(void);
	void printf_detail_info( const char *device  );

	int get_key(int *key_status, long timeout_ms);
};

#define NOKEY_EVENT 1
#define MENUKEY_PRESS_EVENT 2
#define MENUKEY_RELEASE_EVENT 3
#define ENTERKEY_PRESS_EVENT 4
#define ENTERKEY_RELEASE_EVENT 5
#define KEY_TIMEOUT 10




#endif /* BUTTON_H_ */
