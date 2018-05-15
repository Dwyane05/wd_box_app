/*
 * msqueue.cpp
 *
 *  Created on: Jan 16, 2018
 *      Author: cui
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "msqueue.h"
#include "weiding.h"


bool queue_init( int *msqid )
{
	key_t key;

	if ( -1 == (key = ftok( FILEPATH, ID)) ){
		err_sys("ftok error!\n");
		return false;
	}
	if ( -1 == (*msqid = msgget(key, MSG_MODE)) ){
		err_sys("message queue already exitsted!\n");
		return false;
	}
	DBG( "Create queue success, ID: %d\n\n", *msqid );
	return true;
}

void query_queue_status( int msqid )
{
	struct msqid_ds msg_buf;
	if (msgctl(msqid, IPC_STAT, &msg_buf) == -1) {
		err_sys("ftok error!\n");
	}
	DBG("msgid: %d\n", msqid);
	DBG("msg_perm.uid: %d\n", (int)msg_buf.msg_perm.uid);
	DBG("msg_perm.gid: %d\n", (int)msg_buf.msg_perm.gid);
	DBG("msg_stime: %d\n", (int)msg_buf.msg_stime);
	DBG("msg_rtime: %d\n", (int)msg_buf.msg_rtime);
	DBG("msg_qnum: %d\n", (int)msg_buf.msg_qnum);
	DBG("msg_qbytes: %d\n", (int)msg_buf.msg_qbytes);
}

bool send_queue_msg(int *msqid, struct mymesg *sendbuf, int flag )
{
//	DBG( "send_queue_msg start--- msqid: %d\n", *msqid );
	if( -1 == (msgsnd(*msqid, sendbuf, sizeof(struct mymesg), flag)) ){
		err_sys("message sent error!\n");
		return false;
	}
//	DBG( "send_queue_msg end\n" );
	return true;
}

bool rcv_queuq_msg( int msqid, struct mymesg *rcv_buf, long type, int flag)
{
	int err;
	if ( -1 == (err = msgrcv(msqid, rcv_buf, sizeof(struct mymesg), type, flag)) ){
		err_sys( " heart received message error!!!");
		return false;
	}
	return true;
}

bool queue_delete( int msqid )
{
	if( msgctl( msqid, IPC_RMID, 0) == -1){
		err_sys( "delete message queue error!!" );
		return false;
	}
	return true;
}
