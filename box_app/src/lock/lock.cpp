#include "lock.h"

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DBG_EN 	1
#if (DBG_EN == 1)
#define printf_dbg(x,arg...) printf("[lock_debug]"x,##arg)
#else
#define printf_dbg(x,arg...)
#endif

#define printf_info(x,arg...) printf("[lock_info]"x,##arg)
#define printf_warn(x,arg...) printf("[lock_warnning]"x,##arg)
#define printf_err(x,arg...)  printf("[lock_error]"x,##arg)



lock::lock()
{
	fd = -1;
	is_on = false;
	gpio_inited = false;
	polatiry = 1;
}

int lock::init(char const *pin_name)
{
	int ret;

	if(gpio_inited) {
		exit();
	}

	printf_dbg("init pin : %s\n", pin_name);

	ret = GPIO::init(pin_name);
	if(ret < 0) {
		printf_err("init gpio fail,%d\n",ret);
		return ret;
	}
	ret = set_direction(GPIO_OUT);
	if(ret < 0) {
		printf_err("set gpio output direction fail, %d\n",ret);
		return ret;
	}

	fd = output_open();
	if(fd < 0) {
		printf_err("gpio output open fail\n");
	}

	gpio_inited = true;
	return 0;
}

int lock::exit()
{
	if(gpio_inited) {
		output_close(fd);
		GPIO::exit();
		gpio_inited = false;
	}

	return 0;
}

int lock::dev_unlock(void)
{
	int ret = -1;

	if(!gpio_inited) {
		return -1;
	}

	if(is_on)
		return 0;

	printf_dbg("lock on, %d\n", polatiry);

	if(polatiry == 1)
		ret = write(fd, "1", sizeof("1"));
	else if(polatiry == 0)
	 ret =  write(fd, "0", sizeof("0"));

	if(ret > 0)
		is_on = true;

	return ret;
}

int lock::dev_lock(void)
{
	int ret = -1;

	if(!gpio_inited) {
		return -1;
	}

	if(!is_on)
		return 0;

	printf_dbg("lock off, %d\n", polatiry);

	if(polatiry == 1)
		ret =  write(fd, "0", sizeof("0"));
	else if(polatiry == 0)
		ret =  write(fd, "1", sizeof("1"));

	if(ret > 0)
		is_on = false;

	return ret;
}

void lock::set_polatiry(int npolatiry)
{
	polatiry = !!npolatiry;
}

int lock::get_polatiry()
{
	return polatiry;
}

