/*
 * serial.cpp
 *
 *  Created on: Sep 30, 2016
 *      Author: xiao
 */
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "serial.h"

#define DBG_EN 	0
#if (DBG_EN == 1)
#define printf_dbg(x,arg...) printf("[serial_debug]"x,##arg)
#else
#define printf_dbg(x,arg...)
#endif

#define printf_info(x,arg...) printf("[serial_info] "x,##arg)
#define printf_warn(x,arg...) printf("[serial_warnning] "x,##arg)
#define printf_err(x,arg...) printf("[serial_error] "x,##arg)

#define MESSAGE_LEVEL (1)

#define  CMSPAR 010000000000


serial::serial()
{
	fd = -1;
}

int serial::dev_open(char const *file_name, bool block)
{
	if(!file_name)
		return -EINVAL;

	if(block) {
		fd = open(file_name, O_RDWR | O_NOCTTY);
	} else {
		fd = open(file_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
	}

	if(fd < 0) {
		printf_err("open %s fail, %s (%d)\n",
				file_name, strerror(errno), errno);
		return -1;
	}

	printf_dbg("open %s OK, fd = %d\n", file_name, fd);
	return fd;
}

int serial::dev_open(char const *file_name)
{
	if(!file_name)
		return -EINVAL;

	//fd = open(file_name, O_RDWR | O_NOCTTY);
	fd = open(file_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if(fd < 0) {
		printf_err("open %s fail, %s (%d)\n", file_name, strerror(errno), errno);
		return -1;
	}

	printf_dbg("open %s OK, fd = %d\n", file_name, fd);
	return fd;
}

void serial::dev_close(void)
{
	close(fd);
	fd = -1;
}

int serial::setopt(int nSpeed, int flow_ctrl, int nBits, int nStop, char nEvent)
{
	struct termios newtio,oldtio;

	/*保存测试现有串口参数设置,在这里如果串口号等出错,会有相关的出错信息*/
	if (tcgetattr(fd, &oldtio) != 0) {
		perror("Save Serial setting fail");
		return -1;
	}

	bzero(&newtio, sizeof(newtio));
	tcgetattr(fd, &newtio);
	//Initialize to raw mode. PARMRK and PARENB will be over-ridden before calling tcsetattr()
//	cfmakeraw(&newtio);

	/*设置波特率*/
	switch( nSpeed )
	{
		case 2400:
			cfsetispeed(&newtio, B2400);
			cfsetospeed(&newtio, B2400);
			break;

		case 4800:
			cfsetispeed(&newtio, B4800);
			cfsetospeed(&newtio, B4800);
			break;

		case 9600:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
			break;

		case 19200:
			cfsetispeed(&newtio, B19200);
			cfsetospeed(&newtio, B19200);
			break;

		case 38400:
			cfsetispeed(&newtio, B38400);
			cfsetospeed(&newtio, B38400);
			break;

		case 57600:
			cfsetispeed(&newtio, B57600);
			cfsetospeed(&newtio, B57600);
			break;

		case 115200:
			cfsetispeed(&newtio, B115200);
			cfsetospeed(&newtio, B115200);
			break;

		case 460800:
			cfsetispeed(&newtio, B460800);
			cfsetospeed(&newtio, B460800);
			break;

		default:
			return -EINVAL;
	}

//	//Initialize to raw mode. PARMRK and PARENB will be over-ridden before calling tcsetattr()
	cfmakeraw(&newtio);

	/*
	 * Enable the receiver and set local mode...
	 */
	newtio.c_cflag |= (CLOCAL | CREAD);

	/*步骤一,设置字符大小*/
	newtio.c_cflag &= ~CSIZE;
	switch(nBits)
	{
		case 5:
			newtio.c_cflag |= CS5;
			break;

		case 6:
			newtio.c_cflag |= CS6;
			break;

		case 7:
			newtio.c_cflag |= CS7;
			break;

		case 8:
			newtio.c_cflag |= CS8;
			break;

		default:
			return -EINVAL;
	}

	/*设置停止位*/
	if(nStop == 1)
		newtio.c_cflag &= ~CSTOPB;	/*停止位为1*/
	else if (nStop == 2)
		newtio.c_cflag |= CSTOPB;
	else
		return -EINVAL;

	/*设置奇偶校验位*/
	switch( nEvent )
	{
		case 'O': //奇数
		case 'o':
			newtio.c_cflag |= PARENB;
			newtio.c_cflag |= PARODD;
			newtio.c_iflag |= INPCK;
			break;

		case 'E': //偶数
		case 'e':
			newtio.c_cflag |= PARENB;
			newtio.c_cflag &= ~PARODD;
			newtio.c_iflag |= INPCK;
			break;

		case 'N': //无奇偶校验位
		case 'n':
			newtio.c_cflag &= ~PARENB;
			break;

		case 'M'://模式位设置为1
		case 'm':
			newtio.c_cflag |= PARENB;
			newtio.c_cflag |= PARODD;
			newtio.c_cflag |= CMSPAR;
			break;

			// Use space parity to get 3-byte sequence (0xff 0x00 <address>) on address byte
		case 'S':		//模式位为0
		case 's':
			newtio.c_cflag |= CMSPAR;//Set"stick" parity (either mark or space)
			newtio.c_cflag &= ~PARODD;//Select space parity so that only address byte causes error
//			newtio.c_cflag |= PARENB;			// Enable parity generation

			//NOTE: The following block overrides PARMRK and PARENB bits cleared by cfmakeraw.
			newtio.c_cflag |= PARENB;//Enable parity generation
			newtio.c_iflag |= INPCK;//Enable parity checking
			newtio.c_iflag |= PARMRK;//Enable in-band marking
			newtio.c_iflag &= ~IGNPAR;//Make sure input parity errors are not ignored
			break;

		default:
			return -EINVAL;
	}

	//设置数据流控制
	switch(flow_ctrl)
	{
		case 0 :	//不使用流控制
			newtio.c_cflag &= ~CRTSCTS;	//No HW flow control
			newtio.c_cflag &= ~(IXON | IXOFF);	//Set the input flags to disable in-band flow control
			break;

		case 1 :	//使用硬件流控制
			newtio.c_cflag |= CRTSCTS;
			break;

		case 2 :	//使用软件流控制
			newtio.c_iflag |= (IXON | IXOFF | IXANY);
			break;
	}

#if 0
	/*设置等待时间和最小接收字符*/
	newtio.c_cc[VTIME] = 0; //每个单位是0.1秒  20就是2秒
	newtio.c_cc[VMIN] = 2;
#else
	/*设置等待时间和最小接收字符*/
//	newtio.c_cc[VTIME] = 0; //每个单位是0.1秒  20就是2秒
//	newtio.c_cc[VMIN] = 0;
#endif
	//如果不是开发终端之类的，只是串口传输数据，而不需要串口来处理,那么使用原始模式(Raw Mode)方式来通讯.
	//原始输入根本不会被处理,输入字符只是被原封不动的接收.
	//一般情况中，如果要使用原始输入模式，程序中需要去掉ICANON，ECHO，ECHOE和ISIG选项：
//	newtio.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
//	newtio.c_oflag  &= ~OPOST;   /*Output*/

	/*处理未接收字符*/
	tcflush(fd,TCIFLUSH);

	/*激活新配置*/
	if(tcsetattr(fd,TCSANOW, &newtio) != 0) {
		perror("serial com set error");
		return -1;
	}

	return 0;
}

int serial::send_data(unsigned char  const *send_buf, int data_len)
{
#if 0
	//打印调用此函数的线程ID，便于调试分析
	pid_t		pid;
	pthread_t	tid;

	pid = getpid();
	tid = pthread_self();
	printf_dbg("pid %lu tid %lu (0x%lx) call this function: send_data()\n", (unsigned long)pid,
	  (unsigned long)tid, (unsigned long)tid);
	printf_dbg("ready to send data len %d\n", data_len);
#endif
	int ret;
	ret = write(fd, send_buf, data_len);
	if (ret < 0) {
		printf_err("write bytes failed,%d\n",ret);
		return ret;
	} else if(ret != data_len) {
		printf_err("send data len %d ,but just send %d\n",data_len, ret);
		tcflush(fd, TCOFLUSH);
	}
//	fsync(fd);
	printf_dbg("finally send data len: %d\n",ret);
	return ret;
}

/* 毫秒级 延时 */
void serial::msleep(int ms)
{
	struct timeval delay;

	if(ms == 0) return;

	delay.tv_sec = 0;
	delay.tv_usec = ms * 1000;
	select(0, NULL, NULL, NULL, &delay);
}

int serial::recv_data(unsigned char *recv_buf,
		int data_len,
		unsigned int wait_ms)
{
	int ret;
	int retry = 0;
	int bytes;
	int cnt = 0;

	printf_dbg("%s ready to read data len %d\n", __func__, data_len);

	while(cnt < data_len)
	{
		if(retry >= 20)
			return cnt;

		ret = ioctl(fd, FIONREAD, &bytes);
		if(ret >= 0) {
			printf_dbg("FIONREADG get buffer len %d\n", bytes);

			if(bytes > 0) {
				ret = read(fd, recv_buf , bytes);
				if (ret <= 0) {
					printf_err("read bytes failed,%d\n",ret);
					retry++;
					continue;
				}

				printf_dbg("just recev %d\n", ret);

				cnt += ret;
				recv_buf += ret;
			}
		} else {
			printf_err("%s ioctl FIONREAD error\n", __func__);
			retry++;
			continue;
		}

		msleep(wait_ms);
	}

	return cnt;
}


int serial::read_data(unsigned char * const rcv_buf,
		unsigned int data_len,
		unsigned int wait_ms)
{
#if 0
	//打印调用此函数的线程ID，便于调试分析
	pid_t		pid;
	pthread_t	tid;

	pid = getpid();
	tid = pthread_self();
	printf_dbg("pid %lu tid %lu (0x%lx) call this function: read_data()\n", (unsigned long)pid,
	  (unsigned long)tid, (unsigned long)tid);
#endif
	unsigned int len;
	int fs_sel;
	fd_set fs_read;
	struct timeval time;

	FD_ZERO(&fs_read);
	FD_SET(fd, &fs_read);

	if(wait_ms >= 1000)
		time.tv_sec = wait_ms / 1000;
	else
		time.tv_sec = 0;

	time.tv_usec = (wait_ms % 1000) * 1000;

	//使用select实现串口的多路通信
	fs_sel = select(fd + 1, &fs_read, NULL ,NULL, &time);
	if(fs_sel > 0) {
		if (FD_ISSET(fd, &fs_read)) {
			len = read(fd, rcv_buf, data_len);
		} else {
			printf_err("do no isset\n");
			return -1;
		}
	} else if(fs_sel == 0) {
		printf_dbg("read time out\n");
		return -1;
	} else {
		printf_err("select error\n");
		return -1;
	}

	if (len <= 0) {
		printf_err("read bytes failed,%d\n", len);
		return len;
	}
#if	(MESSAGE_LEVEL > 1)
	else if (len != data_len) {
		printf_warn("read data len %d ,but just recev %d\n",data_len, len);
	}
#endif

#if 0 && DEBUG
	for(int i = 0; i < len; i++)
		printf_dbg("read data ,%d : %c -> 0x%x\n", i, rcv_buf[i], rcv_buf[i]);
#endif

	printf_dbg("finally rece data len %d\n", len);

	return len;
}

/*
 *
 *    TCIFLUSH
 *             flushes data received but not read.
 *
 *    TCOFLUSH
 *             flushes data written but not transmitted.
 *
 *    TCIOFLUSH
 *            flushes both data received but not read,
 *             and data written but not transmitted.
 */
void serial::flush_recv()
{
	if(fd < 0) return;
	tcflush(fd, TCIFLUSH);
}

void serial::flush_send()
{
	if(fd < 0) return;
	tcflush(fd, TCOFLUSH);
}

void serial::flush_all()
{
	if(fd < 0) return;
	tcflush(fd, TCIOFLUSH);
}

