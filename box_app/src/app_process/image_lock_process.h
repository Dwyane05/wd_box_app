/*
 * weight_lock.h
 *
 *  Created on: Jan 23, 2018
 *      Author: cui
 */

#ifndef SRC_APP_PROCESS_IMAGE_LOCK_PROCESS_H_
#define SRC_APP_PROCESS_IMAGE_LOCK_PROCESS_H_


#include "msqueue.h"

struct food_weight{
	int open_stable;
	int last_stable;
	int cur_stable;
	int close_stable;
	int this_wg;
	int total_wg;
	int action_times;
};

bool create_image_lock_process_thread(struct thread_arg *arg );

#endif /* SRC_APP_PROCESS_IMAGE_LOCK_PROCESS_H_ */
