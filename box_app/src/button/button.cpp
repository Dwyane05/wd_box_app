/*
 * button.cpp
 *
 *  Created on: Jan 20, 2018
 *      Author: cui
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <errno.h>
#include <linux/input.h>

#include "button.h"

#define DBG_EN  0

#if (DBG_EN == 1)
#define printf_dbg(x,arg...) fprintf(stdout,"[button_debug]"x,##arg)
#else
#define printf_dbg(x,arg...)
#endif

#define printf_info(x,arg...) fprintf(stdout,"[button]"x,##arg)
#define printf_warn(x,arg...) fprintf(stdout,"[button_WARNING]:"x,##arg)
#define printf_err(x,arg...) fprintf(stderr,"[button_ERROR]:"x,##arg)

#define test_bit(bit) (mask[(bit)/8] & (1 << ((bit)%8)))

button::button()
{
	fd = -1;
}

bool button::open_device( const char *device )
{
	fd = open(device, O_RDWR | O_NONBLOCK);
	if(fd < 0) {
		printf_err("Could not open %s, %s\n",device, strerror(errno));
	   goto err;
	}
	return true;
err:
	close(fd);
	fd = -1;
	return false;
}

void button::close_device()
{
	if(fd > 0){
		close(fd);
		fd = -1;
	}
}

void button::print_all_events(void)
{
	char	name[64];           /* RATS: Use ok, but could be better */
	char          buf[256] = { 0, };  /* RATS: Use ok */
	unsigned char mask[EV_MAX/8 + 1]; /* RATS: Use ok */
	int           version;

	for ( int i = 0; i < 32; i++) {
		sprintf(name, "/dev/input/event%d", i);
		if ((fd = open(name, O_RDONLY, 0)) >= 0) {
			ioctl(fd, EVIOCGVERSION, &version);
			ioctl(fd, EVIOCGNAME(sizeof(buf)), buf);
			ioctl(fd, EVIOCGBIT(0, sizeof(mask)), mask);
			printf("%s\n", name);
			printf("    evdev version: %d.%d.%d\n",
				   version >> 16, (version >> 8) & 0xff, version & 0xff);
			printf("    name: %s\n", buf);
			printf("    features:");
			for (int j = 0; j < EV_MAX; j++) {
				if (test_bit(j)) {
					const char *type = "unknown";
					switch(j) {
					case EV_KEY: type = "keys/buttons"; break;
					case EV_REL: type = "relative";     break;
					case EV_ABS: type = "absolute";     break;
					case EV_MSC: type = "reserved";     break;
					case EV_LED: type = "leds";         break;
					case EV_SND: type = "sound";        break;
					case EV_REP: type = "repeat";       break;
					case EV_FF:  type = "feedback";     break;
					}
					printf(" %s", type);
				}
			}
			printf("\n");
			close(fd);
		}//??end of if()
	}//??end of for()
}

void button::printf_detail_info( const char *device )
{
//	int ret;
	/* get driver version */
	int version;
	if(ioctl(fd, EVIOCGVERSION, &version)) {
		fprintf(stderr, "could not get driver version for %s, %s\n", device, strerror(errno));
	}

	printf_info("version: %d.%d.%d\n", version >> 16,
		(version >> 8) & 0xff, version & 0xff);

	/* get device ID */
	struct input_id id;
	if(ioctl(fd, EVIOCGID, &id)) {
		fprintf(stderr, "could not get driver id for %s, %s\n", device, strerror(errno));
	}
	printf_info("bus:      %04x\n"
	   "  vendor    %04x\n"
	   "  product   %04x\n"
	   "  version   %04x\n",
	   id.bustype, id.vendor, id.product, id.version);

	/* get device name */
	char name[80];
	if(ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
		fprintf(stderr, "could not get device name for %s, %s\n", device, strerror(errno));
		name[0] = '\0';
	}
	printf_info("  name:     \"%s\"\n", name);


	char location[80];
	if(ioctl(fd, EVIOCGPHYS(sizeof(location) - 1), &location) < 1) {
		fprintf(stderr, "could not get location for %s, %s\n", device, strerror(errno));
		location[0] = '\0';
	}
	printf_info( "Device location: %s\n", location );

	/* get unique identifier */
	char idstr[80];
	if(ioctl(fd, EVIOCGUNIQ(sizeof(idstr) - 1), &idstr) < 1) {
		fprintf(stderr, "could not get idstring for %s, %s\n", device, strerror(errno));
		idstr[0] = '\0';
	}
	printf_info("  location: \"%s\"\n"
				   "  id:       \"%s\"\n", location, idstr);

#if 0
	/* Set clockid to be used for timestamps */
	int clkid = CLOCK_MONOTONIC;
	if (ioctl(fd, EVIOCSCLOCKID, &clkid) != 0) {
		fprintf(stderr, "Can't enable monotonic clock reporting: %s\n", strerror(errno));
		// a non-fatal error
	}
	printf_dbg("enable monotonic clock reporting OK\n" );
#endif
}


int button::get_key(int *key_status, long timeout_ms)
{
    int ret = -1;
    struct pollfd key_fds[1];
    struct input_event key_event;

    if(fd < 0) {
    	return -1;
    }

	key_fds[0].fd = fd;
	key_fds[0].events = POLLIN;
	ret = poll(key_fds, 1, timeout_ms);
	if (ret == 0) {
		printf_dbg("Read key_event time out\n");
		return KEY_TIMEOUT;
	} else if(ret == -1) {
		printf_err("Read key_event fail, %s\n", strerror(errno));
		return -1;
	}

	*key_status = NOKEY_EVENT;	//默认设置为：无按键事件

	//l_ret = lseek(fd, 0, SEEK_SET);
	ret = read(fd, &key_event, sizeof(key_event));
	if(ret != sizeof(key_event)) {
		printf_err("read key_event value error, %s\n", strerror(errno));
		return -1;
	}

	if(key_event.type == EV_KEY) {
//		printf_dbg( "scankey event: code--%d  status--%s\n",
//				key_event.code,
//				key_event.value ? "release" : "press" );

		switch(key_event.code) {
		case KEY_ENTER:
			*key_status = (key_event.value == 0)?ENTERKEY_PRESS_EVENT:ENTERKEY_RELEASE_EVENT;
			break;
		case KEY_MENU:
			*key_status = (key_event.value == 0 )?MENUKEY_PRESS_EVENT:MENUKEY_RELEASE_EVENT;
			break;
		default:
			*key_status = -1;
			printf_err("Unknow key event code\n");
			break;
		}
	} else {
		printf_err("Read unknow event type::0x%x\n", key_event.type);
	}

	 /*
	  * 检查同步信号
	  */
	//l_ret = lseek(fd, 0, SEEK_SET);
	ret = read(fd, &key_event, sizeof(key_event));
	if(ret != sizeof(key_event)) {
		printf_err("read key_event sync fail, %s\n", strerror(errno));
		return -1;
	}
	if(key_event.type != EV_SYN) {
		printf_err("read key_event sync error, type is:0x%x\n", key_event.type);
		return -1;
	}
	if((key_event.code != 0) || (key_event.value != 0)) {
		printf_err("read key_event sync value error\n");
		return -1;
	}

	return 0;
}

#if 0

int main( int argc, char *argv[] )
{
	int key_status;
	int ret;
	button lock_detect;

	lock_detect.print_all_events();

	if( !lock_detect.open_device( "/dev/input/event1" ) ){
		printf_err( "open device error\n" );
		return -1;
	}
	printf_dbg( "open /dev/input/event1 success\n" );

	lock_detect.printf_detail_info("/dev/input/event1");
	while(1){
		ret = lock_detect.get_key( &key_status, 10);
		if( ret < 0 ){
			printf_err( "Get key error\n" );
			break;
		}else if( ret == KEY_TIMEOUT ){
//			printf_dbg( "get key time out \n" );
			continue;
		}else if( ret == 0 ){
			printf_dbg( "get key ok! key status: %d  ", key_status );
			switch( key_status ){
			case MENUKEY_PRESS_EVENT:
				printf( "menu key pressed\n" );
				break;
			case MENUKEY_RELEASE_EVENT:
				printf( "menu key released\n" );
				break;
			case ENTERKEY_PRESS_EVENT:
				printf( "enter key pressed\n" );
				break;
			case ENTERKEY_RELEASE_EVENT:
				printf( "enter key released\n" );
				break;
			default:
				printf_err( "key type error\n" );
			}
		}
	}

	lock_detect.close_device();
	return 0;

}
#endif
