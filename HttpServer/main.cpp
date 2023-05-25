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

#pragma comment(lib,"libevent.lib")
#pragma comment(lib,"libevent_core.lib")
#pragma comment(lib,"libevent_extras.lib")

//
#pragma comment(lib,"ws2_32.lib")

//
#include "public.h"
#include "json.h"
#include "httpkit.h"
#include "videoOperate.h"
#include "mmRabbitmq.h"
#include "digitalmysql.h"
#include "digitalEntityJson.h"
#include "httpConcurrencyServer.h"



#define CHECK_EMPTY_STR(name,str,msg,result) {if(str.empty()){msg=std::string(name)+" is empty,please check request body";result=false;}}
#define CHECK_EMPTY_NUM(name,num,msg,result) {if(num==0){msg=std::string(name)+" = 0,please check request body";result=false;}}


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

//create folder
#include <iostream>
#include <cstdlib>
#ifdef _WIN32
#include <direct.h>
#define mkdir(x,y) _mkdir(x)
bool create_directory(const char* path) 
{
	int status = 0;
	status = mkdir(path);
	if (status == 0) 
	{
		std::cout << "Directory created at " << path << std::endl;
		return true;
	}
	else 
	{
		std::cerr << "Unable to create directory at " << path << std::endl;
		return false;
	}
}
bool folderExists(const char* folderPath) {
	DWORD attributes = GetFileAttributesA(folderPath);
	return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
}

#else

#include <sys/stat.h>
bool create_directory(const char* path) 
{
	int status = 0;
	status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (status == 0) 
	{
		std::cout << "Directory created at " << path << std::endl;
		return true;
	}
	else 
	{
		std::cerr << "Unable to create directory at " << path << std::endl;
		return false;
	}
}
bool folderExists(const char* folderPath) {
	struct stat info;
	if (stat(folderPath, &info) != 0) {
		return false;
	}
	return (info.st_mode & S_IFDIR);
}

#endif
bool create_directories(const char* path) 
{
	if (folderExists(path))//create exist folder will false
		return true;
	std::string total_path = path;
	if (total_path.find(':') == std::string::npos)//must absolute path
		return false;
	
	std::string current_path = "";
	std::string delimiter = "/";
	size_t pos = 0;
	std::string token;

	int ntimes = 0;
	while ((pos = total_path.find(delimiter)) != std::string::npos) 
	{
		token = total_path.substr(0, pos);
		current_path += token + delimiter;
		if (current_path.length() > 3 && !folderExists(current_path.c_str()))//a= D:/ not need create,b=create exist folder will false
			create_directory(current_path.c_str());
		total_path.erase(0, pos + delimiter.length());

		if (++ntimes > 10)//keep right
			break;
	}

	return create_directory(path);
}


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
std::string getjson_humanlistinfo(std::string contentid = "")
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
												result_item.HumanID = itcontentId_->second;
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
												if (is_existfile(filepath.c_str()))
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


#if 1 //环境参数

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
		if (configbuffer == nullptr) return false;

		fread(configbuffer, length, 1, fp);
		fclose(fp);
	}

	if (length && configbuffer)
	{
		std::string value = "";
		std::string config = configbuffer;
		free(configbuffer);

		value = getnodevalue(config, "actor_count");
		if (value.empty()) return false;
		int count = atoi(value.c_str());
		_debug_to(1, ("CONFIG_actornode count = %d\n"), count);

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
			_debug_to(1, ("CONFIG_actornode actor%d_ip = %s\n"), i, ip.c_str());

			std::string actor_port = actor_pro + "port";
			value = getnodevalue(config, actor_port);
			if (value.empty()) continue;
			port = atoi(value.c_str());
			_debug_to(1, ("CONFIG_actornode actor%d_port = %d\n"), i, port);

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
		if (configbuffer == nullptr) return false;

		fread(configbuffer, length, 1, fp);
		fclose(fp);
	}

	if (length && configbuffer)
	{
		std::string value = "";
		std::string config = configbuffer;
		free(configbuffer);

		value = getnodevalue(config, "playnode_ip");
		if (value.empty()) return false;
		playnode_ip = value;
		_debug_to(1, ("CONFIG_playnode_ip = %s\n"), playnode_ip.c_str());

		value = getnodevalue(config, "playnode_port");
		if (value.empty()) return false;
		playnode_port = atoi(value.c_str());
		_debug_to(1, ("CONFIG_playnode_port = %d\n"), playnode_port);

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
		if (configbuffer == nullptr) return false;

		fread(configbuffer, length, 1, fp);
		fclose(fp);
	}

	if (length && configbuffer)
	{
		std::string value = "";
		std::string config = configbuffer;
		free(configbuffer);

		value = getnodevalue(config, "digitvideo_path1");
		if (value.empty()) return false;
		digitvideo_path1 = value;
		_debug_to(1, ("CONFIG_digitvideo_path1 = %s\n"), digitvideo_path1.c_str());

		value = getnodevalue(config, "digitvideo_path2");
		if (value.empty()) return false;
		digitvideo_path2 = value;
		_debug_to(1, ("CONFIG_digitvideo_path2 = %s\n"), digitvideo_path2.c_str());

		return true;
	}

	return false;
}

unsigned int delay_beforetext = 500;
unsigned int delay_aftertext = 300;
std::string  folder_digitalmodel = "";
std::string  folder_htmldigital = "";//child include <task>+<keyframe>+<resource>
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
		if (configbuffer == nullptr) return false;

		fread(configbuffer, length, 1, fp);
		fclose(fp);
	}

	if (length && configbuffer)
	{
		std::string value = "";
		std::string config = configbuffer;
		free(configbuffer);

		value = getnodevalue(config, "delay_beforetext");
		delay_beforetext = atoi(value.c_str());
		_debug_to(1, ("CONFIG_delay_beforetext = %d\n"), delay_beforetext);

		value = getnodevalue(config, "delay_aftertext");
		delay_aftertext = atoi(value.c_str());
		_debug_to(1, ("CONFIG_delay_aftertext = %d\n"), delay_aftertext);

		value = getnodevalue(config, "folder_digitalmodel");
		if (value.empty()) return false;
		folder_digitalmodel = value.c_str();
		_debug_to(1, ("CONFIG_folder_digitalmodel = %s\n"), folder_digitalmodel.c_str());

		value = getnodevalue(config, "folder_htmldigital");
		if (value.empty()) return false;
		folder_htmldigital = value.c_str();
		_debug_to(1, ("CONFIG_folder_htmldigital = %s\n"), folder_htmldigital.c_str());

		return true;
	}

	return false;
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
		if (configbuffer == nullptr) return false;

		fread(configbuffer, length, 1, fp);
		fclose(fp);
	}

	if (length && configbuffer)
	{
		std::string value = "";
		std::string config = configbuffer;
		free(configbuffer);

		value = getnodevalue(config, "rabbitmq_ip");
		if (value.empty()) return false;
		rabbitmq_ip = value;
		_debug_to(1, ("CONFIG_rabbitmq_ip = %s\n"), rabbitmq_ip.c_str());

		value = getnodevalue(config, "rabbitmq_port");
		if (value.empty()) return false;
		rabbitmq_port = atoi(value.c_str());
		_debug_to(1, ("CONFIG_rabbitmq_port = %d\n"), rabbitmq_port);

		value = getnodevalue(config, "rabbitmq_user");
		if (value.empty()) return false;
		rabbitmq_user = value;
		_debug_to(1, ("CONFIG_rabbitmq_user = %s\n"), rabbitmq_user.c_str());

		value = getnodevalue(config, "rabbitmq_passwd");
		if (value.empty()) return false;
		rabbitmq_passwd = value;
		_debug_to(1, ("CONFIG_rabbitmq_passwd = %s\n"), rabbitmq_passwd.c_str());

		//
		value = getnodevalue(config, "rabbitmq_exchange");
		if (value.empty()) return false;
		rabbitmq_exchange = value;
		_debug_to(1, ("CONFIG_rabbitmq_exchange = %s\n"), rabbitmq_exchange.c_str());

		value = getnodevalue(config, "rabbitmq_routekey");
		if (value.empty()) return false;
		rabbitmq_routekey = value;
		_debug_to(1, ("CONFIG_rabbitmq_routekey = %s\n"), rabbitmq_routekey.c_str());

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
		digitalmysql::VEC_HUMANINFO vechumaninfo;
		digitalmysql::gethumanlistinfo(taskhumanid, vechumaninfo);
		if (!vechumaninfo.empty())
			video_keyframe = vechumaninfo[0].keyframe;
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
		_debug_to(0, ("Rabbitmq object is null,please restart...\n"));
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
std::string getjson_humanlistinfo(std::string humanid = "")
{
	bool result = true;
	std::string errmsg = "";
	std::string result_str = "";
	
	//1-getlist
	digitalmysql::VEC_HUMANINFO vechumaninfo;
	result = digitalmysql::gethumanlistinfo(humanid, vechumaninfo);
	_debug_to( 0, ("gethumanlistinfo: vechumaninfo size=%d\n"), vechumaninfo.size());
	if (!result)
		errmsg = "gethumanlistinfo from mysql failed";

	//2-parsedata
	std::string list_info = "";
	DigitalMan_Items result_object;
	for(size_t i = 0; i < vechumaninfo.size(); i++)
	{
		DigitalMan_Item result_item;

		std::string format = "";
		std::string base64_encode = "";
		unsigned int width = 0, height = 0, bitcount = 32;
		std::string filepath = vechumaninfo[i].keyframe;
		if (is_existfile(filepath.c_str()))
		{
			picture::GetPicInfomation(filepath.c_str(), &width, &height, &bitcount, format);
			//base64_encode = base64::base64_encode_file(filepath);
		}

		result_item.HumanID = vechumaninfo[i].humanid;
		result_item.HumanName = vechumaninfo[i].humanname;
		result_item.SpeakSpeed = vechumaninfo[i].speakspeed;
		result_item.KeyFrame_Format = format;
		result_item.KeyFrame_Width = width;
		result_item.KeyFrame_Height = height;
		result_item.KeyFrame_BitCount = bitcount;
		result_item.KeyFrame_FilePath = filepath;
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
	_debug_to( 0, ("get taskhistory size=%d\n"), vectaskhistory.size());
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
		result_item.BackgroundFile = vectaskhistory[i].background_path;
		result_item.Front_XPos = vectaskhistory[i].front_XPos;
		result_item.Front_YPos = vectaskhistory[i].front_YPos;
		result_item.Front_Scale = vectaskhistory[i].front_Scale;
		result_item.Front_Rotation = vectaskhistory[i].front_Rotation;

		//从视频路径获取关键帧路径
		std::string format = "";
		std::string base64_encode = "";
		unsigned int width = 0, height = 0, bitcount = 32;
		std::string filepath = vectaskhistory[i].video_finalpath; filepath = str_replace(filepath, ".mp4", ".bmp");
		if (is_existfile(filepath.c_str()))
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
			//无任务关键帧则使用数字人关键帧
			std::string humanid = vectaskhistory[i].humanid;
			digitalmysql::VEC_HUMANINFO vechumaninfo;
			digitalmysql::gethumanlistinfo(humanid, vechumaninfo);
			if (vechumaninfo.size() > 0)
			{
				filepath = vechumaninfo[0].keyframe;
				if (is_existfile(filepath.c_str()))
				{
					_debug_to(0, ("task %d: use humansource keyframe, humanid=%s\n"), vectaskhistory[i].taskid, humanid);
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
			result_item.TaskSource_FilePath = vectasksourceinfo[i].sourcepath;
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
			result_item.TaskSource_FilePath = vectasksourceinfo[j].sourcepath;
			result_item.TaskSource_KeyFrame = vectasksourceinfo[j].sourcekeyframe;
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
			result_item.TaskSource_FilePath = vectasksourceinfo[i].sourcepath;
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


//
bool waiting_videomerge(int taskid,int timeout = 1200)//1200s timeout
{
	bool result = false;
	int state_now = 0, progress = 0; int time = 0;
	while ((progress < 95 && time <= timeout))
	{
		progress = digitalmysql::getmergeprogress(taskid);
		state_now = digitalmysql::getmergestate(taskid);
		if (state_now > 0)//state:0=mergeing 1=success 2=failed
			break;

		sleep(1000);
		++time;
	}

	if (progress >= 95)
		result = true;

	return result;
}
//Rec 通知消息
std::string getNotifyMsg_ToRecNode(digitalmysql::taskinfo taskitem)
{
	std::string result_msg = "";
	char buff[BUFF_SZ] = { 0 };

	std::string send_videopath = taskitem.video_path;
	send_videopath = str_replace(send_videopath.c_str(), ".mp4", "");

	std::string messageid = getmessageid();
	char msg_buff[BUFF_SZ] = { 0 };
	snprintf(msg_buff, BUFF_SZ, "{\"ddrinfo\":[	\
				{\"ddr\":\"%s\",\"devid\":\"\",\"offset\":0,\"length\":%d,\"pos\":{\"x\":0,\"y\":0,\"with\":0,\"height\":0}}	\
				],\"background\":\"%s\",\"msgid\":\"%s\"}",
		send_videopath.c_str(), taskitem.video_length, 
		taskitem.background_path.c_str(),messageid.c_str());
	result_msg = msg_buff;

	return result_msg;
}
std::string getjson_runtaskresult(int taskid)
{
	//目前此函数仅用于生成语音，生成视频采用异步方式

	bool result = true;
	std::string errmsg = "";
	std::string result_str = "";

	long long dwS = gettimecount();
	bool ret_waiting = waiting_videomerge(taskid);
	long long dwE = gettimecount();
	_debug_to( 0, ("++++++++++++++[task_%d]合成数字人时间: %d秒++++++++++++++\n"), taskid, dwE - dwS);

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
					std::string sendmsg = "",recvmsg = "";
					sendmsg = getNotifyMsg_ToRecNode(taskitem);

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
		_debug_to( 0, ("++++++++++++++[task_%d]录制数字人时间: %d秒++++++++++++++\n"), taskid, dwE - dwS);
	}
	else
	{
		errmsg = "waiting task result error...";
		result_str = getjson_error(1, errmsg);
	}

	return result_str;
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
	//生成的音频视频需要移动到指定路径
	std::string Move_Path1;
	std::string Move_Path2;
	std::string Move_FileName;
	//指定模型文件
	std::string AcousticModelFullPath;		//../0ModelFile/test/TTS/speak/snapshot_iter_1668699.pdz
	std::string VcoderModelFullPath;		//../0ModelFile/test/TTS/pwg/snapshot_iter_1000000.pdz
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

		Move_Path1 = "";
		Move_Path2 = "";
		Move_FileName = "";

		AcousticModelFullPath = "";
		VcoderModelFullPath = "";
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

		Move_Path1 = item.Move_Path1;
		Move_Path2 = item.Move_Path2;
		Move_FileName = item.Move_FileName;

		AcousticModelFullPath = item.AcousticModelFullPath;
		VcoderModelFullPath = item.VcoderModelFullPath;
		DFMModelsPath = item.DFMModelsPath;
	}

}actortaskinfo, * pactortaskinfo;
typedef std::map<int, actortaskinfo> ACTORTASKINFO_MAP;
ACTORTASKINFO_MAP Container_actortaskinfo;

//Actor 通知消息
std::string getNotifyMsg_ToActor(actortaskinfo actortaskitem)
{
	std::string result_msg = "";
	char buff[BUFF_SZ] = { 0 };
	snprintf(buff, BUFF_SZ, "<MPC><RequestID>%d</RequestID><ColumnName>栏目名</ColumnName><MPCType>%d</MPCType><MoodType>%d</MoodType><SpeakSpeed>%.1f</SpeakSpeed><TaskText>[p%d]%s[p%d]</TaskText><AudioPath>%s</AudioPath><Path1>%s</Path1><Path2>%s</Path2><DestFileName>%s</DestFileName>	\
							<Humanid>%s</Humanid><AcousticModelFullPath>%s</AcousticModelFullPath><VcoderModelFullPath>%s</VcoderModelFullPath><DFMModelsPath>%s</DFMModelsPath></MPC>",
		actortaskitem.ActorTaskID, actortaskitem.ActorTaskType, actortaskitem.ActorMoodType, actortaskitem.ActorTaskSpeed, delay_beforetext, actortaskitem.ActorTaskText.c_str(), delay_aftertext, actortaskitem.ActorTaskAudio.c_str(), actortaskitem.Move_Path1.c_str(), actortaskitem.Move_Path2.c_str(), actortaskitem.Move_FileName.c_str(),
		actortaskitem.ActorTaskHumanID.c_str(), actortaskitem.AcousticModelFullPath.c_str(), actortaskitem.VcoderModelFullPath.c_str(), actortaskitem.DFMModelsPath.c_str());
	result_msg = buff;

	return result_msg;
}

//Actror 状态更新
pthread_mutex_t mutex_actorinfo;// actorinfo互斥量
pthread_mutex_t mutex_actortaskinfo;// actortaskinfo互斥量
pthread_t threadid_updateactorinfo;
void* pthread_updateactorinfo(void* arg)
{
	bool bInit = true;
	while (bInit)
	{
		actorinfo FindActor;
		ACTORINFO_MAP::iterator itFind = Container_actorinfo.begin();
		for (itFind; itFind != Container_actorinfo.end(); ++itFind)
		{
			pthread_mutex_lock(&mutex_actorinfo);
			FindActor.copydata(itFind->second);
			pthread_mutex_unlock(&mutex_actorinfo);

			std::string ip = FindActor.ip;
			short port = FindActor.port;
			std::string msg = "<actorstate></actorstate>";
			std::string recvmsg = "";
			if (SendTcpMsg_DGHDR(ip, port, msg, true, recvmsg))//查询Actor状态
			{
				_debug_to(0, ("addr: %s:%u ,send actor check message success,recv message: %s\n"), ip.c_str(), port, recvmsg.c_str());
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
				if (FindActor.state == 1 && FindActor.firstworktick != 0 && (FindActor.firstworktick - clock()) > 6000000)
				{
					itFind->second.state = -1;						//working too long
					_debug_to(1, ("addr: %s:%u , actor work crash,please restart...\n"), FindActor.ip.c_str(), FindActor.port);
				}
				pthread_mutex_unlock(&mutex_actorinfo);
			}
			else
			{
				_debug_to(0, ("addr: %s:%u ,send actor check message failed: %s\n"), ip.c_str(), port, msg.c_str());
			}
		}

		sleep(5000);
		//pthread_exit(nullptr);//中途退出当前线程
	}

	_debug_to(0, ("pthread_updateactorinfo exit...\n"));
	return nullptr;
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

		int findActorTaskID = 0;
		std::string findActorTaskText = "";
		//找到需合成的任务
		bool bTask = false;
		actortaskinfo find_actortaskitem;
		ACTORTASKINFO_MAP::iterator itFindTask = Container_actortaskinfo.begin();
		for (itFindTask; itFindTask != Container_actortaskinfo.end(); ++itFindTask)
		{
			if (itFindTask->second.ActorTaskState == -1)//need merge
			{
				findActorTaskID = itFindTask->second.ActorTaskID;
				findActorTaskText = itFindTask->second.ActorTaskText;
				find_actortaskitem.copydata(itFindTask->second);
				bTask = true;

				pthread_mutex_lock(&mutex_actortaskinfo);
				Container_actortaskinfo.erase(itFindTask);
				pthread_mutex_unlock(&mutex_actortaskinfo);
				break;
			}
		}

		//运行任务
		bool bContinueRun = (bActor && bTask);
		if (bContinueRun)
		{
			_debug_to(0, ("ASSIGN task run===================================================\n"));
			std::string recvmsg;
			std::string sendmsg = getNotifyMsg_ToActor(find_actortaskitem);
			if (SendTcpMsg_DGHDR(ip, port, sendmsg, false, recvmsg))//向Actor发送任务
			{
				_debug_to(0, ("addr: %s:%u ,send task success, ActorTaskID=%d\n"), ip.c_str(), port, findActorTaskID);
				std::string result_str = getjson_runtaskresult(findActorTaskID);
			}
			else
			{
				_debug_to(0, ("addr: %s:%u ,send task failed, ActorTaskID=%d\n"), ip.c_str(), port, findActorTaskID);
				digitalmysql::setmergestate(find_actortaskitem.ActorTaskID, 2);//任务状态为失败
				digitalmysql::setmergeprogress(find_actortaskitem.ActorTaskID, 100);//合成进度为100
			}

			//notify html
			int times = 0;
			std::string htmlnotifymsg = getNotifyMsg_ToHtml(find_actortaskitem.ActorTaskID);
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

		sleep(1000);
		//pthread_exit(nullptr);//中途退出当前线程
	}

	_debug_to(0, ("pthread_checkactor exit...\n"));
	return nullptr;
}

//MergeTask 立即执行
std::string getjson_runtask_now(actortaskinfo actortaskitem)
{
	std::string errmsg = "";
	std::string result_str = "";

	std::string ip = ""; short port = 0;
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

	//运行任务
	int nowActorTaskID = actortaskitem.ActorTaskID;
	std::string nowActorTaskText = actortaskitem.ActorTaskText;
	if (bActor)
	{
		_debug_to(0, ("SYNC task run===================================================\n"));
		std::string recvmsg;
		std::string sendmsg = getNotifyMsg_ToActor(actortaskitem);
		if (SendTcpMsg_DGHDR(ip, port, sendmsg, false, recvmsg))//向Actor发送任务
		{
			_debug_to(0, ("addr: %s:%u ,send task success, ActorTaskID=%d\n"), ip.c_str(), port, nowActorTaskID);
			result_str = getjson_runtaskresult(nowActorTaskID);
		}
		else
		{
			_debug_to(0, ("addr: %s:%u ,send task failed, ActorTaskID=%d\n"), ip.c_str(), port, nowActorTaskID);
			digitalmysql::setmergestate(actortaskitem.ActorTaskID, 2);//任务状态为失败
			digitalmysql::setmergeprogress(actortaskitem.ActorTaskID, 100);//合成进度为100

			errmsg = "send tcp message to actor failed...";
			result_str = getjson_error(1, errmsg);
		}

		//notify html
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

//fromdata数据保存到文件,用于调试
#define DF_FROMDATA_TOFILE 0

bool buffer_to_file(struct evbuffer* item_evbuffer, const struct evbuffer_ptr* item_offset, size_t item_bufferlen, std::string filefolder, std::string& finalpath, std::string& errmsg)
{
	//以下为fromdata的格式: #代表boundary，--#是起始符和分割符， --#--是结束符
	// --# + [header + \r\n\r\n + data] + --# + [header + \r\n\r\n + data] + ... + --#--
	//函数传入offset+bufferlen，认为这段数据是单个Item的数据

	if (!create_directories(filefolder.c_str()))
	{
		errmsg = "create final humanfolder error. ";
		errmsg += filefolder;
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
			std::string finalfilepath = finalpath;
			if (finalfilepath.empty())
				finalfilepath = filefolder + std::string("/") + partFileName_ansi;

			if (ptritem_end.pos > ptritem_splite.pos)
			{
				size_t item_datalen = ptritem_end.pos - ptritem_splite.pos;//使用修改后的pos

				FILE* fp = nullptr;
				fp = fopen(finalfilepath.c_str(), "wb");
				if (fp == nullptr)
				{
					errmsg = "opening file error... ";
					errmsg += finalfilepath;
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
				finalpath = finalfilepath;
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

//新增数字人接口
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
			if (item_bufferlen < 64)//字符串
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
				std::string filepath = "";
				if (!buffer_to_file(in_buffer, &ptr_start, item_bufferlen, human_sourcefolder, filepath, errmsg))
				{
					_debug_to(1, ("buffer_to_file error, errmsg = %s"), errmsg.c_str());
					continue;
				}
				else
				{
					filecount += 1;
				}
			}
		}
	}

	//update mysql
	if (filecount > 0)
	{
		digitalmysql::humaninfo humanitem_add;
		humanitem_add.humanname = humanname;
		humanitem_add.humanid = md5::getStringMD5(humanname);//名称MD5加密得到humanid
		humanitem_add.contentid = humanitem_add.humanid;
		humanitem_add.sourcefolder = human_sourcefolder;
		humanitem_add.available = 0;//不可用
		humanitem_add.speakspeed = 1.0;
		humanitem_add.seriousspeed = 0.8;
		humanitem_add.imagematting;
		humanitem_add.keyframe = "D:/server/html/static/digitalfile/keyframe/human_default.png";//default image
		humanitem_add.luckeyframe = "D:/server/html/static/digitalfile/keyframe/human_default.png";//default image
		humanitem_add.speakmodelpath;
		humanitem_add.pwgmodelpath;
		humanitem_add.mouthmodelfile;
		humanitem_add.facemodelfile;

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
	std::string audio_sourcefolder = folder_htmldigital + std::string("/task");
	for (size_t i = 0; i < vecoffsetptr.size() - 1; ++i)
	{
		evbuffer_ptr ptr_start = vecoffsetptr[i];
		evbuffer_ptr ptr_end = vecoffsetptr[i + 1];
		if (ptr_start.pos < ptr_end.pos)//数据段
		{
			size_t item_bufferlen = ptr_end.pos - ptr_start.pos;
			if (item_bufferlen < 64)//字符串
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
				if (!buffer_to_file(in_buffer, &ptr_start, item_bufferlen, audio_sourcefolder, filepath, errmsg))
				{
					_debug_to(1, ("buffer_to_file error, errmsg = %s"), errmsg.c_str());
					continue;
				}
				else
				{
					if (is_audiofile(filepath.c_str()))
					{
						audiopath = filepath;
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
			if (item_bufferlen < 64)//字符串
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
		//保存数据库
		digitalmysql::tasksourceinfo add_tasksourceitem;
		add_tasksourceitem.sourcetype = digitalmysql::source_image;
		add_tasksourceitem.sourcepath = vecImagePath[i];
		add_tasksourceitem.sourcekeyframe = "";
		add_tasksourceitem.createtime = gettimecode();

		bool update = digitalmysql::isexisttasksource_path(vecImagePath[i]);
		if (!digitalmysql::addtasksource(add_tasksourceitem, update))
		{
			errmsg = "add image tasksource to mysql failed...";
			return false;
		}
	}
	videocount = vecVideoPath.size();
	for (size_t j = 0; j < videocount; j++)
	{
		//获取关键帧
		std::string sourcekeyframe = vecVideoPath[j];
		sourcekeyframe = background_sourcefolder + std::string("/") + md5::getStringMD5(sourcekeyframe) + std::string(".png");
		getimage_fromvideo(vecVideoPath[j], sourcekeyframe);

		//保存数据库
		digitalmysql::tasksourceinfo add_tasksourceitem;
		add_tasksourceitem.sourcetype = digitalmysql::source_video;
		add_tasksourceitem.sourcepath = vecVideoPath[j];
		add_tasksourceitem.sourcekeyframe = sourcekeyframe;
		add_tasksourceitem.createtime = gettimecode();

		bool update = digitalmysql::isexisttasksource_path(vecVideoPath[j]);
		if (!digitalmysql::addtasksource(add_tasksourceitem, update))
		{
			errmsg = "add video tasksource to mysql failed...";
			return false;
		}
	}
	audiocount = vecAudioPath.size();
	for (size_t k = 0; k < audiocount; k++)
	{
		//保存数据库
		digitalmysql::tasksourceinfo add_tasksourceitem;
		add_tasksourceitem.sourcetype = digitalmysql::source_audio;
		add_tasksourceitem.sourcepath = vecAudioPath[k];
		add_tasksourceitem.sourcekeyframe = "";
		add_tasksourceitem.createtime = gettimecode();

		bool update = digitalmysql::isexisttasksource_path(vecAudioPath[k]);
		if (!digitalmysql::addtasksource(add_tasksourceitem, update))
		{
			errmsg = "add audio tasksource to mysql failed...";
			return false;
		}
	}

	return true;
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
			_debug_to( 0, ("http server receive message from %s, path is %s, query param is %s, body is %s\n"), hostStr.c_str(), pathStr.c_str(), queryStr.c_str(), httpReqBodyStr_ansi.c_str());
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
		if (!httpReqBodyStr_ansi.empty() && !str_existsubstr(pathStr,std::string("Add")))//路径中含Add，则是上传文件接口，Body是fromdata类型
		{
			json::Value body_val = json::Deserialize((char*)httpReqBodyStr_ansi.c_str());
			if (body_val.GetType() == json::ObjectVal)
			{
				json::Object body_obj = body_val.ToObject();
				ParseBodyStr(body_obj, mapBodyStrParameter, mapBodyIntParameter, mapBodyDoubleParameter);
			}
			else
			{
				std::string errormsg = "json not formatted successfully...";
				httpRetStr_ansi = getjson_error(1, errormsg);
				goto http_reply;
			}
		}
		
		//自定义处理
		if (req->type == EVHTTP_REQ_POST)
		{

#if DF_OPEN_PATCH_GET
			if (pathStr.compare(("/v1/videomaker/playout/ImportModel")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/ImportModel}";
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
			else if (pathStr.compare(("/v1/videomaker/playout/GetModel")) == 0)
			{
				httpRetStr_debug = "{GET-/v1/videomaker/playout/GetModel}";
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
					httpRetStr_ansi = getjson_error(1,errmsg);
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
					std::string HumanName = "New Human";
					bool ret = ParseAddHuman(in_header, in_buffer, HumanName, FileCount, errmsg);
					if (ret)
					{
						char temp_buff[256] = { 0 };
						snprintf(temp_buff, 256, "\"HumanName\":\"%s\",\"FileCount\":%d", HumanName.c_str(), FileCount); data = temp_buff;
						httpRetStr_ansi = getjson_error(0, errmsg, data);//not too long,can use function
					}
					else
					{
						httpRetStr_ansi = getjson_error(1, errmsg);
						result_debug = false;
					}
				}
			}
			else if (pathStr.compare(("/v1/videomaker/playout/DeleteHuman"))==0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/DeleteHuman}";
				//删除数字人接口
				bool checkrequest = true; std::string errmsg = "success"; std::string data = "";

				//1
				std::string HumanID = "";
				if (mapBodyStrParameter.find("HumanID") != mapBodyStrParameter.end())
					HumanID = mapBodyStrParameter["HumanID"];
				CHECK_EMPTY_STR("HumanID", HumanID, errmsg, checkrequest);

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
				bool checkrequest = true;std::string errmsg = "success"; std::string data = "";

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
				bool checkrequest = true;std::string errmsg = "success"; std::string data = "";
				
				//1
				std::string TaskName = "";
				if (mapBodyStrParameter.find("TaskName") != mapBodyStrParameter.end())
					TaskName = mapBodyStrParameter["TaskName"];
				CHECK_EMPTY_STR("TaskName", TaskName, errmsg, checkrequest);
				std::string HumanID = "";
				if (mapBodyStrParameter.find("HumanID") != mapBodyStrParameter.end())
					HumanID = mapBodyStrParameter["HumanID"];
				CHECK_EMPTY_STR("HumanID", HumanID, errmsg, checkrequest);
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
				int Front_XPos = 0, Front_YPos = 0, Front_Scale = 100, Front_Rotation = 0;
				if (mapBodyIntParameter.find("Front_XPos") != mapBodyIntParameter.end())
					Front_XPos = mapBodyIntParameter["Front_XPos"]; 
				Front_XPos = (Front_XPos < 0) ? (0) : (Front_XPos);
				if (mapBodyIntParameter.find("Front_YPos") != mapBodyIntParameter.end())
					Front_YPos = mapBodyIntParameter["Front_YPos"]; 
				Front_YPos = (Front_YPos < 0) ? (0) : (Front_YPos);
				if (mapBodyIntParameter.find("Front_Scale") != mapBodyIntParameter.end())
					Front_Scale = mapBodyIntParameter["Front_Scale"]; 
				Front_Scale = (Front_Scale < 10) ? (10) : (Front_Scale);
				if (mapBodyIntParameter.find("Front_Rotation") != mapBodyIntParameter.end())
					Front_Rotation = mapBodyIntParameter["Front_Rotation"]; 
				Front_Rotation = (Front_Rotation < -180) ? (-180) : (Front_Rotation);

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
				if (TaskType == 1)
				{
					CHECK_EMPTY_STR("InputSsml", InputSsml, errmsg, checkrequest);
					InputAudio = "";
				}
				else if (TaskType == 2)
				{
					CHECK_EMPTY_STR("InputAudio", InputAudio, errmsg, checkrequest);
					InputSsml = "";
				}
				//检查对应数字人的available状态，为0表示正在训练中
				else if (!digitalmysql::isavailable_humanid(HumanID))
				{
					errmsg = "the digital man is in training...";
					checkrequest = false;
				}
				
				//2
				if (checkrequest)
				{
					char tempbuff[16] = { 0 };
					std::string speed = ""; snprintf(tempbuff, 16, "%.1f", Speed); speed = tempbuff;
					std::string ssmltext_md5 = InputSsml + speed;
					std::string textguid = md5::getStringMD5(ssmltext_md5);
					bool existtask = digitalmysql::isexisttask_textguid(TaskID, TaskName, textguid);

					if (existtask && TaskType == 0)
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
						_debug_to(0, ("Send HTML Notify[%s]: %s\n"), msgresult.c_str(), htmlnotifymsg.c_str());
					}
					else
					{
						//获取数字人信息
						std::string HumanName = "";
						std::string AcousticModelFullPath = "";	//"../ModelFile/test/TTS/speak/snapshot_iter_1668699.pdz";
						std::string VcoderModelFullPath = "";	//"../ModelFile/test/TTS/pwg/snapshot_iter_1000000.pdz";
						std::string DFMModelsPath = "";			// "../ModelFile/test/W2L/file/shenzhen_v3_20230227.dfm";
						digitalmysql::VEC_HUMANINFO vechumaninfo;
						digitalmysql::gethumanlistinfo(HumanID, vechumaninfo);
						if (vechumaninfo.size() > 0)
						{
							HumanName = vechumaninfo[0].humanname;
							AcousticModelFullPath = vechumaninfo[0].speakmodelpath;
							VcoderModelFullPath = vechumaninfo[0].pwgmodelpath;
							DFMModelsPath = vechumaninfo[0].facemodelfile;
						}

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
						new_taskitem.video_finalpath = "";
						new_taskitem.video_format = "";
						new_taskitem.video_length = 0;
						new_taskitem.video_width = 0;
						new_taskitem.video_height = 0;
						new_taskitem.video_fps = 0.0;
						new_taskitem.background_path = Background;
						new_taskitem.front_XPos = Front_XPos;
						new_taskitem.front_YPos = Front_YPos;
						new_taskitem.front_Scale = Front_Scale;
						new_taskitem.front_Rotation = Front_Rotation;


						//添加新合成任务到数据库
						bool update = (digitalmysql::isexisttask_taskid(TaskID)) ? (true) : (false);
						_debug_to( 0, ("BeforeInsert: TaskID=%d, update=%d\n"), TaskID, update);
						digitalmysql::addtaskinfo(TaskID, new_taskitem, update);
						digitalmysql::setmergestate(TaskID, 0);//任务状态为进行中
						digitalmysql::setmergeprogress(TaskID, 0);//合成进度为0
						_debug_to(0, ("AfterInsert: TaskID=%d, update=%d\n"), TaskID, update);

						//添加合成任务到队列
						bool exist = digitalmysql::isexisttask_taskid(TaskID);
						if (exist)
						{
							std::string TaskVideo_FileName = "task.mp4";
							char buff[256] = { 0 }; snprintf(buff, 256, "%d.mp4", TaskID); TaskVideo_FileName = buff;

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
							new_actortaskitem.Move_Path1 = digitvideo_path1; 
							new_actortaskitem.Move_Path2 = digitvideo_path2;
							new_actortaskitem.Move_FileName = TaskVideo_FileName;
							new_actortaskitem.AcousticModelFullPath = AcousticModelFullPath;
							new_actortaskitem.VcoderModelFullPath = VcoderModelFullPath;
							new_actortaskitem.DFMModelsPath = DFMModelsPath;

							if (!Makesynch)//thread run
							{
								pthread_mutex_lock(&mutex_actortaskinfo);
								Container_actortaskinfo.insert(std::make_pair(TaskID, new_actortaskitem));
								pthread_mutex_unlock(&mutex_actortaskinfo);

								char temp_buff[256] = { 0 };
								snprintf(temp_buff, 256, "\"TaskID\":\"%d\"", TaskID); data = temp_buff;
								httpRetStr_ansi = getjson_error(0, errmsg, data);//not too long,can use function
							}
							else   //now run
							{
								long long dwS = gettimecount();
								httpRetStr_ansi = getjson_runtask_now(new_actortaskitem);
								long long dwE = gettimecount();
								_debug_to( 0, ("++++++++++++++[task_%d]任务执行总时间: %d秒++++++++++++++\n"), TaskID, dwE-dwS);
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
			else if (pathStr.compare(("/v1/videomaker/playout/DeleteTask")) == 0)
			{
				httpRetStr_debug = "{POST-/v1/videomaker/playout/DeleteTask}";
				//删除数字人任务
				bool checkrequest = true; std::string errmsg = "success"; std::string data = "";

				//1
				int TaskID = 0;
				if (mapBodyIntParameter.find("TaskID") != mapBodyIntParameter.end())
					TaskID = mapBodyIntParameter["TaskID"];
				CHECK_EMPTY_NUM("TaskID", TaskID, errmsg, checkrequest);

				int _DeleteFile = 0;
				if (mapBodyIntParameter.find("DeleteFile") != mapBodyIntParameter.end())
					_DeleteFile = mapBodyIntParameter["DeleteFile"];

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
		}
		else if (req->type == EVHTTP_REQ_GET)
		{
			return_databuff = true;
			httpRetStr_debug = "GET";
			if (pathStr.compare(("/v1/videomaker/playout/GetKeyframe")) == 0)
			{
				//获取关键帧接口
				std::string image_path = "C:/default.png";//测试

				//type
				std::string file_type = get_file_extension((char*)image_path.c_str());
				content_type = "image/" + file_type; //告知浏览器文件类型,否则页面无法显示,而是转为下载
				//data
				if (!read_file(image_path.c_str(), content_databuff, content_datalen))
				{
					evhttp_send_reply(req, HTTP_NOTFOUND, "read image data error...", 0);//返回未找到页面
					return;
				}	
			}
		}
		else if (req->type == EVHTTP_REQ_PUT)
		{
			httpRetStr_debug = "PUT";
			httpRetStr_ansi = "{put request return json}";
		}
		else
		{
			httpRetStr_debug = "Other";
			httpRetStr_ansi = "{other request return json}";
		}

		
		//请求返回
http_reply:
		if (return_databuff)
		{
			//return string to html
////////////////////////////////////////////////////////////////////////////////////////////////
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

void setcmdwndsize(int col, int row)
{
	char cmd[64] = {0};
	sprintf(cmd, "mode con cols=%d lines=%d", col, row);
	system(cmd);
}

int main()
{
	setcmdwndsize(160, 40);

	std::string configpath = getexepath();
	configpath = str_replace(configpath, std::string("\\"), std::string("/")); configpath += "/config.txt";
	std::string configpath_utf8; ansi_to_utf8(configpath.c_str(), configpath.length(), configpath_utf8);
	_debug_to(0, ("COFIG PATH=%s\n"), configpath_utf8.c_str());
	if (!getconfig_global(configpath))
	{
		_debug_to(1, ("GLOBAL config load failed,please check <config.txt> \n"));
		getchar();
	}

	if (!digitalmysql::getmysqlconfig(configpath))
	{
		_debug_to(1, ("MYSQL config load failed,please check <config.txt> \n"));
		getchar();
	}
	if (!getconfig_actornode(configpath))
	{
		_debug_to(1, ("ACTOR config load failed,please check <config.txt> \n"));
		getchar();
	}
	if (!getconfig_playnode(configpath))
	{
		_debug_to(1, ("PLAYNODE config load failed,please check <config.txt> \n"));
		getchar();
	}

	if (!getconfig_digitvideopath(configpath))
	{
		_debug_to(1, ("DIGITVIDEO config load failed,please check <config.txt> \n"));
		getchar();
	}

	if (!getconfig_rabbitmq(configpath))
	{
		_debug_to(1, ("RABBITMQ config load failed,please check <config.txt> \n"));
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
		_debug_to(1, ("start_http_server failed\n"));
	}
	else {
		_debug_to(1, ("start_http_server success\n"));
	}

	//初始化Actor容器
	pthread_mutex_init(&mutex_actorinfo, NULL);
	pthread_mutex_init(&mutex_actortaskinfo, NULL);
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

	//开启Actor状态更新线程
	ret = pthread_create(&threadid_updateactorinfo, nullptr, pthread_updateactorinfo, nullptr);
	if (ret != 0)
	{
		_debug_to(1, ("thread_updateactorinfo create error\n"));
	}
	else
	{
		_debug_to(1, ("thread_updateactorinfo is runing\n"));
	}

	//开启合成任务分配线程
	ret = pthread_create(&threadid_runtask_thread, nullptr, pthread_runtask_thread, nullptr);
	if (ret != 0)
	{
		_debug_to(1, ("thread_assigntask create error\n"));
	}
	else
	{
		_debug_to(1, ("thread_assigntask is runing\n"));
	}

#if DF_OPEN_PATCH_GET
	//开启资产入库线程
	pthread_mutex_init(&mutex_taskinfo_patchdata, NULL);
	ret = pthread_create(&threadid_patchdata, nullptr, pthread_patchdata, nullptr);
	if (ret != 0)
	{
		_debug_to(1, ("thread_patchdata create error\n"));
	}
	else
	{
		_debug_to(1, ("thread_patchdata is runing\n"));
	}

	//开启资产获取线程
	pthread_mutex_init(&mutex_taskinfo_getdata, NULL);
	ret = pthread_create(&threadid_getdata, nullptr, pthread_getdata, nullptr);
	if (ret != 0)
	{
		_debug_to(1, ("thread_getdata create error\n"));
	}
	else
	{
		_debug_to(1, ("thread_getdata is runing\n"));
}
#endif

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







