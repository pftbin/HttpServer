// HttpServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <set>

#include "event.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/bufferevent_ssl.h"
#include "event2/buffer_compat.h"
#include "event2/bufferevent_compat.h"
#include "evhttp.h"
#include "event2/http.h"
#include "event2/http_struct.h"
#include "event2/keyvalq_struct.h"
#include "event2/util.h"
#include "event2/listener.h"
#include "event2/thread.h"
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>

//
#include "public.h"
#include "json.h"
#include "httpkit.h"
#include "awsUpload.h"
#include "mmRabbitmq.h"
#include "videoOperate.h"
#include "digitalmysql.h"
#include "digitalEntityJson.h"
#include "httpConcurrencyServer.h"

#pragma comment(lib,"ws2_32.lib")



#define CHECK_REQUEST_STR(name,str,msg,result) {if(str.empty()){msg=std::string(name)+" is empty,please check request body";result=false;}}
#define CHECK_REQUEST_NUM(name,num,msg,result) {if(num==0){msg=std::string(name)+" = 0,please check request body";result=false;}}
#define BUFF_SZ 1024*16  //system max stack size

#define PNP_SYMBOL					0x1111
#define PNP_REGISTER				1
#define PNP_BROADCAST				2
#define PNP_HEARTBEAT				3
#define PNP_MOSMESSAGE				4

#if defined WIN32
#define COMMON_STRCPY(x, y, z) strcpy_s(x, z, y)
#else
#define COMMON_STRCPY(x, y, z) strncpy(x, y, z);
#endif

//sleep function
#if defined WIN32
    #include <windows.h>

    void sleep(unsigned milliseconds)
    {
        Sleep(milliseconds);
    }
#else
    #include <unistd.h>

    void sleep(unsigned milliseconds)
    {
        usleep(milliseconds * 1000); // takes microseconds
    }
#endif

//get exe path 
#if defined WIN32
	#include <direct.h>
	std::string getexepath()
	{
		char* buffer = NULL;
		buffer = _getcwd(NULL, 0);
		if (buffer) 
		{
			std::string path = buffer;
			free(buffer);
			return path;
		}

		return "";
	}
#else
	#include <unistd.h>
	std::string getexepath()
	{
		char* buffer = NULL;
		buffer = getcwd(NULL, 0);
		if (buffer) 
		{
			std::string path = buffer;
			free(buffer);
			return path;
		}

		return "";
	}
#endif


#if 1 //环境参数

//合成视频节点
typedef struct _actornode
{
	std::string ip;
	short port;
}actornode, * pactornode;
typedef std::map<std::string, actornode> ACTORNODE_MAP;
ACTORNODE_MAP Container_actornode;
bool getconfig_actornode(std::string configfilepath, std::string& error)
{
	long length = 0;
	char* configbuffer = nullptr;
	FILE* fp = nullptr;
	fp = fopen(configfilepath.c_str(), "r");
	if (fp != nullptr)
	{
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		rewind(fp);

		configbuffer = (char*)malloc(length * sizeof(char));
		if (configbuffer == nullptr) return false;

		fread(configbuffer, length, 1, fp);
		fclose(fp);
	}

	if (length && configbuffer)
	{
		std::string value = "";
		std::string config = configbuffer;
		free(configbuffer);

		value = getnodevalue(config, "actor_count"); CHECK_CONFIG("actor_count", value, error);
		int count = atoi(value.c_str());
		_debug_to(1, ("CONFIG actornode count = %d\n"), count);

		Container_actornode.clear();
		for (int i = 0; i < count; i++)
		{
			std::string ip = ""; short port = 0;

			std::string actor_pro = ""; char temp[256] = { 0 };
			snprintf(temp, 256, "actor%d_", i); actor_pro = temp;

			std::string actor_ip = actor_pro + "ip";
			value = getnodevalue(config, actor_ip);
			if (value.empty()) continue;
			ip = value;
			_debug_to(1, ("CONFIG actornode actor%d_ip = %s\n"), i, ip.c_str());

			std::string actor_port = actor_pro + "port";
			value = getnodevalue(config, actor_port);
			if (value.empty()) continue;
			port = atoi(value.c_str());
			_debug_to(1, ("CONFIG actornode actor%d_port = %d\n"), i, port);

			actornode actornodeitem;
			actornodeitem.ip = ip;
			actornodeitem.port = port;
			Container_actornode.insert(std::make_pair(ip, actornodeitem));
		}

		return true;
	}

	return false;
}

//AWS上传云盘
bool aws_enable = false;
std::string aws_url = "";
std::string aws_ak = "";
std::string aws_sk = "";
std::string aws_bucket = "";
bool getconfig_aws(std::string configfilepath, std::string& error)
{
	long length = 0;
	char* configbuffer = nullptr;
	FILE* fp = nullptr;
	fp = fopen(configfilepath.c_str(), "r");
	if (fp != nullptr)
	{
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		rewind(fp);

		configbuffer = (char*)malloc(length * sizeof(char));
		if (configbuffer == nullptr) return false;

		fread(configbuffer, length, 1, fp);
		fclose(fp);
	}

	if (length && configbuffer)
	{
		std::string value = "";
		std::string config = configbuffer;
		free(configbuffer);

		value = getnodevalue(config, "aws_enable");
		aws_enable = (atoi(value.c_str())>0);
		_debug_to(1, ("CONFIG aws_enable = %s\n"), (aws_enable?("true") : ("false")));

		value = getnodevalue(config, "aws_url"); CHECK_CONFIG("aws_url", value, error);
		aws_url = value.c_str();
		_debug_to(1, ("CONFIG aws_url = %s\n"), aws_url.c_str());

		value = getnodevalue(config, "aws_ak"); CHECK_CONFIG("aws_ak", value, error);
		aws_ak = value.c_str();
		_debug_to(1, ("CONFIG aws_ak = %s\n"), aws_ak.c_str());

		value = getnodevalue(config, "aws_sk"); CHECK_CONFIG("aws_sk", value, error);
		aws_sk = value.c_str();
		_debug_to(1, ("CONFIG aws_sk = %s\n"), aws_sk.c_str());

		value = getnodevalue(config, "aws_bucket"); CHECK_CONFIG("aws_bucket", value, error);
		aws_bucket = value.c_str();
		_debug_to(1, ("CONFIG aws_bucket = %s\n"), aws_bucket.c_str());

		return true;
	}

	return false;
}

//其他全局配置
std::string  delay_beforetext = "[p500]";
std::string  delay_aftertext = "[p300]";
std::string  folder_digitalmodel = "";//本地模型路径
std::string  folder_htmldigital = "";//本地WEB服务器路径下，数字人文件路径<task>+<keyframe>+<resource>
std::string  key_certificate = "";
std::string  key_private = "";
bool getconfig_global(std::string configfilepath, std::string& error)
{
	long length = 0;
	char* configbuffer = nullptr;
	FILE* fp = nullptr;
	fp = fopen(configfilepath.c_str(), "r");
	if (fp != nullptr)
	{
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		rewind(fp);

		configbuffer = (char*)malloc(length * sizeof(char));
		if (configbuffer == nullptr) return false;

		fread(configbuffer, length, 1, fp);
		fclose(fp);
	}

	if (length && configbuffer)
	{
		std::string value = "";
		std::string config = configbuffer;
		free(configbuffer);

		char temp[256] = { 0 };
		value = getnodevalue(config, "delay_beforetext");
		if (value.empty()) value = "500";
		snprintf(temp, 256, "[p%s]", value.c_str());
		delay_beforetext = temp;
		_debug_to(1, ("CONFIG delay_beforetext = %s\n"), delay_beforetext.c_str());

		value = getnodevalue(config, "delay_aftertext");
		if (value.empty()) value = "300";
		snprintf(temp, 256, "[p%s]", value.c_str());
		delay_aftertext = temp;
		_debug_to(1, ("CONFIG delay_aftertext = %s\n"), delay_aftertext.c_str());

		//
		value = getnodevalue(config, "folder_digitalmodel"); CHECK_CONFIG("folder_digitalmodel", value, error);
		folder_digitalmodel = value.c_str();
		_debug_to(1, ("CONFIG folder_digitalmodel = %s\n"), folder_digitalmodel.c_str());

		value = getnodevalue(config, "folder_htmldigital"); CHECK_CONFIG("folder_htmldigital", value, error);
		folder_htmldigital = value.c_str();
		_debug_to(1, ("CONFIG folder_htmldigital = %s\n"), folder_htmldigital.c_str());

		//
		value = getnodevalue(config, "key_certificate");
		key_certificate = value.c_str();
		_debug_to(1, ("CONFIG key_certificate = %s\n"), key_certificate.c_str());

		value = getnodevalue(config, "key_private");
		key_private = value.c_str();
		_debug_to(1, ("CONFIG key_private = %s\n"), key_private.c_str());

		return true;
	}

	return false;
}

#endif

#if 1//AWS上传+下载

//其他文件(临时/公共)
bool uploadfile_public(std::string sourcepath_local, std::string& sourcepath_http)
{
	if (sourcepath_local.empty())
		return false;
	sourcepath_local = str_replace(sourcepath_local, std::string("//"), std::string("\\\\")); //兼容共享路径

	awsUpload uploadObj;
	uploadObj.SetAWSConfig(aws_url, aws_ak, aws_sk, aws_bucket);
	std::string object_folder = std::string("Public");

	//上传 
	bool result = true;
	if (!uploadObj.UploadFile(object_folder, sourcepath_local, sourcepath_http, true))
	{
		result = false;
		_debug_to(0, ("upload public file failed: %s\n"), sourcepath_local.c_str());
	}
	return result;
}
//背景资源上传
bool uploadfile_backsource(std::string sourcepath_local, std::string& sourcepath_http)
{
	if (sourcepath_local.empty())
		return false;
	sourcepath_local = str_replace(sourcepath_local, std::string("//"), std::string("\\\\")); //兼容共享路径

	awsUpload uploadObj;
	uploadObj.SetAWSConfig(aws_url, aws_ak, aws_sk, aws_bucket);
	std::string object_folder = std::string("BackSource");

	//上传 
	bool result = true;
	if (!uploadObj.UploadFile(object_folder, sourcepath_local, sourcepath_http, true))
	{
		result = false;
		_debug_to(0, ("upload backsource file failed: %s\n"), sourcepath_local.c_str());
	}
	return result;
}
//原始视频上传
bool uploadfile_originalvdo(std::string humanid, std::string sourcepath_local, std::string& sourcepath_http)
{
	if (sourcepath_local.empty() || humanid.empty())
		return false;
	sourcepath_local = str_replace(sourcepath_local, std::string("//"), std::string("\\\\")); //兼容共享路径

	awsUpload uploadObj;
	uploadObj.SetAWSConfig(aws_url, aws_ak, aws_sk, aws_bucket);
	std::string object_folder = std::string("OriginalVideo/") + humanid;

	//上传 
	bool result = true;
	if (!uploadObj.UploadFile(object_folder, sourcepath_local, sourcepath_http, true))
	{
		result = false;
		_debug_to(0, ("upload originalvideo file failed: %s\n"), sourcepath_local.c_str());
	}
	return result;
}
//成品素材上传 [架构决定本程序不调用此函数]
bool uploadfile_product(std::string humanid, std::string sourcepath_local, std::string& sourcepath_http)
{
	if (sourcepath_local.empty() || humanid.empty())
		return false;
	sourcepath_local = str_replace(sourcepath_local, std::string("//"), std::string("\\\\")); //兼容共享路径

	awsUpload uploadObj;
	uploadObj.SetAWSConfig(aws_url, aws_ak, aws_sk, aws_bucket);
	std::string object_folder = std::string("Product/") + humanid;

	//上传 
	bool result = true;
	if (!uploadObj.UploadFile(object_folder, sourcepath_local, sourcepath_http, true))
	{
		result = false;
		_debug_to(0, ("upload product file failed: %s\n"), sourcepath_local.c_str());
	}
	return result;
}
//修复OSS地址[OSS->https / 原地址]
std::string fixpath_from_osspath(std::string objectfile_path)
{
	std::string result = objectfile_path;
	if (str_prefixsame(objectfile_path, std::string("OSS:")))//认为是OSS路径
	{
		objectfile_path = str_replace(objectfile_path, std::string("OSS:"), std::string(""));
		awsUpload uploadObj;
		uploadObj.SetAWSConfig(aws_url, aws_ak, aws_sk, aws_bucket);
		if (!uploadObj.GetHttpFilePath(objectfile_path, result))
			result = objectfile_path;
	}

	return result;
}

#endif

#if 1 //rabbitmq消息

//连接到rabbitmq服务
static std::string rabbitmq_ip = "";
static short	   rabbitmq_port = 5672;
static std::string rabbitmq_user = "";
static std::string rabbitmq_passwd = "";
//发送者指定+接受者使用
static std::string rabbitmq_exchange = "";  //消息属性1
static std::string rabbitmq_routekey = "";  //消息熟悉2
//接收者指定+接受者使用
static std::string rabbitmq_queuename = ""; //消息队列名
bool getconfig_rabbitmq(std::string configfilepath, std::string& error)
{
	long length = 0;
	char* configbuffer = nullptr;
	FILE* fp = nullptr;
	fp = fopen(configfilepath.c_str(), "r");
	if (fp != nullptr)
	{
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		rewind(fp);

		configbuffer = (char*)malloc(length * sizeof(char));
		if (configbuffer == nullptr) return false;

		fread(configbuffer, length, 1, fp);
		fclose(fp);
	}

	if (length && configbuffer)
	{
		std::string value = "";
		std::string config = configbuffer;
		free(configbuffer);

		value = getnodevalue(config, "rabbitmq_ip"); CHECK_CONFIG("rabbitmq_ip", value, error);
		rabbitmq_ip = value;
		_debug_to(1, ("CONFIG rabbitmq_ip = %s\n"), rabbitmq_ip.c_str());

		value = getnodevalue(config, "rabbitmq_port"); CHECK_CONFIG("rabbitmq_port", value, error);
		rabbitmq_port = atoi(value.c_str());
		_debug_to(1, ("CONFIG rabbitmq_port = %d\n"), rabbitmq_port);

		value = getnodevalue(config, "rabbitmq_user"); CHECK_CONFIG("rabbitmq_user", value, error);
		rabbitmq_user = value;
		_debug_to(1, ("CONFIG rabbitmq_user = %s\n"), rabbitmq_user.c_str());

		value = getnodevalue(config, "rabbitmq_passwd"); CHECK_CONFIG("rabbitmq_passwd", value, error);
		rabbitmq_passwd = value;
		_debug_to(1, ("CONFIG rabbitmq_passwd = %s\n"), rabbitmq_passwd.c_str());

		//
		value = getnodevalue(config, "rabbitmq_exchange"); CHECK_CONFIG("rabbitmq_exchange", value, error);
		rabbitmq_exchange = value;
		_debug_to(1, ("CONFIG rabbitmq_exchange = %s\n"), rabbitmq_exchange.c_str());

		value = getnodevalue(config, "rabbitmq_routekey"); CHECK_CONFIG("rabbitmq_routekey", value, error);
		rabbitmq_routekey = value;
		_debug_to(1, ("CONFIG rabbitmq_routekey = %s\n"), rabbitmq_routekey.c_str());

		return true;
	}

	return false;
}

std::string getNotifyMsg_ToHtml(int taskid)
{
	std::string result_str = "";
	digitalmysql::taskinfo newstateitem;
	digitalmysql::gettaskinfo(taskid, newstateitem);

	//message data
	int task_id = newstateitem.taskid;
	int task_state = newstateitem.taskstate;
	int task_progress = newstateitem.taskprogress;
	std::string video_path = fixpath_from_osspath(newstateitem.video_path);
	std::string video_keyframe = fixpath_from_osspath(newstateitem.video_keyframe);
	if (video_path.empty())
	{
		std::string taskhumanid = newstateitem.humanid;
		digitalmysql::humaninfo taskhumanitem;
		if (digitalmysql::gethumaninfo(taskhumanid, taskhumanitem))
			video_keyframe = fixpath_from_osspath(taskhumanitem.keyframe);
	}

	//message json
	char tempbuff[1024] = { 0 };
	snprintf(tempbuff, 1024, "{\"TaskID\":%d, \"State\":%d, \"Progerss\":%d, \"VedioFile\":\"%s\", \"FilePath\":\"%s\" }", task_id, task_state, task_progress, video_path.c_str(), video_keyframe.c_str());
	result_str = tempbuff;

	return result_str;
}

//全局对象
nsRabbitmq::cwRabbitmqPublish* g_RabbitmqSender = nullptr;
bool sendRabbitmqMsg(std::string mqmessage)
{
	if (g_RabbitmqSender == nullptr)
	{
		_debug_to(0, ("Rabbitmq object is null,please restart...\n"));
		return false;
	}

	std::vector<std::string>   vecMessage;
	vecMessage.push_back(mqmessage);

	nsRabbitmq::mmRabbitmqData Rabbitmq_data;
	Rabbitmq_data.index = 0;
	Rabbitmq_data.moreStr = "";
	Rabbitmq_data.moreInt = 0;
	Rabbitmq_data.exchange = rabbitmq_exchange;
	Rabbitmq_data.routekey = rabbitmq_routekey;
	Rabbitmq_data.commandVector.assign(vecMessage.begin(), vecMessage.end());
	g_RabbitmqSender->send(Rabbitmq_data);

	return true;
}

#endif

#if 1//TCP消息

typedef struct tagDGHDR
{
	u_int		type;		// type of packet
	u_int		len;		// total length of packet
	u_short     symbol;
	u_long      msg;
	u_short     checksum;	// checksum of header
	u_short     off;		// data offset in the packet
}DG_HDR, * LPDG_HDR;
typedef struct tagPLAYOUTNODE
{
	u_short		channel;
	u_short		studio;
	u_long		type;
}PLAYOUTNODE, * LPPLAYOUTNODE;
typedef struct tagPNPHDR
{
	u_char			ver;		// version
	u_char			type;		// type of packet
	u_short			symbol;		// symbol of RPC packet, it must equal 0x1111
	u_int			len;		// total length of packet
	PLAYOUTNODE		src;
	u_long			dst;
	u_long			msg;
	u_short			checksum;	// checksum of header
	u_short			off;		// data offset in the packet
}PNP_HDR, * LPPNP_HDR;

bool IsValidPacket_DGHDR(const char* buf, int len)
{
	LPDG_HDR pHdr = (LPDG_HDR)buf;
	if (pHdr == NULL) return false;

	if (len < sizeof(DG_HDR))		return false;
	if (pHdr->symbol != PNP_SYMBOL)	return false;
	if (pHdr->off < sizeof(DG_HDR))	return false;
	if (pHdr->off > pHdr->len)		return false;
	if (pHdr->len > (u_int)len)		return false;

	if (Checksum((u_short*)pHdr, pHdr->off) != 0)
		return false;

	return true;
}
bool IsValidPacket_PNPHDR(const char* buf, int len)
{
	LPPNP_HDR pHdr = (LPPNP_HDR)buf;

	if (len < sizeof(PNP_HDR))       return false;
	if (pHdr->ver > 1)              return false;
	if (pHdr->symbol != PNP_SYMBOL) return false;
	if (pHdr->off < sizeof(PNP_HDR)) return false;
	if (pHdr->off > pHdr->len)      return false;
	if (pHdr->len > (u_int)len)     return false;

	if (Checksum((u_short*)pHdr, pHdr->off) != 0)
		return false;

	return true;
}
bool SendTcpMsg_DGHDR(std::string ip, short port, std::string sendmsg, bool brecv, std::string& recvmsg, long recv_timeout = 3)
{
	std::wstring uni_msg;
	ansi_to_unicode(sendmsg.c_str(), sendmsg.length(), uni_msg);

	char buff[BUFF_SZ] = { 0 };
	snprintf(buff, BUFF_SZ, "%s", sendmsg.c_str());

	int sfd = -1;
	struct sockaddr_in serveraddr;

	bool bRet = true;
	if (bRet)//建立套接字
	{
		sfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sfd == -1)
		{
			perror(("socket failed\n"));
			bRet = false;
		}
	}

	if (bRet)//发起连接请求
	{
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons(port);
		serveraddr.sin_addr.s_addr = inet_addr(ip.c_str());
		if (connect(sfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1)
		{
			_debug_to(0, ("addr: %s:%u ,connect failed\n"), inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port));
			bRet = false;
		}
	}

	if (bRet)//发送buf中的内容
	{
		char* data = (char*)uni_msg.c_str();
		int   datalen = uni_msg.length() * 2;

		DG_HDR hdr;
		hdr.type = 0;
		hdr.len = sizeof(hdr) + datalen;
		hdr.symbol = PNP_SYMBOL;
		hdr.msg = 0;//default
		hdr.checksum = 0;
		hdr.off = sizeof(hdr);
		hdr.checksum = Checksum((u_short*)&hdr, hdr.off);

		int bufferlen = sizeof(hdr) + datalen;
		char* pbuffer = new char[bufferlen];
		memcpy(pbuffer, &hdr, sizeof(hdr));
		memcpy(pbuffer + sizeof(hdr), data, datalen);

		int ret = send(sfd, pbuffer, bufferlen, 0);
		if (ret <= 0)
		{
			std::string sendmsg_utf8; ansi_to_utf8(sendmsg.c_str(), sendmsg.length(), sendmsg_utf8);
			_debug_to(0, ("addr: %s:%u ,send message failed: %s\n"), inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), sendmsg_utf8.c_str());
			bRet = false;
		}
		else
		{
			std::string sendmsg_utf8; ansi_to_utf8(sendmsg.c_str(), sendmsg.length(), sendmsg_utf8);
			_debug_to(0, ("addr: %s:%u ,send message success: %s\n"), inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), sendmsg_utf8.c_str());
		}
		delete[] pbuffer;
	}

	if (bRet && brecv)
	{
		//----------------------------------------------------//
			//timeout recv message
		struct timeval tv;
		tv.tv_sec = recv_timeout;//s
		tv.tv_usec = 0;
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(sfd, &readfds);
		select(sfd + 1, &readfds, NULL, NULL, &tv);

		int recvsize = 0;
		char recvbuff[1024] = { 0 };
		if (FD_ISSET(sfd, &readfds))
			recvsize = recv(sfd, recvbuff, sizeof(recvbuff), 0);//接收消息
		if (recvsize > 0)
		{
			if (IsValidPacket_DGHDR(recvbuff, recvsize))
			{
				LPDG_HDR pHdr = (LPDG_HDR)recvbuff;
				if (pHdr->off == 0) pHdr->off = sizeof(DG_HDR);//fix error
				recvsize = recvsize - pHdr->off;

				char tempbuff[1024] = { 0 };
				memcpy(tempbuff, &(recvbuff[pHdr->off]), sizeof(char) * recvsize);
				memset(recvbuff, 0, sizeof(char) * 1024);
				memcpy(recvbuff, tempbuff, sizeof(char) * 1024);
			}

			std::wstring uni_recvmsg;
			uni_recvmsg = (wchar_t*)recvbuff;
			unicode_to_ansi(uni_recvmsg.c_str(), uni_recvmsg.length(), recvmsg);
		}
		//----------------------------------------------------//
	}

exitsend:
	closesocket(sfd);
	return bRet;
}
bool SendTcpMsg_PNPHDR(std::string ip, short port, std::string sendmsg, bool brecv, std::string& recvmsg, long recv_timeout = 3)
{
	std::wstring uni_msg;
	ansi_to_unicode(sendmsg.c_str(), sendmsg.length(), uni_msg);

	char buff[BUFF_SZ] = { 0 };
	snprintf(buff, BUFF_SZ, "%s", sendmsg.c_str());

	int sfd = -1;
	struct sockaddr_in serveraddr;

	bool bRet = true;
	if (bRet)//建立套接字
	{
		sfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sfd == -1)
		{
			perror(("socket failed\n"));
			bRet = false;
		}
	}

	if (bRet)//发起连接请求
	{
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons(port);
		serveraddr.sin_addr.s_addr = inet_addr(ip.c_str());
		if (connect(sfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1)
		{
			_debug_to(0, ("addr: %s:%u ,connect failed\n"), inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port));
			bRet = false;
		}
	}

	if (bRet)//发送buf中的内容
	{
		char* data = (char*)uni_msg.c_str();
		int   datalen = uni_msg.length() * 2;

		PNP_HDR hdr;
		hdr.ver = 1;
		hdr.type = 0;
		hdr.symbol = PNP_SYMBOL;
		hdr.len = sizeof(hdr)+ datalen;
		hdr.src.channel = 0;
		hdr.src.studio = 0;
		hdr.src.type = 0;
		hdr.dst = 0;
		hdr.msg = 0;
		hdr.checksum = 0;
		hdr.off = sizeof(hdr);
		hdr.checksum = Checksum((u_short*)&hdr, hdr.off);

		int bufferlen = sizeof(hdr) + datalen;
		char* pbuffer = new char[bufferlen];
		memcpy(pbuffer, &hdr, sizeof(hdr));
		memcpy(pbuffer + sizeof(hdr), data, datalen);

		int ret = send(sfd, pbuffer, bufferlen, 0);
		if (ret <= 0)
		{
			_debug_to(0, ("addr: %s:%u ,send message failed: %s\n"), inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), sendmsg.c_str());
			bRet = false;
		}
		else
		{
			_debug_to(0, ("addr: %s:%u ,send message success: %s\n"), inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), sendmsg.c_str());
		}
		delete[] pbuffer;
	}

	if (bRet && brecv)
	{
		//----------------------------------------------------//
			//timeout recv message
		struct timeval tv;
		tv.tv_sec = recv_timeout;//s
		tv.tv_usec = 0;
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(sfd, &readfds);
		select(sfd + 1, &readfds, NULL, NULL, &tv);

		int recvsize = 0;
		char recvbuff[1024] = { 0 };
		if (FD_ISSET(sfd, &readfds))
			recvsize = recv(sfd, recvbuff, sizeof(recvbuff), 0);//接收消息
		if (recvsize > 0)
		{
			if (IsValidPacket_PNPHDR(recvbuff, recvsize))
			{
				LPPNP_HDR pHdr = (LPPNP_HDR)recvbuff;
				if (pHdr->off == 0) pHdr->off = sizeof(PNP_HDR);//fix error
				recvsize = recvsize - pHdr->off;
				

				char tempbuff[1024] = { 0 };
				memcpy(tempbuff, &(recvbuff[pHdr->off]), sizeof(char) * recvsize);
				memset(recvbuff, 0, sizeof(char) * 1024);
				memcpy(recvbuff, tempbuff, sizeof(char) * 1024);
			}

			std::wstring uni_recvmsg;
			uni_recvmsg = (wchar_t*)recvbuff;
			unicode_to_ansi(uni_recvmsg.c_str(), uni_recvmsg.length(), recvmsg);
		}
		//----------------------------------------------------//
	}

exitsend:
	closesocket(sfd);
	return bRet;
}

#endif

#if 1//接口返回json

std::string getjson_error(int code,std::string errmsg,std::string data = "")
{
	std::string result = "";

	char buff[BUFF_SZ] = { 0 };
	snprintf(buff, BUFF_SZ, "{ \"code\": %d, \"msg\": \"%s\",\"data\":{%s} }", code, errmsg.c_str(), data.c_str());result = buff;

	return result;
}

//
std::string getjson_usertoken(std::string username, std::string password)
{
	bool result = true;
	std::string errmsg = "";
	std::string result_str = "";

	char buff[BUFF_SZ] = { 0 };
	std::string key = "secret";
	std::string header = "{\"alg\":\"HS256\",\"type\":\"JWT\"}";
	std::string payload = "";
	snprintf(buff, BUFF_SZ, "{\"username\":\"%s\",\"password\":\"%s\",\"accesstime\":%lld}", username.c_str(), password.c_str(), gettimecount());payload = buff;
	
	std::string token = "";
	if (string_to_token(header, payload, key, token))
		result_str = token;

	//header = "";payload = "";
	//token_to_string(token, key, header, payload);

	return result_str;
}

//
std::string getjson_humanlistinfo(std::string humanid = "")
{
	bool result = true;
	std::string errmsg = "";
	std::string result_str = "";
	
	//1-getlist
	digitalmysql::VEC_HUMANINFO vechumaninfo;
	result = digitalmysql::gethumanlistinfo(humanid, vechumaninfo);
	_debug_to(0, ("gethumanlistinfo: vechumaninfo size=%d\n"), vechumaninfo.size());
	if (!result)
		errmsg = "gethumanlistinfo from mysql failed";

	//2-parsedata
	std::string list_info = "";
	DigitalMan_Items result_object;
	for(size_t i = 0; i < vechumaninfo.size(); i++)
	{
		DigitalMan_Item result_item;
		result_item.HumanID = vechumaninfo[i].humanid;
		result_item.HumanName = vechumaninfo[i].humanname;
		result_item.SpeakSpeed = vechumaninfo[i].speakspeed;
		result_item.Foreground = fixpath_from_osspath(vechumaninfo[i].foreground);
		result_item.Background = fixpath_from_osspath(vechumaninfo[i].background);
		//KeyFrame
		std::string format = "";
		std::string base64_encode = "";
		unsigned int width = 0, height = 0, bitcount = 32;
		std::string human_keyframe = vechumaninfo[i].keyframe;
		if (!aws_enable && is_existfile(human_keyframe.c_str()))//本地模式
		{
			picture::GetPicInfomation(human_keyframe.c_str(), &width, &height, &bitcount, format);
			//base64_encode = base64::base64_encode_file(filepath);
		}
		result_item.KeyFrame_Format = format;
		result_item.KeyFrame_Width = width;
		result_item.KeyFrame_Height = height;
		result_item.KeyFrame_BitCount = bitcount;
		result_item.KeyFrame_FilePath = fixpath_from_osspath(human_keyframe);
		result_item.KeyFrame_KeyData = base64_encode;
		result_object.vecDigitManItems.push_back(result_item);	
	}

	//3-writejson
	if(result)
		list_info = result_object.writeJson();

	//4-return
	if (!result)
	{
		result_str = getjson_error(1, errmsg, "");
	}
	else
	{
		std::string code = "\"code\": 0,";
		std::string msg = "\"msg\": \"success\",";

		std::string temp_humanlist = "\"HumanList\": [ " + list_info + "]";
		list_info = temp_humanlist;

		result_str = "{" + code + msg + "\"data\":{" + list_info + "}" + "}";//too long,must use string append
	}

	return result_str;
}

//
std::string getjson_humanhistoryinfo(digitalmysql::VEC_FILTERINFO& vecfilterinfo, std::string order_key = "createtime", int order_way = 1, int pagesize = 10, int pagenum = 1)
{
	bool result = true;
	std::string errmsg = "";
	std::string result_str = "";

	//1-getlist
	int tasktotal = 0; digitalmysql::VEC_TASKINFO vectaskhistory;
	result = digitalmysql::gettaskhistoryinfo(vecfilterinfo, order_key, order_way, pagesize, pagenum, tasktotal, vectaskhistory);
	_debug_to(0, ("get taskhistory size=%d\n"), vectaskhistory.size());
	if (!result)
		errmsg = "gettaskhistoryinfo from mysql failed";

	//2-parsedata
	std::string other_info = "";
	std::string history_info = "";
	DigitalMan_Tasks result_object;
	for (size_t i = 0; i < vectaskhistory.size(); i++)
	{
		DigitalMan_Task result_item;
		result_item.TaskID = vectaskhistory[i].taskid;
		result_item.TaskType = vectaskhistory[i].tasktype;
		result_item.TaskMoodType = vectaskhistory[i].moodtype;
		result_item.TaskName = vectaskhistory[i].taskname;
		result_item.TaskState = vectaskhistory[i].taskstate;
		result_item.TaskProgerss = vectaskhistory[i].taskprogress;
		result_item.TaskSpeakSpeed = vectaskhistory[i].speakspeed;
		result_item.TaskInputSsml = vectaskhistory[i].ssmltext;
		result_item.TaskCreateTime = vectaskhistory[i].createtime;
		result_item.TaskHumanID = vectaskhistory[i].humanid;
		result_item.TaskHumanName = vectaskhistory[i].humanname;
		result_item.Foreground = fixpath_from_osspath(vectaskhistory[i].foreground);
		result_item.Background = fixpath_from_osspath(vectaskhistory[i].background);
		result_item.Front_left = vectaskhistory[i].front_left;
		result_item.Front_right = vectaskhistory[i].front_right;
		result_item.Front_top = vectaskhistory[i].front_top;
		result_item.Front_bottom = vectaskhistory[i].front_bottom;

		//KeyFrame
		std::string format = "";
		std::string base64_encode = "";
		unsigned int width = 0, height = 0, bitcount = 32;
		std::string task_keyframe = vectaskhistory[i].video_keyframe;
		if (!aws_enable && is_existfile(task_keyframe.c_str()))//本地模式
		{
			picture::GetPicInfomation(task_keyframe.c_str(), &width, &height, &bitcount, format);
			//base64_encode = base64::base64_encode_file(filepath);
		}
		result_item.KeyFrame_Format = format;
		result_item.KeyFrame_Width = width;
		result_item.KeyFrame_Height = height;
		result_item.KeyFrame_BitCount = bitcount;
		result_item.KeyFrame_FilePath = fixpath_from_osspath(task_keyframe);
		result_item.KeyFrame_KeyData = base64_encode;

		result_item.Audio_Format = vectaskhistory[i].audio_format;
		result_item.Audio_File   = fixpath_from_osspath(vectaskhistory[i].audio_path);
		result_item.Video_Format = vectaskhistory[i].video_format;
		result_item.Video_Width  = vectaskhistory[i].video_width;
		result_item.Video_Height = vectaskhistory[i].video_height;
		result_item.Video_Fps    = vectaskhistory[i].video_fps;
		result_item.Video_File   = fixpath_from_osspath(vectaskhistory[i].video_path);
		result_object.vecDigitManTasks.push_back(result_item);
	}

	//3-writejson
	if (result)
		history_info = result_object.writeJson();	

	//4-return
	if (!result)
	{
		result_str = getjson_error(1, errmsg, "");
	}
	else
	{
		std::string code = "\"code\": 0,";
		std::string msg = "\"msg\": \"success\",";

		char temp[256] = { 0 };
		std::string temp_otherinfo;
		snprintf(temp, 256, "\"DataTotal\":%d, \"PageSize\":%d, \"PageNum\":%d,", tasktotal, pagesize, pagenum); temp_otherinfo = temp;
		other_info = temp_otherinfo;

		std::string temp_humandata = "\"HumanData\": [ " + history_info + "]";
		history_info = temp_humandata;

		result_str = "{" + code + msg + "\"data\":{" + other_info + history_info + "}" + "}";//too long,must use string append
	}

	return result_str;
}

//
std::string getjson_tasksourcelistinfo()
{
	bool result = true;
	std::string errmsg = "";
	std::string result_str = "";

	//1-getlist
	digitalmysql::VEC_TASKSOURCEINFO vectasksourceinfo;
	result = digitalmysql::gettasksourcelist(vectasksourceinfo);
	_debug_to(0, ("gettasksourcelist: vectasksourceinfo size=%d\n"), vectasksourceinfo.size());
	if (!result)
		errmsg = "gettasksourcelist from mysql failed";

	//2-parsedata
	std::string image_list_info = "";
	DigitalMan_TaskSources image_result_object;
	for (size_t i = 0; i < vectasksourceinfo.size(); i++)
	{
		if (vectasksourceinfo[i].sourcetype == digitalmysql::source_image)
		{
			DigitalMan_TaskSource result_item;
			result_item.TaskSource_Type = digitalmysql::source_image;
			result_item.TaskSource_FilePath = fixpath_from_osspath(vectasksourceinfo[i].sourcepath);		//此路径为https或本地路径，不会是OSS路径
			result_item.TaskSource_KeyFrame = "";
			image_result_object.vecDigitManTaskSources.push_back(result_item);
		}
	}
	std::string video_list_info = "";
	DigitalMan_TaskSources video_result_object;
	for (size_t j = 0; j < vectasksourceinfo.size(); j++)
	{
		if (vectasksourceinfo[j].sourcetype == digitalmysql::source_video)
		{
			DigitalMan_TaskSource result_item;
			result_item.TaskSource_Type = digitalmysql::source_video;
			result_item.TaskSource_FilePath = fixpath_from_osspath(vectasksourceinfo[j].sourcepath);		//此路径为https或本地路径，不会是OSS路径
			result_item.TaskSource_KeyFrame = fixpath_from_osspath(vectasksourceinfo[j].sourcekeyframe);	//此路径为https或本地路径，不会是OSS路径
			video_result_object.vecDigitManTaskSources.push_back(result_item);
		}
	}
	std::string audio_list_info = "";
	DigitalMan_TaskSources audio_result_object;
	for (size_t i = 0; i < vectasksourceinfo.size(); i++)
	{
		if (vectasksourceinfo[i].sourcetype == digitalmysql::source_audio)
		{
			DigitalMan_TaskSource result_item;
			result_item.TaskSource_Type = digitalmysql::source_audio;
			result_item.TaskSource_FilePath = fixpath_from_osspath(vectasksourceinfo[i].sourcepath);		//此路径为https或本地路径，不会是OSS路径
			result_item.TaskSource_KeyFrame = "";
			audio_result_object.vecDigitManTaskSources.push_back(result_item);
		}
	}

	//3-writejson
	if (result)
	{
		image_list_info = image_result_object.writeJson();
		video_list_info = video_result_object.writeJson();
		audio_list_info = audio_result_object.writeJson();
	}

	//4-return
	if (!result)
	{
		result_str = getjson_error(1, errmsg, "");
	}
	else
	{
		std::string code = "\"code\": 0,";
		std::string msg = "\"msg\": \"success\",";

		std::string temp_imagelist = "\"ImageList\": [ " + image_list_info + "], ";
		image_list_info = temp_imagelist;
		std::string temp_videolist = "\"VideoList\": [ " + video_list_info + "], ";
		video_list_info = temp_videolist;
		std::string temp_audiolist = "\"AudioList\": [ " + audio_list_info + "] ";
		audio_list_info = temp_audiolist;

		result_str = "{" + code + msg + "\"data\":{" + image_list_info + video_list_info + audio_list_info + "}" + "}";//too long,must use string append
	}

	return result_str;
}

#endif

#if 1 //VideoMake

//Actorinfo 结构体
typedef struct _actorinfo
{
	std::string ip;
	short port;
	int state; //-1=error,0=free,1=busy
	long firstworktick;//if tickcount too long,as error

	_actorinfo()
	{
		ip = "";
		port = 0;
		state = -1;
		firstworktick = 0;
	}

	void copydata(const _actorinfo& item)
	{
		ip = item.ip;
		port = item.port;
		state = item.state;
		firstworktick = item.firstworktick;
	}
}actorinfo, * pactorinfo;
typedef std::map<std::string, actorinfo> ACTORINFO_MAP;
ACTORINFO_MAP Container_actorinfo;
pthread_mutex_t mutex_actorinfo;// actorinfo互斥量

//ActorTaskinfo 结构体
typedef struct _actortaskinfo
{
	int			ActorTaskID;
	int			ActorTaskType;
	int			ActorMoodType;
	double		ActorTaskSpeed;
	std::string ActorTaskName;
	std::string ActorTaskText;
	std::string ActorTaskAudio;
	std::string ActorTaskHumanID;
	int			ActorTaskState;				//-1=waitmerge,0=merging,1=mergesuccess,2=mergefailed
	//指定模型文件
	std::string AcousticModelFullPath;		//../0ModelFile/test/TTS/speak/snapshot_iter_1668699.pdz
	std::string VcoderModelFullPath;		//../0ModelFile/test/TTS/pwg/snapshot_iter_1000000.pdz
	std::string PTHModelsPath;				//../0ModelFile/test/W2L/file/xxx.pth
	std::string DFMModelsPath;				//../0ModelFile/test/W2L/file/shenzhen_v3_20230227.dfm

	_actortaskinfo()
	{
		ActorTaskID = 0;
		ActorTaskType = 1;
		ActorMoodType = 0;
		ActorTaskSpeed = 1.0;
		ActorTaskName = "";
		ActorTaskText = "";
		ActorTaskAudio = "";
		ActorTaskHumanID = "";
		ActorTaskState = -1;

		AcousticModelFullPath = "";
		VcoderModelFullPath = "";
		PTHModelsPath = "";
		DFMModelsPath = "";
	}

	void copydata(const _actortaskinfo& item)
	{
		ActorTaskID = item.ActorTaskID;
		ActorTaskType = item.ActorTaskType;
		ActorMoodType = item.ActorMoodType;
		ActorTaskSpeed = item.ActorTaskSpeed;
		ActorTaskName = item.ActorTaskName;
		ActorTaskText = item.ActorTaskText;
		ActorTaskAudio = item.ActorTaskAudio;
		ActorTaskHumanID = item.ActorTaskHumanID;
		ActorTaskState = item.ActorTaskState;

		AcousticModelFullPath = item.AcousticModelFullPath;
		VcoderModelFullPath = item.VcoderModelFullPath;
		PTHModelsPath = item.PTHModelsPath;
		DFMModelsPath = item.DFMModelsPath;
	}

}actortaskinfo, * pactortaskinfo;
typedef std::map<int, actortaskinfo> ACTORTASKINFO_MAP;
ACTORTASKINFO_MAP Container_actortaskinfo;
pthread_mutex_t mutex_actortaskinfo;// actortaskinfo互斥量

//合成消息 
std::string getNotifyMsg_ToActor(digitalmysql::taskinfo taskitem, digitalmysql::humaninfo taskhumanitem, actortaskinfo actortaskitem)
{
	double send_left = taskitem.front_left;
	double send_top = taskitem.front_top;
	double send_right = taskitem.front_right;
	double send_bottom = taskitem.front_bottom;
	std::string send_imagematting = taskhumanitem.imagematting; send_imagematting = str_replace(send_imagematting, "\"", "\\\"");//json传递,双引号前加右斜杠

	int send_taskid = actortaskitem.ActorTaskID;
	int send_tasktype = actortaskitem.ActorTaskType;
	int send_moodtype = actortaskitem.ActorMoodType;
	double send_speakspeed = actortaskitem.ActorTaskSpeed;
	std::string send_tasktext = delay_beforetext + actortaskitem.ActorTaskText + delay_aftertext;
	std::string send_taskaudio = actortaskitem.ActorTaskAudio;
	std::string send_humanid = actortaskitem.ActorTaskHumanID;
	std::string send_speakmodel = actortaskitem.AcousticModelFullPath;
	std::string send_pwgmodel = actortaskitem.VcoderModelFullPath;
	std::string send_mouthmodel = actortaskitem.PTHModelsPath;
	std::string seng_facemodel = actortaskitem.DFMModelsPath;

	std::string send_background = taskitem.background;
	std::string send_backaudio = "";
	std::string send_messageid = getmessageid();

	//
	std::string result_msg = "";
	char msg_buff[BUFF_SZ] = { 0 };
	snprintf(msg_buff, BUFF_SZ,
		"{\"ddrinfo\":[{\"offset\":0,\"length\":0,\"pos\":{\"left\":%.6f,\"top\":%.6f,\"right\":%.6f,\"bottom\":%.6f},\"matte\":\"%s\"}],\
		\"makevideo\":{\"taskid\":%d,\"humanid\":\"%s\",\"tasktype\":%d,\"moodtype\":%d,\"speakspeed\":%.2f,\"tasktext\":\"%s\",\"taskaudio\":\"%s\",\
		\"speakmodel\":\"%s\",\"pwgmodel\":\"%s\",\"mouthmodel\":\"%s\",\"facemodel\":\"%s\"},\
		\"background\":\"%s\",\"backaudio\":\"%s\",\"msgid\":\"%s\"}",
		send_left, send_top, send_right, send_bottom, send_imagematting.c_str(),
		send_taskid, send_humanid.c_str(), send_tasktype, send_moodtype, send_speakspeed, send_tasktext.c_str(), send_taskaudio.c_str(),
		send_speakmodel.c_str(), send_pwgmodel.c_str(), send_mouthmodel.c_str(), seng_facemodel.c_str(),
		send_background.c_str(), send_backaudio.c_str(), send_messageid.c_str());
	result_msg = msg_buff;

	return result_msg;
}
//解析任务结果
bool runtask_result(digitalmysql::taskinfo taskitem, std::string json_message, std::string& json_result)
{
	std::string sourcepath_http;
	bool result = false; int taskid = taskitem.taskid;
	json::Value recv_val = json::Deserialize((char*)json_message.c_str());
	if (recv_val.GetType() == json::ObjectVal)
	{
		json::Object recv_obj = recv_val.ToObject();
		if (recv_obj.HasKey("code"))
		{
			int code = recv_obj["code"].ToInt();
			if (code == 0)
			{
				if (recv_obj.HasKey("audiopath"))
				{
					std::string audiopath_ansi = recv_obj["audiopath"].ToString();
					audiopath_ansi = str_replace(audiopath_ansi, std::string("\\"), std::string("/"));			//兼容共享路径
					digitalmysql::setaudiopath(taskid, audiopath_ansi);
				}
				if (recv_obj.HasKey("keyframe"))
				{
					std::string keyframepath_ansi = recv_obj["keyframe"].ToString();
					keyframepath_ansi = str_replace(keyframepath_ansi, std::string("\\"), std::string("/"));	//兼容共享路径
					digitalmysql::setkeyframepath(taskid, keyframepath_ansi);
				}
				if (recv_obj.HasKey("videopath"))
				{
					std::string videopath_ansi = recv_obj["videopath"].ToString();
					videopath_ansi = str_replace(videopath_ansi, std::string("\\"), std::string("/"));			//兼容共享路径
					digitalmysql::setvideopath(taskid, videopath_ansi);
				}
				if (recv_obj.HasKey("greenpath"))
				{
					std::string greenpath_ansi = recv_obj["greenpath"].ToString();
					greenpath_ansi = str_replace(greenpath_ansi, std::string("\\"), std::string("\\\\"));		//兼容共享路径
					if (is_existfile(greenpath_ansi.c_str()))
						remove(greenpath_ansi.c_str());//删除本地文件
				}
				digitalmysql::setmergestate(taskid, 1);		 //任务状态为成功
				digitalmysql::setmergeprogress(taskid, 100); //合成进度为100
			}
			else
			{
				digitalmysql::setmergestate(taskid, 2);		 //任务状态为失败
				digitalmysql::setmergeprogress(taskid, 100); //合成进度为100
			}
		}
	}

	//构造返回前端的json
	std::string errmsg = "success";
	if (taskid != 0)
	{
		digitalmysql::taskinfo taskitem;
		if (digitalmysql::gettaskinfo(taskid, taskitem))
		{
			std::string data = "";
			char data_buff[BUFF_SZ] = { 0 };
			snprintf(data_buff, BUFF_SZ, "\"TaskID\": %d,\"TaskName\":\"%s\",\"CreateTime\":\"%s\",	\
						\"Audio\":{\"AudioFormat\":\"%s\",\"AudioFile\":\"%s\"},\
						\"Vedio\":{\"VideoFormat\":\"%s\",\"Width\":%d,\"Height\":%d,\"Fps\":%.2f,\"VedioFile\":\"%s\"}",
				taskitem.taskid, taskitem.taskname.c_str(), taskitem.createtime.c_str(),
				taskitem.audio_format.c_str(), taskitem.audio_path.c_str(),
				taskitem.video_format.c_str(), taskitem.video_width, taskitem.video_height, taskitem.video_fps, taskitem.video_path.c_str()); data = data_buff;
			json_result = getjson_error(0, errmsg, data);
			result = true;
		}
		else
		{
			errmsg = "not found task in database...";
			json_result = getjson_error(1, errmsg);
		}	
	}
	else
	{
		errmsg = "recv message not found taskid...";
		json_result = getjson_error(1, errmsg);
	}

	return result;
}
//合成任务立即执行
std::string getjson_runtask_now(std::string actor_ip, short actor_port, actortaskinfo actortaskitem,int recv_timeout = 30)
{
	bool result = true;
	std::string errmsg = "success";
	std::string result_str = "";

	digitalmysql::taskinfo taskitem; digitalmysql::humaninfo taskhumanitem;
	if (digitalmysql::gettaskinfo(actortaskitem.ActorTaskID, taskitem) && digitalmysql::gethumaninfo(taskitem.humanid, taskhumanitem))
	{
		std::string sendmsg = "", recvmsg = "";
		sendmsg = getNotifyMsg_ToActor(taskitem, taskhumanitem, actortaskitem);

		//send message
		_debug_to(0, ("recv playnode message timeout = %d S\n"), recv_timeout);
		bool result = SendTcpMsg_PNPHDR(actor_ip, actor_port, sendmsg, true, recvmsg, recv_timeout);
		if (result)
		{
			_debug_to(0, ("addr: %s:%u, TaskID=%d, recv message: %s\n"), actor_ip.c_str(), actor_port, actortaskitem.ActorTaskID, recvmsg.c_str());
			runtask_result(taskitem, recvmsg, result_str);

			//通知前端
			int times = 0;
			std::string htmlnotifymsg = getNotifyMsg_ToHtml(actortaskitem.ActorTaskID);
			bool notifyresult = sendRabbitmqMsg(htmlnotifymsg);
			while (!notifyresult)//retry
			{
				++times;
				notifyresult = sendRabbitmqMsg(htmlnotifymsg);
				if (times > 3) break;
				sleep(1000);
			}
			std::string msgresult = (notifyresult) ? ("success") : ("failed");
			_debug_to(0, ("Send HTML Notify[%s]: %s\n"), msgresult.c_str(), htmlnotifymsg.c_str());
		}
		else
		{
			errmsg = "send playnode message failed";
			result_str = getjson_error(1, errmsg);
			_debug_to(0, ("error: %s\n"), errmsg.c_str());
		}
	}
	else
	{
		errmsg = "not found task in database...";
		result_str = getjson_error(1, errmsg);
		_debug_to(0, ("error: %s\n"), errmsg.c_str());
	}

	return result_str;
}
//合成任务分配执行
pthread_t threadid_runtask_thread;
void* pthread_runtask_thread(void* arg)
{
	while (true)
	{
		//找到任务
		bool bFindTask = false;
		actortaskinfo find_actortaskitem;
		ACTORTASKINFO_MAP::iterator itFindTask = Container_actortaskinfo.begin();
		for (itFindTask; itFindTask != Container_actortaskinfo.end(); ++itFindTask)
		{
			if (itFindTask->second.ActorTaskState == -1)//need merge
			{
				bFindTask = true;
				find_actortaskitem.copydata(itFindTask->second);
				//
				pthread_mutex_lock(&mutex_actortaskinfo);
				Container_actortaskinfo.erase(itFindTask);
				pthread_mutex_unlock(&mutex_actortaskinfo);
				break;
			}
		}

		//执行任务
		if (bFindTask)
		{
			ACTORINFO_MAP::iterator itFindActor = Container_actorinfo.begin();
			for (itFindActor; itFindActor != Container_actorinfo.end(); ++itFindActor)
			{
				std::string find_actorip = itFindActor->second.ip;
				short find_actorport = itFindActor->second.port;
				std::string ret_json = getjson_runtask_now(find_actorip, find_actorport, find_actortaskitem,1200);//生成视频，阻塞1200秒
			}
		}

		sleep(1000);
		//pthread_exit(nullptr);//中途退出当前线程
	}

	_debug_to(0, ("pthread_runtask_thread exit...\n"));
	return nullptr;
}
#endif

#if 1//接口保存文件

//fromdata数据保存到文件,用于调试
#define DF_FROMDATA_TOFILE 0
bool buffer_to_file(struct evbuffer* item_evbuffer, const struct evbuffer_ptr* item_offset, size_t item_bufferlen, std::string savefilefolder, std::string& savefilepath, std::string& errmsg)
{
	//以下为fromdata的格式: #代表boundary，--#是起始符和分割符， --#--是结束符
	// --# + [header + \r\n\r\n + data] + --# + [header + \r\n\r\n + data] + ... + --#--
	//函数传入offset+bufferlen，认为这段数据是单个Item的数据

	if (!create_directories(savefilefolder.c_str()))
	{
		errmsg = "create savefilefolder error. ";
		errmsg += savefilefolder;
		return false;
	}

	std::string partName_ansi, partFileName_ansi;
	//找到item起始和结束位置
	size_t pos_start_evbuffer = item_offset->pos;
	size_t pos_end_evbuffer = item_offset->pos + item_bufferlen;
	evbuffer_ptr ptritem_start, ptritem_end;
	evbuffer_ptr_set(item_evbuffer, &ptritem_start, pos_start_evbuffer, EVBUFFER_PTR_SET);
	evbuffer_ptr_set(item_evbuffer, &ptritem_end, pos_end_evbuffer, EVBUFFER_PTR_SET);


	std::string splite_tag = "\r\n\r\n";
	evbuffer_ptr ptritem_splite = evbuffer_search_range(item_evbuffer, splite_tag.c_str(), splite_tag.length(), &ptritem_start, &ptritem_end);
	if (ptritem_splite.pos > 0)
	{
		//找到header的部分,并解析name+filename
		size_t item_headlen = ptritem_splite.pos - ptritem_start.pos;// \r\n\r\n的位置 - item数据起始位置
		char* item_head = (char*)malloc(item_headlen + 1);
		if (item_head == nullptr)
		{
			errmsg = "malloc item_head buffer error...";
			return false;
		}
		memset(item_head, 0, sizeof(char) * (item_headlen + 1));
		size_t headcopy = evbuffer_copyout_from(item_evbuffer, &ptritem_start, item_head, item_headlen);
		if (headcopy == item_headlen)
		{
			std::string head_str = item_head;
			std::vector<std::string> headVector;
			globalSpliteString(head_str, headVector, ("\n"));

			int endIdx = headVector.size() - 1;
			while (headVector.size() > 0)
			{
				std::string thisLine = headVector[endIdx--];
				headVector.pop_back();

				int index = thisLine.find(':');
				if (index < 0) continue;
				const std::string header = thisLine.substr(0, index);
				if (header == "Content-Disposition")
				{
					std::string partName = getDispositionValue(thisLine, index + 1, "name");
					std::string partFileName = getDispositionValue(thisLine, index + 1, "filename");
					utf8_to_ansi(partName.c_str(), partName.length(), partName_ansi);
					utf8_to_ansi(partFileName.c_str(), partFileName.length(), partFileName_ansi);
				}
			}
		}
		free(item_head); item_head = nullptr;

		//splite offset
		size_t splite_offset = splite_tag.length();
		evbuffer_ptr_set(item_evbuffer, &ptritem_splite, splite_offset, EVBUFFER_PTR_ADD);

		//找到data的部分,保存到文件【本函数认为Item是文件】
		if (!partFileName_ansi.empty())//该段数据为文件
		{
			std::string fullfilepath = savefilepath;
			if (fullfilepath.empty())
				fullfilepath = savefilefolder + std::string("/") + partFileName_ansi;

			if (ptritem_end.pos > ptritem_splite.pos)
			{
				size_t item_datalen = ptritem_end.pos - ptritem_splite.pos;//使用修改后的pos

				FILE* fp = nullptr;
				fp = fopen(fullfilepath.c_str(), "wb");
				if (fp == nullptr)
				{
					errmsg = "opening file error... ";
					errmsg += fullfilepath;
					return false;
				}

				size_t copylen_now = 10240;
				char   copybuff_now[10240] = { 0 };
				size_t copylen_total = item_datalen;
				while (copylen_total)
				{
					if (copylen_total < 10240)
						copylen_now = copylen_total;

					size_t copylen_real = evbuffer_copyout_from(item_evbuffer, &ptritem_splite, copybuff_now, copylen_now);
					if (copylen_real == copylen_now)
					{
						fwrite(copybuff_now, sizeof(char), copylen_now, fp);
						copylen_total -= copylen_now;
						evbuffer_ptr_set(item_evbuffer, &ptritem_splite, copylen_now, EVBUFFER_PTR_ADD);
					}
				}
				fclose(fp);
				savefilepath = fullfilepath;
			}
		}
	}

	return true;
}
bool buffer_to_string(struct evbuffer* item_evbuffer, const struct evbuffer_ptr* item_offset, size_t item_bufferlen, std::string& partName, std::string& partValue, std::string& errmsg)
{
	//以下为fromdata的格式: #代表boundary，--#是起始符和分割符， --#--是结束符
	// --# + [header + \r\n\r\n + data] + --# + [header + \r\n\r\n + data] + ... + --#--
	//函数传入offset+bufferlen，认为这段数据是单个Item的数据

	std::string partName_ansi, partFileName_ansi;
	//找到item起始和结束位置
	size_t pos_start_evbuffer = item_offset->pos;
	size_t pos_end_evbuffer = item_offset->pos + item_bufferlen;
	evbuffer_ptr ptritem_start, ptritem_end;
	evbuffer_ptr_set(item_evbuffer, &ptritem_start, pos_start_evbuffer, EVBUFFER_PTR_SET);
	evbuffer_ptr_set(item_evbuffer, &ptritem_end, pos_end_evbuffer, EVBUFFER_PTR_SET);
	std::string splite_tag = "\r\n\r\n";
	evbuffer_ptr ptritem_splite = evbuffer_search_range(item_evbuffer, splite_tag.c_str(), splite_tag.length(), &ptritem_start, &ptritem_end);
	if (ptritem_splite.pos > 0)
	{
		//找到header的部分,并解析name+filename
		size_t item_headlen = ptritem_splite.pos - ptritem_start.pos;// \r\n\r\n的位置 - item数据起始位置
		char* item_head = (char*)malloc(item_headlen + 1);
		if (item_head == nullptr)
		{
			errmsg = "malloc item_head buffer error...";
			return false;
		}
		memset(item_head, 0, sizeof(char) * (item_headlen + 1));
		size_t headcopy = evbuffer_copyout_from(item_evbuffer, &ptritem_start, item_head, item_headlen);
		if (headcopy == item_headlen)
		{
			std::string head_str = item_head;
			std::vector<std::string> headVector;
			globalSpliteString(head_str, headVector, ("\n"));

			int endIdx = headVector.size() - 1;
			while (headVector.size() > 0)
			{
				std::string thisLine = headVector[endIdx--];
				headVector.pop_back();

				int index = thisLine.find(':');
				if (index < 0) continue;
				const std::string header = thisLine.substr(0, index);
				if (header == "Content-Disposition")
				{
					std::string partName = getDispositionValue(thisLine, index + 1, "name");
					std::string partFileName = getDispositionValue(thisLine, index + 1, "filename");
					utf8_to_ansi(partName.c_str(), partName.length(), partName_ansi);
					utf8_to_ansi(partFileName.c_str(), partFileName.length(), partFileName_ansi);
				}
			}
		}
		free(item_head); item_head = nullptr;

		//splite offset
		size_t splite_offset = splite_tag.length();
		evbuffer_ptr_set(item_evbuffer, &ptritem_splite, splite_offset, EVBUFFER_PTR_ADD);

		//找到data的部分,保存到文件【本函数认为data是字符串】
		if (partFileName_ansi.empty() && !partName_ansi.empty())//该段数据为字符串
		{
			size_t item_datalen = ptritem_end.pos - ptritem_splite.pos;//使用修改后的pos
			char* item_data = (char*)malloc(item_datalen + 1);
			if (item_data == nullptr)
			{
				errmsg = "malloc item_data buffer error...";
				return false;
			}

			memset(item_data, 0, sizeof(char) * (item_datalen + 1));
			size_t datacopy = evbuffer_copyout_from(item_evbuffer, &ptritem_splite, item_data, item_datalen);
			if (datacopy == item_datalen)
			{
				std::string partValue_ansi, partValue_utf8;
				partValue_utf8 = item_data;

				utf8_to_ansi(partValue_utf8.c_str(), partValue_utf8.length(), partValue_ansi);
				partValue_ansi = str_replace(partValue_ansi, std::string("\r\n"), std::string(""));

				partName = partName_ansi;
				partValue = partValue_ansi;
			}
		}
	}

	return true;
}

//添加数字人接口
bool ParseAddHuman(evkeyvalq* in_header, evbuffer* in_buffer, std::string& humanname, size_t& filecount, std::string& errmsg)
{
	//---------------fromdata----------------//
	//str_boundary_start
	//item_data1
	//str_boundary_start
	//item_data2
	//str_boundary_end
	//---------------fromdata----------------//

#if 1 //解析form-data类型的数据,前几个步骤相同

	//header
	bool bfromdata = false;
	std::string str_boundary_start = "", str_boundary_end = "";
	std::string str_boundary = evhttp_find_header(in_header, "Content-Type");
	std::string::size_type idx = str_boundary.find(std::string("multipart/form-data"));
	if (idx != std::string::npos)
	{
		str_boundary = str_replace(str_boundary, "multipart/form-data; boundary=", "");
		str_boundary_start = std::string("--") + str_boundary;
		str_boundary_end = std::string("--")+ str_boundary + std::string("--");
		bfromdata = true;
	}

	//buffer
	size_t bufflen = evbuffer_get_length(in_buffer);
	evbuffer_ptr ptr_datastart, ptr_dataend;
	evbuffer_ptr_set(in_buffer, &ptr_datastart, 0, EVBUFFER_PTR_SET);
	evbuffer_ptr_set(in_buffer, &ptr_dataend, bufflen, EVBUFFER_PTR_SET);

#if DF_FROMDATA_TOFILE	
	size_t all_bufferlen = bufflen;

	all_bufferlen = 1024;//mydebug
	char* all_buffer = (char*)malloc(all_bufferlen); 
	if (all_buffer == nullptr)
	{
		errmsg = "malloc all_buffer buffer error...";
		return false;
	}
	memset(all_buffer, 0, sizeof(char) * (all_bufferlen));
	size_t all_copy = evbuffer_copyout_from(in_buffer, &ptr_datastart, all_buffer, all_bufferlen);
	if (all_copy == all_bufferlen)
	{
		std::string name_all = "addhuman_all";
		if (!write_file(name_all.c_str(), all_buffer, all_bufferlen))
		{
			errmsg = "write file all_buffer buffer error...";
			free(all_buffer);
			return false;
		}	
	}
	free(all_buffer); all_buffer = nullptr;
#endif

	//parse buffer
	evbuffer_ptr ptr_start, ptr_end;
	ptr_end = evbuffer_search_range(in_buffer, str_boundary_end.c_str(), str_boundary_end.length(), &ptr_datastart, &ptr_dataend);
	std::vector<evbuffer_ptr> vecoffsetptr;
	while (1)
	{
		ptr_start = evbuffer_search_range(in_buffer, str_boundary_start.c_str(), str_boundary_start.length(), &ptr_datastart, &ptr_dataend);
		if (ptr_start.pos < 0 || ptr_end.pos < 0)//错误
			break;

		if (ptr_start.pos < ptr_end.pos)
		{
			vecoffsetptr.push_back(ptr_start);
		}
		if (ptr_start.pos == ptr_end.pos)
		{
			vecoffsetptr.push_back(ptr_start);
			break;
		}

		size_t offset = str_boundary_start.length();
		if (offset <= 0) break;
		evbuffer_ptr_set(in_buffer, &ptr_datastart, ptr_start.pos+offset, EVBUFFER_PTR_SET);
	}

#endif

	//save buffer
	filecount = 0;
	std::string human_sourcefolder = folder_digitalmodel;
	for (size_t i = 0; i < vecoffsetptr.size()-1; ++i)
	{
		evbuffer_ptr ptr_start = vecoffsetptr[i];
		evbuffer_ptr ptr_end = vecoffsetptr[i+1];
		if (ptr_start.pos < ptr_end.pos)//数据段
		{
			size_t item_bufferlen = ptr_end.pos - ptr_start.pos;
			if (item_bufferlen < 256)//字符串
			{
				std::string partName,partValue;
				if (!buffer_to_string(in_buffer, &ptr_start, item_bufferlen, partName, partValue, errmsg))
				{
					_debug_to(1, ("buffer_to_string error, errmsg = %s"), errmsg.c_str());
					continue;
				}
				else
				{
					if (partName == "HumanName")
					{
						humanname = partValue;
						human_sourcefolder += std::string("/");
						human_sourcefolder += md5::getStringMD5(humanname);//名称MD5加密得到文件夹名称
					}
				}
			}
			else
			{
				std::string filepath_originalvdo = "";
				if (!buffer_to_file(in_buffer, &ptr_start, item_bufferlen, human_sourcefolder, filepath_originalvdo, errmsg))
				{
					_debug_to(1, ("buffer_to_file error, errmsg = %s"), errmsg.c_str());
					continue;
				}
				else
				{
					if (aws_enable)
					{
						std::string filepath_http;
						if (!uploadfile_originalvdo(humanname, filepath_originalvdo, filepath_http))
						{
							errmsg = "upload originalvdo file failed...";
							return false;
						}
						else
						{
							remove(filepath_originalvdo.c_str());//删除本地文件
							filepath_originalvdo = filepath_http;
						}
					}
					filecount += 1;
				}
			}
		}
	}
	if (humanname.empty())
	{
		errmsg = "parse <HumanName> failed...";
		return false;
	}

	//update mysql
	if (filecount > 0)
	{
		digitalmysql::humaninfo humanitem_add;
		humanitem_add.humanname = humanname;
		humanitem_add.humanid = md5::getStringMD5(humanname);//名称MD5加密得到humanid
		humanitem_add.contentid = md5::getStringMD5(humanname);//名称MD5加密得到contentid;
		humanitem_add.sourcefolder = "";
		humanitem_add.available = 0;//不可用
		humanitem_add.speakspeed = 1.0;
		humanitem_add.seriousspeed = 0.8;
		humanitem_add.imagematting="";
		humanitem_add.keyframe = "";//default null image
		humanitem_add.foreground = "";//default null image
		humanitem_add.background = "";//default null image
		humanitem_add.speakmodelpath = "";
		humanitem_add.pwgmodelpath = "";
		humanitem_add.mouthmodelfile = "";
		humanitem_add.facemodelfile = "";

		bool updatehuman = false;
		if (digitalmysql::isexisthuman_humanid(humanitem_add.humanid))
			updatehuman = true;
		if (!digitalmysql::addhumaninfo(humanitem_add, updatehuman))
		{
			errmsg = "add humaninfo to mysql failed...";
			return false;
		}
	}
	else
	{
		errmsg = "parse file data error...";
		return false;
	}

	return true;
}

//添加音频接口(用于音频生成视频)
bool ParseAddAudio(evkeyvalq* in_header, evbuffer* in_buffer, std::string& audiopath, std::string& errmsg)
{
#if 1 //解析form-data类型的数据,前几个步骤相同

	//header
	bool bfromdata = false;
	std::string str_boundary_start = "", str_boundary_end = "";
	std::string str_boundary = evhttp_find_header(in_header, "Content-Type");
	std::string::size_type idx = str_boundary.find(std::string("multipart/form-data"));
	if (idx != std::string::npos)
	{
		str_boundary = str_replace(str_boundary, "multipart/form-data; boundary=", "");
		str_boundary_start = std::string("--") + str_boundary;
		str_boundary_end = std::string("--") + str_boundary + std::string("--");
		bfromdata = true;
	}

	//buffer
	size_t bufflen = evbuffer_get_length(in_buffer);
	evbuffer_ptr ptr_datastart, ptr_dataend;
	evbuffer_ptr_set(in_buffer, &ptr_datastart, 0, EVBUFFER_PTR_SET);
	evbuffer_ptr_set(in_buffer, &ptr_dataend, bufflen, EVBUFFER_PTR_SET);

#if DF_FROMDATA_TOFILE	
	size_t all_bufferlen = bufflen;

	all_bufferlen = 1024;//mydebug
	char* all_buffer = (char*)malloc(all_bufferlen);
	if (all_buffer == nullptr)
	{
		errmsg = "malloc all_buffer buffer error...";
		return false;
	}
	memset(all_buffer, 0, sizeof(char) * (all_bufferlen));
	size_t all_copy = evbuffer_copyout_from(in_buffer, &ptr_datastart, all_buffer, all_bufferlen);
	if (all_copy == all_bufferlen)
	{
		std::string name_all = "addaudio_all";
		if (!write_file(name_all.c_str(), all_buffer, all_bufferlen))
		{
			errmsg = "write file all_buffer buffer error...";
			free(all_buffer);
			return false;
		}
	}
	free(all_buffer); all_buffer = nullptr;
#endif

	//parse buffer
	evbuffer_ptr ptr_start, ptr_end;
	ptr_end = evbuffer_search_range(in_buffer, str_boundary_end.c_str(), str_boundary_end.length(), &ptr_datastart, &ptr_dataend);
	std::vector<evbuffer_ptr> vecoffsetptr;
	while (1)
	{
		ptr_start = evbuffer_search_range(in_buffer, str_boundary_start.c_str(), str_boundary_start.length(), &ptr_datastart, &ptr_dataend);
		if (ptr_start.pos < 0 || ptr_end.pos < 0)//错误
			break;

		if (ptr_start.pos < ptr_end.pos)
		{
			vecoffsetptr.push_back(ptr_start);
		}
		if (ptr_start.pos == ptr_end.pos)
		{
			vecoffsetptr.push_back(ptr_start);
			break;
		}

		size_t offset = str_boundary_start.length();
		if (offset <= 0) break;
		evbuffer_ptr_set(in_buffer, &ptr_datastart, ptr_start.pos + offset, EVBUFFER_PTR_SET);
	}

#endif

	//save buffer
	std::string input_audiofolder = folder_htmldigital + std::string("/task");
	for (size_t i = 0; i < vecoffsetptr.size() - 1; ++i)
	{
		evbuffer_ptr ptr_start = vecoffsetptr[i];
		evbuffer_ptr ptr_end = vecoffsetptr[i + 1];
		if (ptr_start.pos < ptr_end.pos)//数据段
		{
			size_t item_bufferlen = ptr_end.pos - ptr_start.pos;
			if (item_bufferlen < 256)//字符串
			{
				std::string partName, partValue;
				if (!buffer_to_string(in_buffer, &ptr_start, item_bufferlen, partName, partValue, errmsg))
				{
					_debug_to(1, ("buffer_to_string error, errmsg = %s"), errmsg.c_str());
					continue;
				}
			}
			else
			{
				std::string filepath_inputaudio = "";
				if (!buffer_to_file(in_buffer, &ptr_start, item_bufferlen, input_audiofolder, filepath_inputaudio, errmsg))
				{
					_debug_to(1, ("buffer_to_file error, errmsg = %s"), errmsg.c_str());
					continue;
				}
				else
				{
					if (is_audiofile(filepath_inputaudio.c_str()))
					{
						if (aws_enable)
						{
							std::string filepath_http;
							if (!uploadfile_public(filepath_inputaudio, filepath_http))
							{
								errmsg = "upload audio file failed...";
								return false;
							}
							else
							{
								remove(filepath_inputaudio.c_str());//删除本地文件
								filepath_inputaudio = filepath_http;
							}
						}
						audiopath = filepath_inputaudio;
						break;//only save one file
					}
				}
			}
		}
	}

	return true;
}

//添加背景资源接口
bool ParseAddBackground(evkeyvalq* in_header, evbuffer* in_buffer, size_t& imagecount, size_t& videocount, size_t& audiocount, std::string& errmsg)
{
#if 1 //解析form-data类型的数据,前几个步骤相同

	//header
	bool bfromdata = false;
	std::string str_boundary_start = "", str_boundary_end = "";
	std::string str_boundary = evhttp_find_header(in_header, "Content-Type");
	std::string::size_type idx = str_boundary.find(std::string("multipart/form-data"));
	if (idx != std::string::npos)
	{
		str_boundary = str_replace(str_boundary, "multipart/form-data; boundary=", "");
		str_boundary_start = std::string("--") + str_boundary;
		str_boundary_end = std::string("--") + str_boundary + std::string("--");
		bfromdata = true;
	}

	//buffer
	size_t bufflen = evbuffer_get_length(in_buffer);
	evbuffer_ptr ptr_datastart, ptr_dataend;
	evbuffer_ptr_set(in_buffer, &ptr_datastart, 0, EVBUFFER_PTR_SET);
	evbuffer_ptr_set(in_buffer, &ptr_dataend, bufflen, EVBUFFER_PTR_SET);

#if DF_FROMDATA_TOFILE	
	size_t all_bufferlen = bufflen;

	all_bufferlen = 1024;//mydebug
	char* all_buffer = (char*)malloc(all_bufferlen);
	if (all_buffer == nullptr)
	{
		errmsg = "malloc all_buffer buffer error...";
		return false;
	}
	memset(all_buffer, 0, sizeof(char) * (all_bufferlen));
	size_t all_copy = evbuffer_copyout_from(in_buffer, &ptr_datastart, all_buffer, all_bufferlen);
	if (all_copy == all_bufferlen)
	{
		std::string name_all = "addbackground_all";
		if (!write_file(name_all.c_str(), all_buffer, all_bufferlen))
		{
			errmsg = "write file all_buffer buffer error...";
			free(all_buffer);
			return false;
		}
	}
	free(all_buffer); all_buffer = nullptr;
#endif

	//parse buffer
	evbuffer_ptr ptr_start, ptr_end;
	ptr_end = evbuffer_search_range(in_buffer, str_boundary_end.c_str(), str_boundary_end.length(), &ptr_datastart, &ptr_dataend);
	std::vector<evbuffer_ptr> vecoffsetptr;
	while (1)
	{
		ptr_start = evbuffer_search_range(in_buffer, str_boundary_start.c_str(), str_boundary_start.length(), &ptr_datastart, &ptr_dataend);
		if (ptr_start.pos < 0 || ptr_end.pos < 0)//错误
			break;

		if (ptr_start.pos < ptr_end.pos)
		{
			vecoffsetptr.push_back(ptr_start);
		}
		if (ptr_start.pos == ptr_end.pos)
		{
			vecoffsetptr.push_back(ptr_start);
			break;
		}

		size_t offset = str_boundary_start.length();
		if (offset <= 0) break;
		evbuffer_ptr_set(in_buffer, &ptr_datastart, ptr_start.pos + offset, EVBUFFER_PTR_SET);
	}

#endif

	//save buffer
	std::vector<std::string> vecImagePath;
	std::vector<std::string> vecVideoPath;
	std::vector<std::string> vecAudioPath;
	std::string background_sourcefolder = folder_htmldigital + std::string("/source");
	for (size_t i = 0; i < vecoffsetptr.size() - 1; ++i)
	{
		evbuffer_ptr ptr_start = vecoffsetptr[i];
		evbuffer_ptr ptr_end = vecoffsetptr[i + 1];
		if (ptr_start.pos < ptr_end.pos)//数据段
		{
			size_t item_bufferlen = ptr_end.pos - ptr_start.pos;
			if (item_bufferlen < 256)//字符串
			{
				std::string partName, partValue;
				if (!buffer_to_string(in_buffer, &ptr_start, item_bufferlen, partName, partValue, errmsg))
				{
					_debug_to(1, ("buffer_to_string error, errmsg = %s"), errmsg.c_str());
					continue;
				}
			}
			else
			{
				std::string filepath = "";
				if (!buffer_to_file(in_buffer, &ptr_start, item_bufferlen, background_sourcefolder, filepath, errmsg))
				{
					_debug_to(1, ("buffer_to_file error, errmsg = %s"), errmsg.c_str());
					continue;
				}
				else
				{
					if (is_imagefile(filepath.c_str()))
						vecImagePath.push_back(filepath);
					else if (is_videofile(filepath.c_str()))
						vecVideoPath.push_back(filepath);
					else if (is_audiofile(filepath.c_str()))
						vecAudioPath.push_back(filepath);
				}
			}
		}
	}

	//update mysql
	imagecount = vecImagePath.size();
	for (size_t i = 0; i < imagecount; i++)
	{
		std::string filepath_imagesource = vecImagePath[i];
		//上传
		if (aws_enable)
		{
			std::string filepath_http;
			if (!uploadfile_backsource(filepath_imagesource, filepath_http))
			{
				errmsg = "upload image tasksource failed...";
				return false;
			}
			else
			{
				remove(filepath_imagesource.c_str());//删除本地文件
				filepath_imagesource = filepath_http;
			}
		}

		//保存数据库
		digitalmysql::tasksourceinfo add_tasksourceitem;
		add_tasksourceitem.sourcetype = digitalmysql::source_image;
		add_tasksourceitem.sourcepath = filepath_imagesource;
		add_tasksourceitem.sourcekeyframe = "";
		add_tasksourceitem.createtime = gettimecode();

		bool update = digitalmysql::isexisttasksource_path(filepath_imagesource);
		if (!digitalmysql::addtasksource(add_tasksourceitem, update))
		{
			errmsg = "add image tasksource to mysql failed...";
			return false;
		}
	}
	videocount = vecVideoPath.size();
	for (size_t j = 0; j < videocount; j++)
	{
		std::string filepath_videosource = vecVideoPath[j];

		//获取关键帧
		std::string filepath_videokeyframe = background_sourcefolder + std::string("/") + md5::getStringMD5(filepath_videosource) + std::string(".png");
		getimage_fromvideo(filepath_videosource, filepath_videokeyframe);
		//上传
		if (aws_enable)
		{
			std::string filepath_http;
			if (!uploadfile_backsource(filepath_videosource, filepath_http))
			{
				errmsg = "upload video tasksource failed...";
				return false;
			}
			else
			{
				remove(filepath_videosource.c_str());//删除本地文件
				filepath_videosource = filepath_http;
			}
			//上传关键帧
			if (!uploadfile_backsource(filepath_videokeyframe, filepath_http))
			{
				errmsg = "upload video tasksource[keyframe] failed...";
				return false;
			}
			else
			{
				remove(filepath_videokeyframe.c_str());//删除本地文件
				filepath_videokeyframe = filepath_http;
			}
		}

		//保存数据库
		digitalmysql::tasksourceinfo add_tasksourceitem;
		add_tasksourceitem.sourcetype = digitalmysql::source_video;
		add_tasksourceitem.sourcepath = filepath_videosource;
		add_tasksourceitem.sourcekeyframe = filepath_videokeyframe;
		add_tasksourceitem.createtime = gettimecode();

		bool update = digitalmysql::isexisttasksource_path(filepath_videosource);
		if (!digitalmysql::addtasksource(add_tasksourceitem, update))
		{
			errmsg = "add video tasksource to mysql failed...";
			return false;
		}
	}
	audiocount = vecAudioPath.size();
	for (size_t k = 0; k < audiocount; k++)
	{
		std::string filepath_audiosource = vecAudioPath[k];

		//上传
		if (aws_enable)
		{
			std::string filepath_http;
			if (!uploadfile_backsource(filepath_audiosource, filepath_http))
			{
				errmsg = "upload audio tasksource failed...";
				return false;
			}
			else
			{
				remove(filepath_audiosource.c_str());//删除本地文件
				filepath_audiosource = filepath_http;
			}
		}

		//保存数据库
		digitalmysql::tasksourceinfo add_tasksourceitem;
		add_tasksourceitem.sourcetype = digitalmysql::source_audio;
		add_tasksourceitem.sourcepath = filepath_audiosource;
		add_tasksourceitem.sourcekeyframe = "";
		add_tasksourceitem.createtime = gettimecode();

		bool update = digitalmysql::isexisttasksource_path(filepath_audiosource);
		if (!digitalmysql::addtasksource(add_tasksourceitem, update))
		{
			errmsg = "add audio tasksource to mysql failed...";
			return false;
		}
	}

	return true;
}

#endif

#if 1//多线程HttpServer

//HTTP命令支持跨域访问
bool checkOptionsRequest(struct evhttp_request* req)
{
	try
	{
		if (req->type == EVHTTP_REQ_OPTIONS) //为支持跨域访问，需要对OPTIONS进行快速处理
		{
			evkeyvalq* header = evhttp_request_get_output_headers(req);
			if (header)
			{
				evhttp_add_header(header, "Access-Control-Allow-Origin", "*");
				evhttp_add_header(header, "Access-Control-Allow-Methods", "GET,PUT,POST,HEAD,OPTIONS,DELETE,PATCH");
				evhttp_add_header(header, "Access-Control-Allow-Headers", "Content-Type,Content-Length,Authorization,Accept,X-Requested-With");
				evhttp_add_header(header, "Access-Control-Max-Age", "3600");
			}
			evhttp_send_reply(req, HTTP_OK, nullptr, nullptr);
			return true;
		}
	}
	catch (...)
	{
		;
	}
	return false;
}

//解析Json参数
void ParseBodyStr(json::Object json_obj, std::map<std::string, std::string>& mapStrParameter, std::map<std::string, int>& mapIntParameter, std::map<std::string, double>& mapDoubleParameter)
{
	json::Object::ValueMap::iterator it = json_obj.begin();
	for (it; it != json_obj.end(); ++it)
	{
		std::string key = it->first;
		if (it->second.GetType() == json::StringVal)
		{
			std::string value = it->second.ToString();
			mapStrParameter[key] = value;
		}
		if (it->second.GetType() == json::IntVal)
		{
			int value = it->second.ToInt();
			mapIntParameter[key] = value;
		}
		if (it->second.GetType() == json::DoubleVal)
		{
			double value = it->second.ToDouble();
			mapDoubleParameter[key] = value;
		}
		if (it->second.GetType() == json::ObjectVal)
		{
			json::Object json_subobj = it->second.ToObject();
			ParseBodyStr(json_subobj, mapStrParameter, mapIntParameter, mapDoubleParameter);
		}
		if (it->second.GetType() == json::ArrayVal)
		{
			json::Array json_subarray = it->second.ToArray();
			for (size_t j = 0; j < json_subarray.size(); ++j)
			{
				if (json_subarray[j].GetType() == json::ObjectVal)
				{
					json::Object json_subarray_obj = json_subarray[j].ToObject();
					ParseBodyStr(json_subarray_obj, mapStrParameter, mapIntParameter, mapDoubleParameter);
				}
			}
		}
	}
}

//HTTP服务回调函数,是在线程里回调的，注意线程安全
void global_http_generic_handler(struct evhttp_request* req, void* arg)
{
	if (checkOptionsRequest(req)) return;
	httpServer::httpThread* pServer = reinterpret_cast<httpServer::httpThread*>(arg);
	if (pServer == nullptr) return;
	int http_code = HTTP_OK;

	//Authorization
	std::string Authorization_info = "";
	

	//input
	std::string httpReqBodyStr_ansi;
	//debug outout
	std::string httpRetStr_debug = "unsupported request"; bool result_debug = true;
	
	bool is_getdata = false;
	//return string
	std::string httpRetStr_ansi = "unsupported request", httpRetStr_utf8 = "unsupported request";
	//return databuff
	std::string content_type = "";
	char* content_databuff = nullptr; long  content_datalen = 0;

	//start
	long long start_time = gettimecount();
	try {
		char pathchar[MAX_PATH];
		char querychar[MAX_PATH];
		char hostchar[MAX_PATH];
		char urichar[MAX_PATH];
		memset(pathchar, 0, sizeof(char) * MAX_PATH);
		memset(querychar, 0, sizeof(char) * MAX_PATH);
		memset(hostchar, 0, sizeof(char) * MAX_PATH);
		memset(urichar, 0, sizeof(char) * MAX_PATH);

		const char* temppathchar = evhttp_uri_get_path(req->uri_elems);
		if (temppathchar)COMMON_STRCPY(pathchar, temppathchar, static_cast<ULONG>(MAX_PATH - 1));
		const char* tempquerychar = evhttp_uri_get_query(req->uri_elems);
		if (tempquerychar) COMMON_STRCPY(querychar, tempquerychar, static_cast<ULONG>(MAX_PATH - 1));
		if (req->remote_host) COMMON_STRCPY(hostchar, req->remote_host, static_cast<ULONG>(MAX_PATH - 1));
		if (req->input_headers) {
			const char* tempurichar = evhttp_find_header(req->input_headers, "Host");
			if (tempurichar) COMMON_STRCPY(urichar, tempurichar, static_cast<ULONG>(MAX_PATH - 1));

			//Authorization
			const char* temp_Authorization = evhttp_find_header(req->input_headers, "Authorization");
			if (temp_Authorization) Authorization_info = temp_Authorization;
		}

		struct evkeyvalq params;
		size_t querylen = 0U;
		char* querydecodechar = evhttp_uridecode(querychar, 0, &querylen);
		if (querydecodechar) {
			memset(querychar, 0, sizeof(char) * MAX_PATH);
			COMMON_STRCPY(querychar, querydecodechar, static_cast<ULONG>(MAX_PATH - 1));
			evhttp_parse_query_str(querydecodechar, &params);
			delete[]querydecodechar;
		}

		std::string pathStr,queryStr, hostStr, uriStr;
		pathStr = pathchar;  //pathStr = UrlDecode(pathStr);  //网络路径字符串解码
		queryStr = querychar;
		hostStr = hostchar;
		uriStr = urichar;

		char* post_data = reinterpret_cast<char*>(EVBUFFER_DATA(req->input_buffer));//(char *)EVBUFFER_DATA(req->input_buffer);//注：我们要求的是UTF-8封装的json格式
		size_t post_len = EVBUFFER_LENGTH(req->input_buffer);
		if (post_data && post_len > 0U)
		{
			std::string bodyStr;// = post_data;
			if (req->body_size > 0U){// && bodyStr.length() >= req->body_size)
				char* tempchar = new char[req->body_size + 1U];
				memset(tempchar, 0, sizeof(char) * (req->body_size + 1U));
				//::CopyMemory(tempchar, post_data, req->body_size);
				memcpy(tempchar, post_data, req->body_size);
				bodyStr = tempchar;
				delete[]tempchar;
			}

#if defined WIN32
			utf8_to_ansi(bodyStr.c_str(), bodyStr.length(), httpReqBodyStr_ansi);
#else
			httpReqBodyStr_ansi = bodyStr;
#endif

			_debug_to(0, ("http server receive message from %s, path is %s, query param is %s, body is %s\n"), hostStr.c_str(), pathStr.c_str(), queryStr.c_str(), bodyStr.c_str());
		}

		//解析路径
		std::vector<std::string> pathVector;
		globalSpliteString(pathStr, pathVector, ("/"));
//		std::vector<std::string>::iterator path_it = pathVector.begin();

		//解析路径中参数
		size_t tempPos;
		int overtime;
		std::vector<std::string> queryVector;
		globalSpliteString(queryStr, queryVector, ("&"));
		std::map<std::string, std::string> queryMap;
		std::string ParamName_utf8, ParamValue_utf8;
		for (std::vector<std::string>::iterator query_it = queryVector.begin(); query_it != queryVector.end(); query_it++)
		{
			tempPos = query_it->find(("="));
			if (tempPos == std::string::npos || tempPos == 0U) continue;
			ParamName_utf8 = query_it->substr(0U, tempPos);
			ParamValue_utf8 = query_it->substr(tempPos + 1U, query_it->length() - tempPos - 1U);

			//
			std::string ParamName_ansi; utf8_to_ansi(ParamName_utf8.c_str(), ParamName_utf8.length(), ParamName_ansi);
			std::string ParamValue_ansi; utf8_to_ansi(ParamValue_utf8.c_str(), ParamValue_utf8.length(), ParamValue_ansi);
			queryMap[ParamName_ansi] = ParamValue_ansi;
		}

		//解析Body参数
		std::map<std::string, std::string> mapBodyStrParameter;
		std::map<std::string, int> mapBodyIntParameter;
		std::map<std::string, double> mapBodyDoubleParameter;
		if (str_existsubstr(pathStr, std::string("Add")))
		{
			//路径中含Add，则是上传文件接口，Body是fromdata类型，不做处理
		}
		else
		{
			if (!httpReqBodyStr_ansi.empty())
			{
				json::Value body_val = json::Deserialize((char*)httpReqBodyStr_ansi.c_str());
				if (body_val.GetType() == json::ObjectVal)
				{
					json::Object body_obj = body_val.ToObject();
					ParseBodyStr(body_obj, mapBodyStrParameter, mapBodyIntParameter, mapBodyDoubleParameter);
				}
				else
				{
					httpRetStr_debug = "parse body data failed...";
					std::string errormsg = "json not formatted successfully...";
					httpRetStr_ansi = getjson_error(1, errormsg);
					goto http_reply;
				}
			}
		}
		
#if 1		
		//凌云部分
		if (req->type == EVHTTP_REQ_POST)
		{
			if (pathStr.compare(("/action")) == 0 && queryStr.compare("action-id=open") == 0)
			{
				httpRetStr_debug = "{POST-/action?action-id=open}";
				//凌云-开通服务接口

				httpRetStr_ansi = "{\"state\":\"success\", \"message\":\"\"}";
			}
		}

#endif


#if 1		
		//数字人服务部分
		if (req->type == EVHTTP_REQ_POST)
		{
			if (pathStr.compare(("/v1/videomaker/playout/HumanList")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/HumanList}";
				//获取数字人列表
				bool checkrequest = true; std::string errmsg = "success"; std::string data = "";

				//1
				std::string HumanID = "";
				if (mapBodyStrParameter.find("HumanID") != mapBodyStrParameter.end())
					HumanID = mapBodyStrParameter["HumanID"];

				//2
				if (checkrequest)
				{
					httpRetStr_ansi = getjson_humanlistinfo(HumanID);
				}
				else
				{
					httpRetStr_ansi = getjson_error(1, errmsg);
					result_debug = false;
				}
			}
			else if (pathStr.compare(("/v1/videomaker/playout/AddHuman")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/AddHuman}";
				//添加数字人接口
				bool checkrequest = true; std::string errmsg = "success"; std::string data = "";

				//
				evkeyvalq* in_header = evhttp_request_get_input_headers(req);
				evbuffer* in_buffer = evhttp_request_get_input_buffer(req); //body
				if (in_header && in_buffer)
				{
					size_t FileCount = 0;
					std::string HumanName = "";
					bool ret = ParseAddHuman(in_header, in_buffer, HumanName, FileCount, errmsg);
					if (ret)
					{
						if (HumanName.empty())
						{
							httpRetStr_ansi = getjson_error(1, errmsg, data);//not too long,can use function
						}
						else
						{
							char temp_buff[256] = { 0 };
							snprintf(temp_buff, 256, "\"HumanName\":\"%s\",\"FileCount\":%d", HumanName.c_str(), FileCount); data = temp_buff;
							httpRetStr_ansi = getjson_error(0, errmsg, data);//not too long,can use function
						}
					}
					else
					{
						httpRetStr_ansi = getjson_error(1, errmsg);
						result_debug = false;
					}
				}
			}
			else if (pathStr.compare(("/v1/videomaker/playout/DeleteHuman")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/DeleteHuman}";
				//删除数字人接口
				bool checkrequest = true; std::string errmsg = "success"; std::string data = "";

				//1
				std::string HumanID = "";
				if (mapBodyStrParameter.find("HumanID") != mapBodyStrParameter.end())
					HumanID = mapBodyStrParameter["HumanID"];
				CHECK_REQUEST_STR("HumanID", HumanID, errmsg, checkrequest);

				int _DeleteFile = 0;
				if (mapBodyIntParameter.find("DeleteFile") != mapBodyIntParameter.end())
					_DeleteFile = mapBodyIntParameter["DeleteFile"];

				//2
				if (checkrequest)
				{
					bool del_ret = digitalmysql::deletehuman_humanid(HumanID, _DeleteFile, errmsg);
					httpRetStr_ansi = getjson_error((int)del_ret, errmsg);
				}
				else
				{
					httpRetStr_ansi = getjson_error(1, errmsg);
					result_debug = false;
				}

			}
			else if (pathStr.compare(("/v1/videomaker/playout/HumanHistory")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/HumanHistory}";
				//获取数字人历史数据
				bool checkrequest = true; std::string errmsg = "success"; std::string data = "";

				//1
				digitalmysql::VEC_FILTERINFO vecfilterinfo;
				std::string Field1 = "", Value1 = "";
				if (mapBodyStrParameter.find("Field1") != mapBodyStrParameter.end())
					Field1 = mapBodyStrParameter["Field1"];
				if (mapBodyStrParameter.find("Value1") != mapBodyStrParameter.end())
					Value1 = mapBodyStrParameter["Value1"];
				digitalmysql::filterinfo filteritem1;
				filteritem1.filterfield = Field1; filteritem1.filtervalue = Value1;
				vecfilterinfo.push_back(filteritem1);

				std::string Field2 = "", Value2 = "";
				if (mapBodyStrParameter.find("Field2") != mapBodyStrParameter.end())
					Field2 = mapBodyStrParameter["Field2"];
				if (mapBodyStrParameter.find("Value2") != mapBodyStrParameter.end())
					Value2 = mapBodyStrParameter["Value2"];
				digitalmysql::filterinfo filteritem2;
				filteritem2.filterfield = Field2; filteritem2.filtervalue = Value2;
				vecfilterinfo.push_back(filteritem2);

				int PageSize = 10, PageNum = 1;
				if (mapBodyIntParameter.find("PageSize") != mapBodyIntParameter.end())
					PageSize = mapBodyIntParameter["PageSize"];
				if (mapBodyIntParameter.find("PageNum") != mapBodyIntParameter.end())
					PageNum = mapBodyIntParameter["PageNum"];

				std::string SortField = "createtime"; int SortValue = 1;
				if (mapBodyStrParameter.find("SortField") != mapBodyStrParameter.end())
					SortField = mapBodyStrParameter["SortField"];
				if (mapBodyIntParameter.find("SortValue") != mapBodyIntParameter.end())
					SortValue = mapBodyIntParameter["SortValue"];

				//2
				if (checkrequest)
				{
					httpRetStr_ansi = getjson_humanhistoryinfo(vecfilterinfo, SortField, SortValue, PageSize, PageNum);
				}
				else
				{
					httpRetStr_ansi = getjson_error(1, errmsg);
					result_debug = false;
				}
			}
			else if (pathStr.compare(("/v1/videomaker/playout/VideoMake")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/VideoMake}";
				//合成数字人视频
				bool checkrequest = true; std::string errmsg = "success"; std::string data = "";

				//1
				std::string TaskName = "";
				if (mapBodyStrParameter.find("TaskName") != mapBodyStrParameter.end())
					TaskName = mapBodyStrParameter["TaskName"];
				CHECK_REQUEST_STR("TaskName", TaskName, errmsg, checkrequest);
				std::string HumanID = "";
				if (mapBodyStrParameter.find("HumanID") != mapBodyStrParameter.end())
					HumanID = mapBodyStrParameter["HumanID"];
				CHECK_REQUEST_STR("HumanID", HumanID, errmsg, checkrequest);
				std::string InputSsml = "";
				if (mapBodyStrParameter.find("InputSsml") != mapBodyStrParameter.end())
					InputSsml = mapBodyStrParameter["InputSsml"];
				std::string InputAudio = "";
				if (mapBodyStrParameter.find("InputAudio") != mapBodyStrParameter.end())
					InputAudio = mapBodyStrParameter["InputAudio"];
				//
				std::string Background = "";
				if (mapBodyStrParameter.find("Background") != mapBodyStrParameter.end())
					Background = mapBodyStrParameter["Background"];
				double Front_left = 0.0, Front_right = 1.0, Front_top = 0.0, Front_bottom = 1.0;
				if (mapBodyDoubleParameter.find("left") != mapBodyDoubleParameter.end())
					Front_left = mapBodyDoubleParameter["left"];
				Front_left = (Front_left < 0.0) ? (0.0) : (Front_left);
				if (mapBodyDoubleParameter.find("right") != mapBodyDoubleParameter.end())
					Front_right = mapBodyDoubleParameter["right"];
				Front_right = (Front_right < 0.0) ? (0.0) : (Front_right);
				if (mapBodyDoubleParameter.find("top") != mapBodyDoubleParameter.end())
					Front_top = mapBodyDoubleParameter["top"];
				Front_top = (Front_top < 0.0) ? (0.0) : (Front_top);
				if (mapBodyDoubleParameter.find("bottom") != mapBodyDoubleParameter.end())
					Front_bottom = mapBodyDoubleParameter["bottom"];
				Front_bottom = (Front_bottom < 0.0) ? (0.0) : (Front_bottom);

				//
				std::string BackMSg = "";
				int TaskID = 0; int TaskType = 1, Makesynch = 0, TaskMoodType = 0;
				double Speed = 1.0;
				if (mapBodyStrParameter.find("BackMSg") != mapBodyStrParameter.end())
					BackMSg = mapBodyStrParameter["BackMSg"];
				if (mapBodyIntParameter.find("TaskID") != mapBodyIntParameter.end())
					TaskID = mapBodyIntParameter["TaskID"];
				if (mapBodyIntParameter.find("TaskType") != mapBodyIntParameter.end())
					TaskType = mapBodyIntParameter["TaskType"];
				if (mapBodyIntParameter.find("Makesynch") != mapBodyIntParameter.end())
					Makesynch = mapBodyIntParameter["Makesynch"];
				if (mapBodyIntParameter.find("TaskMoodType") != mapBodyIntParameter.end())
					TaskMoodType = mapBodyIntParameter["TaskMoodType"];
				if (mapBodyDoubleParameter.find("Speed") != mapBodyDoubleParameter.end())
					Speed = mapBodyDoubleParameter["Speed"];

				//检查任务参数
				if (TaskType == 1 || TaskType == 0)
				{
					CHECK_REQUEST_STR("InputSsml", InputSsml, errmsg, checkrequest);
					InputAudio = "";
				}
				else if (TaskType == 2)
				{
					CHECK_REQUEST_STR("InputAudio", InputAudio, errmsg, checkrequest);
					InputSsml = "";
				}
				//检查对应数字人的available状态，为0表示正在训练中
				else if (!digitalmysql::isavailable_humanid(HumanID))
				{
					if (HumanID.empty())
						errmsg = "the request humanid is empty...";//前端发起请求偶现
					else
						errmsg = "the digital man is in training...";
					checkrequest = false;
				}

				//2
				if (checkrequest)
				{
					//获取数字人信息
					std::string HumanName = "";
					std::string Foreground = "";
					std::string AcousticModelFullPath = "";	//"../ModelFile/test/TTS/speak/snapshot_iter_1668699.pdz";
					std::string VcoderModelFullPath = "";	//"../ModelFile/test/TTS/pwg/snapshot_iter_1000000.pdz";
					std::string PTHModelsPath = "";			// "../ModelFile/test/W2L/file/shenzhen_v3_20230227.pth";
					std::string DFMModelsPath = "";			// "../ModelFile/test/W2L/file/shenzhen_v3_20230227.dfm";
					digitalmysql::humaninfo HumanItem;
					if (digitalmysql::gethumaninfo(HumanID, HumanItem))
					{
						HumanName = HumanItem.humanname;
						Foreground = fixpath_from_osspath(HumanItem.foreground);
						AcousticModelFullPath = HumanItem.speakmodelpath;
						VcoderModelFullPath = HumanItem.pwgmodelpath;
						PTHModelsPath = HumanItem.mouthmodelfile;
						DFMModelsPath = HumanItem.facemodelfile;
					}

					//
					digitalmysql::taskinfo new_taskitem;
					new_taskitem.taskid = TaskID;
					new_taskitem.tasktype = TaskType;
					new_taskitem.moodtype = TaskMoodType;
					new_taskitem.speakspeed = Speed;
					new_taskitem.taskname = TaskName;
					new_taskitem.taskstate = 0;
					new_taskitem.taskprogress = 0;
					new_taskitem.createtime = gettimecode();
					new_taskitem.humanid = HumanID;
					new_taskitem.humanname = HumanName;
					new_taskitem.ssmltext = InputSsml;
					new_taskitem.audio_path = "";
					new_taskitem.audio_format = "";
					new_taskitem.audio_length = 0;
					new_taskitem.video_path = "";
					new_taskitem.video_format = "";
					new_taskitem.video_length = 0;
					new_taskitem.video_width = 0;
					new_taskitem.video_height = 0;
					new_taskitem.video_fps = 0.0;
					new_taskitem.foreground = Foreground;
					new_taskitem.background = Background;
					new_taskitem.front_left = Front_left;
					new_taskitem.front_right = Front_right;
					new_taskitem.front_top = Front_top;
					new_taskitem.front_bottom = Front_bottom;

					//添加新合成任务到数据库
					bool update = (digitalmysql::isexisttask_taskid(TaskID)) ? (true) : (false);
					_debug_to(0, ("BeforeInsert: TaskID=%d, update=%d\n"), TaskID, update);
					digitalmysql::addtaskinfo(TaskID, new_taskitem, update);
					digitalmysql::setmergestate(TaskID, 0);//任务状态为进行中
					digitalmysql::setmergeprogress(TaskID, 0);//合成进度为0
					_debug_to(0, ("AfterInsert: TaskID=%d, update=%d\n"), TaskID, update);

					//添加合成任务到队列
					bool exist = digitalmysql::isexisttask_taskid(TaskID);
					if (exist)
					{
						actortaskinfo new_actortaskitem;
						new_actortaskitem.ActorTaskID = TaskID;
						new_actortaskitem.ActorTaskType = TaskType;
						new_actortaskitem.ActorMoodType = TaskMoodType;
						new_actortaskitem.ActorTaskSpeed = Speed;
						new_actortaskitem.ActorTaskName = TaskName;
						new_actortaskitem.ActorTaskText = InputSsml;
						new_actortaskitem.ActorTaskAudio = InputAudio;
						new_actortaskitem.ActorTaskHumanID = HumanID;
						new_actortaskitem.ActorTaskState = -1;
						new_actortaskitem.AcousticModelFullPath = AcousticModelFullPath;
						new_actortaskitem.VcoderModelFullPath = VcoderModelFullPath;
						new_actortaskitem.PTHModelsPath = PTHModelsPath;
						new_actortaskitem.DFMModelsPath = DFMModelsPath;

						if (!Makesynch)//thread run
						{
							pthread_mutex_lock(&mutex_actortaskinfo);
							Container_actortaskinfo.insert(std::make_pair(TaskID, new_actortaskitem));
							pthread_mutex_unlock(&mutex_actortaskinfo);
							_debug_to(0, ("add actortask %d to queue success.\n"), TaskID);

							char temp_buff[256] = { 0 };
							snprintf(temp_buff, 256, "\"TaskID\":\"%d\"", TaskID); data = temp_buff;
							httpRetStr_ansi = getjson_error(0, errmsg, data);//not too long,can use function
						}
						else   //now run
						{
							long long dwS = gettimecount();
							ACTORINFO_MAP::iterator itFindActor = Container_actorinfo.begin();
							for (itFindActor; itFindActor != Container_actorinfo.end(); ++itFindActor)
							{
								std::string find_actorip = itFindActor->second.ip;
								short find_actorport = itFindActor->second.port;
								std::string ret_json = getjson_runtask_now(find_actorip, find_actorport, new_actortaskitem, 120);//生成音频，阻塞120秒
								httpRetStr_ansi = ret_json;
							}
							long long dwE = gettimecount();
							_debug_to(0, ("++++++++++++++[task_%d]REQ RUN: %d S++++++++++++++\n"), TaskID, dwE - dwS);
						}
					}
					else
					{
						errmsg = "add task to database failed";
						char temp_buff[1024] = { 0 };
						snprintf(temp_buff, 1024, "\"TaskID\":\"%d\"", TaskID); data = temp_buff;
						httpRetStr_ansi = getjson_error(1, errmsg, data);
						result_debug = false;
					}
				}
				else
				{
					httpRetStr_ansi = getjson_error(1, errmsg);
					result_debug = false;
				}
			}
			else if (pathStr.compare(("/v1/videomaker/playout/DeleteTask")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/DeleteTask}";
				//删除数字人任务
				bool checkrequest = true; std::string errmsg = "success"; std::string data = "";

				//1
				int TaskID = 0;
				if (mapBodyIntParameter.find("TaskID") != mapBodyIntParameter.end())
					TaskID = mapBodyIntParameter["TaskID"];
				CHECK_REQUEST_NUM("TaskID", TaskID, errmsg, checkrequest);

				int _DeleteFile = 1;
				//if (mapBodyIntParameter.find("DeleteFile") != mapBodyIntParameter.end())
				//	_DeleteFile = mapBodyIntParameter["DeleteFile"];

				//2
				if (checkrequest)
				{
					bool del_ret = digitalmysql::deletetask_taskid(TaskID, _DeleteFile, errmsg);
					httpRetStr_ansi = getjson_error((int)del_ret, errmsg);
				}
				else
				{
					httpRetStr_ansi = getjson_error(1, errmsg);
					result_debug = false;
				}
			}
			else if (pathStr.compare(("/v1/videomaker/playout/AddAudio")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/AddAudio}";
				//添加音频接口
				bool checkrequest = true; std::string errmsg = "success"; std::string data = "";

				//
				evkeyvalq* in_header = evhttp_request_get_input_headers(req);
				evbuffer* in_buffer = evhttp_request_get_input_buffer(req); //body
				if (in_header && in_buffer)
				{
					std::string AudioPath = "";
					bool ret = ParseAddAudio(in_header, in_buffer, AudioPath, errmsg);
					if (ret)
					{
						char temp_buff[256] = { 0 };
						snprintf(temp_buff, 256, "\"AudioPath\":\"%s\"", AudioPath.c_str()); data = temp_buff;
						httpRetStr_ansi = getjson_error(0, errmsg, data);//not too long,can use function
					}
					else
					{
						httpRetStr_ansi = getjson_error(1, errmsg);
						result_debug = false;
					}
				}
			}
			else if (pathStr.compare(("/v1/videomaker/playout/AddBackground")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/AddBackground}";
				//添加背景接口
				bool checkrequest = true; std::string errmsg = "success"; std::string data = "";

				//
				evkeyvalq* in_header = evhttp_request_get_input_headers(req);
				evbuffer* in_buffer = evhttp_request_get_input_buffer(req); //body
				if (in_header && in_buffer)
				{
					size_t imagecnt = 0, videocnt = 0, audiocnt = 0;
					bool ret = ParseAddBackground(in_header, in_buffer, imagecnt, videocnt, audiocnt, errmsg);
					if (ret)
					{
						char temp_buff[256] = { 0 };
						snprintf(temp_buff, 256, "\"ImageCount\":%d, \"VideoCount\":%d, \"AudioCount\":%d", imagecnt, videocnt, audiocnt); data = temp_buff;
						httpRetStr_ansi = getjson_error(0, errmsg, data);//not too long,can use function
					}
					else
					{
						httpRetStr_ansi = getjson_error(1, errmsg);
						result_debug = false;
					}
				}
			}
			else if (pathStr.compare(("/v1/videomaker/playout/BackgroundList")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/BackgroundList}";
				//添加背景接口
				bool checkrequest = true; std::string errmsg = "success"; std::string data = "";

				//1


				//2
				if (checkrequest)
				{
					httpRetStr_ansi = getjson_tasksourcelistinfo();
				}
				else
				{
					httpRetStr_ansi = getjson_error(1, errmsg);
					result_debug = false;
				}
			}
			else if (pathStr.compare(("/v1/videomaker/playout/SetWindowPos")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/SetWindowPos}";
				//设置窗口位置接口
				bool checkrequest = true; std::string errmsg = "success"; std::string data = "";

				//1

			}
		}

#endif

		
		//请求返回
http_reply:
		if (is_getdata)
		{
			try
			{
				evkeyvalq* out_header = evhttp_request_get_output_headers(req);
				if (out_header) 
				{
					evhttp_add_header(out_header, "Access-Control-Allow-Origin", "*");
					evhttp_add_header(out_header, "Access-Control-Allow-Methods", "GET,PUT,POST,HEAD,OPTIONS,DELETE,PATCH");
					evhttp_add_header(out_header, "Access-Control-Allow-Headers", "Content-Type,Content-Length,Authorization,Accept,X-Requested-With");
					evhttp_add_header(out_header, "Access-Control-Max-Age", "3600");
					evhttp_add_header(out_header, "Content-Type", content_type.c_str());	
				}

				evbuffer* out_buffer = evhttp_request_get_output_buffer(req); //返回的body
				if (content_databuff && content_datalen)
				{
					evbuffer_add(out_buffer, content_databuff, content_datalen);
				}
				evhttp_send_reply(req, http_code, "proxy", out_buffer);
				free(content_databuff);

				//
				long long end_time = gettimecount();
				std::string result = (result_debug) ? ("OK") : ("FAILED");
				_debug_to(1, ("http server: return databuff. debug = %s, result=%s, runtime=[%lld]s\n"), httpRetStr_debug.c_str(), result.c_str(), end_time - start_time);
			}
			catch(...)
			{
				_debug_to(1, ("http server: return databuff throw exception\n"));
			}

		}
		else
		{
#if defined WIN32//这里进行了修改，只有WINDOWS下需要转化，linux下直接认为是UTF8编码
			ansi_to_utf8(httpRetStr_ansi.c_str(), httpRetStr_ansi.length(), httpRetStr_utf8);
#else
			httpRetStr_utf8 = httpRetStr_ansi;
#endif

			//回应
			try 
			{
				evkeyvalq* out_header = evhttp_request_get_output_headers(req);
				if (out_header) {
					evhttp_add_header(out_header, "Access-Control-Allow-Origin", "*");
					evhttp_add_header(out_header, "Access-Control-Allow-Methods", "GET,PUT,POST,HEAD,OPTIONS,DELETE,PATCH");
					evhttp_add_header(out_header, "Access-Control-Allow-Headers", "Content-Type,Content-Length,Authorization,Accept,X-Requested-With");
					evhttp_add_header(out_header, "Access-Control-Max-Age", "3600");
					evhttp_add_header(out_header, "Content-Type", "application/json; charset=UTF-8");
				}

				struct evbuffer* out_buffer = evbuffer_new();
				if (!out_buffer) {
					_debug_to(1, ("http server: return string fail for malloc buffer fail\n"));
					return;
				}
				evbuffer_add_printf(out_buffer, ("%s\n"), httpRetStr_utf8.c_str());//httpRetStr_utf8是返回的内容
				evhttp_send_reply(req, http_code, "proxy", out_buffer);
				evbuffer_free(out_buffer);

				//
				long long end_time = gettimecount();
				_debug_to(0, ("http server: return string. json = %s, runtime=[%lld]s\n"), httpRetStr_utf8.c_str(), end_time - start_time);
				std::string result = (result_debug)? ("OK"):("FAILED");
				_debug_to(1, ("http server: return string. debug = %s, result=%s, runtime=[%lld]s\n"), httpRetStr_debug.c_str(), result.c_str(), end_time-start_time);
			}
			catch (...) 
			{
				_debug_to(1, ("http server: return string throw exception\n"));
			}

		}	
	}
	catch (...) {
		_debug_to(1, ("http server handler throw exception\n"));
	}
}
#endif

void InitCMDWnd()
{
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode;
	GetConsoleMode(hStdin, &mode);
	mode &= ~ENABLE_QUICK_EDIT_MODE; //移除快速编辑模式
	mode &= ~ENABLE_INSERT_MODE; //移除插入模式
	mode &= ~ENABLE_MOUSE_INPUT;
	SetConsoleMode(hStdin, mode);

	char cmd[64] = {0};
	sprintf(cmd, "mode con cols=%d lines=%d", 160, 40);
	system(cmd);
}

int main()
{
	InitCMDWnd();
	std::string apppath = getexepath(); apppath = str_replace(apppath, std::string("\\"), std::string("/"));

	std::string config_error = "";
	std::string configpath = apppath + "/config.txt";
	std::string configpath_utf8; ansi_to_utf8(configpath.c_str(), configpath.length(), configpath_utf8);
	_debug_to(0, ("COFIG PATH=%s\n"), configpath_utf8.c_str());
	if (!getconfig_global(configpath, config_error))
	{
		_debug_to(1, ("GLOBAL config load failed: %s\n", config_error.c_str()));
		getchar();
	}
	if (!getconfig_aws(configpath, config_error))
	{
		_debug_to(1, ("AWS config load failed: %s\n", config_error.c_str()));
		getchar();
	}
	if (!digitalmysql::getconfig_mysql(configpath, config_error))
	{
		_debug_to(1, ("MYSQL config load failed: %s\n", config_error.c_str()));
		getchar();
	}
	if (!getconfig_actornode(configpath, config_error))
	{
		_debug_to(1, ("ACTOR config load failed: %s\n", config_error.c_str()));
		getchar();
	}
	if (!getconfig_rabbitmq(configpath, config_error))
	{
		_debug_to(1, ("RABBITMQ config load failed: %s\n", config_error.c_str()));
		getchar();
	}
	g_RabbitmqSender = new nsRabbitmq::cwRabbitmqPublish(rabbitmq_ip, rabbitmq_port, rabbitmq_user, rabbitmq_passwd, nullptr, nullptr);

	//初始化
	std::string path_certificate = apppath + std::string("/cert/") + key_certificate;
	std::string path_private = apppath + std::string("/cert/") + key_private;
	httpServer::openssl_common_init(path_certificate, path_private);//OpenSSL Need
	httpServer::complex_httpServer server;
	int httpPort = 8081;//监听端口
	int threadCount = 10;//开启线程池个数
	int ret = server.start_http_server(global_http_generic_handler, nullptr, httpPort, threadCount, 10240);//开启监听
	if (ret < 0)
	{
		_debug_to(1, ("start_http_server failed\n"));
	}
	else {
		_debug_to(1, ("start_http_server success\n"));
	}

	//初始化Actor容器
	pthread_mutex_init(&mutex_actorinfo, NULL);
	_debug_to(1, ("Container_actornode size = %d\n"), Container_actornode.size());
	ACTORNODE_MAP::iterator itActorNode = Container_actornode.begin();
	for (itActorNode; itActorNode != Container_actornode.end(); ++itActorNode)
	{
		std::string ip = itActorNode->second.ip;
		short port = itActorNode->second.port;

		if (!ip.empty() && port != 0)
		{
			actorinfo actoritem;
			actoritem.ip = ip;
			actoritem.port = port;
			actoritem.state = 0;
			actoritem.firstworktick = 0;
			pthread_mutex_lock(&mutex_actorinfo);
			Container_actorinfo.insert(std::make_pair(ip, actoritem));
			pthread_mutex_unlock(&mutex_actorinfo);
		}
	}

	//开启合成任务分配线程
	pthread_mutex_init(&mutex_actortaskinfo, NULL);
	ret = pthread_create(&threadid_runtask_thread, nullptr, pthread_runtask_thread, nullptr);
	if (ret != 0)
	{
		_debug_to(1, ("thread_assigntask create error\n"));
	}
	else
	{
		_debug_to(1, ("thread_assigntask is runing\n"));
	}

	sendRabbitmqMsg(std::string("httpserver starting..."));
	//keep runing
	while (1)
	{
		char ch[256] = { 0 };
		printf(("输入'Q'或‘q’退出程序:\n"));
		gets_s(ch, 255);
		std::string str; str = ch;
		if (str.compare("Q") == 0 || str.compare("q") == 0)
		{
			//结束http监听
			server.stop_http_server();
			break;
		}
	}
}






