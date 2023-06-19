#pragma once
#include <string>
#include <vector>
#include "mysql.h"


namespace digitalmysql
{
	bool getconfig_mysql(std::string configfilepath, std::string& error);

	//
	typedef struct _humaninfo
	{
		int id;
		int belongid;		//所属用户
		int privilege;		//权限,0=公有,1=私有

		std::string humanid;
		std::string humanname;
		std::string contentid;
		std::string sourcefolder;
		int			available;
		double		speakspeed;
		double		seriousspeed;
		std::string imagematting;
		std::string keyframe;
		std::string foreground;
		std::string background;
		std::string speakmodelpath;
		std::string pwgmodelpath;
		std::string mouthmodelfile;
		std::string facemodelfile;
		

		_humaninfo()
		{
			id = -1;
			belongid = 0;
			privilege = 0;

			humanid = "";
			humanname = "";
			contentid = "";
			sourcefolder = "";
			available = 0;
			speakspeed = 1.0;
			seriousspeed = 0.8;
			imagematting = "";
			keyframe = "";
			foreground = "";
			background = "";
			speakmodelpath = "";
			pwgmodelpath = "";
			mouthmodelfile = "";
			facemodelfile = "";
		}
	}humaninfo,*phumaninfo;
	bool addhumaninfo(humaninfo humanitem, bool update = false);
	bool gethumaninfo(std::string humanid, humaninfo& humanitem);
	bool isexisthuman_humanid(std::string humanid);
	bool isavailable_humanid(std::string humanid);
	bool deletehuman_humanid(std::string humanid, bool deletefile, std::string& errmsg);

	typedef std::vector<humaninfo> VEC_HUMANINFO;
	bool gethumanlistinfo(std::string humanid, VEC_HUMANINFO& vechumaninfo, int belongid = -1);

	//
	typedef struct _taskinfo
	{
		int			taskid;
		int			belongid;		//所属用户
		int			privilege;		//权限,0=公有,1=私有

		int			tasktype;		//0-onlyaudio,1-audio+video
		int			moodtype;		//0-nomal,1-sad
		double		speakspeed;
		std::string taskname;
		int			taskstate;		//0-merging 1-success 2-failed
		int			taskprogress;
		std::string createtime;
		std::string humanid;
		std::string humanname;
		std::string ssmltext;

		std::string audio_path;
		std::string audio_format;
		int			audio_length;
		std::string video_path;
		std::string video_keyframe;
		std::string video_format;
		int			video_length;
		int			video_width;
		int			video_height;
		double		video_fps;

		std::string foreground;
		std::string background;
		double		front_left;
		double		front_right;
		double		front_top;
		double		front_bottom;

		_taskinfo()
		{
			taskid = -1;
			belongid = 0;
			privilege = 0;

			tasktype = -1;
			moodtype = 0;
			speakspeed = 1.0;
			taskname = "";
			taskstate = -1;
			taskprogress = -1;
			createtime = "";
			humanid = "";
			humanname = "";
			ssmltext = "";

			audio_path = "";
			audio_format = "";
			audio_length = 0;
			video_path = "";
			video_keyframe = "";
			video_format = "";
			video_length = 0;
			video_width = 0;
			video_height = 0;
			video_fps = 0.0;

			foreground = "";
			background = "";
			front_left = 0;
			front_right = 0;
			front_top = 100;
			front_bottom = 0;
		}
	}taskinfo, * ptaskinfo;
	typedef std::vector<taskinfo> VEC_TASKINFO;
	bool addtaskinfo(int& taskid, taskinfo taskitem, bool update = false);
	bool gettaskinfo(int taskid, taskinfo& taskitem);
	bool setaudiopath(int taskid, std::string audiopath);
	bool setvideopath(int taskid, std::string videopath);
	bool setkeyframepath(int taskid, std::string keyframepath);
	bool deletetask_taskid(int taskid, bool deletefile, std::string& errmsg);

	typedef struct _filterinfo
	{
		std::string filterfield;
		std::string filtervalue;

		_filterinfo()
		{
			filterfield = "";
			filtervalue = "";
		}
	}filterinfo, * pfilterinfo;
	typedef std::vector<filterinfo> VEC_FILTERINFO;
	bool gettaskhistoryinfo(VEC_FILTERINFO& vecfilterinfo, std::string order_key, int order_way, int pagesize, int pagenum, int& tasktotal, VEC_TASKINFO& vectaskhistory, int belongid = -1);

	bool isexisttask_taskid(int taskid);
	bool isexisttask_textguid(int& taskid, std::string taskname, std::string textguid);

	//
	enum sourceType
	{
		source_image = 0,
		source_video,
		source_audio,
	};
	typedef struct _tasksourceinfo 
	{
		int			id;
		int			belongid;		//所属用户
		int			privilege;		//权限,0=公有,1=私有

		int			sourcetype;		//0-image,1-video,2-audio
		std::string sourcepath;
		std::string sourcekeyframe;
		std::string createtime;

		_tasksourceinfo()
		{
			id = 0;
			belongid = 0;
			privilege = 0;

			sourcetype = -1;
			sourcepath = "";
			sourcekeyframe = "";
			createtime = "";
		}

	}tasksourceinfo,*ptasksourceinfo;
	typedef std::vector<tasksourceinfo> VEC_TASKSOURCEINFO;
	bool addtasksource(tasksourceinfo tasksourceitem, bool update = false);
	bool gettasksourcelist(VEC_TASKSOURCEINFO& vectasksource, int belongid = -1);

	bool isexisttasksource_path(std::string sourcepath);

	//
	int  getmergeprogress(int taskid);
	bool setmergeprogress(int taskid, int nprogress);
	//
	int  getmergestate(int taskid);
	bool setmergestate(int taskid, int nstate);
};

