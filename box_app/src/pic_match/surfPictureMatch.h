/*
 * surfPictureMatch.h
 *
 *  Created on: May 11, 2018
 *      Author: cui
 */

#ifndef SRC_PIC_MATCH_SURFPICTUREMATCH_H_
#define SRC_PIC_MATCH_SURFPICTUREMATCH_H_

bool pic_match_init( int camera );

bool get_good_num( std::string &result );

void grab_capture( bool newest );
//int surfFeatureDotMatch(Mat src, int s);

#endif /* SRC_PIC_MATCH_SURFPICTUREMATCH_H_ */
