#include "digitalmysql.h"
#include "public.h"

#pragma comment(lib,"libmysql.lib")

static bool simulation = false;
std::string g_database_ip = "10.245.90.122";
short	    g_database_port = 3306;
std::string g_database_name = "sbnetdba";
std::string g_database_password = "Sobey123";
std::string g_database_db = "netdb";

#define BUFF_SZ 1024*16  //system max stack size


namespace digitalmysql
{
	std::string row_value(char* pChar)
	{
		std::string result_str = "";
		if (pChar != nullptr)
			result_str = pChar;
		return result_str;
	}

	//
	bool getmysqlconfig(std::string configfilepath)
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

			value = getnodevalue(config, "mysql_ip");
			if (value.empty()) return false;
			g_database_ip = value;

			value = getnodevalue(config, "mysql_port");
			if (value.empty()) return false;
			g_database_port = atoi(value.c_str());

			value = getnodevalue(config, "mysql_username");
			if (value.empty()) return false;
			g_database_name = value;

			value = getnodevalue(config, "mysql_password");
			if (value.empty()) return false;
			g_database_password = value;

			value = getnodevalue(config, "mysql_dbname");
			if (value.empty()) return false;
			g_database_db = value;

			return true;
		}

		return false;
	}

	//
	std::string transsequencename(std::string strname)
	{
		std::string strvalue = "";
		if (strname.compare("sbt_doctask") == 0)
		{
			strvalue = "sbt_sqt_doctask";
		}
		else if (strname.compare("sbt_humansource") == 0)
		{
			strvalue = "sbt_sqt_humansource";
		}

		return strvalue;
	}
	int newgetsequencenextvalue(std::string strsequencename, MYSQL* pmysql)
	{
		strsequencename = transsequencename(strsequencename);
		if (strsequencename.empty()) return 0;

		bool ret = true;int sequence = 0;

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "select id from %s order by id desc", strsequencename.c_str());
		if (!mysql_query(pmysql, sql_buff))	//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			MYSQL_ROW row;							//table row data
			result = mysql_store_result(pmysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			if (rownum >= 1 && colnum >= 1)//keep right
			{
				row = mysql_fetch_row(result);
				sequence = atoi(row[0]);
			}
			else
			{
				sequence = 0;
			}
			sequence++;

			//
			snprintf(sql_buff, BUFF_SZ, "insert into %s (id) value(%d)", strsequencename.c_str(), sequence);// insert
			if (!mysql_query(pmysql, sql_buff))	//success return 0,failed return random number
			{
				printf("[newgetsequencenextvalue], insert sqt success, sequence=%d\n", sequence);
			}
			else
			{
				ret = false;
				std::string error = mysql_error(pmysql);
				printf("[newgetsequencenextvalue], insert sqt failed: %s\n", error.c_str());
			}
			mysql_free_result(result);				//free result
		}
		else
		{
			ret = false;
			std::string error = mysql_error(pmysql);
			printf("[newgetsequencenextvalue], select failed: %s\n", error.c_str());
		}

		if (!ret) return 0;
		return sequence;
	}

	int gethumanlistinfo(std::string humanid, VEC_HUMANINFO& vechumaninfo)
	{
		if (simulation) return 0;

		bool ret = true; int ncount = 0;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_name.c_str(), g_database_password.c_str(), g_database_db.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[gethumanlistinfo]MySQL database connect failed: %s\n", error.c_str());
		}

		char sql_buff[BUFF_SZ] = { 0 };

		if(humanid.empty())
			snprintf(sql_buff, BUFF_SZ, "select id,name,humanid,contentid,mouthmodefile,facemodefile,keyframe,videofile,speakpath,pwgpath from sbt_humansource");//select	
		else
			snprintf(sql_buff, BUFF_SZ, "select id,name,humanid,contentid,mouthmodefile,facemodefile,keyframe,videofile,speakpath,pwgpath from sbt_humansource where humanid='%s'", humanid.c_str());//select	

		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			MYSQL_ROW row;							//table row data
			result = mysql_store_result(&mysql);    //sava dada to result

			while (row = mysql_fetch_row(result))
			{
				int rownum = mysql_num_rows(result);	//get row number
				int colnum = mysql_num_fields(result);  //get col number
				if (colnum >= 10) //keep right
				{
					int i = 0;
					int row_id = atoi(row_value(row[i++]).c_str());
					std::string row_humanname = row_value(row[i++]); std::string humanname_ansi; utf8_to_ansi(row_humanname.c_str(), row_humanname.length(), humanname_ansi);
					std::string row_humanid = row_value(row[i++]);
					std::string row_contentid = row_value(row[i++]);
					std::string row_mouthmodelfile = row_value(row[i++]);
					std::string row_facemodelpath = row_value(row[i++]);
					std::string row_keyframe = row_value(row[i++]);
					std::string row_videofile = row_value(row[i++]);
					std::string row_speakmodelpath = row_value(row[i++]);
					std::string row_pwgmodelpath = row_value(row[i++]);

					humaninfo humanitem;
					humanitem.id = row_id;
					humanitem.humanname = humanname_ansi;
					humanitem.humanid = row_humanid;
					humanitem.contentid = row_contentid;
					humanitem.mouthmodelfile = row_mouthmodelfile;
					humanitem.facemodelpath = row_facemodelpath;
					humanitem.keyframe = row_keyframe;
					humanitem.videofile = row_videofile;
					humanitem.speakmodelpath = row_speakmodelpath;
					humanitem.pwgmodelpath = row_pwgmodelpath;
					vechumaninfo.push_back(humanitem);

					ncount++;
				}
			}	
			mysql_free_result(result);				//free result

			printf("[gethumanlistinfo]MySQL select success,ncount=%d\n",ncount);
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[gethumanlistinfo]MySQL select failed: %s\n", error.c_str());
		}
		//=====================
		mysql_close(&mysql);	//close connect
		if (!ret) return 0;

		return ncount;
	}

	//
	bool addtaskinfo(int& taskid, taskinfo taskitem, bool update)
	{
		if (simulation) return true;
		if (taskid == 0) update = false;

		bool ret = true;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_name.c_str(), g_database_password.c_str(), g_database_db.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[addmergetask]MySQL database connect failed: %s\n", error.c_str());
		}

		std::string taskname_utf8 = "test taskname";
		ansi_to_utf8(taskitem.taskname.c_str(), taskitem.taskname.length(), taskname_utf8);
		taskitem.taskname = taskname_utf8;

		char tempbuff[16] = { 0 };
		std::string speed = ""; snprintf(tempbuff, 16, "%.1f", taskitem.speed); speed = tempbuff;
		std::string ssmltext_md5 = taskitem.ssmltext + speed;
		std::string textguid = md5::getStringMD5(ssmltext_md5);//must before utf8 convert,because isexist input textguid from ansi calc

		std::string ssmltext_utf8 = "test ssmltext";
		ansi_to_utf8(taskitem.ssmltext.c_str(), taskitem.ssmltext.length(), ssmltext_utf8);
		taskitem.ssmltext = ssmltext_utf8;

		std::string createtime = gettimecode();
		taskitem.createtime = createtime;

		std::string OPERATION = "";
		//check
		char sql_buff[BUFF_SZ] = { 0 };
		if (update)
		{
			OPERATION = "UPDATE";
			//update
			snprintf(sql_buff, BUFF_SZ, "update sbt_doctask set tasktype=%d,taskname='%s',createtime='%s',humanid='%s',ssmltext='%s',textguid='%s',audiofile='%s',audioformat='%s',audiolength=%d,videofile='%s',finalvideo='%s',videoformat='%s',videolength=%d,videowidth=%d,videoheight=%d,videofps=%.1f where taskid=%d",
				taskitem.tasktype, taskitem.taskname.c_str(), taskitem.createtime.c_str(), taskitem.humanid.c_str(), taskitem.ssmltext.c_str(), textguid.c_str(),
				taskitem.audio_path.c_str(), taskitem.audio_format.c_str(), taskitem.audio_length,
				taskitem.video_path.c_str(), taskitem.video_finalpath.c_str(), taskitem.video_format.c_str(), taskitem.video_length, taskitem.video_width, taskitem.video_height, taskitem.video_fps, taskid);//update
		}
		else
		{
			OPERATION = "INSERT";
			taskid = newgetsequencenextvalue("sbt_doctask", &mysql);
			taskitem.taskid = taskid;
			//insert 
			snprintf(sql_buff, BUFF_SZ, "insert into sbt_doctask (taskid,tasktype,taskname,createtime,humanid,ssmltext,textguid,audiofile,audioformat,audiolength,videofile,finalvideo,videoformat,videolength,videowidth,videoheight,videofps) values(%d, %d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s', '%s', '%s', %d, %d, %d, %.1f)",
				taskid, taskitem.tasktype, taskitem.taskname.c_str(), taskitem.createtime.c_str(), taskitem.humanid.c_str(), taskitem.ssmltext.c_str(), textguid.c_str(),
				taskitem.audio_path.c_str(), taskitem.audio_format.c_str(), taskitem.audio_length,
				taskitem.video_path.c_str(), taskitem.video_finalpath.c_str(), taskitem.video_format.c_str(), taskitem.video_length, taskitem.video_width, taskitem.video_height, taskitem.video_fps);//insert	
		}

		//run sql
		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			printf("[addmergetask]task %d, %s success\n", taskid, OPERATION.c_str());
		}
		else 
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[addmergetask]task %d, %s failed: %s\n", taskid, OPERATION.c_str(), error.c_str());
		}
		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
	}
	bool gettaskinfo(int taskid, taskinfo& taskitem)
	{
		if (simulation) return 0;

		bool ret = true;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_name.c_str(), g_database_password.c_str(), g_database_db.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[gettaskinfo]MySQL database connect failed: %s\n", error.c_str());
		}

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "select tasktype,taskname,state,progress,createtime,humanid,ssmltext,audiofile,audioformat,audiolength,videofile,finalvideo,videoformat,videolength,videowidth,videoheight,videofps from sbt_doctask where taskid=%d", taskid);//select	

		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			MYSQL_ROW row;							//table row data
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			if (rownum >= 1 && colnum >= 17)//keep right
			{
				row = mysql_fetch_row(result);
				int i = 0;
				int row_tasktype = atoi(row_value(row[i++]).c_str());
				std::string row_taskname  = row_value(row[i++]);   std::string taskname_ansi; utf8_to_ansi(row_taskname.c_str(), row_taskname.length(), taskname_ansi);
				int row_taskstate = atoi(row_value(row[i++]).c_str());
				int row_taskprogress = atoi(row_value(row[i++]).c_str());
				std::string row_createtime= row_value(row[i++]);
				std::string row_humanid   = row_value(row[i++]);
				std::string row_ssmltext  = row_value(row[i++]);   std::string ssmltext_ansi; utf8_to_ansi(row_ssmltext.c_str(), row_ssmltext.length(), ssmltext_ansi);
				std::string row_audiofile = row_value(row[i++]);
				std::string row_audioformat = row_value(row[i++]);
				int			row_audiolength = atoi(row_value(row[i++]).c_str());
				std::string row_videofile   = row_value(row[i++]);
				std::string row_finalvideo  = row_value(row[i++]);
				std::string row_videoformat = row_value(row[i++]);
				int			row_videolength = atoi(row_value(row[i++]).c_str());
				int			row_videowidth  = atoi(row_value(row[i++]).c_str());
				int			row_videoheight = atoi(row_value(row[i++]).c_str());
				double		row_videofps	= atof(row_value(row[i++]).c_str());

				taskitem.taskid = taskid;
				taskitem.tasktype = row_tasktype;
				taskitem.taskname = taskname_ansi;
				taskitem.taskstate = row_taskstate;
				taskitem.taskprogress = row_taskprogress;
				taskitem.createtime = row_createtime;
				taskitem.humanid = row_humanid;
				taskitem.ssmltext = ssmltext_ansi;
				taskitem.audio_path = row_audiofile;
				taskitem.audio_format = row_audioformat;
				taskitem.audio_length = row_audiolength;
				taskitem.video_path = row_videofile;
				taskitem.video_finalpath = row_finalvideo;
				taskitem.video_format = row_videoformat;
				taskitem.video_length = row_videolength;
				taskitem.video_width = row_videowidth;
				taskitem.video_height = row_videoheight;
				taskitem.video_fps = row_videofps;
			}
			else
			{
				ret = false;
				printf("[gettaskinfo] select task count not only one\n");
			}
			mysql_free_result(result);				//free result
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[gettaskinfo]MySQL select failed: %s\n", error.c_str());
		}
		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
	}
	int  gettaskhistoryinfo(std::string humanid, std::string orderkey , int aspect, VEC_TASKINFO& vectaskhistory)
	{
		if (simulation) return 0;

		std::string orderway = (aspect == 0) ? ("ASC") : ("DESC");
		bool ret = true; int ncount = 0;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_name.c_str(), g_database_password.c_str(), g_database_db.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[gettaskhistoryinfo]MySQL database connect failed: %s\n", error.c_str());
		}

		char sql_buff[BUFF_SZ] = { 0 };

		if (humanid.empty())
		{
			snprintf(sql_buff, BUFF_SZ, "select taskid,tasktype,taskname,state,progress,createtime,humanid,ssmltext,audiofile,audioformat,audiolength,videofile,finalvideo,videoformat,videolength,videowidth,videoheight,videofps from sbt_doctask ORDER BY %s %s", orderkey.c_str(), orderway.c_str());//select
			printf("[gettaskhistoryinfo]select all humaninfo\n");
		}
		else
		{
			snprintf(sql_buff, BUFF_SZ, "select taskid,tasktype,taskname,state,progress,createtime,humanid,ssmltext,audiofile,audioformat,audiolength,videofile,finalvideo,videoformat,videolength,videowidth,videoheight,videofps from sbt_doctask where humanid='%s' ORDER BY %s %s", humanid.c_str(), orderkey.c_str(), orderway.c_str());//select
			printf("[gettaskhistoryinfo]select one humaninfo\n");
		}

		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			MYSQL_ROW row;							//table row data
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			while (row = mysql_fetch_row(result))
			{
				if (colnum >= 18)//keep right
				{
					int i = 0;
					int row_taskid = atoi(row_value(row[i++]).c_str());
					int row_tasktype = atoi(row_value(row[i++]).c_str());
					std::string row_taskname = row_value(row[i++]);  std::string taskname_ansi; utf8_to_ansi(row_taskname.c_str(), row_taskname.length(), taskname_ansi);
					int row_taskstate = atoi(row_value(row[i++]).c_str());
					int row_taskprogress = atoi(row_value(row[i++]).c_str());
					std::string row_createtime = row_value(row[i++]);
					std::string row_humanid = row_value(row[i++]);
					std::string row_ssmltext = row_value(row[i++]);  std::string ssmltext_ansi; utf8_to_ansi(row_ssmltext.c_str(), row_ssmltext.length(), ssmltext_ansi);
					std::string row_audiofile = row_value(row[i++]);
					std::string row_audioformat = row_value(row[i++]);
					int			row_audiolength = atoi(row_value(row[i++]).c_str());
					std::string row_videofile = row_value(row[i++]);
					std::string row_finalvideo = row_value(row[i++]);
					std::string row_videoformat = row_value(row[i++]);
					int			row_videolength = atoi(row_value(row[i++]).c_str());
					int			row_videowidth = atoi(row_value(row[i++]).c_str());
					int			row_videoheight = atoi(row_value(row[i++]).c_str());
					double		row_videofps = atof(row_value(row[i++]).c_str());
					
					taskinfo historyitem;
					historyitem.taskid = row_taskid;
					historyitem.tasktype = row_tasktype;
					historyitem.taskname = taskname_ansi;
					historyitem.taskstate = row_taskstate;
					historyitem.taskprogress = row_taskprogress;
					historyitem.createtime = row_createtime;
					historyitem.humanid = row_humanid;
					historyitem.ssmltext = ssmltext_ansi;
					historyitem.audio_path = row_audiofile;
					historyitem.audio_format = row_audioformat;
					historyitem.audio_length = row_audiolength;
					historyitem.video_path = row_videofile;
					historyitem.video_finalpath = row_finalvideo;
					historyitem.video_format = row_videoformat;
					historyitem.video_length = row_videolength;
					historyitem.video_width = row_videowidth;
					historyitem.video_height = row_videoheight;
					historyitem.video_fps = row_videofps;
					vectaskhistory.push_back(historyitem);

					ncount++;
				}
			}
			mysql_free_result(result);				//free result

			printf("[gettaskhistoryinfo]MySQL select success,ncount=%d\n", ncount);
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[gettaskhistoryinfo]MySQL select failed: %s\n", error.c_str());
		}
		//=====================
		mysql_close(&mysql);	//close connect
		if (!ret) return 0;

		return ncount;
	}
	bool setfinalvideopath(int taskid, std::string vdofinalpath)
	{
		if (simulation) return true;

		bool ret = true;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_name.c_str(), g_database_password.c_str(), g_database_db.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[setfinalvideopath]MySQL database connect failed: %s\n", error.c_str());
		}

		//
		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "update sbt_doctask set finalvideo='%s' where taskid=%d", vdofinalpath.c_str(), taskid);//update		

		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			printf("[setfinalvideopath]task %d, update finalvideo success\n", taskid);
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[setfinalvideopath]task %d, update finalvideo failed: %s\n", taskid, error.c_str());
		}
		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
	}

	bool isexisttask_taskid(int taskid)
	{
		if (simulation) return true;
		if (taskid == 0) return false;

		bool ret = false;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_name.c_str(), g_database_password.c_str(), g_database_db.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[isexisttask]MySQL database connect failed: %s\n", error.c_str());
		}

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "select * from sbt_doctask where taskid = %d", taskid);

		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			if (rownum == 1)
				ret = true;

			mysql_free_result(result);				//free result
		}

		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
	}
	bool isexisttask_textguid(int& taskid, std::string taskname, std::string textguid)
	{
		if (simulation) return true;
		if (taskid == 0) return false;

		bool ret = false; int progress = 0; std::string audiofile = "";
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_name.c_str(), g_database_password.c_str(), g_database_db.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[isexisttask]MySQL database connect failed: %s\n", error.c_str());
		}

		char sql_buff[BUFF_SZ] = { 0 };
		//check textguid
		snprintf(sql_buff, BUFF_SZ, "select taskid,progress,audiofile from sbt_doctask where textguid = '%s'", textguid.c_str());

		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			MYSQL_ROW row;							//table row data
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			printf("[isexisttask]sql=%s, row=%d, col=%d\n", sql_buff, rownum, colnum);
			if (rownum >= 1 && colnum >= 3)//keep right
			{
				int i = 0;
				row = mysql_fetch_row(result);

				taskid = atoi(row[i++]);
				progress = atoi(row[i++]);
				audiofile = row_value(row[i++]);
				if (progress == 100 && is_existfile(audiofile))
					ret = true;
			}
			mysql_free_result(result);				//free result
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[isexisttask]MySQL select failed: %s\n", error.c_str());
		}

		//update taskname
		if (ret)
		{
			std::string taskname_utf8 = "test taskname";
			ansi_to_utf8(taskname.c_str(), taskname.length(), taskname_utf8);
			snprintf(sql_buff, BUFF_SZ, "update sbt_doctask set taskname='%s' where textguid = '%s'", taskname_utf8.c_str(), textguid.c_str());//update

			mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
			if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
			{
				printf("[isexisttask]task %d, update taskname success\n", taskid);
			}
			else
			{
				ret = false;
				std::string error = mysql_error(&mysql);
				printf("[isexisttask]task %d, update taskname failed: %s\n", taskid, error.c_str());
			}
		}

		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
	}

	bool deletetask_taskid(int taskid, std::string& errmsg)
	{
		if (simulation) return true;

		bool ret = false;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_name.c_str(), g_database_password.c_str(), g_database_db.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql); errmsg = error;
			printf("[isexisttask]MySQL database connect failed: %s\n", error.c_str());
		}

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "delete from sbt_doctask where taskid = %d", taskid);

		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			printf("[deletetask_taskid]delete task by taskid success, taskid=%d\n", taskid);
			errmsg = "delete task by taskid success";
		}
		else
		{
			printf("[deletetask_taskid]delete task by taskid failed, taskid=%d\n", taskid);
			std::string error = mysql_error(&mysql); errmsg = error;
		}

		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
	}
	
	//
	int  getmergeprogress(int taskid)
	{
		if (simulation) return 0;

		bool ret = true; int nprogress = 0;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_name.c_str(), g_database_password.c_str(), g_database_db.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[getmergeprogress]MySQL database connect failed: %s\n", error.c_str());
		}

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "select sbt_doctask.progress from sbt_doctask where taskid = %d", taskid);

		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			MYSQL_ROW row;							//table row data
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			if (rownum >= 1 && colnum >= 1)//keep right
			{
				row = mysql_fetch_row(result);
				nprogress = atoi(row[0]);
			}
			else
			{
				ret = false;
				printf("[getmergeprogress]task %d, select mergetask count not only one\n", taskid);
			}
			mysql_free_result(result);				//free result

			printf("[getmergeprogress]task %d, select mergetask progress success, progress=%d\n", taskid, nprogress);
		}
		else 
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[getmergeprogress]task %d, select progress failed: %s\n", taskid, error.c_str());
		}
		
		//=====================
		mysql_close(&mysql);	//close connect
		if (!ret) return -1;

		return nprogress;
	}
	bool setmergeprogress(int taskid, int nprogress)
	{
		if (simulation) return true;

		bool ret = true;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_name.c_str(), g_database_password.c_str(), g_database_db.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[setmergeprogress]MySQL database connect failed: %s\n", error.c_str());
		}

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "select sbt_doctask.progress from sbt_doctask where taskid = %d", taskid);

		mysql_query(&mysql, "SET NAMES UTF8");			//support chinese text
		if (!mysql_query(&mysql, sql_buff))				//success return 0,failed return random number
		{
			MYSQL_RES* result;							//table data struct
			result = mysql_store_result(&mysql);		//sava dada to result

			snprintf(sql_buff, BUFF_SZ, "update sbt_doctask set progress=%d where taskid = %d", nprogress, taskid);	//update

			mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
			if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
			{
				printf("[setmergeprogress]task %d, update progress success, progress=%d\n", taskid, nprogress);
			}
			else
			{
				ret = false;
				std::string error = mysql_error(&mysql);
				printf("[setmergeprogress]task %d, update progress failed: %s\n", taskid, error.c_str());
			}
			mysql_free_result(result);					//free result
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[setmergeprogress]task %d, select progress failed: %s\n", taskid, error.c_str());
		}

		//=====================
		mysql_close(&mysql);	//close connect

		return nprogress;
	}

	//-1=waitmerge,0=merging,1=mergesuccess,2=mergefailed,3=movefailed
	int  getmergestate(int taskid)
	{
		if (simulation) return 0;

		bool ret = true; int nstate = 0;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_name.c_str(), g_database_password.c_str(), g_database_db.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[getmergestate]MySQL database connect failed: %s\n", error.c_str());
		}

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "select sbt_doctask.state from sbt_doctask where taskid = %d", taskid);

		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			MYSQL_ROW row;							//table row data
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			if (rownum >= 1 && colnum >= 1)//keep right
			{
				row = mysql_fetch_row(result);
				nstate = atoi(row[0]);
			}
			mysql_free_result(result);				//free result

			printf("[getmergestate]task %d, select mergetask state success, state=%d\n", taskid, nstate);
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[getmergestate]task %d, select state failed: %s\n", taskid, error.c_str());
		}

		//=====================
		mysql_close(&mysql);	//close connect
		if (!ret) return -1;

		return nstate;
	}
	bool setmergestate(int taskid, int nstate)
	{
		if (simulation) return true;

		bool ret = true;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_name.c_str(), g_database_password.c_str(), g_database_db.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[setmergestate]MySQL database connect failed: %s\n", error.c_str());
		}

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "select sbt_doctask.state from sbt_doctask where taskid = %d", taskid);

		mysql_query(&mysql, "SET NAMES UTF8");			//support chinese text
		if (!mysql_query(&mysql, sql_buff))				//success return 0,failed return random number
		{
			MYSQL_RES* result;							//table data struct
			result = mysql_store_result(&mysql);		//sava dada to result

			snprintf(sql_buff, BUFF_SZ, "update sbt_doctask set state=%d where taskid = %d", nstate, taskid);	//update

			mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
			if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
			{
				printf("[setmergestate]task %d, update state success, state=%d\n", taskid, nstate);
			}
			else
			{
				ret = false;
				std::string error = mysql_error(&mysql);
				printf("[setmergestate]task %d, update state failed: %s\n", taskid, error.c_str());
			}
			mysql_free_result(result);					//free result
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			printf("[setmergestate]task %d, select state failed: %s\n", taskid, error.c_str());
		}

		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
	}
};