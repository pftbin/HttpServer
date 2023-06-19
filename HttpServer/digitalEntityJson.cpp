#include "digitalEntityJson.h"

#define BUFF_SZ			1024*16		//system max stack size

#define DF_IMAGE_USEKEYDATA		0

//===========================================================================================================================================
std::string DigitalSearch_Body::writeJson()
{
	std::string sResultJson = "";
	char buff[BUFF_SZ] = { 0 };
	std::string skeyvalue = "";

	//1
	restrictionFields = "";
	for (size_t i = 0; i < vecRestrictionFields.size(); ++i)
	{
		std::string field;
		char tempbuff[256] = { 0 };
		if (i != (vecRestrictionFields.size() - 1))
			snprintf(tempbuff, 256, "\"%s\" ,", vecRestrictionFields[i].c_str()); 
		else
			snprintf(tempbuff, 256, "\"%s\"  ", vecRestrictionFields[i].c_str()); 

		field = tempbuff;
		restrictionFields += field;
	}
	snprintf(buff, BUFF_SZ, "\"extensionCondition\":{\"restrictionFields\":[ %s ]}, ", restrictionFields.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;

	//2
	snprintf(buff, BUFF_SZ, "\"queryCondition\":{\"fieldConditionGroup\":{\"fieldConditions\":[{\"field\":\"%s\",\"searchRelation\":\"AND\",\"value\":\"%s\"}] ,\"searchRelation\":\"AND\"},\"page\":1,\"size\":200}, ", conditionField.c_str(), conditionValue.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;

	//3
	snprintf(buff, BUFF_SZ, "\"resourceName\":\"%s\" ", resourceName.c_str()); skeyvalue = buff;//last no ","
	sResultJson += skeyvalue;

	return sResultJson;
}

//===========================================================================================================================================
std::string DigitalMan_Item::writeJson()
{
	std::string sResultJson = "";
	char buff[BUFF_SZ] = { 0 };
	std::string skeyvalue = "";

	//
	snprintf(buff, BUFF_SZ, "\"HumanID\":\"%s\" ,", HumanID.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	//
	snprintf(buff, BUFF_SZ, "\"HumanName\":\"%s\" ,", HumanName.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	//
	snprintf(buff, BUFF_SZ, "\"SpeakSpeed\":%.6f ,", SpeakSpeed); skeyvalue = buff;
	sResultJson += skeyvalue;
	//
	snprintf(buff, BUFF_SZ, "\"Foreground\":\"%s\" ,", Foreground.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	//
	snprintf(buff, BUFF_SZ, "\"Background\":\"%s\" ,", Background.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	//
	snprintf(buff, BUFF_SZ, "\"KeyFrame\":{\"Format\":\"%s\",\"Width\": %d,\"Height\": %d,\"BitCount\": %d, ", KeyFrame_Format.c_str(), KeyFrame_Width, KeyFrame_Height, KeyFrame_BitCount); skeyvalue = buff;
	sResultJson += skeyvalue;

#if DF_IMAGE_USEKEYDATA
	std::string keydata = "\"KeyData\":\"";
	keydata += KeyFrame_KeyData;		//KeyData too long,use string
	keydata += "\" },";	
	sResultJson += keydata;
#else
	std::string filepath = "\"FilePath\":\"" + KeyFrame_FilePath + "\"},";
	sResultJson += filepath;
#endif

	//5,select database and add to this class
	sResultJson += "\"HumanData\": [";
	for (size_t i = 0; i < vecDigitManTaskObj.size(); i++)
	{
		sResultJson += vecDigitManTaskObj[i];

		if (i != vecDigitManTaskObj.size()-1)
			sResultJson += ",";	//last no ","
	}
	sResultJson += "]";

	return sResultJson;
}

std::string DigitalMan_Items::writeJson()
{
	std::string sResultJson = "";
	char buff[BUFF_SZ] = { 0 };
	std::string skeyvalue = "";

	//0
	for (size_t i = 0; i < vecDigitManItems.size(); ++i)
	{
		std::string vecObjTemp = vecDigitManItems[i].writeJson();
		std::string vecObjJson = "{" + vecObjTemp + "}";

		sResultJson += vecObjJson;
		if (i != vecDigitManItems.size() - 1)//last no ","
			sResultJson += ",";
	}

	return sResultJson;
}

//===========================================================================================================================================

std::string DigitalMan_Task::writeJson()
{
	std::string sResultJson = "";
	char buff[BUFF_SZ] = { 0 };
	std::string skeyvalue = "";

	snprintf(buff, BUFF_SZ, "\"TaskID\":%d,", TaskID); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"TaskType\":%d,", TaskType); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"TaskMoodType\":%d,", TaskMoodType); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"TaskName\":\"%s\",", TaskName.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"State\":%d,", TaskState); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"Progress\":%d,", TaskProgerss); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"SpeakSpeed\":%.6f,", TaskSpeakSpeed); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"InputSsml\":\"%s\",", TaskInputSsml.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"Createtime\":\"%s\",", TaskCreateTime.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"HumanID\":\"%s\",", TaskHumanID.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"HumanName\":\"%s\",", TaskHumanName.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;

	snprintf(buff, BUFF_SZ, "\"Foreground\":\"%s\",", Foreground.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"Background\":\"%s\",", Background.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"left\":%.6f,", Front_left); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"right\":%.6f,", Front_right); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"top\":%.6f,", Front_top); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"bottom\":%.6f,", Front_bottom); skeyvalue = buff;
	sResultJson += skeyvalue;

	snprintf(buff, BUFF_SZ, "\"KeyFrame\":{\"Format\":\"%s\",\"Width\": %d,\"Height\": %d,\"BitCount\": %d, ", KeyFrame_Format.c_str(), KeyFrame_Width, KeyFrame_Height, KeyFrame_BitCount); skeyvalue = buff;
	sResultJson += skeyvalue;

#if DF_IMAGE_USEKEYDATA
	std::string keydata = "\"KeyData\":\"";
	keydata += KeyFrame_KeyData;		//KeyData too long,use string
	keydata += "\" },";
	sResultJson += keydata;
#else
	std::string filepath = "\"FilePath\":\"" + KeyFrame_FilePath + "\"},";
	sResultJson += filepath;
#endif

	snprintf(buff, BUFF_SZ, "\"Audio\":{\"AudioFormat\":\"%s\",\"AudioFile\":\"%s\"},", Audio_Format.c_str(), Audio_File.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;

	snprintf(buff, BUFF_SZ, "\"Vedio\":{\"VideoFormat\":\"%s\",\"Width\":%d,\"Height\":%d,\"Fps\":%.2f,\"VedioFile\":\"%s\"}", Video_Format.c_str(), Video_Width, Video_Height, Video_Fps, Video_File.c_str()); skeyvalue = buff;//last no ","
	sResultJson += skeyvalue;

	return sResultJson;
}

std::string DigitalMan_Tasks::writeJson()
{
	std::string sResultJson = "";
	char buff[BUFF_SZ] = { 0 };
	std::string skeyvalue = "";

	//1
	for (size_t i = 0; i < vecDigitManTasks.size(); ++i)
	{
		std::string vecObjTemp = vecDigitManTasks[i].writeJson();
		std::string vecObjJson = "{" + vecObjTemp + "}";

		sResultJson += vecObjJson;
		if (i != vecDigitManTasks.size() - 1)//last no ","
			sResultJson += ",";
	}

	return sResultJson;
}

//===========================================================================================================================================
std::string DigitalMan_TaskSource::writeJson()
{
	std::string sResultJson = "";
	char buff[BUFF_SZ] = { 0 };
	std::string skeyvalue = "";

	if (TaskSource_Type == 0)//image
	{
		snprintf(buff, BUFF_SZ, "\"ImageFile\":\"%s\"", TaskSource_FilePath.c_str()); skeyvalue = buff;//last no ","
		sResultJson += skeyvalue;
	}

	if (TaskSource_Type == 1)//video
	{
		snprintf(buff, BUFF_SZ, "\"VideoFile\":\"%s\",", TaskSource_FilePath.c_str()); skeyvalue = buff;
		sResultJson += skeyvalue;
		snprintf(buff, BUFF_SZ, "\"KeyFrameFile\":\"%s\"", TaskSource_KeyFrame.c_str()); skeyvalue = buff;//last no ","
		sResultJson += skeyvalue;
	}

	if (TaskSource_Type == 2)//audio
	{
		snprintf(buff, BUFF_SZ, "\"AudioFile\":\"%s\"", TaskSource_FilePath.c_str()); skeyvalue = buff;//last no ","
		sResultJson += skeyvalue;
	}

	return sResultJson;
}

std::string DigitalMan_TaskSources::writeJson()
{
	std::string sResultJson = "";
	char buff[BUFF_SZ] = { 0 };
	std::string skeyvalue = "";

	//1
	for (size_t i = 0; i < vecDigitManTaskSources.size(); ++i)
	{
		std::string vecObjTemp = vecDigitManTaskSources[i].writeJson();
		std::string vecObjJson = "{" + vecObjTemp + "}";

		sResultJson += vecObjJson;
		if (i != vecDigitManTaskSources.size() - 1)//last no ","
			sResultJson += ",";
	}

	return sResultJson;
}


