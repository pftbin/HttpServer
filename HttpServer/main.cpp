// HttpServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "event.h"
#include "evhttp.h"
#include "event2/http.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/bufferevent_compat.h"
#include "event2/http_struct.h"
#include "event2/http_compat.h"
#include "event2/util.h"
#include "event2/listener.h"
#pragma comment(lib,"libevent.lib")
#pragma comment(lib,"libevent_core.lib")
#pragma comment(lib,"libevent_extras.lib")

//
#pragma comment(lib,"ws2_32.lib")

//
#include "public.h"
#include "json.h"
#include "httpkit.h"
#include "mmRabbitmq.h"
#include "digitalmysql.h"
#include "digitalEntityJson.h"
#include "httpConcurrencyServer.h"





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


#define DF_OPEN_PATCH_GET  0
#if DF_OPEN_PATCH_GET //数字人资产

//from http - Interface1
std::string getjson_conditionsearch(std::string contentid = "")
{
	DigitalSearch_Body test;
	test.resourceName = "entity";
	test.vecRestrictionFields.push_back(std::string("keyframe_"));
	test.vecRestrictionFields.push_back(std::string("name_"));
	if (contentid.empty())
	{
		test.conditionField = "type_";
		test.conditionValue = "biz_assets_digital";
	}
	else
	{
		test.conditionField = "contentId_";
		test.conditionValue = contentid;
	}

	std::string sRetJson = test.writeJson();
	return sRetJson;
}
std::string getjson_humanlistinfo(std::string contentid = "", int history = 0)
{
	//
	std::string name = "ys";
	std::string password = "123";
	std::string url_get_token = "http://172.16.152.137:88/sobeyhive-fp/v2/kernel/configs/user/authentication";
	std::string url_search_data = "http://172.16.152.137:88/sobeyhive-bp/v1/search";

	bool result = true;
	std::string errmsg = "";
	std::string result_str = "";

	//1-get token
	std::string userCode = "";
	std::string userToken = "";
	if (result)
	{
		//url
		char sUrlbuff_gettoken[BUFF_SZ] = { 0 }; snprintf(sUrlbuff_gettoken, BUFF_SZ, "%s?loginName=%s&password=%s", url_get_token.c_str(), name.c_str(), password.c_str());
		std::string sUrl_gettoken = sUrlbuff_gettoken;
		std::string inputUrl_gettoken;
		ansi_to_utf8(sUrl_gettoken.c_str(), sUrl_gettoken.length(), inputUrl_gettoken);
		//header
		const int headercount_gettoken = 2;
		char* pInputHeader_gettoken[headercount_gettoken] = { 0 };
		std::string inputHeaader0_gettoken, inputHeaader1_gettoken;
		std::string sHeaader0_gettoken("sobeyhive-http-system:SobeyHive");
		std::string sHeaader1_gettoken("sobeyhive-http-site:S1");
		ansi_to_utf8(sHeaader0_gettoken.c_str(), sHeaader0_gettoken.length(), inputHeaader0_gettoken);
		ansi_to_utf8(sHeaader1_gettoken.c_str(), sHeaader1_gettoken.length(), inputHeaader1_gettoken);
		pInputHeader_gettoken[0] = (char*)inputHeaader0_gettoken.c_str();
		pInputHeader_gettoken[1] = (char*)inputHeaader1_gettoken.c_str();
		//http-get
		httpkit::DataBlock outHeaderData_gettoken;
		httpkit::DataBlock outBodyData_gettoken;
		bool bhttp_gettoken = httprequest_get((char*)inputUrl_gettoken.c_str(), pInputHeader_gettoken, outHeaderData_gettoken, outBodyData_gettoken, headercount_gettoken);
		if (bhttp_gettoken)
		{
			json::Value json_data = json::Deserialize((char*)outBodyData_gettoken.pBuff);
			json::Object json_obj = json_data.ToObject();
			if (json_obj.HasKey("userCode") && json_obj.HasKey("userToken"))
			{
				userCode = json_obj["userCode"].ToString();
				_debug_to( 0, ("userCode: %s\n"), userCode.c_str());

				userToken = json_obj["userToken"].ToString();
				_debug_to( 0, ("userToken: %s\n"), userToken.c_str());
			}
			else
			{
				//error
				std::string input = ((char*)outBodyData_gettoken.pBuff);
				std::string errormsg;
				utf8_to_ansi(input.c_str(), input.length(), errormsg);
				_debug_to( 0, ("errormsg: %s\n"), errormsg.c_str());

				//ret
				result = false;
				errmsg = errormsg;
			}
		}
		else
		{
			//ret
			result = false;
			errmsg = "get userToken infomation error";
		}
	}

	//2-search data
	std::string retbodydata = "";
	if (result)
	{
		//url
		std::string sUrl_searchdata = url_search_data;
		std::string inputUrl_searchdata;
		ansi_to_utf8(sUrl_searchdata.c_str(), sUrl_searchdata.length(), inputUrl_searchdata);
		//body
		std::string body_searchdata = getjson_conditionsearch(contentid);
		char bodybuff_searchdata[BUFF_SZ] = { 0 }; snprintf(bodybuff_searchdata, BUFF_SZ, "{%s}", body_searchdata.c_str());
		std::string sBody_searchdata = bodybuff_searchdata;
		std::string inputBody_searchdata;
		ansi_to_utf8(sBody_searchdata.c_str(), sBody_searchdata.length(), inputBody_searchdata);
		//header
		const int headercount_searchdata = 4;
		char* pInputHeader_searchdata[headercount_searchdata] = { 0 };
		std::string inputHeaader0_searchdata, inputHeaader1_searchdata, inputHeaader2_searchdata, inputHeaader3_searchdata;
		std::string sHeaader0_searchdata("Content-Type:application/json;charset=utf-8");
		std::string sHeaader1_searchdata("sobeyhive-http-system:SobeyHive");
		std::string sHeaader2_searchdata("sobeyhive-http-site:S1");
		char header3buff_searchdata[BUFF_SZ] = { 0 }; snprintf(header3buff_searchdata, BUFF_SZ, "sobeyhive-http-token:%s", userToken.c_str());
		std::string sHeaader3_searchdata = header3buff_searchdata;
		ansi_to_utf8(sHeaader0_searchdata.c_str(), sHeaader0_searchdata.length(), inputHeaader0_searchdata);
		ansi_to_utf8(sHeaader1_searchdata.c_str(), sHeaader1_searchdata.length(), inputHeaader1_searchdata);
		ansi_to_utf8(sHeaader2_searchdata.c_str(), sHeaader2_searchdata.length(), inputHeaader2_searchdata);
		ansi_to_utf8(sHeaader3_searchdata.c_str(), sHeaader3_searchdata.length(), inputHeaader3_searchdata);
		pInputHeader_searchdata[0] = (char*)inputHeaader0_searchdata.c_str();
		pInputHeader_searchdata[1] = (char*)inputHeaader1_searchdata.c_str();
		pInputHeader_searchdata[2] = (char*)inputHeaader2_searchdata.c_str();
		pInputHeader_searchdata[3] = (char*)inputHeaader3_searchdata.c_str();
		//http-post
		httpkit::DataBlock outHeaderData_searchdata;
		httpkit::DataBlock outBodyData_searchdata;
		bool bhttp_searchdata = httprequest_post((char*)inputUrl_searchdata.c_str(), pInputHeader_searchdata, outHeaderData_searchdata, outBodyData_searchdata, headercount_searchdata, (char*)inputBody_searchdata.c_str());
		if (bhttp_searchdata)
		{
			std::string retbodydata_utf8 = ((char*)outBodyData_searchdata.pBuff);
			utf8_to_ansi(retbodydata_utf8.c_str(), retbodydata_utf8.length(), retbodydata);
		}
		else
		{
			//ret
			result = false;
			errmsg = "get userToken infomation error";
		}
	}

	//3-parse search data
	std::string list_info = "";
	if (result)
	{
		result = false;
		errmsg = "parse bodydata to json error";
		DigitalMan_Items result_object;

		json::Value json_value = json::Deserialize((char*)retbodydata.c_str());
		if (json_value.GetType() == json::ObjectVal)
		{
			json::Object json_obj = json_value.ToObject();
			if (json_obj.HasKey("code"))
			{
				int code = json_obj["code"].ToInt();
				if (code == 200)
				{
					if (json_obj.HasKey("queryResult"))
					{
						json::Value queryResult_val = json_obj["queryResult"];
						if (queryResult_val.GetType() == json::ObjectVal)
						{
							json::Object queryResult_obj = queryResult_val.ToObject();
							if (queryResult_obj.HasKey("result"))
							{
								json::Value result_val = queryResult_obj["result"];
								if (result_val.GetType() == json::ArrayVal)
								{
									json::Array result_array = result_val.ToArray();
									for (size_t i = 0; i < result_array.size(); ++i)
									{
										DigitalMan_Item result_item;
										if (result_array[i].GetType() != json::ObjectVal) break;
										json::Object itemobj = result_array[i].ToObject();
										if (itemobj.HasKey("fieldAndValues"))
										{
											std::map<std::string, std::string> mapfieldvalue;
											json::Value fieldAndValues_val = itemobj["fieldAndValues"];
											if (fieldAndValues_val.GetType() == json::ArrayVal)
											{
												json::Array fieldAndValues_array = fieldAndValues_val.ToArray();
												for (size_t j = 0; j < fieldAndValues_array.size(); ++j)
												{
													if (fieldAndValues_array[j].GetType() != json::ObjectVal) break;
													json::Object fieldvalue_obj = fieldAndValues_array[j].ToObject();
													if (fieldvalue_obj.HasKey("field") && fieldvalue_obj.HasKey("value"))
													{
														std::string key = fieldvalue_obj["field"].ToString();
														std::string value = fieldvalue_obj["value"].ToString();
														mapfieldvalue.insert(std::make_pair(key, value));
													}
												}
											}

											//
											std::map<std::string, std::string>::iterator itcontentId_ = mapfieldvalue.find("contentId_");
											if (itcontentId_ != mapfieldvalue.end())
											{
												result_item.VirtualmanKey = itcontentId_->second;
											}
											//
											std::map<std::string, std::string>::iterator itname_ = mapfieldvalue.find("name_");
											if (itname_ != mapfieldvalue.end())
											{
												result_item.HumanName = itname_->second;
											}
											//
											std::map<std::string, std::string>::iterator itkeyframe_ = mapfieldvalue.find("keyframe_");
											if (itkeyframe_ != mapfieldvalue.end())
											{
												std::string downloadurl = itkeyframe_->second;

												std::string format = "";
												std::string base64_encode = "";
												unsigned int width = 0, height = 0, bitcount = 32;
												std::string filepath = "C:\\TEMP"; filepath += format;
												httpkit::httprequest_download((char*)downloadurl.c_str(), (char*)filepath.c_str());
												if (is_existfile(filepath))
												{
													picture::GetPicInfomation(filepath.c_str(), &width, &height, &bitcount, format);
													//base64_encode = base64::base64_encode_file(filepath);
												}

												result_item.KeyFrame_Format = format;
												result_item.KeyFrame_Width = width;
												result_item.KeyFrame_Height = height;
												result_item.KeyFrame_BitCount = bitcount;
												result_item.KeyFrame_FilePath = filepath;
												result_item.KeyFrame_KeyData = base64_encode;
											}
										}
										//add to object
										result = true;
										result_object.vecDigitManItems.push_back(result_item);
									}
								}
							}
						}
					}

					//code==200,end
				}
			}
		}
		if (history)
		{
			//result_item add human data from database
			for (int i = 0; i < (int)result_object.vecDigitManItems.size(); ++i)
			{
				//need add , at first
				std::string humandata = ", \"HumanData\":[ ]";

				//......

				result_object.vecDigitManItems[i].vecDigitManTaskObj.push_back(humandata);
			}
		}

		if (result)
			list_info = result_object.writeJson();
	}

	//4-return
	if (!result)
	{
		result_str = getjson_error(1, errmsg);
	}
	else
	{
		std::string code = "\"code\": 0,";
		std::string msg = "\"msg\": \"success\",";
		std::string temp_listinfo = "\"HumanList\": [ " + list_info + "]";
		list_info = temp_listinfo;

		result_str = "{" + code + msg + "\"data\":{" + list_info + "}" + "}";//too long,must use string append
	}

	return result_str;
}

std::string getjson_patchdatabody(bool brace = false)
{
	DigitalEntity test;
	test.contentId = "00000000000000000000000000000001";
	test.createUser = "9b71eee832bb44ac9c6979b38e64c544";
	test.name = "新入库测试";
	test.privilege = "public_S1";
	test.site_ = "S1";
	test.tree = "global_sobey_defaultclass/digital_model";
	test.type = "biz_assets_digital";

	//TTS目录
	test.entityData.type = "digital";
	test.entityData.physicalfolder = ("\\\\\\\\172.16.152.123\\\\130-mch21sp\\\\137\\\\hivefiles\\\\sobeyhive\\\\bucket-z\\\\u-0nul7l908e60esq6\\\\新入库测试\\\\TTS模型");
	test.entityData.tree_ = ("global_sobey_defaultclass\\\\digital_model");

	//Group1
	FileGroup trainingvideogroup;
	trainingvideogroup.groupName = "trainingvideogroup";
	trainingvideogroup.groupType = "other";
	trainingvideogroup.status = "ready";
	//Group1-1
	FileItem trainingvideo1;
	trainingvideo1.fileGUID = "e2d9892b504c42a5a56373d366a7a5a9";
	trainingvideo1.filePath = ("\\\\\\\\172.16.152.123\\\\130-mch21sp\\\\137\\\\hivefiles\\\\sobeyhive\\\\bucket-z\\\\u-0nul7l908e60esq6\\\\新入库测试\\\\训练生成视频\\\\深圳台主持人训练换脸视频\\\\data_src_old.mp4");
	trainingvideo1.fileState = "ready";
	trainingvideogroup.fileItems.vecFileItems.push_back(trainingvideo1);

	//Group2
	FileGroup modelfilegroup;
	modelfilegroup.groupName = "modelfilegroup";
	modelfilegroup.groupType = "other";
	modelfilegroup.status = "ready";
	//Group2-1
	FileItem modelfile1;
	modelfile1.fileGUID = "05305c62f19b45a3abdd754f1e7dc787";
	modelfile1.filePath = ("\\\\\\\\172.16.152.123\\\\130-mch21sp\\\\137\\\\hivefiles\\\\sobeyhive\\\\bucket-z\\\\u-0nul7l908e60esq6\\\\新入库测试\\\\wav2lip嘴型驱动模型.pth");
	modelfile1.fileState = "ready";
	modelfilegroup.fileItems.vecFileItems.push_back(modelfile1);
	//Group2-2
	FileItem modelfile2;
	modelfile2.fileGUID = "43f50db5301b4b149b6080afc331c192";
	modelfile2.filePath = ("\\\\\\\\172.16.152.123\\\\130-mch21sp\\\\137\\\\hivefiles\\\\sobeyhive\\\\bucket-z\\\\u-0nul7l908e60esq6\\\\新入库测试\\\\深圳台主持人视频换脸模型.dfm");
	modelfile2.fileState = "ready";
	modelfilegroup.fileItems.vecFileItems.push_back(modelfile2);

	//Group3
	FileGroup modelvideogroup;
	modelvideogroup.groupName = "modelvideogroup";
	modelvideogroup.groupType = "other";
	modelvideogroup.status = "ready";
	//Group3-1
	FileItem modelvideo1;
	modelvideo1.fileGUID = "815c62f8d1f9493297d86cd04ef3ffa0";
	modelvideo1.filePath = ("\\\\\\\\172.16.152.123\\\\130-mch21sp\\\\137\\\\hivefiles\\\\sobeyhive\\\\bucket-z\\\\u-0nul7l908e60esq6\\\\新入库测试\\\\模板视频\\\\数字人待驱动1分钟.mp4");
	modelvideo1.fileState = "ready";
	modelvideogroup.fileItems.vecFileItems.push_back(modelvideo1);
	//Group3-2
	FileItem modelvideo2;
	modelvideo2.fileGUID = "8fc845cfca68448dabc2d1497c38ae1a";
	modelvideo2.filePath = ("\\\\\\\\172.16.152.123\\\\130-mch21sp\\\\137\\\\hivefiles\\\\sobeyhive\\\\bucket-z\\\\u-0nul7l908e60esq6\\\\新入库测试\\\\模板视频\\\\数字人待驱动5分钟.mxf");
	modelvideo2.fileState = "ready";
	modelvideogroup.fileItems.vecFileItems.push_back(modelvideo2);
	//Group3-3
	FileItem modelvideo3;
	modelvideo3.fileGUID = "06558628516143c9a1040d48f025e19c";
	modelvideo3.filePath = ("\\\\\\\\172.16.152.123\\\\130-mch21sp\\\\137\\\\hivefiles\\\\sobeyhive\\\\bucket-z\\\\u-0nul7l908e60esq6\\\\新入库测试\\\\模板视频\\\\数字人待驱动10秒.mp4");
	modelvideo3.fileState = "ready";
	modelvideogroup.fileItems.vecFileItems.push_back(modelvideo3);
	//Group3-4
	FileItem modelvideo4;
	modelvideo4.fileGUID = "8b6f44064d2e4a6eb1b10d2688cb7069";
	modelvideo4.filePath = ("\\\\\\\\172.16.152.123\\\\130-mch21sp\\\\137\\\\hivefiles\\\\sobeyhive\\\\bucket-z\\\\u-0nul7l908e60esq6\\\\新入库测试\\\\模板视频\\\\数字人待驱动20秒.mp4");
	modelvideo4.fileState = "ready";
	modelvideogroup.fileItems.vecFileItems.push_back(modelvideo4);
	//Group3-5
	FileItem modelvideo5;
	modelvideo5.fileGUID = "1d4a8ca338d44dcfbcf49da5ba856126";
	modelvideo5.filePath = ("\\\\\\\\172.16.152.123\\\\130-mch21sp\\\\137\\\\hivefiles\\\\sobeyhive\\\\bucket-z\\\\u-0nul7l908e60esq6\\\\新入库测试\\\\模板视频\\\\数字人待驱动30秒.mp4");
	modelvideo5.fileState = "ready";
	modelvideogroup.fileItems.vecFileItems.push_back(modelvideo5);


	//Group4
	FileGroup keyframegroup;
	keyframegroup.groupName = "keyframe_";
	keyframegroup.groupType = "keyframe_";
	keyframegroup.status = "ready";
	//Group4-1
	FileItem keyframe1;
	keyframe1.fileGUID = "eb27e6fb98124a189edbec26d0bbb8dc";
	keyframe1.filePath = ("\\\\\\\\172.16.152.123\\\\130-mch21sp\\\\137\\\\hivefiles\\\\sobeyhive\\\\bucket-z\\\\u-0nul7l908e60esq6\\\\新入库测试\\\\关键帧.png");
	keyframe1.fileState = "ready";
	keyframegroup.fileItems.vecFileItems.push_back(keyframe1);

	test.fileGroups.vecFileGroups.push_back(trainingvideogroup);
	test.fileGroups.vecFileGroups.push_back(modelfilegroup);
	test.fileGroups.vecFileGroups.push_back(modelvideogroup);
	test.fileGroups.vecFileGroups.push_back(keyframegroup);

	std::string sRetJson = test.writeJson();
	if (brace)//add {}
	{
		char buff[BUFF_SZ] = { 0 };
		snprintf(buff, BUFF_SZ, "{%s}", sRetJson.c_str());
		sRetJson = buff;
	}
	return sRetJson;
}

//patchdata线程
typedef struct _taskinfo_patchdata
{
	//state
	bool result;
	std::string message;
	//data
	int  taskid;
	std::string name;
	std::string password;
	std::string url_get_token;
	std::string url_patch_data;
	std::string body_patch_data;

	_taskinfo_patchdata()
	{
		result = false;
		message = "";

		taskid = 0;
		name = "";
		password = "";
		url_get_token = "";
		url_patch_data = "";
		body_patch_data = "";
	}

	void copydata(const _taskinfo_patchdata& item)
	{
		result = item.result;
		message = item.message;

		taskid = item.taskid;
		name = item.name;
		password = item.password;
		url_get_token = item.url_get_token;
		url_patch_data = item.url_patch_data;
		body_patch_data = item.body_patch_data;
	}

}taskinfo_patchdata, * ptaskinfo_patchdata;
typedef std::vector<taskinfo_patchdata> TASKINFO_PATCHDATA_VEC;

TASKINFO_PATCHDATA_VEC Container_taskinfo_patchdata;
pthread_mutex_t mutex_taskinfo_patchdata;// patchdata互斥量
pthread_t threadid_patchdata;
void* pthread_patchdata(void* arg)
{
	bool bInit = true;
	while (bInit)
	{
		bool bRun = false;
		taskinfo_patchdata FindTask;
		TASKINFO_PATCHDATA_VEC::iterator itFind = Container_taskinfo_patchdata.begin();
		if (Container_taskinfo_patchdata.size() > 0)
		{
			bRun = true;
			FindTask.copydata(*itFind);
			pthread_mutex_lock(&mutex_taskinfo_patchdata);
			Container_taskinfo_patchdata.erase(itFind);
			pthread_mutex_unlock(&mutex_taskinfo_patchdata);
		}

		if (bRun)
		{
			bool bContinueRun = true;
			//1-get token
			std::string userCode = "";
			std::string userToken = "";
			if (bContinueRun)
			{
				//url
				char sUrlbuff_gettoken[BUFF_SZ] = { 0 }; snprintf(sUrlbuff_gettoken, BUFF_SZ, "%s?loginName=%s&password=%s", FindTask.url_get_token.c_str(), FindTask.name.c_str(), FindTask.password.c_str());
				std::string sUrl_gettoken = sUrlbuff_gettoken;
				std::string inputUrl_gettoken;
				ansi_to_utf8(sUrl_gettoken.c_str(), sUrl_gettoken.length(), inputUrl_gettoken);
				//header
				const int headercount_get_token = 2;
				char* pInputHeader_gettoken[headercount_get_token] = { 0 };
				std::string inputHeaader0_gettoken, inputHeaader1_gettoken;
				std::string sHeaader0_get_token("sobeyhive-http-system:SobeyHive");
				std::string sHeaader1_get_token("sobeyhive-http-site:S1");
				ansi_to_utf8(sHeaader0_get_token.c_str(), sHeaader0_get_token.length(), inputHeaader0_gettoken);
				ansi_to_utf8(sHeaader1_get_token.c_str(), sHeaader1_get_token.length(), inputHeaader1_gettoken);
				pInputHeader_gettoken[0] = (char*)inputHeaader0_gettoken.c_str();
				pInputHeader_gettoken[1] = (char*)inputHeaader1_gettoken.c_str();
				//http-get
				httpkit::DataBlock outHeaderData_gettoken;
				httpkit::DataBlock outBodyData_gettoken;
				bool bhttp_gettoken = httprequest_get((char*)inputUrl_gettoken.c_str(), pInputHeader_gettoken, outHeaderData_gettoken, outBodyData_gettoken, headercount_get_token);
				if (bhttp_gettoken)
				{
					json::Value json_data = json::Deserialize((char*)outBodyData_gettoken.pBuff);
					json::Object json_obj = json_data.ToObject();
					if (json_obj.HasKey("userCode") && json_obj.HasKey("userToken"))
					{
						userCode = json_obj["userCode"].ToString();
						_debug_to( 0, ("userCode: %s\n"), userCode.c_str());

						userToken = json_obj["userToken"].ToString();
						_debug_to( 0, ("userToken: %s\n"), userToken.c_str());
					}
					else
					{
						//error
						std::string result = ((char*)outBodyData_gettoken.pBuff);
						std::string errormsg;
						utf8_to_ansi(result.c_str(), result.length(), errormsg);
						_debug_to( 0, ("errormsg: %s\n"), errormsg.c_str());

						//ret
						bContinueRun = false;
						FindTask.result = false;
						FindTask.message = errormsg;
					}
				}
			}

			//2-patch data
			int nstatus = 0;
			std::string contentId_patch = "";
			if (bContinueRun)
			{
				//url
				std::string sUrl_patchdata = FindTask.url_patch_data;
				std::string inputUrl_patchdata;
				ansi_to_utf8(sUrl_patchdata.c_str(), sUrl_patchdata.length(), inputUrl_patchdata);
				//body
				char bodybuff_patchdata[BUFF_SZ] = { 0 }; snprintf(bodybuff_patchdata, BUFF_SZ, "{%s}", FindTask.body_patch_data.c_str());
				std::string sBody_patchdata = bodybuff_patchdata;
				std::string inputBody_patchdata;
				ansi_to_utf8(sBody_patchdata.c_str(), sBody_patchdata.length(), inputBody_patchdata);
				//header
				const int headercount_patchdata = 4;
				char* pInputHeader_patchdata[headercount_patchdata] = { 0 };
				std::string inputHeaader0_patchdata, inputHeaader1_patchdata, inputHeaader2_patchdata, inputHeaader3_patchdata;
				std::string sHeaader0_patchdata("Content-Type:application/json;charset=utf-8");
				std::string sHeaader1_patchdata("sobeyhive-http-system:SobeyHive");
				std::string sHeaader2_patchdata("sobeyhive-http-site:S1");
				char header3buff_patchdata[BUFF_SZ] = { 0 }; snprintf(header3buff_patchdata, BUFF_SZ, "sobeyhive-http-token:%s", userToken.c_str());
				std::string sHeaader3_patchdata = header3buff_patchdata;
				ansi_to_utf8(sHeaader0_patchdata.c_str(), sHeaader0_patchdata.length(), inputHeaader0_patchdata);
				ansi_to_utf8(sHeaader1_patchdata.c_str(), sHeaader1_patchdata.length(), inputHeaader1_patchdata);
				ansi_to_utf8(sHeaader2_patchdata.c_str(), sHeaader2_patchdata.length(), inputHeaader2_patchdata);
				ansi_to_utf8(sHeaader3_patchdata.c_str(), sHeaader3_patchdata.length(), inputHeaader3_patchdata);
				pInputHeader_patchdata[0] = (char*)inputHeaader0_patchdata.c_str();
				pInputHeader_patchdata[1] = (char*)inputHeaader1_patchdata.c_str();
				pInputHeader_patchdata[2] = (char*)inputHeaader2_patchdata.c_str();
				pInputHeader_patchdata[3] = (char*)inputHeaader3_patchdata.c_str();
				//http-patch
				httpkit::DataBlock outHeaderData_patchdata;
				httpkit::DataBlock outBodyData_patchdata;
				bool bhttp_patchdata = httprequest_patch((char*)inputUrl_patchdata.c_str(), pInputHeader_patchdata, outHeaderData_patchdata, outBodyData_patchdata, headercount_patchdata, (char*)inputBody_patchdata.c_str());
				if (bhttp_patchdata)
				{
					json::Value json_data = json::Deserialize((char*)outBodyData_patchdata.pBuff);
					json::Object json_obj = json_data.ToObject();
					if (json_obj.HasKey("status") && json_obj.HasKey("message"))
					{
						nstatus = json_obj["status"].ToInt();
						_debug_to( 0, ("status: %d\n"), nstatus);
						if (nstatus == 200)
						{
							contentId_patch = json_obj["contentId"].ToString();
							_debug_to( 0, ("contentId: %s\n"), contentId_patch.c_str());

							_debug_to( 0, ("TaskID: %d, patch data success\n"), FindTask.taskid);
						}
						else
						{
							std::string errmsg = json_obj["message"].ToString();
							std::string ret_message;
							utf8_to_ansi(errmsg.c_str(), errmsg.length(), ret_message);
							_debug_to( 0, ("message: %s\n"), ret_message.c_str());
						}
					}
					else
					{
						//error
						std::string input = ((char*)outBodyData_patchdata.pBuff);
						std::string errormsg;
						utf8_to_ansi(input.c_str(), input.length(), errormsg);
						_debug_to( 0, ("message: %s\n"), errormsg.c_str());

						//ret
						bContinueRun = false;
						FindTask.result = false;
						FindTask.message = errormsg;
					}
				}
			}
		}

		sleep(50);
		//pthread_exit(nullptr);//中途退出当前线程
	}

	_debug_to( 0, ("pthread_patchdata exit...\n"));
	return nullptr;
}

//getdata线程
typedef struct _taskinfo_getdata
{
	//state
	bool result;
	std::string message;
	//data
	int  taskid;
	std::string name;
	std::string password;
	std::string url_get_token;
	std::string url_get_data;
	std::string contentId_get_data;

	_taskinfo_getdata()
	{
		result = false;
		message = "";

		taskid = 0;
		name = "";
		password = "";
		url_get_token = "";
		url_get_data = "";
		contentId_get_data = "";
	}

	void copydata(const _taskinfo_getdata& item)
	{
		result = item.result;
		message = item.message;

		taskid = item.taskid;
		name = item.name;
		password = item.password;
		url_get_token = item.url_get_token;
		url_get_data = item.url_get_data;
		contentId_get_data = item.contentId_get_data;
	}

}taskinfo_getdata, * ptaskinfo_getdata;
typedef std::vector<taskinfo_getdata> TASKINFO_GETDATA_VEC;
TASKINFO_GETDATA_VEC Container_taskinfo_getdata;
pthread_mutex_t mutex_taskinfo_getdata;// getdata互斥量
pthread_t threadid_getdata;
void* pthread_getdata(void* arg)
{
	bool bInit = true;
	while (bInit)
	{
		bool bRun = false;
		taskinfo_getdata FindTask;
		TASKINFO_GETDATA_VEC::iterator itFind = Container_taskinfo_getdata.begin();
		if (Container_taskinfo_getdata.size() > 0)
		{
			bRun = true;
			FindTask.copydata(*itFind);
			pthread_mutex_lock(&mutex_taskinfo_getdata);
			Container_taskinfo_getdata.erase(itFind);
			pthread_mutex_unlock(&mutex_taskinfo_getdata);
		}

		if (bRun)
		{
			bool bContinueRun = true;
			//1-get token
			std::string userCode = "";
			std::string userToken = "";
			if (bContinueRun)
			{
				//url
				char sUrlbuff_gettoken[BUFF_SZ] = { 0 }; snprintf(sUrlbuff_gettoken, BUFF_SZ, "%s?loginName=%s&password=%s", FindTask.url_get_token.c_str(), FindTask.name.c_str(), FindTask.password.c_str());
				std::string sUrl_gettoken = sUrlbuff_gettoken;
				std::string inputUrl_gettoken;
				ansi_to_utf8(sUrl_gettoken.c_str(), sUrl_gettoken.length(), inputUrl_gettoken);
				//header
				const int headercount_gettoken = 2;
				char* pInputHeader_gettoken[headercount_gettoken] = { 0 };
				std::string inputHeaader0_gettoken, inputHeaader1_gettoken;
				std::string sHeaader0_gettoken("sobeyhive-http-system:SobeyHive");
				std::string sHeaader1_gettoken("sobeyhive-http-site:S1");
				ansi_to_utf8(sHeaader0_gettoken.c_str(), sHeaader0_gettoken.length(), inputHeaader0_gettoken);
				ansi_to_utf8(sHeaader1_gettoken.c_str(), sHeaader1_gettoken.length(), inputHeaader1_gettoken);
				pInputHeader_gettoken[0] = (char*)inputHeaader0_gettoken.c_str();
				pInputHeader_gettoken[1] = (char*)inputHeaader1_gettoken.c_str();
				//http-get
				httpkit::DataBlock outHeaderData_gettoken;
				httpkit::DataBlock outBodyData_gettoken;
				bool bhttp_gettoken = httprequest_get((char*)inputUrl_gettoken.c_str(), pInputHeader_gettoken, outHeaderData_gettoken, outBodyData_gettoken, headercount_gettoken);
				if (bhttp_gettoken)
				{
					json::Value json_data = json::Deserialize((char*)outBodyData_gettoken.pBuff);
					json::Object json_obj = json_data.ToObject();
					if (json_obj.HasKey("userCode") && json_obj.HasKey("userToken"))
					{
						userCode = json_obj["userCode"].ToString();
						_debug_to( 0, ("userCode: %s\n"), userCode.c_str());

						userToken = json_obj["userToken"].ToString();
						_debug_to( 0, ("userToken: %s\n"), userToken.c_str());
					}
					else
					{
						//error
						std::string input = ((char*)outBodyData_gettoken.pBuff);
						std::string errormsg;
						utf8_to_ansi(input.c_str(), input.length(), errormsg);
						_debug_to( 0, ("errormsg: %s\n"), errormsg.c_str());

						//ret
						bContinueRun = false;
						FindTask.result = false;
						FindTask.message = errormsg;
					}
				}
			}

			//2-get data
			if (bContinueRun)
			{
				//url
				char sUrlbuff_getdata[BUFF_SZ] = { 0 }; snprintf(sUrlbuff_getdata, BUFF_SZ, "%s?contentid=%s", FindTask.url_get_data.c_str(), FindTask.contentId_get_data.c_str());
				std::string sUrl_getdata = sUrlbuff_getdata;
				std::string inputUrl_getdata;
				ansi_to_utf8(sUrl_getdata.c_str(), sUrl_getdata.length(), inputUrl_getdata);
				//header
				const int headercount_getdata = 4;
				char* pInputHeader_getdata[headercount_getdata] = { 0 };
				std::string inputHeaader0_getdata, inputHeaader1_getdata, inputHeaader2_getdata, inputHeaader3_getdata;
				std::string sHeaader0_getdata("Content-Type:application/json;charset=utf-8");
				std::string sHeaader1_getdata("sobeyhive-http-system:SobeyHive");
				std::string sHeaader2_getdata("sobeyhive-http-site:S1");
				char header3buff_getdata[BUFF_SZ] = { 0 }; snprintf(header3buff_getdata, BUFF_SZ, "sobeyhive-http-token:%s", userToken.c_str());
				std::string sHeaader3_getdata = header3buff_getdata;
				ansi_to_utf8(sHeaader0_getdata.c_str(), sHeaader0_getdata.length(), inputHeaader0_getdata);
				ansi_to_utf8(sHeaader1_getdata.c_str(), sHeaader1_getdata.length(), inputHeaader1_getdata);
				ansi_to_utf8(sHeaader2_getdata.c_str(), sHeaader2_getdata.length(), inputHeaader2_getdata);
				ansi_to_utf8(sHeaader3_getdata.c_str(), sHeaader3_getdata.length(), inputHeaader3_getdata);
				pInputHeader_getdata[0] = (char*)inputHeaader0_getdata.c_str();
				pInputHeader_getdata[1] = (char*)inputHeaader1_getdata.c_str();
				pInputHeader_getdata[2] = (char*)inputHeaader2_getdata.c_str();
				pInputHeader_getdata[3] = (char*)inputHeaader3_getdata.c_str();
				//http-get
				httpkit::DataBlock outHeaderData_getdata;
				httpkit::DataBlock outBodyData_getdata;
				bool bhttp_getdata = httprequest_get((char*)inputUrl_getdata.c_str(), pInputHeader_getdata, outHeaderData_getdata, outBodyData_getdata, headercount_getdata);
				if (bhttp_getdata)
				{
					json::Value json_data = json::Deserialize((char*)outBodyData_getdata.pBuff);
					json::Object json_obj = json_data.ToObject();
					if (json_obj.HasKey("version_"))
					{
						_debug_to( 0, ("TaskID: %d, get data success\n"), FindTask.taskid);

						//mydebug
						std::string input_patch_data_ansi, input_patch_data_utf8;
						input_patch_data_utf8 = (char*)outBodyData_getdata.pBuff;
						utf8_to_ansi(input_patch_data_utf8.c_str(), input_patch_data_utf8.length(), input_patch_data_ansi);
						json2object(input_patch_data_ansi);
					}
					else
					{
						//error
						std::string input = ((char*)outBodyData_getdata.pBuff);
						std::string errormsg;
						utf8_to_ansi(input.c_str(), input.length(), errormsg);
						_debug_to( 0, ("errormsg: %s\n"), errormsg.c_str());

						//ret
						bContinueRun = false;
						FindTask.result = false;
						FindTask.message = errormsg;
					}
				}
			}
		}

		sleep(50);
		//pthread_exit(nullptr);//中途退出当前线程
	}

	_debug_to( 0, ("pthread_getdata exit...\n"));
	return nullptr;
}
#endif

#if 1 //数字人

//环境参数配置
typedef struct _actornode
{
	std::string ip;
	short port;
}actornode, * pactornode;
typedef std::map<std::string, actornode> ACTORNODE_MAP;
ACTORNODE_MAP Container_actornode;
bool getconfig_actornode(std::string configfilepath)
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
		fread(configbuffer, length, 1, fp);
		fclose(fp);
	}

	if (length && configbuffer)
	{
		std::string value = "";
		std::string config = configbuffer;

		value = getnodevalue(config, "actor_count");
		if (value.empty()) return false;
		int count = atoi(value.c_str());
		_debug_to( 0, ("CONFIG_actornode count = %d\n"), count);

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
			_debug_to( 0, ("CONFIG_actornode actor%d_ip = %s\n"), i, ip.c_str());

			std::string actor_port = actor_pro + "port";
			value = getnodevalue(config, actor_port);
			if (value.empty()) continue;
			port = atoi(value.c_str());
			_debug_to( 0, ("CONFIG_actornode actor%d_port = %d\n"), i, port);

			actornode actornodeitem;
			actornodeitem.ip = ip;
			actornodeitem.port = port;
			Container_actornode.insert(std::make_pair(ip, actornodeitem));
		}

		return true;
	}

	return false;
}

std::string playnode_ip = "";
short playnode_port = 0;
bool getconfig_playnode(std::string configfilepath)
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
		fread(configbuffer, length, 1, fp);
		fclose(fp);
	}

	if (length && configbuffer)
	{
		std::string value = "";
		std::string config = configbuffer;

		value = getnodevalue(config, "playnode_ip");
		if (value.empty()) return false;
		playnode_ip = value;
		_debug_to( 0, ("CONFIG_playnode_ip = %s\n"), playnode_ip.c_str());

		value = getnodevalue(config, "playnode_port");
		if (value.empty()) return false;
		playnode_port = atoi(value.c_str());
		_debug_to( 0, ("CONFIG_playnode_port = %d\n"), playnode_port);

		return true;
	}

	return false;
}

std::string digitvideo_path1 = "";
std::string digitvideo_path2 = "";
bool getconfig_digitvideopath(std::string configfilepath)
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
		fread(configbuffer, length, 1, fp);
		fclose(fp);
	}

	if (length && configbuffer)
	{
		std::string value = "";
		std::string config = configbuffer;

		value = getnodevalue(config, "digitvideo_path1");
		if (value.empty()) return false;
		digitvideo_path1 = value;
		_debug_to( 0, ("CONFIG_digitvideo_path1 = %s\n"), digitvideo_path1.c_str());

		value = getnodevalue(config, "digitvideo_path2");
		if (value.empty()) return false;
		digitvideo_path2 = value;
		_debug_to( 0, ("CONFIG_digitvideo_path2 = %s\n"), digitvideo_path2.c_str());

		return true;
	}

	return false;
}

unsigned int delay_beforetext = 500;
unsigned int delay_aftertext = 300;
bool getconfig_global(std::string configfilepath)
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
		fread(configbuffer, length, 1, fp);
		fclose(fp);
	}

	if (length && configbuffer)
	{
		std::string value = "";
		std::string config = configbuffer;

		value = getnodevalue(config, "delay_beforetext");
		delay_beforetext = atoi(value.c_str());
		_debug_to( 0, ("CONFIG_delay_beforetext = %d\n"), delay_beforetext);

		value = getnodevalue(config, "delay_aftertext");
		delay_aftertext = atoi(value.c_str());
		_debug_to( 0, ("CONFIG_delay_aftertext = %d\n"), delay_aftertext);

		return true;
	}

	return false;
}

//合成
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
			_debug_to( 0, ("addr: %s:%u ,connect failed\n"), inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port));
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
			_debug_to( 0, ("addr: %s:%u ,send failed: %s\n"), inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), sendmsg.c_str());
			bRet = false;
		}
		else
		{
			_debug_to( 0, ("addr: %s:%u ,send success: %s\n"), inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), sendmsg.c_str());
		}
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
			_debug_to( 0, ("addr: %s:%u ,connect failed\n"), inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port));
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
			_debug_to( 0, ("addr: %s:%u ,send failed: %s\n"), inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), sendmsg.c_str());
			bRet = false;
		}
		else
		{
			_debug_to( 0, ("addr: %s:%u ,send success: %s\n"), inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), sendmsg.c_str());
		}
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

//
std::string getjson_error(int code,std::string errmsg,std::string data = "")
{
	std::string result = "";

	char buff[BUFF_SZ] = { 0 };
	snprintf(buff, BUFF_SZ, "{ \"code\": %d, \"msg\": \"%s\",\"data\":{%s} }", code, errmsg.c_str(), data.c_str());result = buff;

	return result;
}

//
std::string getjson_humanlistinfo(std::string humanid = "", int history = 0)
{
	bool result = true;
	std::string errmsg = "";
	std::string result_str = "";
	
	//1-gethumanlist
	digitalmysql::VEC_HUMANINFO vechumaninfo;
	int human_cnt = digitalmysql::gethumanlistinfo(humanid, vechumaninfo);
	_debug_to( 0, ("humaninfo size=%d\n"), human_cnt);
	if (human_cnt <= 0)
	{
		result = false;
		errmsg = "gethumanlistinfo from mysql failed";
	}

	//2-parsedata
	std::string list_info = "";
	DigitalMan_Items result_object;
	_debug_to( 0, ("humaninfo size=%d\n"), vechumaninfo.size());
	for(int i = 0; i < vechumaninfo.size(); i++)
	{
		DigitalMan_Item result_item;

		std::string format = "";
		std::string base64_encode = "";
		unsigned int width = 0, height = 0, bitcount = 32;
		std::string filepath = vechumaninfo[i].keyframe;
		if (is_existfile(filepath))
		{
			picture::GetPicInfomation(filepath.c_str(), &width, &height, &bitcount, format);
			//base64_encode = base64::base64_encode_file(filepath);
		}
		else
		{
			result = false;
			errmsg = "not found keyframe, please keep file exist and not readonly";
			_debug_to( 0, ("not found keyframe, please keep file exist and not readonly.\n filepath = [%s]\n"), filepath.c_str());
		}

		result_item.VirtualmanKey = vechumaninfo[i].humanid;
		result_item.HumanName = vechumaninfo[i].humanname;
		result_item.KeyFrame_Format = format;
		result_item.KeyFrame_Width = width;
		result_item.KeyFrame_Height = height;
		result_item.KeyFrame_BitCount = bitcount;
		result_item.KeyFrame_FilePath = filepath;
		result_item.KeyFrame_KeyData = base64_encode;
		result_object.vecDigitManItems.push_back(result_item);
	}

	//3-add HasHistory
	if (history)
	{
		std::string orderkey = "createtime"; int aspect = 1;
		digitalmysql::VEC_TASKINFO vectaskhistory;
		int history_cnt = digitalmysql::gettaskhistoryinfo(humanid, orderkey, aspect, vectaskhistory);
		_debug_to( 0, ("taskhistory size=%d\n"), history_cnt);
		for (int i = 0; i < vectaskhistory.size(); i++)
		{
			std::string final_video_path = vectaskhistory[i].video_path;//if playnode return error,use green background video
			if (!vectaskhistory[i].video_finalpath.empty())
				final_video_path = vectaskhistory[i].video_finalpath;

			std::string json_history = "";
			char json_buff[BUFF_SZ] = { 0 };
			snprintf(json_buff, BUFF_SZ, "{\"TaskID\":%d,\"TaskName\":\"%s\",\"State\":%d,\"Progress\":%d,\"CreateTime\":\"%s\",\"InputSsml\":\"%s\", \"Audio\":{\"AudioFormat\":\"%s\",\"AudioFile\":\"%s\"},\"Vedio\":{\"VideoFormat\":\"%s\",\"Width\":%d,\"Height\":%d,\"Fps\":%.1f,\"VedioFile\":\"%s\"}}",
				vectaskhistory[i].taskid,vectaskhistory[i].taskname.c_str(), vectaskhistory[i].taskstate, vectaskhistory[i].taskprogress,vectaskhistory[i].createtime.c_str(), vectaskhistory[i].ssmltext.c_str(), vectaskhistory[i].audio_format.c_str(), vectaskhistory[i].audio_path.c_str(),
				vectaskhistory[i].video_format.c_str(), vectaskhistory[i].video_width, vectaskhistory[i].video_height, vectaskhistory[i].video_fps, final_video_path.c_str());
			json_history = json_buff;

			_debug_to( 0, ("history[%d], TaskID=%d, humanid=%s\n"), i, vectaskhistory[i].taskid, vectaskhistory[i].humanid.c_str());

			//add to every digitman object
			for (int j = 0; j < result_object.vecDigitManItems.size(); j++)
			{
				if (result_object.vecDigitManItems[j].VirtualmanKey == vectaskhistory[i].humanid)
				{
					result_object.vecDigitManItems[j].vecDigitManTaskObj.push_back(json_history);
				}
			}
		}
	}

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
		std::string temp_listinfo = "\"HumanList\": [ " + list_info + "]";
		list_info = temp_listinfo;

		result_str = "{" + code + msg + "\"data\":{" + list_info + "}" + "}";//too long,must use string append
	}

	return result_str;
}

//
std::string getjson_humanhistoryinfo(std::string humanid = "", std::string orderkey = "createtime", int aspect = 1)
{
	bool result = true;
	std::string errmsg = "";
	std::string result_str = "";

	digitalmysql::VEC_HUMANINFO vechumaninfo;
	int human_cnt = digitalmysql::gethumanlistinfo(humanid, vechumaninfo);
	_debug_to( 0, ("humaninfo size=%d\n"), human_cnt);

	digitalmysql::VEC_TASKINFO vectaskhistory;
	int history_cnt = digitalmysql::gettaskhistoryinfo(humanid, orderkey, aspect, vectaskhistory);
	_debug_to( 0, ("taskhistory size=%d\n"), history_cnt);

	//
	std::string history_info = "";
	DigitalMan_Tasks result_object;
	for (int i = 0; i < vectaskhistory.size(); i++)
	{
		//临时修改
		int current_humanidx = 0;
		for (int j = 0; j < vechumaninfo.size(); j++)
		{
			if (vechumaninfo[j].humanid == vectaskhistory[i].humanid)
			{
				current_humanidx = j;
				break;
			}
		}

		DigitalMan_Task result_item;
		result_item.TaskID = vectaskhistory[i].taskid;
		result_item.TaskName = vectaskhistory[i].taskname;
		result_item.TaskState = vectaskhistory[i].taskstate;
		result_item.TaskProgerss = vectaskhistory[i].taskprogress;
		result_item.TaskInputSsml = vectaskhistory[i].ssmltext;
		result_item.TaskCreateTime = vectaskhistory[i].createtime;
//		result_item.TaskHumanName = vectaskhistory[i].humanname;
		result_item.TaskVirtualmanKey = vectaskhistory[i].humanid;

		//临时修改，从数字人全列表获取数字人名称（后续需调整数据库结构+taskinfo数据结构）
		result_item.TaskHumanName = vechumaninfo[current_humanidx].humanname;

		//临时修改，从视频路径获取关键帧路径（后续需调整数据库结构+taskinfo数据结构）
		std::string format = "";
		std::string base64_encode = "";
		unsigned int width = 0, height = 0, bitcount = 32;
		std::string filepath = vectaskhistory[i].video_finalpath; filepath = str_replace(filepath, ".mp4", ".bmp");
		if (is_existfile(filepath))
		{
			picture::GetPicInfomation(filepath.c_str(), &width, &height, &bitcount, format);
			//base64_encode = base64::base64_encode_file(filepath);

			result_item.KeyFrame_Format = format;
			result_item.KeyFrame_Width = width;
			result_item.KeyFrame_Height = height;
			result_item.KeyFrame_BitCount = bitcount;
			result_item.KeyFrame_FilePath = filepath;
			result_item.KeyFrame_KeyData = base64_encode;
		}
		else
		{
			_debug_to( 0, ("not found keyframe, please keep file exist and not readonly.\n filepath = [%s]\n"), filepath.c_str());

			//无任务关键帧则使用数字人关键帧
			filepath = vechumaninfo[current_humanidx].keyframe;
			if (is_existfile(filepath))
			{
				picture::GetPicInfomation(filepath.c_str(), &width, &height, &bitcount, format);
				//base64_encode = base64::base64_encode_file(filepath);

				result_item.KeyFrame_Format = format;
				result_item.KeyFrame_Width = width;
				result_item.KeyFrame_Height = height;
				result_item.KeyFrame_BitCount = bitcount;
				result_item.KeyFrame_FilePath = filepath;
				result_item.KeyFrame_KeyData = base64_encode;
			}
		}

		result_item.Audio_Format = vectaskhistory[i].audio_format;
		result_item.Audio_File = vectaskhistory[i].audio_path;
		result_item.Video_Format = vectaskhistory[i].video_format;
		result_item.Video_Width = vectaskhistory[i].video_width;
		result_item.Video_Height = vectaskhistory[i].video_height;
		result_item.Video_Fps = vectaskhistory[i].video_fps;
		result_item.Video_File = vectaskhistory[i].video_finalpath;
		result_object.vecDigitManTasks.push_back(result_item);
	}

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
		std::string temp_listinfo = "\"HumanData\": [ " + history_info + "]";
		history_info = temp_listinfo;

		result_str = "{" + code + msg + "\"data\":{" + history_info + "}" + "}";//too long,must use string append
	}

	return result_str;
}

//
bool waiting_videomerge(int taskid,int timeout = 1200)//1200s timeout
{
	bool result = false;
	int state_now = 0, progress = 0; int time = 0;
	while ((progress < 95 && time <= timeout))
	{
		progress = digitalmysql::getmergeprogress(taskid);
		state_now = digitalmysql::getmergestate(taskid);

		sleep(1000);
		++time;
	}

	if (progress >= 95)
		result = true;

	return result;
}
std::string getjson_runtaskresult(int taskid)
{
	bool result = true;
	std::string errmsg = "";
	std::string result_str = "";

	long long dwS = gettimecount();
	bool ret_waiting = waiting_videomerge(taskid);
	long long dwE = gettimecount();
	_debug_to( 0, ("++++++++++++++合成数字人时间: %d秒++++++++++++++\n"), dwE - dwS);

	//continue add background
	if (ret_waiting)
	{
		long long dwS = gettimecount();
		digitalmysql::taskinfo taskitem;
		if (digitalmysql::gettaskinfo(taskid, taskitem))
		{
			if (taskitem.tasktype == 1 && taskitem.video_finalpath.empty())//need send playnode message
			{
				if (!playnode_ip.empty() && playnode_port != 0)
				{
					std::string send_videopath = taskitem.video_path;
					send_videopath = str_replace(send_videopath.c_str(), ".mp4", "");
					send_videopath = str_replace(send_videopath.c_str(), "/", "\\\\");

					std::string sendmsg = "";
					std::string recvmsg = "";
					//make message
					std::string messageid = getmessageid();
					char msg_buff[BUFF_SZ] = { 0 };
					snprintf(msg_buff, BUFF_SZ, "{\"ddrinfo\":[	\
				{\"ddr\":\"%s\",\"devid\":\"\",\"offset\":0,\"length\":%d,\"pos\":{\"x\":0,\"y\":0,\"with\":0,\"height\":0}},	\
				],\"background\":\"\",\"msgid\":\"%s\"}",
						send_videopath.c_str(), taskitem.video_length,
						messageid.c_str());
					sendmsg = msg_buff;

					//send message
					int recv_timeout = taskitem.audio_length/1000 + 60;//音频时长+60秒
					_debug_to( 0, ("==recv_timeout==  %d\n"), recv_timeout);
					bool result = SendTcpMsg_PNPHDR(playnode_ip, playnode_port, sendmsg, true, recvmsg, recv_timeout);
					if (result)
					{
						_debug_to( 0, ("addr: %s:%u, send playnode message success, TaskID=%d, recv message: %s\n"), playnode_ip.c_str(), playnode_port, taskid, recvmsg.c_str());

						bool recv_result = false;
						std::string final_video_path = taskitem.video_path;//if playnode return error,use green background video
						//parse recvmsg update videopath
						json::Value recv_val = json::Deserialize((char*)recvmsg.c_str());
						if (recv_val.GetType() == json::ObjectVal)
						{
							json::Object recv_obj = recv_val.ToObject();
							if (recv_obj.HasKey("code"))
							{
								int code = recv_obj["code"].ToInt();
								if (code != 0)
								{
									if (recv_obj.HasKey("msg"))
									{
										std::string msg_ansi;
										std::string msg_utf8 = recv_obj["msg"].ToString();
										utf8_to_ansi(msg_utf8.c_str(), msg_utf8.length(), msg_ansi);
										_debug_to( 0, ("recv error: %s \n"), msg_ansi.c_str());
									}
								}
								else
								{
									if (recv_obj.HasKey("videopath"))
									{
										taskitem.video_finalpath = recv_obj["videopath"].ToString();
										final_video_path = taskitem.video_finalpath;
										digitalmysql::setfinalvideopath(taskitem.taskid, final_video_path);
										digitalmysql::setmergestate(taskitem.taskid, 1);//任务状态为成功
										digitalmysql::setmergeprogress(taskitem.taskid, 100);//合成进度为100
										recv_result = true;
									}
								}
							}
						}

						//recv is false
						if (!recv_result)
						{
							final_video_path = str_replace(taskitem.audio_path.c_str(), "wav", "mp4");//失败使用本地绿幕的视频
							digitalmysql::setfinalvideopath(taskitem.taskid, final_video_path);
							digitalmysql::setmergestate(taskitem.taskid, 2);//任务状态为失败
							digitalmysql::setmergeprogress(taskitem.taskid, 100);//合成进度为100
						}

						//return json
						char json_buff[BUFF_SZ] = { 0 };
						snprintf(json_buff, BUFF_SZ, "{\"code\":0,\"msg\":\"success\",\"data\":{\"TaskID\": %d,\"TaskName\":\"%s\",\"CreateTime\":\"%s\",	\
						\"Audio\":{\"AudioFormat\":\"%s\",\"AudioFile\":\"%s\"},\"Vedio\":{\"VideoFormat\":\"%s\",\"Width\":%d,\"Height\":%d,\"Fps\":%.1f,\"VedioFile\":\"%s\"}	\
						}}",
							taskitem.taskid, taskitem.taskname.c_str(), taskitem.createtime.c_str(),
							taskitem.audio_format.c_str(), taskitem.audio_path.c_str(), taskitem.video_format.c_str(), taskitem.video_width, taskitem.video_height, taskitem.video_fps, final_video_path.c_str());
						result_str = json_buff;
					}
					else
					{
						errmsg = "send playnode message failed";
						result_str = getjson_error(1, errmsg);
					}
				}
				else
				{
					errmsg = "playnode infomation unused...";
					result_str = getjson_error(1, errmsg);
				}
			}
			else
			{
				
				//return json
				char json_buff[BUFF_SZ] = { 0 };
				snprintf(json_buff, BUFF_SZ, "{\"code\":0,\"msg\":\"success\",\"data\":{\"TaskID\": %d,\"TaskName\":\"%s\",\"CreateTime\":\"%s\",	\
						\"Audio\":{\"AudioFormat\":\"%s\",\"AudioFile\":\"%s\"},\"Vedio\":{\"VideoFormat\":\"%s\",\"Width\":%d,\"Height\":%d,\"Fps\":%.1f,\"VedioFile\":\"%s\"}	\
						}}",
					taskitem.taskid, taskitem.taskname.c_str(), taskitem.createtime.c_str(),
					taskitem.audio_format.c_str(), taskitem.audio_path.c_str(), taskitem.video_format.c_str(), taskitem.video_width, taskitem.video_height, taskitem.video_fps, taskitem.video_path.c_str());
				result_str = json_buff;
			}
		}
		else
		{
			errmsg = "playnode infomation unused...";
			result_str = getjson_error(1, errmsg);
		}
		long long dwE = gettimecount();
		_debug_to( 0, ("++++++++++++++录制数字人时间: %d秒++++++++++++++\n"), dwE - dwS);
	}
	else
	{
		errmsg = "waiting task result error...";
		result_str = getjson_error(1, errmsg);
	}

	return result_str;
}


#endif


#if 1 //rabbitmq 消息

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
bool getconfig_rabbitmq(std::string configfilepath)
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
		fread(configbuffer, length, 1, fp);
		fclose(fp);
	}

	if (length && configbuffer)
	{
		std::string value = "";
		std::string config = configbuffer;

		value = getnodevalue(config, "rabbitmq_ip");
		if (value.empty()) return false;
		rabbitmq_ip = value;
		_debug_to( 0, ("CONFIG_rabbitmq_ip = %s\n"), rabbitmq_ip.c_str());

		value = getnodevalue(config, "rabbitmq_port");
		if (value.empty()) return false;
		rabbitmq_port = atoi(value.c_str());
		_debug_to( 0, ("CONFIG_rabbitmq_port = %d\n"), rabbitmq_port);

		value = getnodevalue(config, "rabbitmq_user");
		if (value.empty()) return false;
		rabbitmq_user = value;
		_debug_to( 0, ("CONFIG_rabbitmq_user = %s\n"), rabbitmq_user.c_str());

		value = getnodevalue(config, "rabbitmq_passwd");
		if (value.empty()) return false;
		rabbitmq_passwd = value;
		_debug_to( 0, ("CONFIG_rabbitmq_passwd = %s\n"), rabbitmq_passwd.c_str());

		//
		value = getnodevalue(config, "rabbitmq_exchange");
		if (value.empty()) return false;
		rabbitmq_exchange = value;
		_debug_to( 0, ("CONFIG_rabbitmq_exchange = %s\n"), rabbitmq_exchange.c_str());

		value = getnodevalue(config, "rabbitmq_routekey");
		if (value.empty()) return false;
		rabbitmq_routekey = value;
		_debug_to( 0, ("CONFIG_rabbitmq_routekey = %s\n"), rabbitmq_routekey.c_str());
		
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
	std::string video_finalpath = newstateitem.video_finalpath;
	std::string video_keyframe = "";
	if (video_finalpath.empty())
	{
		std::string taskhumanid = newstateitem.humanid;
		digitalmysql::VEC_HUMANINFO taskhumaninfo;
		digitalmysql::gethumanlistinfo(taskhumanid, taskhumaninfo);
		if (!taskhumaninfo.empty())
			video_keyframe = taskhumaninfo[0].keyframe;
	}
	else
	{
		video_keyframe = str_replace(video_finalpath, ".mp4", ".bmp");
	}

	//message json
	char tempbuff[1024] = { 0 };
	snprintf(tempbuff, 1024, "{\"TaskID\":%d, \"State\":%d, \"Progerss\":%d, \"VedioFile\":\"%s\", \"FilePath\":\"%s\" }", task_id, task_state, task_progress, video_finalpath.c_str(), video_keyframe.c_str());
	result_str = tempbuff;

	return result_str;
}

//全局对象
nsRabbitmq::cwRabbitmqPublish* g_RabbitmqSender = nullptr;
bool sendRabbitmqMsg(std::string mqmessage)
{
	if (g_RabbitmqSender == nullptr)
	{
		_debug_to( 0, ("Rabbitmq object is null,please restart...\n"));
		return false;
	}

	std::vector<std::string>   vecMessage;
	vecMessage.push_back(mqmessage);

	nsRabbitmq::mmRabbitmqData Rabbitmq_testdata;
	Rabbitmq_testdata.index = 0;
	Rabbitmq_testdata.moreStr = "";
	Rabbitmq_testdata.moreInt = 0;
	Rabbitmq_testdata.exchange = rabbitmq_exchange;
	Rabbitmq_testdata.routekey = rabbitmq_routekey;
	Rabbitmq_testdata.commandVector.assign(vecMessage.begin(), vecMessage.end());
	g_RabbitmqSender->send(Rabbitmq_testdata);

	return true;
}

#endif



#if 1 //合成任务分配

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

//MergeTaskinfo 结构体
typedef struct _mergetaskinfo
{
	int taskid;
	int tasktype;
	double speakspeed;
	std::string taskname;
	std::string tasktext;
	std::string path1;
	std::string path2;
	std::string filename;

	std::string VirtualmanKey;
	std::string AcousticModelFullPath;		//../0ModelFile/test/TTS/speak/snapshot_iter_1668699.pdz
	std::string VcoderModelFullPath;		//../0ModelFile/test/TTS/pwg/snapshot_iter_1000000.pdz
	std::string DFMModelsPath;				//../0ModelFile/test/W2L/file/shenzhen_v3_20230227.dfm
	std::string OriVideoPath;				//../0ModelFile/test/W2L/video/bule22.mp4

	int state;//-1=waitmerge,0=merging,1=mergesuccess,2=mergefailed

	_mergetaskinfo()
	{
		taskid = 0;
		tasktype = 1;
		tasktext = "";
		speakspeed = 1.0;
		path1 = "";
		path2 = "";
		filename = "";

		VirtualmanKey = "";
		AcousticModelFullPath = "";
		VcoderModelFullPath = "";
		DFMModelsPath = "";
		OriVideoPath = "";
		state = -1;
	}

	void copydata(const _mergetaskinfo& item)
	{
		taskid = item.taskid;
		tasktype = item.tasktype;
		tasktext = item.tasktext;
		speakspeed = item.speakspeed;
		path1 = item.path1;
		path2 = item.path2;
		filename = item.filename;

		VirtualmanKey = item.VirtualmanKey;
		AcousticModelFullPath = item.AcousticModelFullPath;
		VcoderModelFullPath = item.VcoderModelFullPath;
		DFMModelsPath = item.DFMModelsPath;
		OriVideoPath = item.OriVideoPath;
		state = item.state;
	}

}mergetaskinfo, * pmergetaskinfo;
typedef std::map<int, mergetaskinfo> MERGETASKINFO_MAP;
MERGETASKINFO_MAP Container_mergetaskinfo;

//Actror 状态更新
pthread_mutex_t mutex_actorinfo;// actorinfo互斥量
pthread_mutex_t mutex_mergetaskinfo;// mergetaskinfo互斥量
pthread_t threadid_updateactorinfo;
void* pthread_updateactorinfo(void* arg)
{
    bool bInit = true;
    while(bInit)
    {
        actorinfo FindActor;
        ACTORINFO_MAP::iterator itFind = Container_actorinfo.begin();
        for(itFind; itFind != Container_actorinfo.end(); ++itFind)
        {
            pthread_mutex_lock(&mutex_actorinfo);
            FindActor.copydata(itFind->second);
            pthread_mutex_unlock(&mutex_actorinfo);

            std::string ip = FindActor.ip;
            short port = FindActor.port;
            std::string msg = "<actorstate></actorstate>";
            std::string recvmsg = "";
            if(SendTcpMsg_DGHDR(ip,port,msg,true,recvmsg))//查询Actor状态
            {
				_debug_to( 0, ("addr: %s:%u ,send actor check message success,recv message: %s\n"), ip.c_str(), port, recvmsg.c_str());
				//
				int nNewState = FindActor.state;
				json::Value json_val = json::Deserialize((char*)recvmsg.c_str());
				if (json_val.GetType() == json::ObjectVal)
				{
					json::Object json_obj = json_val.ToObject();
					if (json_obj.HasKey("state"))
						nNewState = json_obj["state"].ToInt();
				}

				pthread_mutex_lock(&mutex_actorinfo);
				if (nNewState != FindActor.state)
					itFind->second.state = nNewState;				//update state
				if (FindActor.firstworktick == 0 && nNewState == 1)
					itFind->second.firstworktick = (long)clock();	//start working
				if (FindActor.firstworktick != 0 && nNewState == 0)
					itFind->second.firstworktick = 0;				//stop working
				if (FindActor.state == 1 && FindActor.firstworktick != 0 && (FindActor.firstworktick-clock()) > 6000000)
					itFind->second.state = -1;						//working too long
				pthread_mutex_unlock(&mutex_actorinfo);
            }
            else
            {
				_debug_to( 0, ("addr: %s:%u ,send actor check message failed: %s\n"), ip.c_str(), port, msg.c_str());
            }
        }

        sleep(5000);
		//pthread_exit(nullptr);//中途退出当前线程
    }

	_debug_to( 0, ("pthread_updateactorinfo exit...\n"));
    return nullptr;
}

//MergeTask 通知消息
std::string getNotifyMsg_ToActor(mergetaskinfo taskitem)
{
	std::string result_msg = "";
	char buff[BUFF_SZ] = { 0 };
	snprintf(buff, BUFF_SZ, "<MPC><RequestID>%d</RequestID><ColumnName>测试栏目</ColumnName><MPCType>%d</MPCType><SpeakSpeed>%.1f</SpeakSpeed><TaskText>[p%d]%s[p%d]</TaskText><Path1>%s</Path1><Path2>%s</Path2><DestFileName>%s</DestFileName>	\
							<Humanid>%s</Humanid><AcousticModelFullPath>%s</AcousticModelFullPath><VcoderModelFullPath>%s</VcoderModelFullPath><DFMModelsPath>%s</DFMModelsPath><OriVideoPath>%s</OriVideoPath></MPC>",
		taskitem.taskid, taskitem.tasktype, taskitem.speakspeed, delay_beforetext, taskitem.tasktext.c_str(), delay_aftertext,taskitem.path1.c_str(), taskitem.path2.c_str(), taskitem.filename.c_str(),
		taskitem.VirtualmanKey.c_str(), taskitem.AcousticModelFullPath.c_str(), taskitem.VcoderModelFullPath.c_str(), taskitem.DFMModelsPath.c_str(), taskitem.OriVideoPath.c_str());
	result_msg = buff;

	return result_msg;
}

//MergeTask 分配执行
pthread_t threadid_runtask_thread;
void* pthread_runtask_thread(void* arg)
{
	bool bInit = true;
	while (bInit)
	{
		std::string ip = "";
		short port = 0;
		//找到空闲Actor
		bool bActor = false;
		ACTORINFO_MAP::iterator itFindActor = Container_actorinfo.begin();
		for (itFindActor; itFindActor != Container_actorinfo.end(); ++itFindActor)
		{
			if (itFindActor->second.state == 0)//actor free
			{
				ip = itFindActor->second.ip;
				port = itFindActor->second.port;
				bActor = true;
				break;
			}
		}

		int taskid = 0;
		std::string tasktext = "";
		//找到需合成的任务
		bool bTask = false;
		mergetaskinfo findtaskitem;
		MERGETASKINFO_MAP::iterator itFindTask = Container_mergetaskinfo.begin();
		for (itFindTask; itFindTask != Container_mergetaskinfo.end(); ++itFindTask)
		{
			if (itFindTask->second.state == -1)//need merge
			{
				taskid = itFindTask->second.taskid;
				tasktext = itFindTask->second.tasktext;
				findtaskitem.copydata(itFindTask->second);
				bTask = true;

				pthread_mutex_lock(&mutex_mergetaskinfo);
				Container_mergetaskinfo.erase(itFindTask);
				pthread_mutex_unlock(&mutex_mergetaskinfo);
				break;
			}
		}

		//运行任务
		bool bContinueRun = (bActor&&bTask);
		if (bContinueRun)
		{
			_debug_to( 0, ("ASSIGN task run===================================================\n"));
			std::string recvmsg;
			std::string sendmsg = getNotifyMsg_ToActor(findtaskitem);
			if (SendTcpMsg_DGHDR(ip, port, sendmsg, false, recvmsg))//向Actor发送任务
			{
				_debug_to( 0, ("addr: %s:%u ,send task success, TaskID=%d, text: %s\n"), ip.c_str(), port, taskid, tasktext.c_str());
				std::string result_str = getjson_runtaskresult(taskid);
			}
			else
			{
				_debug_to( 0, ("addr: %s:%u ,send task failed, TaskID=%d, text: %s\n"), ip.c_str(), port, taskid, tasktext.c_str());
				digitalmysql::setmergestate(findtaskitem.taskid, 2);//任务状态为失败
				digitalmysql::setmergeprogress(findtaskitem.taskid, 100);//合成进度为100
			}

			//notify html
			int times = 0;
			std::string htmlnotifymsg = getNotifyMsg_ToHtml(findtaskitem.taskid);
			bool notifyresult = sendRabbitmqMsg(htmlnotifymsg);
			while (!notifyresult)//retry
			{
				++times;
				notifyresult = sendRabbitmqMsg(htmlnotifymsg);
				if (times > 3) break;
				sleep(1000);
			}
			std::string msgresult = (notifyresult) ? ("success") : ("failed");
			_debug_to( 0, ("Send HTML Notify[%s]: %s\n"), msgresult.c_str(), htmlnotifymsg.c_str());
		}

		sleep(1000);
		//pthread_exit(nullptr);//中途退出当前线程
	}

	_debug_to( 0, ("pthread_checkactor exit...\n"));
	return nullptr;
}

//MergeTask 立即执行
std::string getjson_runtask_now(mergetaskinfo taskitem)
{
	std::string errmsg = "";
	std::string result_str = "";

	std::string ip = "";short port = 0;
	//找到空闲Actor
	bool bActor = false;
	ACTORINFO_MAP::iterator itFindActor = Container_actorinfo.begin();
	for (itFindActor; itFindActor != Container_actorinfo.end(); ++itFindActor)
	{
		if (itFindActor->second.state == 0)//actor free
		{
			ip = itFindActor->second.ip;
			port = itFindActor->second.port;
			bActor = true;
			break;
		}
	}


	int taskid = taskitem.taskid;
	std::string tasktext = taskitem.tasktext;
	if (bActor)
	{
		_debug_to( 0, ("SYNC task run===================================================\n"));
		std::string recvmsg;
		std::string sendmsg = getNotifyMsg_ToActor(taskitem);
		if (SendTcpMsg_DGHDR(ip, port, sendmsg, false, recvmsg))//向Actor发送任务
		{
			_debug_to( 0, ("addr: %s:%u ,send task success, TaskID=%d, text: %s\n"), ip.c_str(), port, taskid, tasktext.c_str());
			result_str = getjson_runtaskresult(taskid);
		}
		else
		{
			errmsg = "can not send tcp message...";
			result_str = getjson_error(1, errmsg);
		}

		//notify html
		int times = 0;
		std::string htmlnotifymsg = getNotifyMsg_ToHtml(taskitem.taskid);
		bool notifyresult = sendRabbitmqMsg(htmlnotifymsg);
		while (!notifyresult)//retry
		{
			++times;
			notifyresult = sendRabbitmqMsg(htmlnotifymsg);
			if (times > 3) break;
			sleep(1000);
		}
		std::string msgresult = (notifyresult) ? ("success") : ("failed");
		_debug_to( 0, ("Send HTML Notify[%s]: %s\n"), msgresult.c_str(), htmlnotifymsg.c_str());
	}
	else
	{
		errmsg = "can not fount free acter";
		result_str = getjson_error(1, errmsg);
	}

	return result_str;
}

#endif

#if  1 //多线程HttpServer

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
	}
}

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

//HTTP服务回调函数,是在线程里回调的，注意线程安全
void global_http_generic_handler(struct evhttp_request* req, void* arg)
{
	if (checkOptionsRequest(req)) return;
	httpServer::httpThread* pServer = reinterpret_cast<httpServer::httpThread*>(arg);
	if (pServer == nullptr) return;
	int http_code = HTTP_OK;

	//input
	std::string httpReqBodyStr_ansi;
	//debug outout
	std::string httpRetStr_debug; bool result_debug = true;

	bool return_databuff = false;
	//return string
	std::string httpRetStr_ansi, httpRetStr_utf8;
	//return databuff
	std::string content_type = "";
	char* content_databuff = nullptr; 
	long  content_datalen = 0;

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
		if (post_len > 0U) 
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
			_debug_to( 0, ("\nhttp server receive message from %s, path is %s, query param is %s, body is %s\n"), hostStr.c_str(), pathStr.c_str(), queryStr.c_str(), httpReqBodyStr_ansi.c_str());
		}

		//解析路径
		std::vector<std::string> pathVector;
		globalSpliteString(pathStr, pathVector, ("/"));
		std::vector<std::string>::iterator path_it = pathVector.begin();

		//解析路径中参数
		size_t tempPos;
		int overtime;
		std::vector<std::string> queryVector;
		globalSpliteString(queryStr, queryVector, ("&"));
		std::map<std::string, std::string> queryMap;
		std::string tempParamName, tempParamValue, messageId, paramIdStr;
		for (std::vector<std::string>::iterator query_it = queryVector.begin(); query_it != queryVector.end(); query_it++)
		{
			tempPos = query_it->find(("="));
			if (tempPos == std::string::npos || tempPos == 0U) continue;
			tempParamName = query_it->substr(0U, tempPos);
			tempParamValue = query_it->substr(tempPos + 1U, query_it->length() - tempPos - 1U);


			if (tempParamName == ("message")) messageId = tempParamValue;
			else if (tempParamName == ("overtime")) globalStrToIntDef(const_cast<LPTSTR>(tempParamValue.c_str()), overtime,3,10);
			queryMap[tempParamName] = tempParamValue;
		}

		//解析Body参数
		std::map<std::string, std::string> mapBodyStrParameter;
		std::map<std::string, int> mapBodyIntParameter;
		std::map<std::string, double> mapBodyDoubleParameter;
		json::Value body_val = json::Deserialize((char*)httpReqBodyStr_ansi.c_str());
		if (body_val.GetType() == json::ObjectVal)
		{
			json::Object body_obj = body_val.ToObject();
			ParseBodyStr(body_obj, mapBodyStrParameter, mapBodyIntParameter, mapBodyDoubleParameter);
		}

		
		//自定义处理
		if (req->type == EVHTTP_REQ_POST)
		{

#if DF_OPEN_PATCH_GET
			if (pathStr.compare(("/v1/videomaker/playout/importmodel")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/importmodel}";
				//模型文件入库
				taskinfo_patchdata NewTask;
				NewTask.taskid = 1111;
				NewTask.name = "ys";
				NewTask.password = "123";
				NewTask.url_get_token = "http://172.16.152.137:88/sobeyhive-fp/v2/kernel/configs/user/authentication";
				NewTask.url_patch_data = "http://172.16.152.137:88/sobeyhive-bp/v1/entity";
				NewTask.body_patch_data = getjson_patchdatabody();
				pthread_mutex_lock(&mutex_taskinfo_patchdata);
				Container_taskinfo_patchdata.push_back(NewTask);
				pthread_mutex_unlock(&mutex_taskinfo_patchdata);

				httpRetStr_ansi = "{\"message\":\"task_patchdata 1111 push_back to container\"}";
			}
			else if (pathStr.compare(("/v1/videomaker/playout/getmodel")) == 0)
			{
				httpRetStr_debug = "{GET-/v1/videomaker/playout/getmodel}";
				//获取模型文件
				taskinfo_getdata NewTask;
				NewTask.taskid = 2222;
				NewTask.name = "ys";
				NewTask.password = "123";
				NewTask.url_get_token = "http://172.16.152.137:88/sobeyhive-fp/v2/kernel/configs/user/authentication";
				NewTask.url_get_data = "http://172.16.152.137:88/sobeyhive-bp/v1/entity";
				NewTask.contentId_get_data = "00000000000000000000000000000001";
				pthread_mutex_lock(&mutex_taskinfo_getdata);
				Container_taskinfo_getdata.push_back(NewTask);
				pthread_mutex_unlock(&mutex_taskinfo_getdata);

				httpRetStr_ansi = "{\"message\":\"task_getdata 2222 push_back to container\"}";
			}
#endif
			//
			if (pathStr.compare(("/v1/videomaker/playout/HumanList")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/HumanList}";
				//获取数字人列表
				bool checkrequest = true;std::string errmsg = "success"; std::string data = "";
			
				//1
				std::string VirtualmanKey = "";
				if (mapBodyStrParameter.find("VirtualmanKey") != mapBodyStrParameter.end())
					VirtualmanKey = mapBodyStrParameter["VirtualmanKey"];

				int HasHistory = 0;
				if (mapBodyIntParameter.find("HasHistory") != mapBodyIntParameter.end())
				{
					HasHistory = mapBodyIntParameter["HasHistory"];
				}
				else
				{
					errmsg = "not find <HasHistory> in body";
					checkrequest = false;
				}

				//2
				if (checkrequest)
				{
					httpRetStr_ansi = getjson_humanlistinfo(VirtualmanKey, HasHistory);
				}
				else
				{
					httpRetStr_ansi = getjson_error(1,errmsg);
					result_debug = false;
				}	
			}
			if (pathStr.compare(("/v1/videomaker/playout/HumanHistory")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/HumanHistory}";
				//获取数字人历史数据
				bool checkrequest = true;std::string errmsg = "success"; std::string data = "";

				//1
				std::string VirtualmanKey = "";
				if (mapBodyStrParameter.find("VirtualmanKey") != mapBodyStrParameter.end())
					VirtualmanKey = mapBodyStrParameter["VirtualmanKey"];
				std::string OrderKey = "createtime";
				if (mapBodyStrParameter.find("Key") != mapBodyStrParameter.end())
					OrderKey = mapBodyStrParameter["Key"];
				int Aspect = 1;
				if (mapBodyIntParameter.find("Aspect") != mapBodyIntParameter.end())
					Aspect = mapBodyIntParameter["Aspect"];

				//2
				if (checkrequest)
				{
					httpRetStr_ansi = getjson_humanhistoryinfo(VirtualmanKey,OrderKey,Aspect);
				}
				else
				{
					httpRetStr_ansi = getjson_error(1, errmsg);
					result_debug = false;
				}
			}
			else if (pathStr.compare(("/v1/videomaker/playout/videomake")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/videomake}";
				//合成数字人视频
				bool checkrequest = true;std::string errmsg = "success"; std::string data = "";
				
				//1
				std::string TaskName = "", BackMSg = "";
				int TaskID = 0; int MakeVedio = 1;
				double Speed = 1.0;
				if (mapBodyIntParameter.find("TaskID") != mapBodyIntParameter.end())
					TaskID = mapBodyIntParameter["TaskID"];
				if (mapBodyStrParameter.find("TaskName") != mapBodyStrParameter.end())
					TaskName = mapBodyStrParameter["TaskName"];
				if (mapBodyStrParameter.find("BackMSg") != mapBodyStrParameter.end())
					BackMSg = mapBodyStrParameter["BackMSg"];
				if (mapBodyIntParameter.find("MakeVedio") != mapBodyIntParameter.end())
					MakeVedio = mapBodyIntParameter["MakeVedio"];
				if (mapBodyDoubleParameter.find("Speed") != mapBodyDoubleParameter.end())
					Speed = mapBodyDoubleParameter["Speed"];
				_debug_to( 0, ("TaskID=%d,TaskName=%s, BackMSg=%s, MakeVedio=%d, Speed=%.1f\n"), TaskID, TaskName.c_str(), BackMSg.c_str(), MakeVedio, Speed);

				std::string VirtualmanKey = "";
				if (mapBodyStrParameter.find("VirtualmanKey") != mapBodyStrParameter.end())
				{
					VirtualmanKey = mapBodyStrParameter["VirtualmanKey"];
				}
				else
				{
					errmsg = "not find <VirtualmanKey> in body";
					checkrequest = false;
				}
				std::string InputSsml = "";
				if (mapBodyStrParameter.find("InputSsml") != mapBodyStrParameter.end())
				{
					InputSsml = mapBodyStrParameter["InputSsml"];
				}
				else
				{
					errmsg = "not find <InputSsml> in body";
					checkrequest = false;
				}
				int Makesynch = 0;
				if (mapBodyIntParameter.find("Makesynch") != mapBodyIntParameter.end())
				{
					Makesynch = mapBodyIntParameter["Makesynch"];
				}
				else
				{
					errmsg = "not find <Makesynch> in body";
					checkrequest = false;
				}

				//2
				if (checkrequest)
				{
					char tempbuff[16] = { 0 };
					std::string speed = ""; snprintf(tempbuff, 16, "%.1f", Speed); speed = tempbuff;
					std::string ssmltext_md5 = InputSsml + speed;
					std::string textguid = md5::getStringMD5(ssmltext_md5);
					if (!MakeVedio && digitalmysql::isexisttask_textguid(TaskID, TaskName, textguid))
					{
						httpRetStr_ansi = getjson_runtaskresult(TaskID);

						//notify html
						int times = 0;
						std::string htmlnotifymsg = getNotifyMsg_ToHtml(TaskID);
						bool notifyresult = sendRabbitmqMsg(htmlnotifymsg);
						while (!notifyresult)//retry
						{
							++times;
							notifyresult = sendRabbitmqMsg(htmlnotifymsg);
							if (times > 3) break;
							sleep(1000);
						}
						std::string msgresult = (notifyresult) ? ("success") : ("failed");
						_debug_to( 0, ("Send HTML Notify[%s]: %s\n"), msgresult.c_str(), htmlnotifymsg.c_str());
					}
					else
					{
						digitalmysql::taskinfo new_taskitem;
						new_taskitem.speed = Speed;
						//
						new_taskitem.taskid = TaskID;
						new_taskitem.tasktype = MakeVedio;
						new_taskitem.taskname = TaskName;
						new_taskitem.taskstate = 0;
						new_taskitem.taskprogress = 0;
						new_taskitem.createtime = "";
						new_taskitem.humanid = VirtualmanKey;
						new_taskitem.ssmltext = InputSsml;
						new_taskitem.audio_path = "";
						new_taskitem.audio_format = "";
						new_taskitem.audio_length = 0;
						new_taskitem.video_path = "";
						new_taskitem.video_finalpath = "";
						new_taskitem.video_format = "";
						new_taskitem.video_length = 0;
						new_taskitem.video_width = 0;
						new_taskitem.video_height = 0;
						new_taskitem.video_fps = 0.0;

						//添加新合成任务到数据库
						bool update = (digitalmysql::isexisttask_taskid(TaskID)) ? (true) : (false);
						_debug_to( 0, ("TaskID=%d, update=%d\n"), TaskID, update);
						digitalmysql::addtaskinfo(TaskID, new_taskitem, update);
						digitalmysql::setmergestate(TaskID, 0);//任务状态为进行中
						digitalmysql::setmergeprogress(TaskID, 0);//合成进度为0

						//获取数字人信息
						std::string AcousticModelFullPath = "";	//"../0ModelFile/test/TTS/speak/snapshot_iter_1668699.pdz";
						std::string VcoderModelFullPath = "";	//"../0ModelFile/test/TTS/pwg/snapshot_iter_1000000.pdz";
						std::string DFMModelsPath = "";			// "../0ModelFile/test/W2L/file/shenzhen_v3_20230227.dfm";
						std::string OriVideoPath = "";			// "../0ModelFile/test/W2L/video/bule22.mp4";
						digitalmysql::VEC_HUMANINFO humaninfo;
						int ncount = digitalmysql::gethumanlistinfo(VirtualmanKey, humaninfo);
						if (ncount >= 1)
						{
							AcousticModelFullPath = humaninfo[0].speakmodelpath;
							VcoderModelFullPath = humaninfo[0].pwgmodelpath;
							DFMModelsPath = humaninfo[0].facemodelpath;
							OriVideoPath = humaninfo[0].videofile;
						}

						//添加合成任务到队列
						bool exist = digitalmysql::isexisttask_taskid(TaskID);
						if (exist)
						{
							std::string task_filename = "task.mp4";
							char buff[256] = { 0 }; snprintf(buff, 256, "%d.mp4", TaskID); task_filename = buff;

							mergetaskinfo new_mergetaskitem;
							new_mergetaskitem.tasktype = MakeVedio;
							new_mergetaskitem.speakspeed = Speed;
							new_mergetaskitem.taskid = TaskID;
							new_mergetaskitem.tasktext = InputSsml;
							new_mergetaskitem.path1 = digitvideo_path1; 
							new_mergetaskitem.path2 = digitvideo_path2;
							new_mergetaskitem.filename = task_filename;
							new_mergetaskitem.VirtualmanKey = VirtualmanKey;
							new_mergetaskitem.AcousticModelFullPath = AcousticModelFullPath;
							new_mergetaskitem.VcoderModelFullPath = VcoderModelFullPath;
							new_mergetaskitem.DFMModelsPath = DFMModelsPath;
							new_mergetaskitem.OriVideoPath = OriVideoPath;
							new_mergetaskitem.state = -1;

							if (!Makesynch)//thread run
							{
								pthread_mutex_lock(&mutex_mergetaskinfo);
								Container_mergetaskinfo.insert(std::make_pair(TaskID, new_mergetaskitem));
								pthread_mutex_unlock(&mutex_mergetaskinfo);

								char temp_buff[1024] = { 0 };
								snprintf(temp_buff, 1024, "\"TaskID\":\"%d\"", TaskID); data = temp_buff;
								httpRetStr_ansi = getjson_error(0, errmsg, data);//not too long,can use function
							}
							else   //now run
							{
								long long dwS = gettimecount();
								httpRetStr_ansi = getjson_runtask_now(new_mergetaskitem);
								long long dwE = gettimecount();
								_debug_to( 0, ("++++++++++++++任务执行总时间: %d秒++++++++++++++\n"), dwE-dwS);
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
				}
				else
				{
					httpRetStr_ansi = getjson_error(1, errmsg);
					result_debug = false;
				}
			}
			else if (pathStr.compare(("/v1/videomaker/playout/taskstate")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/taskstate}";
				//查询数字人合成进度
				bool checkrequest = true; std::string errmsg = "success"; std::string data = "";

				//1
				int TaskID = 0;
				if (mapBodyIntParameter.find("TaskID") != mapBodyIntParameter.end())
				{
					TaskID = mapBodyIntParameter["TaskID"];
				}
				else
				{
					errmsg = "not find <TaskID> in body";
					checkrequest = false;
				}

				//2
				if (checkrequest)
				{
					int nprogress = -1, taskstate = 0;
					if (TaskID != 0)
					{
						nprogress = digitalmysql::getmergeprogress(TaskID);
						taskstate = digitalmysql::getmergestate(TaskID);

						char buff[256] = { 0 };
						snprintf(buff, 256, "{\"TaskID\": %d,\"State\": %d,\"Progress\": %d}", TaskID, taskstate, nprogress);
						httpRetStr_ansi = getjson_error(0, errmsg, data);//not too long,can use function
					}
					else
					{
						errmsg = "TaskID is 0, is invalide ";
						httpRetStr_ansi = getjson_error(1, errmsg);
						result_debug = false;
					}
				}
				else
				{
					httpRetStr_ansi = getjson_error(1, errmsg);
					result_debug = false;
				}
			}
			else if (pathStr.compare(("/v1/videomaker/playout/deletetask")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/deletetask}";
				//删除数字人任务
				bool checkrequest = true; std::string errmsg = "success"; std::string data = "";

				//1
				int taskid = 0;
				if (mapBodyIntParameter.find("TaskID") != mapBodyIntParameter.end())
				{
					taskid = mapBodyIntParameter["TaskID"];
				}
				else
				{
					errmsg = "not find <TaskID> in body";
					checkrequest = false;
				}

				//2
				if (checkrequest)
				{
					bool del_ret = digitalmysql::deletetask_taskid(taskid, errmsg);
					httpRetStr_ansi = getjson_error((int)del_ret, errmsg);
				}
				else
				{
					httpRetStr_ansi = getjson_error(1, errmsg);
					result_debug = false;
				}
			}
		}
		else if (req->type == EVHTTP_REQ_GET)
		{
			//获取关键帧接口
			return_databuff = true;
			if (pathStr.compare(("/v1/videomaker/playout/getkeyframe")) == 0)
			{
				//测试
				std::string image_path = "C:/default.png";

				//type
				std::string file_type = get_file_extension((char*)image_path.c_str());
				content_type = "image/" + file_type; //告知浏览器文件类型,否则页面无法显示,而是转为下载
				//data
				content_databuff = read_file(image_path.c_str(), content_datalen);
				if (!content_databuff || content_datalen<=0)
				{
					evhttp_send_reply(req, HTTP_NOTFOUND, "read image data error...", 0);//返回未找到页面
					return;
				}	
			}
		}
		else
		{
			//自己处理
			httpRetStr_ansi = "{other request return json}";
		}

		
		



		//这里一般我会阻塞等待外部处理



		if (return_databuff)
		{
			//return string to html
////////////////////////////////////////////////////////////////////////////////////////////////
			try
			{
				evkeyvalq* header = evhttp_request_get_output_headers(req);
				if (header) 
				{
					evhttp_add_header(header, "Access-Control-Allow-Origin", "*");
					evhttp_add_header(header, "Access-Control-Allow-Methods", "GET,PUT,POST,HEAD,OPTIONS,DELETE,PATCH");
					evhttp_add_header(header, "Access-Control-Allow-Headers", "Content-Type,Content-Length,Authorization,Accept,X-Requested-With");
					evhttp_add_header(header, "Access-Control-Max-Age", "3600");
					evhttp_add_header(header, "Content-Type", content_type.c_str());	
				}

				evbuffer* outbuf = evhttp_request_get_output_buffer(req); //返回的body
				if (content_databuff && content_datalen)
				{
					evbuffer_add(outbuf, content_databuff, content_datalen);
				}
				evhttp_send_reply(req, http_code, "proxy", outbuf);
				free(content_databuff);

				//
				long long end_time = gettimecount();
				std::string result = (result_debug) ? ("OK") : ("FAILED");
				_debug_to( 0, ("\nhttp server: return string output %s, length=%d, result=%s, runtime=[%lld]s\n"), httpRetStr_debug.c_str(), (int)httpRetStr_ansi.length(), result.c_str(), end_time - start_time);
			}
			catch(...)
			{
				_debug_to( 1, ("\nhttp server: return databuff throw exception\n"));
			}

		}
		else
		{
			//return string to html
////////////////////////////////////////////////////////////////////////////////////////////////
#if defined WIN32//这里进行了修改，只有WINDOWS下需要转化，linux下直接认为是UTF8编码
			ansi_to_utf8(httpRetStr_ansi.c_str(), httpRetStr_ansi.length(), httpRetStr_utf8);
#else
			httpRetStr_utf8 = httpRetStr_ansi;
#endif

			//回应
			try 
			{
				evkeyvalq* header = evhttp_request_get_output_headers(req);
				if (header) {
					evhttp_add_header(header, "Access-Control-Allow-Origin", "*");
					evhttp_add_header(header, "Access-Control-Allow-Methods", "GET,PUT,POST,HEAD,OPTIONS,DELETE,PATCH");
					evhttp_add_header(header, "Access-Control-Allow-Headers", "Content-Type,Content-Length,Authorization,Accept,X-Requested-With");
					evhttp_add_header(header, "Access-Control-Max-Age", "3600");
					evhttp_add_header(header, "Content-Type", "application/json; charset=UTF-8");
				}

				struct evbuffer* buf = evbuffer_new();
				if (!buf) {
					_debug_to( 1, ("\nhttp server: return string fail for malloc buffer fail\n"));
					return;
				}
				evbuffer_add_printf(buf, ("%s\n"), httpRetStr_utf8.c_str());//httpRetStr_utf8是返回的内容
				evhttp_send_reply(req, http_code, "proxy", buf);
				evbuffer_free(buf);

				//
				long long end_time = gettimecount();
				std::string result = (result_debug)? ("OK"):("FAILED");
				_debug_to( 0, ("\nhttp server: return string output %s, length=%d, result=%s, runtime=[%lld]s\n"), httpRetStr_debug.c_str(), (int)httpRetStr_ansi.length(), result.c_str(), end_time-start_time);
			}
			catch (...) 
			{
				_debug_to( 1, ("\nhttp server: return string throw exception\n"));
			}

		}	
	}
	catch (...) {
		_debug_to( 1, ("\nhttp server handler throw exception\n"));
	}
}

int main()
{
	std::string configpath = getexepath(); 
	configpath = str_replace(configpath, std::string("\\"), std::string("/"));configpath += "/config.txt";
	_debug_to( 0, ("COFIG PATH=%s\n"), configpath.c_str());
	if (!getconfig_global(configpath))
	{
		_debug_to( 1, ("GLOBAL config load failed,please check <config.txt> \n"));
		getchar();
	}

	if (!digitalmysql::getmysqlconfig(configpath))
	{
		_debug_to( 1, ("MYSQL config load failed,please check <config.txt> \n"));
		getchar();
	}
	if (!getconfig_actornode(configpath))
	{
		_debug_to( 1, ("ACTOR config load failed,please check <config.txt> \n"));
		getchar();
	}
	if (!getconfig_playnode(configpath))
	{
		_debug_to( 1, ("PLAYNODE config load failed,please check <config.txt> \n"));
		getchar();
	}

	if (!getconfig_digitvideopath(configpath))
	{
		_debug_to( 1, ("DIGITVIDEO config load failed,please check <config.txt> \n"));
		getchar();
	}

	if (!getconfig_rabbitmq(configpath))
	{
		_debug_to( 1, ("RABBITMQ config load failed,please check <config.txt> \n"));
		getchar();
	}
	g_RabbitmqSender = new nsRabbitmq::cwRabbitmqPublish(rabbitmq_ip, rabbitmq_port, rabbitmq_user, rabbitmq_passwd, nullptr, nullptr);


	//初始化
	httpServer::complex_httpServer server;
	int httpPort = 8081;//监听端口
	int threadCount = 10;//开启线程池个数
	int ret = server.start_http_server(global_http_generic_handler, nullptr, httpPort, threadCount, 10240);//开启监听
	if (ret < 0) 
	{
		_debug_to( 1, ("start_http_server failed\n"));
	}
	else {
		_debug_to( 1, ("start_http_server success\n"));
	}

    //初始化Actor容器
	pthread_mutex_init(&mutex_actorinfo, NULL);
	pthread_mutex_init(&mutex_mergetaskinfo, NULL);
	_debug_to( 1, ("Container_actornode size = %d\n"), Container_actornode.size());
	ACTORNODE_MAP::iterator itActorNode = Container_actornode.begin();
	for (itActorNode; itActorNode != Container_actornode.end(); ++itActorNode)
	{
		std::string ip = itActorNode->second.ip;
		short port = itActorNode->second.port;

		if (!ip.empty() && port!=0)
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

    //开启Actor状态更新线程
    ret = pthread_create(&threadid_updateactorinfo, nullptr, pthread_updateactorinfo, nullptr);
    if (ret != 0)
    {
		_debug_to( 1, ("thread_updateactorinfo create error\n"));
    }
    else
    {
		_debug_to( 1, ("thread_updateactorinfo is runing\n"));
    }

    //开启合成任务分配线程
	ret = pthread_create(&threadid_runtask_thread, nullptr, pthread_runtask_thread, nullptr);
	if (ret != 0)
	{
		_debug_to( 1, ("thread_assigntask create error\n"));
	}
	else
	{
		_debug_to( 1, ("thread_assigntask is runing\n"));
	}

#if DF_OPEN_PATCH_GET
	//开启资产入库线程
	pthread_mutex_init(&mutex_taskinfo_patchdata, NULL);
	ret = pthread_create(&threadid_patchdata, nullptr, pthread_patchdata, nullptr);
	if (ret != 0)
	{
		_debug_to( 1, ("thread_patchdata create error\n"));
	}
	else
	{
		_debug_to( 1, ("thread_patchdata is runing\n"));
	}

	//开启资产获取线程
	pthread_mutex_init(&mutex_taskinfo_getdata, NULL);
	ret = pthread_create(&threadid_getdata, nullptr, pthread_getdata, nullptr);
	if (ret != 0)
	{
		_debug_to( 1, ("thread_getdata create error\n"));
	}
	else
	{
		_debug_to( 1, ("thread_getdata is runing\n"));
	}
#endif

	//keep runing
	while (1)
	{
		char ch[256] = {0};
		_debug_to( 1, ("输入'Q'或‘q’退出程序:\n"));
		gets_s(ch, 255);
		std::string str; str = ch;
		if (str.compare("Q")==0 || str.compare("q")==0)
		{
			//结束http监听
			server.stop_http_server();
			break;
		}	
	}
}
#endif







