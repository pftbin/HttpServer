#include "digitalEntityJson.h"

#define BUFF_SZ			1024*16		//system max stack size

#define DF_IMAGE_USEKEYDATA		0


//============object to json============
//A
std::string FileItem::writeJson()
{
	std::string sResultJson = "";
	char buff[BUFF_SZ] = {0};
	std::string skeyvalue = "";

	//1
	snprintf(buff, BUFF_SZ, "\"fileGUID\":\"%s\", ", fileGUID.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	//2
	snprintf(buff, BUFF_SZ, "\"filePath\":\"%s\", ", filePath.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	//3
	snprintf(buff, BUFF_SZ, "\"fileState\":\"%s\" ", fileState.c_str()); skeyvalue = buff;//last no ","
	sResultJson += skeyvalue;

	return sResultJson;
}
std::string FileItems::writeJson()
{
	std::string sResultJson = "";
	char buff[BUFF_SZ] = { 0 };
	std::string skeyvalue = "";

	//0
	for (size_t i = 0; i < vecFileItems.size(); ++i)
	{
		std::string vecObjTemp = vecFileItems[i].writeJson();
		std::string vecObjJson = "{" + vecObjTemp + "}";

		sResultJson += vecObjJson;
		if (i != vecFileItems.size() - 1)//last no ","
			sResultJson += ",";
	}

	return sResultJson;
}
//B
std::string FileGroup::writeJson()
{
	std::string sResultJson = "";
	char buff[BUFF_SZ] = { 0 };
	std::string skeyvalue = "";

	//1
	snprintf(buff, BUFF_SZ, "\"fileItems\":[%s],", fileItems.writeJson().c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	//2
	snprintf(buff, BUFF_SZ, "\"groupName\":\"%s\", ", groupName.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	//3
	snprintf(buff, BUFF_SZ, "\"groupType\":\"%s\", ", groupType.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	//4
	snprintf(buff, BUFF_SZ, "\"status\":\"%s\" ", status.c_str()); skeyvalue = buff;//last no ","
	sResultJson += skeyvalue;

	return sResultJson;
}
std::string FileGroups::writeJson()
{
	std::string sResultJson = "";
	char buff[BUFF_SZ] = { 0 };
	std::string skeyvalue = "";

	//0
	for (size_t i = 0; i < vecFileGroups.size(); ++i)
	{
		std::string vecObjTemp = vecFileGroups[i].writeJson();
		std::string vecObjJson = "{" + vecObjTemp + "}";

		sResultJson += vecObjJson;
		if (i != vecFileGroups.size() - 1)//last no ","
			sResultJson += ",";
	}

	return sResultJson;
}
//C
std::string EntityData::writeJson()
{
	std::string sResultJson = "";
	char buff[BUFF_SZ] = { 0 };
	std::string skeyvalue = "";

	//1
	snprintf(buff, BUFF_SZ, "\"type\":\"%s\", ", type.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	//2
	snprintf(buff, BUFF_SZ, "\"digital\":{\"physicalfolder\":\"%s\"}, ", physicalfolder.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	//3
	snprintf(buff, BUFF_SZ, "\"tree_\":[\"%s\"] ", tree_.c_str()); skeyvalue = buff;//last no ","
	sResultJson += skeyvalue;


	return sResultJson;
}
//A+B+C
std::string DigitalEntity::writeJson()
{
	std::string sResultJson = "";
	char buff[BUFF_SZ] = { 0 };
	std::string skeyvalue = "";

	//1
	snprintf(buff, BUFF_SZ, "\"contentId\":\"%s\", ", contentId.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	//2
	snprintf(buff, BUFF_SZ, "\"createUser\":\"%s\", ", createUser.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	//3
	snprintf(buff, BUFF_SZ, "\"entityData\":{%s}, ", entityData.writeJson().c_str()); skeyvalue = buff;	//entityData为object对象
	sResultJson += skeyvalue;
	//4
	snprintf(buff, BUFF_SZ, "\"fileGroups\":[%s], ", fileGroups.writeJson().c_str()); skeyvalue = buff;//fileGroups为object对象数组
	sResultJson += skeyvalue;
	//5
	snprintf(buff, BUFF_SZ, "\"name\":\"%s\", ", name.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	//6
	snprintf(buff, BUFF_SZ, "\"privilege\":\"%s\", ", privilege.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	//7
	snprintf(buff, BUFF_SZ, "\"site_\":\"%s\", ", site_.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	//8
	snprintf(buff, BUFF_SZ, "\"tree\":[\"%s\"], ", tree.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	//9
	snprintf(buff, BUFF_SZ, "\"type\":\"%s\" ", type.c_str()); skeyvalue = buff;//last no ","
	sResultJson += skeyvalue;

	return sResultJson;
}

//============json to object============
std::string my_replace(std::string str, std::string old, std::string now)
{
	int oldPos = 0;
	while (str.find(old, oldPos) != -1)
	{
		int start = str.find(old, oldPos);
		str.replace(start, old.size(), now);
		oldPos = start + now.size();
	}
	return str;
}
DigitalEntity json2object(std::string sJson)
{
	//
	sJson = my_replace(sJson, "\\", "\\\\");// \ => \\ , c++ need

	DigitalEntity objResultObject;
	json::Value json_val = json::Deserialize((char*)sJson.c_str());
	if (json_val.GetType() != json::ObjectVal)
		return objResultObject;
	json::Object json_obj = json_val.ToObject();

	int nValue = 0;
	std::string strValue_ansi = "";
	//
	if (json_obj.HasKey("contentId"))
	{
		strValue_ansi = json_obj["contentId"].ToString();
		objResultObject.contentId = strValue_ansi;
	}
	//
	if (json_obj.HasKey("createUser"))
	{
		strValue_ansi = json_obj["createUser"].ToString();
		objResultObject.createUser = strValue_ansi;
	}
	//
	if (json_obj.HasKey("entityData"))
	{
		json::Value entityData_val = json_obj["entityData"];
		if (entityData_val.GetType() == json::ObjectVal)
		{
			json::Object entityData_obj = entityData_val.ToObject();
			if (entityData_obj.HasKey("type"))
			{
				strValue_ansi = entityData_obj["type"].ToString();
				objResultObject.entityData.type = strValue_ansi;
			}
			if (entityData_obj.HasKey("digital"))
			{
				json::Value digital_val = entityData_obj["digital"];
				if (digital_val.GetType() == json::ObjectVal)
				{
					json::Object digital_obj = digital_val.ToObject();
					if (digital_obj.HasKey("physicalfolder"))
					{ 
						strValue_ansi = digital_obj["physicalfolder"].ToString();
						objResultObject.entityData.physicalfolder = strValue_ansi;
					}
				}
			}
			if (entityData_obj.HasKey("tree_"))//have []
			{
				json::Value tree_val = entityData_obj["tree_"];
				if (tree_val.GetType() == json::ArrayVal)
				{
					json::Array tree_array = tree_val.ToArray();
					strValue_ansi = tree_array[0].ToString();//only,not use loop 
					objResultObject.entityData.tree_ = strValue_ansi;
				}	
			}
		}
	}

	//
	if (json_obj.HasKey("fileGroups"))//have []
	{
		json::Value fileGroups_val = json_obj["fileGroups"];
		if (fileGroups_val.GetType() == json::ArrayVal)
		{
			json::Array fileGroups_array = fileGroups_val.ToArray();
			for (size_t i = 0; i < fileGroups_array.size(); ++i)
			{
				FileGroup inputFileGroup;
				if (fileGroups_array[i].GetType() != json::ObjectVal) break;
				json::Object fileGroupObj = fileGroups_array[i].ToObject();
				//
				if (fileGroupObj.HasKey("fileItems"))//have []
				{
					json::Value fileItems_val = fileGroupObj["fileItems"];
					if (fileItems_val.GetType() == json::ArrayVal)
					{
						json::Array fileItems_array = fileItems_val.ToArray();
						for (size_t j = 0; j < fileItems_array.size(); ++j)
						{
							FileItem inputFileItem;
							if (fileItems_array[j].GetType() != json::ObjectVal) break;
							json::Object fileItemObj = fileItems_array[j].ToObject();
							//
							if (fileItemObj.HasKey("fileGUID"))
							{
								strValue_ansi = fileItemObj["fileGUID"].ToString();
								inputFileItem.fileGUID = strValue_ansi;
							}
							//
							if (fileItemObj.HasKey("filePath"))
							{
								strValue_ansi = fileItemObj["filePath"].ToString();
								inputFileItem.filePath = strValue_ansi;
							}
							//
							if (fileItemObj.HasKey("fileState"))
							{
								strValue_ansi = fileItemObj["fileState"].ToString();
								inputFileItem.fileState = strValue_ansi;
							}
							//input
							inputFileGroup.fileItems.vecFileItems.push_back(inputFileItem);
						}
					}
				}
				//
				strValue_ansi = fileGroupObj["groupName"].ToString();
				inputFileGroup.groupName = strValue_ansi;
				//
				strValue_ansi = fileGroupObj["groupType"].ToString();
				inputFileGroup.groupType = strValue_ansi;
				//
				strValue_ansi = fileGroupObj["status"].ToString();
				inputFileGroup.status = strValue_ansi;
				//input
				objResultObject.fileGroups.vecFileGroups.push_back(inputFileGroup);
			}
		}

	}
	//
	if (json_obj.HasKey("name"))
	{
		strValue_ansi = json_obj["name"].ToString();
		objResultObject.name = strValue_ansi;
	}
	//
	if (json_obj.HasKey("privilege"))
	{
		strValue_ansi = json_obj["privilege"].ToString();
		objResultObject.privilege = strValue_ansi;
	}
	//
	if (json_obj.HasKey("site_"))
	{
		strValue_ansi = json_obj["site_"].ToString();
		objResultObject.site_ = strValue_ansi;
	}
	//
	if (json_obj.HasKey("tree"))//have []
	{
		json::Value tree_val = json_obj["tree"];
		if (tree_val.GetType() == json::ArrayVal)
		{
			json::Array tree_array = tree_val.ToArray();
			strValue_ansi = tree_array[0].ToString();//only,not use loop 
			objResultObject.tree = strValue_ansi;
		}
	}
	//
	if (json_obj.HasKey("type"))
	{
		strValue_ansi = json_obj["type"].ToString();
		objResultObject.type = strValue_ansi;
	}

	std::string strTest = objResultObject.writeJson();
	return objResultObject;
}


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
	snprintf(buff, BUFF_SZ, "\"SpeakSpeed\":%.1f ,", SpeakSpeed); skeyvalue = buff;
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
	snprintf(buff, BUFF_SZ, "\"SpeakSpeed\":%.1f,", TaskSpeakSpeed); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"InputSsml\":\"%s\",", TaskInputSsml.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"Createtime\":\"%s\",", TaskCreateTime.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"HumanID\":\"%s\",", TaskHumanID.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"HumanName\":\"%s\",", TaskHumanName.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;

	snprintf(buff, BUFF_SZ, "\"Background\":\"%s\",", BackgroundFile.c_str()); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"Front_XPos\":%d,", Front_XPos); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"Front_YPos\":%d,", Front_YPos); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"Front_Scale\":%d,", Front_Scale); skeyvalue = buff;
	sResultJson += skeyvalue;
	snprintf(buff, BUFF_SZ, "\"Front_Rotation\":%d,", Front_Rotation); skeyvalue = buff;
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

	snprintf(buff, BUFF_SZ, "\"Vedio\":{\"VideoFormat\":\"%s\",\"Width\":%d,\"Height\":%d,\"Fps\":%.1f,\"VedioFile\":\"%s\"}", Video_Format.c_str(), Video_Width, Video_Height, Video_Fps, Video_File.c_str()); skeyvalue = buff;//last no ","
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


