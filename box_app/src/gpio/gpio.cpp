#include <cstdio>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "gpio.h"

#define DBG_EN 	0
#if (DBG_EN == 1)
#define printf_dbg(x,arg...) printf("[gpio_debug]"x,##arg)
#else
#define printf_dbg(x,arg...)
#endif

#define printf_info(x,arg...) printf("[gpio_info]"x,##arg)
#define printf_warn(x,arg...) printf("[gpio_warnning]"x,##arg)
#define printf_err(x,arg...) printf("[gpio_error]"x,##arg)



GPIO::GPIO()
{
	num[0] = '\0';
	memset( direct, 0, GPIO_MAXLEN);
	memset( value, 0, GPIO_MAXLEN);
	direction = '\0';
}


int GPIO::sunxi_name_to_gpio(char const *name, char *gpio)
{
	char bank;
	int num;

	if (!name || !gpio) {
		return -EINVAL;
	}

	sscanf(name,"P%c%d", &bank, &num);
	if(num >= 32 || num < 0) {
		return -EINVAL;
	}

	if (bank >= 'a' && bank <= 'z') {
		num = num + SUNXI_BANK_SIZE * (bank - 'a');
	} else if (bank >= 'A' && bank <= 'Z') {
		num = num + SUNXI_BANK_SIZE * (bank - 'A');
	} else {
		return -EINVAL;
	}

	return sprintf(gpio, "%d",  num);
}

int GPIO::init(char const *pin_name)
{
	int fd_gpio;
	int ret;

	if(!pin_name) {
		return -EINVAL;
	}
	if(num[0] != '\0') {
		return -EINVAL;
	}

	ret = sunxi_name_to_gpio(pin_name, num);
	if(ret < 0) {
		printf_err("get name fail,%d\n",ret);
		return -ret;
	}
	direction = GPIO_INVAL;

	//设置GPIO管脚
	fd_gpio = open("/sys/class/gpio/export",O_WRONLY);
	if(fd_gpio < 0)
	{
		printf_err("Export gpio fail, %s(%d)\n", strerror(errno), errno);
		return fd_gpio;
	}

	printf_dbg("init num:%s\n", num);

	ret = write(fd_gpio, num, NUM_MAXLEN);
	if(ret < 0) {
		//printf_err("gpio write init error.(%d)\n",ret);
		int fd_gpio_exit = open("/sys/class/gpio/unexport", O_WRONLY);
		if(fd_gpio_exit == -1)
		{
			printf_err("Open and try unexport gpio error\n");
			return -1;
		} else {
			printf_dbg("try unexport num:%s\n",num);

			ret = write(fd_gpio_exit, num, NUM_MAXLEN);
			if(ret < 0) {
				printf_err("ERR: gpio write unexport error.\n");
			}
			close(fd_gpio_exit);
		}

		ret = write(fd_gpio, num, NUM_MAXLEN);
		if(ret < 0) {
			printf_err("Try again gpio write init error.\n");
			return ret;
		}
	}

	close(fd_gpio);

	char gpio_path[50] = "/sys/class/gpio/gpio";

	strcpy(direct,gpio_path);
	strcat(direct,num);
	strcat(direct,"/direction");
	printf_dbg("gpio direction path is : %s\n", direct);

	strcpy(value, gpio_path);
	strcat(value, num);
	strcat(value,"/value");
	printf_dbg("gpio value path is : %s\n", value);

	return 0;
}

int GPIO::exit()
{
	int fd_gpio;
	int ret = 0;

	if(num[0] == '\0') {
		return -EINVAL;
	}

	fd_gpio = open("/sys/class/gpio/unexport", O_WRONLY);
	if(fd_gpio == -1)
	{
		printf_err("open unexport file fail.\n");
	}

//	printf_dbg("exit num:%s\n",gpio->num);

	ret = write(fd_gpio, num, NUM_MAXLEN);
	if(ret < 0) {
		printf_err("write exit error.\n");
	}
	close(fd_gpio);

	direction = GPIO_INVAL;
	num[0] = '\0';
	return ret;
}

int GPIO::set_direction(int flags)
{
	int fd_gpio;
	int ret = 0;

	fd_gpio = open(direct, O_WRONLY);
	if(fd_gpio == -1)
	{
		printf_err("open %s fail\n", direct);
		return -EIO;
	}

	//设置GPIO方向
	if(GPIO_OUT == flags) {
		ret = write(fd_gpio,"out",strlen("out"));
		if(ret < 0) {
			goto direction_out;
		}
		direction = GPIO_OUT;
	} else if(GPIO_IN == flags) {
		ret= write(fd_gpio,"in",strlen("in"));
		if(ret < 0) {
			goto direction_out;
		}
		direction = GPIO_IN;
	} else {
		direction = GPIO_INVAL;
		close(fd_gpio);
		return -EINVAL;
	}

direction_out:
	if(ret < 0) {
		printf_err("ERR: gpio direction write error.\n");
	}

	close(fd_gpio);
	return ret;
}

int GPIO::output_open()
{
	int fd_gpio;

	if(direction != GPIO_OUT) {
		return -EIO;
	}

	fd_gpio = open(value, O_WRONLY);
	if(fd_gpio == -1)
	{
		printf_err("open %s\n",value);
		return -EIO;
	}

	return fd_gpio;
}

int GPIO::output_close(int fd_gpio)
{
	if(fd_gpio < 0) {
		return -EINVAL;
	}

	if(direction != GPIO_OUT) {
		return -EIO;
	}

	close(fd_gpio);
	return 0;
}

