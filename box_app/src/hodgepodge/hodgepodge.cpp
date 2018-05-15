/*
 * hodgepodge.cpp
 *
 *  Created on: Jan 11, 2018
 *      Author: cui
 */

//#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "msg_process.h"
//
//#include "weight_lock_process.h"
#include "gprs.h"
#include "json/json.h"
#include "weiding.h"


void get_time(char *tm )
{
//    const char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    char strtime[20] = {0};
    time_t timep;
    struct tm *p_tm;

    timep = time(NULL);
    p_tm = localtime(&timep); /*获取本地时区时间*/
//    printf("%d-%d-%d ", (p_tm->tm_year+1900), (p_tm->tm_mon+1), p_tm->tm_mday);
//    printf("%s %d:%d:%d\n", wday[p_tm->tm_wday], p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);

    strftime(strtime, sizeof(strtime), "%Y-%m-%d %H:%M:%S", p_tm);
    strncpy( tm, strtime, sizeof(strtime));
//    printf( "strtime: %s\n", strtime );
}


int analysis_type(std::string &reply_info )
{
	std::string type;
	std::string good;
	Json::Reader reader;
	Json::Value value;

	if (reader.parse(reply_info, value)){
		type = value["category"].asString();
		if( !type.empty() ){
			DBG( "Receive message category: %s\n", type.c_str() );
			return strtol( type.c_str(), NULL, 10 );
		}

		good = value["dev_id"].asString();
		if( !good.empty() && type.empty() ){
			DBG( "category = 4; dev_id: %s\n", good.c_str() );
			return 4;
		}
	}
	DBG( "pthread_id: %lu Receive message category error, received message: %s\n", (unsigned long)pthread_self(), reply_info.c_str() );
	return -1;
}

bool device_register_confirm( std::string &reply_info, char *vers  )
{
	std::string category, status, version, error;
	Json::Reader reader;
	Json::Value value;

	DBG( "Device registered: server back message %s\n", reply_info.c_str() );

	reader.parse(reply_info, value);
	category = value["category"].asString();
	if(category.compare("1") ){
		fprintf( stderr, "device register back message category error\n" );
		return false;
	}
	status = value["status"].asString();
	if(status.compare("0") ){
		fprintf( stderr, "device register back message status error\n" );
		return false;
	}
	error = value["errno"].asString();
	if(error.compare("0") ){
		fprintf( stderr, "device register back message errno error\n" );
		return false;
	}
	version = value["version"].asString();
	if(version.compare( vers ) ){
		fprintf( stderr, "device register back message version error\n" );
		return false;
	}
	return true;
}

bool opendoor_cmd_confirm( std::string &reply_info, struct opendoor_store_msg *open_door_msg )
{

	std::string category, open_id, user_id, door;
	Json::Reader reader;
	Json::Value value;

	if (reader.parse(reply_info, value)){
		category = value["category"].asString();
		if(category.compare("2") ){
			fprintf( stderr, "open door cmd category error\n" );
			return false;
		}
		door = value["door"].asString();
		if( door.compare("0") ){
			fprintf( stderr, "open door cmd door error\n" );
			return false;
		}
//		open_door_msg->open_id.assign( value["open_id"].asString() );
//		DBG( "open id: %s\n", open_door_msg->open_id.c_str() );
//		open_door_msg->user_id.assign( value["user_id"].asString() );
//		DBG( "user id: %s\n", open_door_msg->user_id.c_str() );
		open_door_msg->timestamp.assign( value["timestamp"].asString() );
		DBG( "timestamp: %s\n", open_door_msg->timestamp.c_str() );
		return true;
	}
	return false;
}

void close_door_creat_type( std::string &msg_buf, std::string &id )
{
	using namespace std;

	Json::Value root;

	root["dev_id"] = id.c_str();
	root["category"] = "4";

//	root.toStyledString();
	msg_buf = root.toStyledString();

	DBG( "close door create type:" );
	std::cout <<  msg_buf << std::endl;
}

void close_door_msg_creat_p( std::string &msg_buf, std::string &id )
{
	using namespace std;

	Json::Value root;
	Json::Value arrayObj;
	Json::Value item;

	item["wd0001"] = "1";
	item["wd0003"] = "1";

	arrayObj.append(item);

	root["dev_id"] = id.c_str();
//	root["sp_begin"] = "yes";
	root["good"] = arrayObj;
//	root["sp_end"] = "yes";


//	root.toStyledString();
	msg_buf = root.toStyledString();
//	msg_buf.append("\x1A");
	DBG( "close door msg create p:" );
	std::cout <<  msg_buf << std::endl;
}
#if 0
void wg_change_msg_creat_p( std::string &msg_buf, struct food_weight *fd_wg )
{
	using namespace std;

	Json::Value root;

	root["category"] = "3";
	fd_wg->this_wg < 0 ? root["action"] = "0" : root["action"] = "1";
	char ac_Buf[16] = "";
	sprintf(ac_Buf, "%d", fd_wg->action_times );
	std::string ac_ts;
	ac_ts.assign(ac_Buf);

	root["action_times"] = ac_ts.c_str();
	std::string wg;
	char wg_Buf[16] = "";
	sprintf(wg_Buf, "%d", abs(10 * fd_wg->this_wg) );
	wg.assign(wg_Buf);
	root["weight"] = wg.c_str();

//	root.toStyledString();
	msg_buf = root.toStyledString();
	DBG( "Weight changed msg create p:" );
	std::cout <<  msg_buf << std::endl;
}
#endif
void readJson() {
	using namespace std;
	std::string strValue = "{\"name\":\"json\",\"array\":[{\"cpp\":\"jsoncpp\"},{\"java\":\"jsoninjava\"},{\"php\":\"support\"}]}";

	Json::Reader reader;
	Json::Value value;

	if (reader.parse(strValue, value))
	{
		std::string out = value["name"].asString();
		std::cout << out << std::endl;
		const Json::Value arrayObj = value["array"];
		for (unsigned int i = 0; i < arrayObj.size(); i++)
		{
			if (!arrayObj[i].isMember("cpp"))
				continue;
			out = arrayObj[i]["cpp"].asString();
			std::cout << out;
			if (i != (arrayObj.size() - 1))
				std::cout << std::endl;
		}
	}
}

void writeJson() {
	using namespace std;

	Json::Value root;
	Json::Value arrayObj;
	Json::Value item;

	item["cpp"] = "jsoncpp";
	item["java"] = "jsoninjava";
	item["php"] = "support";
	arrayObj.append(item);

	root["name"] = "json";
	root["array"] = arrayObj;

//	root.toStyledString();
	std::string out = root.toStyledString();
	std::cout << out << std::endl;
}
