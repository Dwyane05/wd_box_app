/*
 * led.h
 *
 *  Created on: 5 Jul 2016
 *      Author: xiao
 */

#ifndef LOCK_H_
#define LOCK_H_


#include "gpio.h"

class lock : public GPIO {
private:
	int fd;
	int polatiry;
	bool is_on;
	bool gpio_inited;

public:
	lock();

	int init(char const *pin_name);
	int exit();
	int dev_lock(void);
	int dev_unlock(void);

	void set_polatiry(int npolatiry);
	int get_polatiry();


private:
	void  delay(unsigned int i);
};




#endif /* LOCK_H_ */
