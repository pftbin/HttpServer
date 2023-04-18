#pragma once
#include <string>
#include <vector>
#include "mysql.h"


namespace digitalmysql
{
	bool getmysqlconfig(std::string configfilepath);

	//
	typedef struct _humaninfo
	{
		int id;
		std::string humanname;
		std::string humanid;
		std::string contentid;
		std::string mouthmodelfile;
		std::string facemodelpath;
		std::string keyframe;
		std::string videofile;
		std::string speakmodelpath;
		std::string pwgmodelpath;

		_humaninfo()
		{
			id = -1;
			humanname = "";
			humanid = "";
			contentid = "";
			mouthmodelfile = "";
			facemodelpath = "";
			keyframe = "";
			videofile = "";
			speakmodelpath = "";
			pwgmodelpath = "";
		}
	}humaninfo,*phumaninfo;
	typedef std::vector<humaninfo> VEC_HUMANINFO;
	int gethumanlistinfo(std::string humanid, VEC_HUMANINFO& vechumaninfo);

	typedef struct _taskinfo
	{
		//不存入数据库的变量
		double		speed;//语速，与textguid有关

		int			taskid;
		int			tasktype;//0-onlyaudio,1-audio+video
		std::string taskname;
		int			taskstate;
		int			taskprogress;
		std::string createtime;
		std::string humanid;
		std::string ssmltext;

		std::string audio_path;
		std::string audio_format;
		int			audio_length;
		std::string video_path;
		std::string video_finalpath;
		std::string video_format;
		int			video_length;
		int			video_width;
		int			video_height;
		double		video_fps;

		_taskinfo()
		{
			speed = 1.0;

			taskid = -1;
			tasktype = -1;
			taskname = "";
			taskstate = -1;
			taskprogress = -1;
			createtime = "";
			humanid = "";
			ssmltext = "";

			audio_path = "";
			audio_format = "";
			audio_length = 0;
			video_path = "";
			video_finalpath;
			video_format = "";
			video_length = 0;
			video_width = 0;
			video_height = 0;
			video_fps = 0.0;
		}
	}taskinfo, * ptaskinfo;
	typedef std::vector<taskinfo> VEC_TASKINFO;
	
	bool addtaskinfo(int& taskid, taskinfo taskitem, bool update=false);
	bool gettaskinfo(int taskid, taskinfo& taskitem);
	int  gettaskhistoryinfo(std::string humanid, std::string orderkey, int aspect, VEC_TASKINFO& vectaskhistory);
	bool setfinalvideopath(int taskid, std::string vdofinalpath);

	bool isexisttask_taskid(int taskid);
	bool isexisttask_textguid(int& taskid, std::string taskname, std::string textguid);

	bool deletetask_taskid(int taskid, std::string& errmsg);

	//
	int  getmergeprogress(int taskid);
	bool setmergeprogress(int taskid, int nprogress);

	//
	int  getmergestate(int taskid);
	bool setmergestate(int taskid, int nstate);
};

