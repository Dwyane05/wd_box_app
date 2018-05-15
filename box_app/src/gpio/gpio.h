/*
 * gpio.h
 *
 *  Created on: 4 Jul 2016
 *      Author: xiao
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <errno.h>

/* pin group base number name space,
 * the max pin number : 26*32=832.
 */
#define SUNXI_BANK_SIZE 32
#define SUNXI_PA_BASE	0
#define SUNXI_PB_BASE	32
#define SUNXI_PC_BASE	64
#define SUNXI_PD_BASE	96
#define SUNXI_PE_BASE	128
#define SUNXI_PF_BASE	160
#define SUNXI_PG_BASE	192
#define SUNXI_PH_BASE	224
#define SUNXI_PI_BASE	256
#define SUNXI_PJ_BASE	288
#define SUNXI_PK_BASE	320
#define SUNXI_PL_BASE	352
#define SUNXI_PM_BASE	384
#define SUNXI_PN_BASE	416
#define AXP_PIN_BASE	1024

#define SUNXI_PIN_NAME_MAX_LEN	8

#define GPIO_IN 0x01
#define GPIO_OUT 0X02
#define GPIO_INVAL 0X03

#define NUM_MAXLEN 10
#define GPIO_MAXLEN 50

class GPIO{
private:
	char direction;

	char direct[GPIO_MAXLEN];
	char value[GPIO_MAXLEN];
	char num[NUM_MAXLEN];

	int sunxi_name_to_gpio(const char *name, char *gpio);

public:
	GPIO();

	int init(const char *pin_name);
	int exit();

	int output_open();
	int output_close(int fd_gpio);

	int set_direction(int flags);

};



#endif /* GPIO_H_ */
