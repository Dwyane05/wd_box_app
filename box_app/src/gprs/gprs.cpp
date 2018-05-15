/*
 * gprs.cpp
 *
 *  Created on: Apr 2, 2018
 *      Author: cui
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <pthread.h>

#include "weiding.h"
#include "gprs.h"
#include "lock.h"


//#define DBG_EN  0
//#if (DBG_EN == 1)
//#define DBG(...) fprintf(stderr, " DBG(%s, %s(), %d): ", __FILE__, __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__)
//#else
//#define DBG(...)
//#endif

#define printf_info(x,arg...) printf("[gprs_info]"x,##arg)
#define printf_warn(x,arg...) printf("[gprs_warn]"x,##arg)
#define printf_err(x,arg...) printf("[gprs]ERROR:"x,##arg)

char reset_dev[] = "PB10";
lock 	gprs_ret;

#define MAX_REC	512


GPRS::GPRS( void )
{
	fd = -1;
	echo_enable = true;
	nochk = false;
}


bool GPRS::gprs_init( char *dev_name )
{
	if( gprs_ret.init( reset_dev ) != 0){
		printf_err( "%s init error\n",  reset_dev);
		gprs_ret.exit();
			return false;
	}

	gprs_ret.dev_unlock();		//输出高电平，复位GPRS模块
	sleep(1);					//持续1秒
	gprs_ret.dev_lock();		//输出低电平

	int ret;
	bool bolck = true;

	DBG("init serial\n");
	ret = dev_open(dev_name, bolck);
	if(ret < 0) {
		printf_err("serial com open fail\n");
		return false;
	}

	ret = setopt(9600, 0, 8, 1, 'N');
	if(ret < 0) {
		printf_err("serial com set options fail\n");
		dev_close();
		return false;
	}

	return true;
}

/*
 * 系统启动后第一个调用的方法
 * 使用系统默认的配置：
 *  1. 回显打开
 *  2. 返回字符串执行结果
 */
bool GPRS::check_test()
{
	int ret;
	unsigned char command[] = "AT\r\n";
	unsigned char answer[32];

	ret = send_data(command, 4);
	if(ret < 0) {
		printf_err("%s send command fail\n", __func__);
		return false;
	}

	if( (ret = read_data(answer, 32, 2000)) < 0){
		printf_err("%s read command fail\n", __func__);
		return false;
	}
#if 0
	for( int i = 0; i < ret; i++ ){
		printf( "answer[%d]: 0x%.2X ", i, answer[i] );
	}
	printf("\n");
#endif
	char result[] = { 0x41, 0x54, 0x0D, 0x0D, 0x0A, 0x4F, 0x4B, 0x0D, 0x0A};

	if( strncmp( (const char *)result, (const char *)answer, sizeof(result) ) ){
		for( int i = 0; i < ret; i++ ){
			printf( "answer[%d]: 0x%.2X ", i, answer[i] );
		}
		printf("\n");
		return false;
	}
	DBG("check connect end\n");

	return true;
}

bool GPRS::disable_echo( void )
{
	int ret;
	unsigned char command[] = "ATE0\r\n";	//ATE0 回显模式关闭

	DBG("%s begin\n",__func__);

	ret = send_data(command, 6);
	if(ret < 0) {
		printf_err("%s send command fail\n", __func__);
		return false;
	}

	DBG("set no echo end\n");
	msleep(500);
	echo_enable = false;
	return true;
}

int GPRS::get_sigquality( int *sig )
{
	int ret;
	unsigned char sim_cmd[] = "AT+CPIN?\r\n";
	unsigned char ok_chk[] = "OK";

	unsigned char command[] = "AT+CSQ\r\n";
	unsigned char GsmRcv[256] = {0};

	memset(GsmRcv, 0, sizeof(GsmRcv));

	if( !send_command_check(sim_cmd, sizeof(sim_cmd), ok_chk)) {
		printf_err( "Please check SIM card isn't exist?\n" );
		return false;
	}
	DBG( "SIM card check success\n" );
	sleep(1);
	//先清空接受缓冲区
	flush_recv();
	DBG("get signal send cmd: %s", command);

	ret = send_data(command, 8);
	if(ret <= 0) {
		printf_err("send data fail\n");
		return -EIO;
	}

	memset(GsmRcv, 0, sizeof(GsmRcv));
	ret = read_data(GsmRcv, 22, 2000);
	if(ret < 0) {
		printf_warn("read data fail\n");
		return -1;
	}

//	for(int i = 0; i < ret; i++)
//		printf_err("GsmRcv[%d] = 0x%x\n", i, GsmRcv[i]);
//	printf_err( "\n" );

	if(ret >= 6 && strncmp((const char *) &GsmRcv[ret - 6], "\r\nOK\r\n", 6) == 0) {
		DBG("signal quality: %s\n", GsmRcv);
		*sig = (GsmRcv[8] - '0')*10 + (GsmRcv[9] - '0');
	}
//	sleep(3);
	return 0;
}


bool GPRS::check_regnet()
{
	int ret;
	int retry = 3;

//	unsigned char command[] = "AT+CGREG?\r\n";			//GPRS网络注册状态
	unsigned char command[] = "AT+CREG?\r\n";
	unsigned char GsmRcv[256] = {0};

	memset(GsmRcv, 0, sizeof(GsmRcv));


	while(retry--)
	{
		//先清空接受缓冲区
		flush_recv();
		DBG("check register send %s, len: %d", command, sizeof(command) );

		ret = send_data(command, sizeof(command));
		if(ret <= 0) {
			printf_err("send data fail\n");
			return -EIO;
		}

		if(echo_enable) {
			ret = read_data(GsmRcv, sizeof(command), 2000);
			if(ret <= 0) {
				printf_err("%s read data fail\n", __func__);
				continue;
			}

			if(ret != 10) {
				printf_err("%s get echo num error\n", __func__);
				continue;
			} else {
				if(strncmp((const char *) GsmRcv, (const char *)command, 10) != 0) {
					printf_err("%s get echo data error\n", __func__);
					continue;
				}
			}
		}


		memset(GsmRcv, 0, sizeof(GsmRcv));
		//	+CREG: 0,1
		//	OK

		ret = read_data(GsmRcv, 22, 2000);
		if(ret < 0) {
			printf_warn("read data fail\n");
			continue;
		}

	//	for(int i = 0; i < ret; i++)
	//		printf_err("GsmRcv[%d] = 0x%x -> %c\n", i, GsmRcv[i], GsmRcv[i]);
		DBG("read net: %s\n", GsmRcv);

		if(ret >= 11 && strncmp((const char *) &GsmRcv[ret - 4], "OK\r\n", 4) == 0) {
			if(GsmRcv[ret - 11] == '0') {	//关闭显示网络注册未知结果
				if(GsmRcv[ret - 9] == '1') {	//注册的，归属网络
					printf_info("card had register to net\n");
					break;
				} else if(GsmRcv[ret - 9] == '5') {	//注册的，漫游中
					printf_err("card is Roaming\n");
				}
				printf_err("card had  not register to net\n");
			}

//			printf_err("register err\n");
		}

		sleep(3);
	}

	DBG("retry is %d\n", retry);

	if(retry <= 0)
		return false;

	unsigned char gprs_attech[] = "AT+CGATT?\r\n";			//AT+CGREG?
	unsigned char ok_chk[] = "OK";
	if( !send_command_check(gprs_attech, sizeof(gprs_attech), ok_chk)) {
			printf_err( "GPRS isn't attached\n" );
			return false;
		}
	DBG( "GPRS attached\n" );

	return true;
}


bool GPRS::get_imei( char *imei  )
{
	int ret;
	unsigned char command[] = "AT+CGSN\r\n";
	unsigned char GsmRcv[256] = {0};

	memset(GsmRcv, 0, sizeof(GsmRcv));

	//先清空接受缓冲区
	flush_recv();
	DBG("Get IMEI  send %s", command);

	ret = send_data(command, sizeof(command) );
	if(ret <= 0) {
		printf_err("send data fail\n");
		return false;
	}

	//AT+CSQ
	if(echo_enable) {
		DBG( " echo_enable is true\n" );
		ret = read_data(GsmRcv, sizeof(command), 2000);
		if(ret <= 0) {
			printf_err("%s read data fail\n", __func__);
			return false;
		}

		if(strncmp((const char *) GsmRcv, (const char *)command, sizeof(command) ) != 0) {
			printf_err("%s get echo data error\n", __func__);
			return false;
		}
	}

	memset(GsmRcv, 0, sizeof(GsmRcv));

	ret = read_data(GsmRcv, 100, 2000);
	if(ret < 0) {
		printf_warn("read data fail\n");
		return false;
	}

//	for(int i = 0; i < ret; i++)
//		printf_err("GsmRcv[%d] = 0x%x\n", i, GsmRcv[i]);
//	printf_err( "\n" );

	if(ret >= 6 && strncmp((const char *) &GsmRcv[ret - 6], "\r\nOK\r\n", 6) == 0) {
		DBG("Get IMEI back: %s\n", GsmRcv);
		memcpy( imei, (const void*)(&GsmRcv[2]), 15);		//IMEI是15位的串号
	}
	return true;
}

bool GPRS::connect_server( unsigned char * ip_server, int ip_len )
{
//	unsigned char test[] =  "AT\r\n";		//测试
	unsigned char single_cmd[] =  "AT+CIPMUX=0\r\n";		//设置单链路
	unsigned char queryapn_cmd[] =  "AT+CSTT?\r\n";		//设置APN，
	unsigned char setapn_cmd[] =  "AT+CSTT=\"CMNET\"\r\n";		//设置APN，
	unsigned char creat_wireless[] =  "AT+CIICR\r\n";		//建立无线链路 GPRS或者CSD
	unsigned char get_localip[] =  "AT+CIFSR\r\n";		//获取本地IP地址
	unsigned char localip_chk[] =  ".";

//	unsigned char close_cmd[] = "AT+CIPCLOSE=1\r\n";		//关闭连接
//	unsigned char error_chk[] = "ERROR";
//
//	unsigned char shutup_cmd[] = "AT+CIPSHUT\r\n";			//关闭移动场景
//	unsigned char shutup_cmd_chk[] = "SHUT OK";

//	unsigned char single_cmd[] =  "AT+CIPMUX=0\r\n";		//设置单链路
	unsigned char set_move_type[] = "AT+CGCLASS=\"B\"\r\n";		//设置GPRS移动台类别为B,支持包交换和数据交换
	unsigned char pdp_context[] = "AT+CGDCONT=1,\"IP\",\"CMNET\"\r\n";		//设置PDP上下文,互联网接协议,接入点等信息
	unsigned char attach_gprs[] = "AT+CGATT=1\r\n";		//附着GPRS业务
//	unsigned char gprs_connect[] = "AT+CIPCSGP=1,\"CMNET\"\r\n";		//设置为GPRS连接模式
	unsigned char ip_title[] = "AT+CIPHEAD=0\r\n";		//设置接收数据显示IP头(方便判断数据来源,仅在单路连接有效)+IPD, length:
	unsigned char senddata_mode[] = "AT+CIPQSEND=0\r\n";	//设置慢发送模式
	unsigned char smd_addcheck[] =  "AT+CIPSPRT=1\r\n";		//发送命令是有> SEND OK

	unsigned char ok_chk[] = "OK";

#if 0
	printf("AT+CIPCLOSE=1\r\n");	//关闭连接
  delay_ms(100);
	Second_AT_Command("AT+CIPSHUT","SHUT OK",2,2);		//关闭移动场景
  delay_ms(5);
	Second_AT_Command("AT+CGCLASS=\"B\"","OK",2,2);//设置GPRS移动台类别为B,支持包交换和数据交换
  delay_ms(5);
	Second_AT_Command("AT+CGDCONT=1,\"IP\",\"CMNET\"","OK",2,2);//设置PDP上下文,互联网接协议,接入点等信息
  delay_ms(5);
	Second_AT_Command("AT+CGATT=1","OK",2,2);//附着GPRS业务
  delay_ms(5);
	Second_AT_Command("AT+CIPCSGP=1,\"CMNET\"","OK",2,2);//设置为GPRS连接模式
  delay_ms(5);
	Second_AT_Command("AT+CIPHEAD=1","OK",2,2);//设置接收数据显示IP头(方便判断数据来源,仅在单路连接有效)
  delay_ms(5);
	Second_AT_Command((char*)string_TCP,"OK",5,2);//发送tcp ip连接
#endif

	int retry = 3;
	msleep(200);
//	if( !send_command_check(test, sizeof(test), ok_chk)) return false;
	if( !send_command_check(single_cmd, sizeof(single_cmd), ok_chk)) return false;
	msleep(200);
	if( !send_command_check(queryapn_cmd, sizeof(queryapn_cmd), ok_chk)) return false;
	msleep(200);
	if( !send_command_check(setapn_cmd, sizeof(setapn_cmd), ok_chk)) return false;
	msleep(200);
	if( !send_command_check(creat_wireless, sizeof(creat_wireless), ok_chk)) return false;
	msleep(200);
	if( !send_command_check(get_localip, sizeof(get_localip), localip_chk)) return false;
	msleep(200);

	if( !send_command_check(set_move_type, sizeof(set_move_type), ok_chk)) return false;
	msleep(200);
	if( !send_command_check(pdp_context, sizeof(pdp_context), ok_chk)) return false;
	msleep(200);
	if( !send_command_check(attach_gprs, sizeof(attach_gprs), ok_chk)) return false;
//	if( !send_command_check(gprs_connect, sizeof(gprs_connect), ok_chk)) return false;
	msleep(200);
	if( !send_command_check(ip_title, sizeof(ip_title), ok_chk)) return false;
	msleep(200);
	if( !send_command_check(senddata_mode, sizeof(senddata_mode), ok_chk)) return false;
	msleep(200);
	if( !send_command_check(smd_addcheck, sizeof(smd_addcheck), ok_chk)) return false;
	msleep(200);
//	if( !send_command_check(close_cmd, sizeof(close_cmd), ok_chk) &&
//			!send_command_check(close_cmd, sizeof(close_cmd), error_chk)) return false;
//	if( !send_command_check(shutup_cmd, sizeof(shutup_cmd), shutup_cmd_chk)) return false;
//	if( !send_command_check(single_cmd, sizeof(single_cmd), ok_chk)) return false;
//	if( !send_command_check(set_move_type, sizeof(set_move_type), ok_chk)) return false;
//	if( !send_command_check(pdp_context, sizeof(pdp_context), ok_chk)) return false;
//	if( !send_command_check(attach_gprs, sizeof(attach_gprs), ok_chk)) return false;
//	if( !send_command_check(gprs_connect, sizeof(gprs_connect), ok_chk)) return false;
//	if( !send_command_check(ip_title, sizeof(ip_title), ok_chk)) return false;


	while( retry-- ){
		if(! send_command_check( ip_server, ip_len, ok_chk) ){
			msleep(100);
			printf_err( "connetc server ip failed %d times\n", 3-retry );
			continue;
		}else{
			DBG( "connecting to server......\n" );
			break;
		}
	}


	if( retry <= 0 )
		return false;

	if( !tcp_status_check() )			//测试socket是否连接成功
		return false;

	return true;
}

bool GPRS::send_command_check( unsigned char *cmd, int cmd_len, unsigned char *check)
{
#if 0
	//打印调用此函数的线程ID，便于调试分析
	pid_t		pid;
	pthread_t	tid;

	pid = getpid();
	tid = pthread_self();
	DBG("pid %lu tid %lu (0x%lx) call this function: send_command_check()\n", (unsigned long)pid,
	  (unsigned long)tid, (unsigned long)tid);
#endif

	int ret;
	unsigned char GsmRcv[256] = {0};
	int retry = 3;
	unsigned char sms_ready[] = "SMS Ready";

//	DBG("send command len :%d---%s\n", cmd_len, cmd);

	while( retry-- ){

		ret = send_data(cmd, cmd_len );
		if(ret <= 0) {
			printf_err("send data fail\n");
			continue;
		}

		memset(GsmRcv, 0, sizeof(GsmRcv));
		ret = read_data(GsmRcv, 100, 5000);
		if(ret < 0) {
			printf_warn("read data fail\n");
			continue;
		}

		if( strstr((const char *)GsmRcv, (const char *)sms_ready) != NULL ){
			DBG( "recived SMS Ready\n" );
			retry++;
			continue;
		}

		if( strstr((const char *)GsmRcv, (const char *)check) == NULL){
			DBG("error-----------cmd: %s\n back: %s\n", cmd, GsmRcv);
			msleep(50);
			DBG("command back right: %s\n", GsmRcv);
			continue;
		}else{
//			DBG("command back right: %s\n", GsmRcv);
			break;
		}
	}

	if( retry <= 0 ){
		return false;
	}

	return true;

}

bool GPRS::send_command_nocheck( unsigned char *cmd, int cmd_len )
{
	if( !nochk ){
		unsigned char smd_addcheck[] =  "AT+CIPSPRT=2\r\n";		//发送命令是无> SEND OK
		unsigned char ok_chk[] = "OK";
		if( !send_command_check(smd_addcheck, sizeof(smd_addcheck), ok_chk)) return false;
		msleep(200);
		nochk = true;
		DBG( "Set data format: no > && SEND OK\n" );
	}
#if 0
	//打印调用此函数的线程ID，便于调试分析
	pid_t		pid;
	pthread_t	tid;

	pid = getpid();
	tid = pthread_self();
	DBG("pid %lu tid %lu (0x%lx) call this function: send_command_check()\n", (unsigned long)pid,
	  (unsigned long)tid, (unsigned long)tid);
#endif

	int ret;
	int retry = 3;

//	DBG("send command len :%d---%s\n", cmd_len, cmd);

	while( retry-- ){
		//先清空发送缓冲区
		flush_send();

		ret = send_data(cmd, cmd_len );
		if( (ret <= 0) || (ret!=cmd_len) ) {
			printf_err("send data fail\n");
			continue;
		}

		if( ret == cmd_len ){
//			DBG( "send command no check success\n " );
			break;
		}
	}

	if( retry <= 0 ){
//		printf_err( "\n\n***********************can't register net***************************\n\n" );
		return false;
	}

	return true;
}
bool GPRS::send_tcp_data( unsigned char * tcp_data, int data_len )
{
#if 0
	//打印调用此函数的线程ID，便于调试分析
	pid_t		pid;
	pthread_t	tid;

	pid = getpid();
	tid = pthread_self();
	DBG("pid %lu tid %lu (0x%lx) call this function: send_tcp_data()\n", (unsigned long)pid,
	  (unsigned long)tid, (unsigned long)tid);
#endif
//	unsigned char tcp_status[] = "AT+CIPSTATUS\r\n";
//	unsigned char ok_chk[] = "OK";

	unsigned char tcp_start[] = "AT+CIPSEND\r\n";
	unsigned char tcp_start_chk[] = ">";

//	unsigned char data_start[] = "\x1A";
	unsigned char tcp_data_chk[] = "SEND OK";

	int times = 3;
	bool flags = true;

//	if( !send_command_check(tcp_status, sizeof(tcp_status), ok_chk)) return false;

	while( times-- ){
		if( !send_command_check(tcp_start, sizeof(tcp_start), tcp_start_chk)) {
			flags = false;
			msleep(500);
			continue;
		}else{
			flags = true;
			break;
		}
	}

	if( (flags == false) || (times <= 0) ){
		printf_err( "send tcp start error\n" );
		return false;
	}

//	DBG("send command len :%d---%s\n", data_len, tcp_data );
	if( !send_command_check(tcp_data, data_len, tcp_data_chk))  return false;

//	if( !send_command_check(data_start, sizeof(data_start), tcp_data_chk))  return false;

	return true;
}

bool GPRS::send_tcp_data_no_chk( unsigned char * tcp_data, int data_len )
{
#if 0
	//打印调用此函数的线程ID，便于调试分析
	pid_t		pid;
	pthread_t	tid;

	pid = getpid();
	tid = pthread_self();
	DBG("pid %lu tid %lu (0x%lx) call this function: send_tcp_data()\n", (unsigned long)pid,
	  (unsigned long)tid, (unsigned long)tid);
#endif

	unsigned char tcp_start[] = "AT+CIPSEND\r\n";
	int times = 3;
	bool flags = true;

	while( times-- ){
		if( !send_command_nocheck(tcp_start, sizeof(tcp_start)) ) {
			flags = false;
			msleep(500);
			continue;
		}else{
			flags = true;
			break;
		}
	}

	if( (flags == false) || (times <= 0) ){
		printf_err( "send tcp start error\n" );
		return false;
	}

	if( !send_command_nocheck(tcp_data, data_len)) {
		fprintf( stderr, "Send data no check failed\n" );
		return false;
	}
	return true;
}

bool GPRS::tcp_status_check()
{
#if 0
	//打印调用此函数的线程ID，便于调试分析
	pid_t		pid;
	pthread_t	tid;

	pid = getpid();
	tid = pthread_self();
	DBG("pid %lu tid %lu (0x%lx) call this function: tcp_status_check()\n", (unsigned long)pid,
	  (unsigned long)tid, (unsigned long)tid);
#endif
	unsigned char tcp_status[] = "AT+CIPSTATUS\r\n";
	unsigned char tcp_chk[] = "CONNECT OK";
	unsigned char ing_chk[] = "TCP CONNECTING";
	unsigned char close_chk[] = "CLOSED";
	unsigned char GsmRcv[256] = {0};

	int ret;
	int times = 100;		//3分钟
	while( times-- ){
		//先清空接受缓冲区
		flush_recv();

		ret = send_data(tcp_status, sizeof(tcp_status) );
		if(ret <= 0) {
			printf_err("send data fail\n");
			continue;
		}

		memset(GsmRcv, 0, sizeof(GsmRcv));
		ret = read_data(GsmRcv, 150, 2000);
		if(ret < 0) {
			printf_warn("read data fail\n");
			continue;
		}
		fprintf( stderr, " ......\n");
		if( strstr((const char *)GsmRcv, (const char *)tcp_chk) == NULL){
			msleep(200);
//			DBG("command back right: %s\n", GsmRcv);
			if( strstr((const char *)GsmRcv, (const char *)ing_chk) == NULL ){
				fprintf( stderr, " ......\n");
				continue;
			}
			if( strstr((const char *)GsmRcv, (const char *)close_chk) != NULL ){
				fprintf( stderr, " Server has closed socket......\n");
				return false;
			}
			continue;
		}else{
			DBG("connected to server: %s\n", GsmRcv);
			break;
		}
	}
	if( times <= 0 )
		return false;

	return true;
}


bool GPRS::rec_tcp_data( std::string &data_buf, int &data_len, bool &has_heart )
{
#if 0
	//打印调用此函数的线程ID，便于调试分析
	pid_t		pid;
	pthread_t	tid;

	pid = getpid();
	tid = pthread_self();
	DBG("pid %lu tid %lu (0x%lx) call this function: rec_tcp_data()\n", (unsigned long)pid,
	  (unsigned long)tid, (unsigned long)tid);
#endif
	has_heart = false;
	int ret;
	unsigned  char rec_buf[MAX_REC];
	char json_result[MAX_REC];
	unsigned  char tmp_buf[100];

	memset( rec_buf, 0, MAX_REC);
	memset( tmp_buf, 0, sizeof(tmp_buf) );

//	+IPD,
	msleep(500);
	ret = read_data( tmp_buf, sizeof(tmp_buf), 2000);
	if(ret < 0) {
		printf_warn("read data fail\n");
		return false;
	}
	DBG( "Received temp data: %s\n", tmp_buf );

	if( strcmp( (const char*)tmp_buf, "Heart" ) && strlen((const char*)tmp_buf) == sizeof("Heart") ){
		has_heart = true;
		data_buf = "Heart";
		return true;
	}

	//获取数据长度
	char *p_l, *p_r;
	int chr_l = '{';
	int chr_r = '}';
	p_l = strchr((char *)tmp_buf, chr_l);
	if( p_l == NULL ){
		printf_err( "First receive without '{', so judge error\n" );
		data_buf = "ERRORED";
		return false;
	}

	strncpy( (char *)rec_buf, (const char *)tmp_buf, ret );

	int times = 1;
	while( times < (MAX_REC/32-1) ){
		memset( tmp_buf, 0, sizeof(tmp_buf) );
		ret = read_data( tmp_buf, sizeof(tmp_buf), 1000);
		if(ret < 0) {
			printf_warn("read data fail\n");
			continue;
		}

		DBG( "Received temp data: %s\n", tmp_buf );
		p_r = strchr((char *)tmp_buf, chr_r);
		if(p_r == NULL ){
			times++;
			strncat((char *)rec_buf, (const char*)tmp_buf, ret );
			continue;
		}else{
			strncat((char *)rec_buf, (const char*)tmp_buf, ret );
			break;
		}
	}
	if(times == (MAX_REC/32-1) && strchr((char *)tmp_buf, chr_r) == NULL ){
		printf_err( "Recived too much times or can't recived '{', so judge error\n" );
		data_buf = "ERRORED";
		return false;
	}
	char heart[] = "Heart";
	if( strstr((const char*)rec_buf, (const char*)heart) != NULL ){
		DBG( "Received Heart******\n" );
		has_heart = true;
	}
	p_l = strchr((char *)rec_buf, chr_l);
	p_r = strchr((char *)rec_buf, chr_r);

	int length_l_r = p_r - p_l + 1;
	if( length_l_r > 0 ){
		strncpy(json_result, p_l, length_l_r);
	}

	data_buf = json_result;
	DBG( "Received data len: %d, %s\n", data_buf.length(), data_buf.c_str() );

	return true;
#if 0
	//获取数据长度
	char *p_l, *p_r;
	int chr_l = '{';
	int chr_r = '}';
	p_l = strchr((char *)tmp_buf, chr_l);
	p_r = strchr((char *)tmp_buf, chr_r);
	if( p_l == NULL || p_r == NULL ){
		printf_err( "can't received , or : \n" );
		return false;
	}

	int num_wid = p_m - p_d -1;
//	DBG( "p_m-p_d: %d\n", num_wid );
	int len_tmp = 0;
	for( int i = 0; i < num_wid; i++ ){
		len_tmp += p_d[i+1] - '0' + 0;
		len_tmp *= 10;
	}
	len_tmp = len_tmp/10+8+num_wid;
//	DBG( "len_tmp: %d\n", len_tmp );

	strncpy( (char *)rec_buf, (const char *)tmp_buf, ret );

	int times = 0, len_sum = ret;
	while( (len_sum < len_tmp) && (times < MAX_REC/32-1) ){
		memset( tmp_buf, 0, sizeof(tmp_buf) );
		ret = read_data( tmp_buf, sizeof(tmp_buf), 1000);
		if(ret < 0) {
			printf_warn("read data fail\n");
			continue;
		}
//		DBG( "Received temp data: %s\n", tmp_buf );
		times++;
		strncat((char *)rec_buf, (const char*)tmp_buf, ret );
		len_sum += ret;
		if( len_sum == len_tmp){
//			DBG( "Received data %d times correct\n", times+1 );
			break;
		}
	}

	if( (len_sum < len_tmp) || (times >= MAX_REC/32-1) ){		//最大接受MAX_REC 512字节，每次接受32字节，次数为512/32-1
		printf_err( "received data error\n" );
		return false;
	}

	data_buf = strchr((char *)rec_buf, chr_m)+1;
	data_len = len_tmp-8-num_wid;
//
//	DBG( "Received data: %s\n", rec_buf );
	DBG( "Received data len: %d, %s\n", data_buf.length(), data_buf.c_str() );

	return true;
#endif
}
