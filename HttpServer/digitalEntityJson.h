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
	DigitalMan_Item() {};
	virtual~DigitalMan_Item() {};

	virtual std::string writeJson();

public:
	std::string HumanName;
	std::string VirtualmanKey;
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
	DigitalMan_Task() {};
	virtual~DigitalMan_Task() {};

	virtual std::string writeJson();

public:
	int			TaskID;
	std::string TaskName;
	int			TaskState;
	int			TaskProgerss;
	std::string TaskInputSsml;
	std::string TaskCreateTime;
	std::string TaskHumanName;
	std::string TaskVirtualmanKey;

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


