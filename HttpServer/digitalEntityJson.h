#pragma once
#include <string>
#include <vector>
#include "json.h"

//============object to json============
//A
class FileItem
{
public:
	FileItem() {};
	virtual~FileItem() {};

	virtual std::string writeJson();
public:
	std::string fileGUID;
	std::string filePath;
	std::string fileState;
};
class FileItems
{
public:
	FileItems() {};
	virtual~FileItems() {};

	virtual std::string writeJson();
public:
	std::vector<FileItem> vecFileItems;
};

//B
class FileGroup
{
public:
	FileGroup() {};
	virtual~FileGroup() {};

	virtual std::string writeJson();
public:
	FileItems   fileItems;
	std::string groupName;
	std::string groupType;
	std::string status;
};
class FileGroups
{
public:
	FileGroups() {};
	virtual~FileGroups() {};

	virtual std::string writeJson();
public:
	std::vector<FileGroup> vecFileGroups;
};

//C
class EntityData
{
public:
	EntityData() {};
	virtual~EntityData() {};

	virtual std::string writeJson();
public:
	std::string type;
	std::string physicalfolder;
	std::string tree_;
};

//A+B+C
class DigitalEntity
{
public:
	DigitalEntity() {};
	virtual~DigitalEntity() {};

	virtual std::string writeJson();
public:
	std::string contentId;
	std::string createUser;
	EntityData  entityData;
	FileGroups  fileGroups;
	std::string name;
	std::string privilege;
	std::string site_;
	std::string tree;
	std::string type;
};

//============json to object============

DigitalEntity json2object(std::string sJson);

//===========================================================================================================================================

//获取数字人列表接口支持
//http
class DigitalSearch_Body
{
public:
	DigitalSearch_Body() {};
	virtual~DigitalSearch_Body() {};

	virtual std::string writeJson();

public:
	std::vector<std::string> vecRestrictionFields;
	std::string restrictionFields;

	std::string conditionField;
	std::string conditionValue;
	std::string resourceName;
};

//HumanList
//===========================================================================================================================================
class DigitalMan_Item
{
public:
	DigitalMan_Item() 
	{
		HumanID = "";
		HumanName="";
		SpeakSpeed = 1.0;
		KeyFrame_Format="";
		KeyFrame_Width=0;
		KeyFrame_Height=0;
		KeyFrame_BitCount=0;
		KeyFrame_FilePath="";
		KeyFrame_KeyData="";
	};
	virtual~DigitalMan_Item() {};

	virtual std::string writeJson();

public:
	std::string HumanID;
	std::string HumanName;
	double		SpeakSpeed;
	std::string Foreground;
	std::string Background;
	std::string KeyFrame_Format;
	int			KeyFrame_Width;
	int			KeyFrame_Height;
	int			KeyFrame_BitCount;
	std::string KeyFrame_FilePath;
	std::string KeyFrame_KeyData;

	std::vector<std::string> vecDigitManTaskObj;//用于区分数字人时
};
class DigitalMan_Items
{
public:
	DigitalMan_Items() {};
	virtual~DigitalMan_Items() {};

	virtual std::string writeJson();
public:
	std::vector<DigitalMan_Item> vecDigitManItems;
};

//HumanHistory
//===========================================================================================================================================
class DigitalMan_Task
{
public:
	DigitalMan_Task() 
	{
		TaskID=0;
		TaskName="";
		TaskState=0;
		TaskProgerss=0;
		TaskInputSsml="";
		TaskCreateTime="";
		TaskHumanID = "";
		TaskHumanName="";
		TaskSpeakSpeed = 1.0;

		KeyFrame_Format="";
		KeyFrame_Width=0;
		KeyFrame_Height=0;
		KeyFrame_BitCount=0;
		KeyFrame_FilePath="";
		KeyFrame_KeyData="";

		Audio_Format="";
		Audio_File="";

		Video_Format="";
		Video_Width=0;
		Video_Height=0;
		Video_Fps=0.0;
		Video_File="";
	};
	virtual~DigitalMan_Task() {};

	virtual std::string writeJson();

public:
	int			TaskID;
	int			TaskType;
	int			TaskMoodType;
	std::string TaskName;
	int			TaskState;
	int			TaskProgerss;
	double		TaskSpeakSpeed;
	std::string TaskInputSsml;
	std::string TaskCreateTime;
	std::string TaskHumanID;
	std::string TaskHumanName;

	std::string Foreground;
	std::string Background;
	int			Front_XPos;
	int			Front_YPos;
	int			Front_Scale;
	int			Front_Rotation;

	std::string KeyFrame_Format;
	int			KeyFrame_Width;
	int			KeyFrame_Height;
	int			KeyFrame_BitCount;
	std::string KeyFrame_FilePath;
	std::string KeyFrame_KeyData;

	std::string Audio_Format;
	std::string Audio_File;

	std::string Video_Format;
	int			Video_Width;
	int			Video_Height;
	double		Video_Fps;
	std::string Video_File;
};
class DigitalMan_Tasks
{
public:
	DigitalMan_Tasks() {};
	virtual~DigitalMan_Tasks() {};

	virtual std::string writeJson();

public:
	std::vector<DigitalMan_Task> vecDigitManTasks;
};

//TaskSourceList
//===========================================================================================================================================
class DigitalMan_TaskSource
{
public:
	DigitalMan_TaskSource()
	{

	};
	~DigitalMan_TaskSource() {};

	virtual std::string writeJson();

public:
	int				TaskSource_Type;
	std::string		TaskSource_FilePath;
	std::string		TaskSource_KeyFrame;
};
class DigitalMan_TaskSources
{
public:
	DigitalMan_TaskSources(){};
	~DigitalMan_TaskSources() {};

	virtual std::string writeJson();

public:
	std::vector<DigitalMan_TaskSource> vecDigitManTaskSources;
};


