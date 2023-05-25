#include "digitalmysql.h"
#include "public.h"

#pragma comment(lib,"libmysql.lib")
#pragma warning(error:4477)  //<snprintf> warning as error


static bool simulation = false;
std::string g_database_ip = "";
short	    g_database_port = 3306;
std::string g_database_username = "";
std::string g_database_password = "";
std::string g_database_dbname = "";

#define BUFF_SZ 1024*16  //system max stack size

//custom loger
static FileWriter loger_mysql("mysql.log");

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
			if (configbuffer == nullptr) return false;

			fread(configbuffer, length, 1, fp);
			fclose(fp);
		}

		if (length && configbuffer)
		{
			std::string value = "";
			std::string config = configbuffer;
			free(configbuffer);

			value = getnodevalue(config, "mysql_ip");
			if (value.empty()) return false;
			g_database_ip = value;
			_debug_to(1, ("CONFIG_database_ip = %s\n"), g_database_ip.c_str());

			value = getnodevalue(config, "mysql_port");
			if (value.empty()) return false;
			g_database_port = atoi(value.c_str());
			_debug_to(1, ("CONFIG_database_port = %d\n"), g_database_port);

			value = getnodevalue(config, "mysql_username");
			if (value.empty()) return false;
			g_database_username = value;
			_debug_to(1, ("CONFIG_database_username = %s\n"), g_database_username.c_str());

			value = getnodevalue(config, "mysql_password");
			if (value.empty()) return false;
			g_database_password = value;
			_debug_to(1, ("CONFIG_database_password = %s\n"), g_database_password.c_str());

			value = getnodevalue(config, "mysql_dbname");
			if (value.empty()) return false;
			g_database_dbname = value;
			_debug_to(1, ("CONFIG_database_dbname = %s\n"), g_database_dbname.c_str());

			return true;
		}

		return false;
	}
	std::string transsequencename(std::string strname)
	{
		std::string strvalue = "";
		if (strname.compare("sbt_doctask") == 0)
		{
			strvalue = "sbt_sqt_doctask";
		}
		else if (strname.compare("sbt_doctasksource") == 0)
		{
			strvalue = "sbt_sqt_doctasksource";
		}
		else if (strname.compare("sbt_humansource") == 0)
		{
			strvalue = "sbt_sqt_humansource";
		}
		else if (strname.compare("sbt_humanvideo") == 0)
		{
			strvalue = "sbt_sqt_humanvideo";
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
		_debug_to(loger_mysql, 0, ("[newgetsequencenextvalue] sql: %s\n"), sql_buff);
		if (!mysql_query(pmysql, sql_buff))	//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			result = mysql_store_result(pmysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			_debug_to(loger_mysql, 0, ("[newgetsequencenextvalue] rownum=%d,colnum=%d\n"), rownum, colnum);

			MYSQL_ROW row = mysql_fetch_row(result);//table row data
			if (rownum >= 1 && colnum >= 1)//keep right
			{
				sequence = atoi(row[0]);
			}
			else
			{
				sequence = 0;
			}
			sequence++;

			//
			snprintf(sql_buff, BUFF_SZ, "insert into %s (id) value(%d)", strsequencename.c_str(), sequence);// insert
			_debug_to(loger_mysql, 0, ("[newgetsequencenextvalue] sql: %s\n"), sql_buff);
			if (!mysql_query(pmysql, sql_buff))	//success return 0,failed return random number
			{
				_debug_to(loger_mysql, 0, ("[newgetsequencenextvalue], insert sqt table success, sequence=%d\n"), sequence);
			}
			else
			{
				ret = false;
				std::string error = mysql_error(pmysql);
				_debug_to(loger_mysql, 1, ("[newgetsequencenextvalue], insert sqt table failed: %s\n"), error.c_str());
			}
			mysql_free_result(result);				//free result
		}
		else
		{
			ret = false;
			std::string error = mysql_error(pmysql);
			_debug_to(loger_mysql, 1, ("[newgetsequencenextvalue], select sqt table failed: %s\n"), error.c_str());
		}

		if (!ret) return 0;
		return sequence;
	}

	//
	bool addhumaninfo(humaninfo humanitem, bool update)
	{
		if (simulation) return true;

		bool ret = true;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[addhumanlistinfo]MySQL database connect failed: %s\n"), error.c_str());
			return false;
		}

		//a=coding convert
		std::string humanname_utf8 = "test humanname";
		ansi_to_utf8(humanitem.humanname.c_str(), humanitem.humanname.length(), humanname_utf8);
		humanitem.humanname = humanname_utf8;

		std::string OPERATION = "";
		char sql_buff[BUFF_SZ] = { 0 };
		if (update)
		{
			OPERATION = "UPDATE";
			//update
			snprintf(sql_buff, BUFF_SZ, "update sbt_humansource set humanname='%s',contentid='%s',sourcefolder='%s',available=%d,speakspeed=%.1f,seriousspeed=%.1f,imagematting='%s',keyframe='%s',luckeyframe='%s',speakpath='%s',pwgpath='%s',mouthmodefile='%s',facemodefile='%s' where humanid='%s'",
				humanitem.humanname.c_str(), humanitem.contentid.c_str(), humanitem.sourcefolder.c_str(), humanitem.available, humanitem.speakspeed, humanitem.seriousspeed, humanitem.imagematting.c_str(), humanitem.keyframe.c_str(), humanitem.luckeyframe.c_str(), humanitem.speakmodelpath.c_str(), humanitem.pwgmodelpath.c_str(), humanitem.mouthmodelfile.c_str(), humanitem.facemodelfile.c_str(),
				humanitem.humanid.c_str());//update
		}
		else
		{
			OPERATION = "INSERT";
			int next_id = newgetsequencenextvalue("sbt_humansource", &mysql);
			humanitem.id = next_id;

			//insert 
			snprintf(sql_buff, BUFF_SZ, "insert into sbt_humansource (id,humanid,humanname,contentid,sourcefolder,available,speakspeed,seriousspeed,imagematting,keyframe,luckeyframe,speakpath,pwgpath,mouthmodefile,facemodefile) values(%d, '%s', '%s', '%s', '%s', %d, %.1f, %.1f, '%s', '%s', '%s', '%s', '%s', '%s', '%s')",
				humanitem.id,humanitem.humanid.c_str(), 
				humanitem.humanname.c_str(), humanitem.contentid.c_str(), humanitem.sourcefolder.c_str(), humanitem.available, humanitem.speakspeed, humanitem.seriousspeed,humanitem.imagematting.c_str(), humanitem.keyframe.c_str(), humanitem.luckeyframe.c_str(), humanitem.speakmodelpath.c_str(), humanitem.pwgmodelpath.c_str(),humanitem.mouthmodelfile.c_str(), humanitem.facemodelfile.c_str());
		}

		//run sql
		_debug_to(loger_mysql, 0, ("[addhumanlistinfo] sql: %s\n"), sql_buff);
		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			_debug_to(loger_mysql, 0, ("[addhumanlistinfo]humanid=%d, %s success\n"), humanitem.humanid, OPERATION.c_str());
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[addhumanlistinfo]humanid=%d, %s failed: %s\n"), humanitem.humanid, OPERATION.c_str(), error.c_str());
		}

		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
	}
	bool isexisthuman_humanid(std::string humanid)
	{
		if (simulation) return true;
		if (humanid.empty()) return false;

		bool ret = false;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[isexisthuman_humanid]MySQL database connect failed: %s\n"), error.c_str());
			return false;
		}

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "select * from sbt_humansource where humanid = '%s'", humanid.c_str());
		_debug_to(loger_mysql, 0, ("[isexisthuman_humanid] sql: %s\n"), sql_buff);
		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			_debug_to(loger_mysql, 0, ("[isexisthuman_humanid] rownum=%d,colnum=%d\n"), rownum, colnum);

			if (rownum >= 1)//keep right
				ret = true;

			mysql_free_result(result);				//free result
		}

		//=====================
		mysql_close(&mysql);	//close connect

		return ret;

	}
	bool isavailable_humanid(std::string humanid)
	{
		if (simulation) return true;
		if (humanid.empty()) return false;

		bool ret = false;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[isavailable_humanid]MySQL database connect failed: %s\n"), error.c_str());
			return false;
		}

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "select available from sbt_humansource where humanid = '%s'", humanid.c_str());
		_debug_to(loger_mysql, 0, ("[isavailable_humanid] sql: %s\n"), sql_buff);
		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			_debug_to(loger_mysql, 0, ("[isavailable_humanid] rownum=%d,colnum=%d\n"), rownum, colnum);

			MYSQL_ROW row = mysql_fetch_row(result);//table row data
			if (rownum >= 1 && colnum >= 1)//keep right
			{
				int available = atoi(row_value(row[0]).c_str());
				ret = (available > 0) ? (true) : (false);
			}

			mysql_free_result(result);				//free result
		}

		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
	}
	bool deletehuman_humanid(std::string humanid, bool deletefile, std::string& errmsg)
	{
		if (simulation) return true;

		bool ret = false;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql); errmsg = error;
			_debug_to(loger_mysql, 1, ("[deletehuman_humanid]MySQL database connect failed: %s\n"), error.c_str());
			return false;
		}

		char sql_buff[BUFF_SZ] = { 0 };
		if (deletefile)
		{
			snprintf(sql_buff, BUFF_SZ, "select imagematting,mouthmodefile,facemodefile,keyframe,luckeyframe,speakpath,pwgpath from sbt_humansource where humanid = '%s'", humanid.c_str());
			_debug_to(loger_mysql, 0, ("[deletehuman_humanid] sql: %s\n"), sql_buff);
			mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
			if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
			{
				MYSQL_RES* result;						//table data struct
				result = mysql_store_result(&mysql);    //sava dada to result
				int rownum = mysql_num_rows(result);	//get row number
				int colnum = mysql_num_fields(result);  //get col number
				_debug_to(loger_mysql, 0, ("[deletehuman_humanid] rownum=%d,colnum=%d\n"), rownum, colnum);

				MYSQL_ROW row = mysql_fetch_row(result);//table row data
				if (rownum >= 1)//keep right
				{
					if (colnum >= 1)
					{
						int i = 0;
						while (rownum--)
						{
							std::string physicalfilepath = row_value(row[i++]);
							if (is_existfile(physicalfilepath.c_str()))
								remove(physicalfilepath.c_str());
						}	
					}
				}
				mysql_free_result(result);				//free result
				_debug_to(loger_mysql, 0, ("[deletehuman_humanid]MySQL select filepath before delete human success\n"));
			}
			else
			{
				ret = false;
				std::string error = mysql_error(&mysql);
				_debug_to(loger_mysql, 1, ("[deletehuman_humanid]MySQL select filepath before delete human failed: %s\n"), error.c_str());
			}
		}

		snprintf(sql_buff, BUFF_SZ, "delete from sbt_humansource where humanid = '%s'", humanid.c_str());
		_debug_to(loger_mysql, 0, ("[deletehuman_humanid] sql: %s\n"), sql_buff);
		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			_debug_to(loger_mysql, 0, ("[deletehuman_humanid]delete task by taskid success, humanid = '%s'\n"), humanid.c_str());
			errmsg = "delete task by taskid success";
		}
		else
		{
			std::string error = mysql_error(&mysql); errmsg = error;
			_debug_to(loger_mysql, 1, ("[deletehuman_humanid]delete task by taskid failed, humanid = '%s'\n"), humanid.c_str());
		}

		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
	}

	bool gethumanlistinfo(std::string humanid, VEC_HUMANINFO& vechumaninfo)
	{
		if (simulation) return 0;

		bool ret = true;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[gethumanlistinfo]MySQL database connect failed: %s\n"), error.c_str());
			return false;
		}

		char sql_buff[BUFF_SZ] = { 0 };
		if(humanid.empty())
			snprintf(sql_buff, BUFF_SZ, "select id,humanid,humanname,contentid,sourcefolder,available,speakspeed,seriousspeed,imagematting,keyframe,luckeyframe,speakpath,pwgpath,mouthmodefile,facemodefile from sbt_humansource");//select	
		else
			snprintf(sql_buff, BUFF_SZ, "select id,humanid,humanname,contentid,sourcefolder,available,speakspeed,seriousspeed,imagematting,keyframe,luckeyframe,speakpath,pwgpath,mouthmodefile,facemodefile from sbt_humansource where humanid='%s'", humanid.c_str());//select	
		_debug_to(loger_mysql, 0, ("[gethumanlistinfo] sql: %s\n"), sql_buff);
		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			_debug_to(loger_mysql, 0, ("[gethumanlistinfo] rownum=%d,colnum=%d\n"), rownum, colnum);

			MYSQL_ROW row;							//table row data
			while (row = mysql_fetch_row(result))
			{
				if (colnum >= 15) //keep right
				{
					int i = 0;
					int row_id = atoi(row_value(row[i++]).c_str());
					std::string row_humanid = row_value(row[i++]);
					std::string row_humanname = row_value(row[i++]); std::string humanname_ansi; utf8_to_ansi(row_humanname.c_str(), row_humanname.length(), humanname_ansi);
					std::string row_contentid = row_value(row[i++]);
					std::string row_sourcefolder = row_value(row[i++]);
					int row_available = atoi(row_value(row[i++]).c_str());
					double row_speakspeed = atof(row_value(row[i++]).c_str());
					double row_seriousspeed = atof(row_value(row[i++]).c_str());
					std::string row_imagematting = row_value(row[i++]);   std::string imagematting_ansi; utf8_to_ansi(row_imagematting.c_str(), row_imagematting.length(), imagematting_ansi);
					std::string row_keyframe = row_value(row[i++]);       std::string keyframe_ansi; utf8_to_ansi(row_keyframe.c_str(), row_keyframe.length(), keyframe_ansi);
					std::string row_luckeyframe = row_value(row[i++]);	  std::string luckeyframe_ansi; utf8_to_ansi(row_luckeyframe.c_str(), row_luckeyframe.length(), luckeyframe_ansi);
					std::string row_speakmodelpath = row_value(row[i++]); std::string speakmodelpath_ansi; utf8_to_ansi(row_speakmodelpath.c_str(), row_speakmodelpath.length(), speakmodelpath_ansi);
					std::string row_pwgmodelpath = row_value(row[i++]);   std::string pwgmodelpath_ansi; utf8_to_ansi(row_pwgmodelpath.c_str(), row_pwgmodelpath.length(), pwgmodelpath_ansi);
					std::string row_mouthmodelfile = row_value(row[i++]); std::string mouthmodelfile_ansi; utf8_to_ansi(row_mouthmodelfile.c_str(), row_mouthmodelfile.length(), mouthmodelfile_ansi);
					std::string row_facemodelfile = row_value(row[i++]);  std::string facemodelfile_ansi; utf8_to_ansi(row_facemodelfile.c_str(), row_facemodelfile.length(), facemodelfile_ansi);

					humaninfo humanitem;
					humanitem.id = row_id;
					humanitem.humanid = row_humanid;
					humanitem.humanname = humanname_ansi;
					humanitem.contentid = row_contentid;
					humanitem.sourcefolder = row_sourcefolder;
					humanitem.available = row_available;
					humanitem.speakspeed = row_speakspeed;
					humanitem.seriousspeed = row_seriousspeed;
					humanitem.imagematting = row_imagematting;
					humanitem.keyframe = keyframe_ansi;
					humanitem.luckeyframe = luckeyframe_ansi;
					humanitem.speakmodelpath = speakmodelpath_ansi;
					humanitem.pwgmodelpath = pwgmodelpath_ansi;
					humanitem.mouthmodelfile = mouthmodelfile_ansi;
					humanitem.facemodelfile = facemodelfile_ansi;
					
					vechumaninfo.push_back(humanitem);
				}
			}	
			mysql_free_result(result);				//free result

			_debug_to(loger_mysql, 0, ("[gethumanlistinfo]MySQL select humanlist success\n"));
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[gethumanlistinfo]MySQL select humanlist failed: %s\n"), error.c_str());
		}
		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
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
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[addmergetask]MySQL database connect failed: %s\n"), error.c_str());
			return false;
		}

		char tempbuff[16] = { 0 };
		std::string speak_speed = ""; snprintf(tempbuff, 16, "%.1f", taskitem.speakspeed); speak_speed = tempbuff;
		std::string ssmltext_md5 = taskitem.ssmltext + speak_speed;
		std::string textguid = md5::getStringMD5(ssmltext_md5);//must before utf8 convert,because isexist input textguid from ansi calc

		//a=coding convert
		std::string taskname_utf8 = "";
		ansi_to_utf8(taskitem.taskname.c_str(), taskitem.taskname.length(), taskname_utf8);
		taskitem.taskname = taskname_utf8;
		//b=coding convert
		std::string humanname_utf8 = "";
		ansi_to_utf8(taskitem.humanname.c_str(), taskitem.humanname.length(), humanname_utf8);
		taskitem.humanname = humanname_utf8;
		//c=coding convert
		std::string ssmltext_utf8 = "";
		ansi_to_utf8(taskitem.ssmltext.c_str(), taskitem.ssmltext.length(), ssmltext_utf8);
		taskitem.ssmltext = ssmltext_utf8;

		//
		std::string OPERATION = "";
		char sql_buff[BUFF_SZ] = { 0 };
		if (update)
		{
			//update
			OPERATION = "UPDATE";
			snprintf(sql_buff, BUFF_SZ, "update sbt_doctask set tasktype=%d,moodtype=%d,speakspeed=%.1f,taskname='%s',createtime='%s',humanid='%s',humanname='%s',ssmltext='%s',textguid='%s',audiofile='%s',audioformat='%s',audiolength=%d,videofile='%s',finalvideo='%s',videoformat='%s',videolength=%d,videowidth=%d,videoheight=%d,videofps=%.1f,background='%s',front_xpos=%d,front_ypos=%d,front_scale=%d,front_rotation=%d where taskid=%d",
				taskitem.tasktype, taskitem.moodtype, taskitem.speakspeed, taskitem.taskname.c_str(), taskitem.createtime.c_str(), taskitem.humanid.c_str(), taskitem.humanname.c_str(), taskitem.ssmltext.c_str(), textguid.c_str(),
				taskitem.audio_path.c_str(), taskitem.audio_format.c_str(), taskitem.audio_length,
				taskitem.video_path.c_str(), taskitem.video_finalpath.c_str(), taskitem.video_format.c_str(), taskitem.video_length, taskitem.video_width, taskitem.video_height, taskitem.video_fps, taskitem.background_path.c_str(), taskitem.front_XPos,taskitem.front_YPos,taskitem.front_Scale,taskitem.front_Rotation,
				taskid);
		}
		else
		{
			//insert 
			OPERATION = "INSERT";
			taskid = newgetsequencenextvalue("sbt_doctask", &mysql);
			taskitem.taskid = taskid;
			snprintf(sql_buff, BUFF_SZ, "insert into sbt_doctask (taskid,tasktype,moodtype,speakspeed,taskname,createtime,humanid,humanname,ssmltext,textguid,audiofile,audioformat,audiolength,videofile,finalvideo,videoformat,videolength,videowidth,videoheight,videofps,background,front_xpos,front_ypos,front_scale,front_rotation) values(%d, %d, %d, %.1f, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s', '%s', '%s', %d, %d, %d, %.1f,'%s', %d, %d, %d, %d)",
		taskid, taskitem.tasktype, taskitem.moodtype, taskitem.speakspeed, taskitem.taskname.c_str(), taskitem.createtime.c_str(), taskitem.humanid.c_str(), taskitem.humanname.c_str(), taskitem.ssmltext.c_str(), textguid.c_str(),
				taskitem.audio_path.c_str(), taskitem.audio_format.c_str(), taskitem.audio_length,
				taskitem.video_path.c_str(), taskitem.video_finalpath.c_str(), taskitem.video_format.c_str(), taskitem.video_length, taskitem.video_width, taskitem.video_height, taskitem.video_fps, taskitem.background_path.c_str(), taskitem.front_XPos, taskitem.front_YPos, taskitem.front_Scale, taskitem.front_Rotation );
		}

		//run sql
		_debug_to(loger_mysql, 0, ("[addmergetask] sql: %s\n"), sql_buff);
		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			_debug_to(loger_mysql, 0, ("[addmergetask]task %d, %s success\n"), taskid, OPERATION.c_str());
		}
		else 
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[addmergetask]task %d, %s failed: %s\n"), taskid, OPERATION.c_str(), error.c_str());
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
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[gettaskinfo]MySQL database connect failed: %s\n"), error.c_str());
			return false;
		}

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "select tasktype,moodtype,speakspeed,taskname,state,progress,createtime,humanid,humanname,ssmltext,audiofile,audioformat,audiolength,videofile,finalvideo,videoformat,videolength,videowidth,videoheight,videofps,background,front_xpos,front_ypos,front_scale,front_rotation from sbt_doctask where taskid=%d", taskid);//select	
		_debug_to(loger_mysql, 0, ("[gettaskinfo] sql: %s\n"), sql_buff);
		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			_debug_to(loger_mysql, 0, ("[gettaskinfo] rownum=%d,colnum=%d\n"), rownum, colnum);

			MYSQL_ROW row = mysql_fetch_row(result);//table row data
			if (rownum >= 1 && colnum >= 24)//keep right
			{
				int i = 0;
				int row_tasktype = atoi(row_value(row[i++]).c_str());
				int row_moodtype = atoi(row_value(row[i++]).c_str());
				double row_speakspeed = atof(row_value(row[i++]).c_str());
				std::string row_taskname  = row_value(row[i++]);   std::string taskname_ansi; utf8_to_ansi(row_taskname.c_str(), row_taskname.length(), taskname_ansi);
				int row_taskstate = atoi(row_value(row[i++]).c_str());
				int row_taskprogress = atoi(row_value(row[i++]).c_str());
				std::string row_createtime= row_value(row[i++]);
				std::string row_humanid   = row_value(row[i++]);
				std::string row_humanname = row_value(row[i++]);  std::string humanname_ansi; utf8_to_ansi(row_humanname.c_str(), row_humanname.length(), humanname_ansi);
				std::string row_ssmltext = row_value(row[i++]);   std::string ssmltext_ansi; utf8_to_ansi(row_ssmltext.c_str(), row_ssmltext.length(), ssmltext_ansi); ssmltext_ansi = str_replace(ssmltext_ansi, "\n", "");
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
				std::string row_background = row_value(row[i++]).c_str();
				int			row_front_xpos = atoi(row_value(row[i++]).c_str());
				int			row_front_ypos = atoi(row_value(row[i++]).c_str());
				int			row_front_scale = atoi(row_value(row[i++]).c_str());
				int			row_front_rotation = atoi(row_value(row[i++]).c_str());

				taskitem.taskid = taskid;
				taskitem.tasktype = row_tasktype;
				taskitem.moodtype = row_moodtype;
				taskitem.speakspeed = row_speakspeed;
				taskitem.taskname = taskname_ansi;//a=coding convert
				taskitem.taskstate = row_taskstate;
				taskitem.taskprogress = row_taskprogress;
				taskitem.createtime = row_createtime;
				taskitem.humanid = row_humanid;
				taskitem.humanname = humanname_ansi;//b=coding convert
				taskitem.ssmltext = ssmltext_ansi;//c=coding convert
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
				taskitem.background_path = row_background;
				taskitem.front_XPos = row_front_xpos;
				taskitem.front_YPos = row_front_ypos;
				taskitem.front_Scale = row_front_scale;
				taskitem.front_Rotation = row_front_rotation;
			}
			else
			{
				ret = false;
				_debug_to(loger_mysql, 1, ("[gettaskinfo] select taskinfo not only one\n"));
			}
			mysql_free_result(result);				//free result
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[gettaskinfo]MySQL select taskinfo failed: %s\n"), error.c_str());
		}
		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
	}
	bool setfinalvideopath(int taskid, std::string vdofinalpath)
	{
		if (simulation) return true;

		bool ret = true;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[setfinalvideopath]MySQL database connect failed: %s\n"), error.c_str());
			return false;
		}

		//
		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "update sbt_doctask set finalvideo='%s' where taskid=%d", vdofinalpath.c_str(), taskid);//update
		_debug_to(loger_mysql, 0, ("[setfinalvideopath] sql: %s\n"), sql_buff);
		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			_debug_to(loger_mysql, 0, ("[setfinalvideopath]task %d, update finalvideo success\n"), taskid);
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[setfinalvideopath]task %d, update finalvideo failed: %s\n"), taskid, error.c_str());
		}
		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
	}
	bool deletetask_taskid(int taskid, bool deletefile, std::string& errmsg)
	{
		if (simulation) return true;

		bool ret = false;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql); errmsg = error;
			_debug_to(loger_mysql, 1, ("[deletetask_taskid]MySQL database connect failed: %s\n"), error.c_str());
			return false;
		}

		char sql_buff[BUFF_SZ] = { 0 };
		if (deletefile)
		{
			snprintf(sql_buff, BUFF_SZ, "select audiofile,videofile,finalvideo from sbt_doctask where taskid = %d", taskid);
			_debug_to(loger_mysql, 0, ("[deletetask_taskid] sql: %s\n"), sql_buff);
			mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
			if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
			{
				MYSQL_RES* result;						//table data struct
				result = mysql_store_result(&mysql);    //sava dada to result
				int rownum = mysql_num_rows(result);	//get row number
				int colnum = mysql_num_fields(result);  //get col number
				_debug_to(loger_mysql, 0, ("[deletetask_taskid] rownum=%d,colnum=%d\n"), rownum, colnum);

				MYSQL_ROW row = mysql_fetch_row(result);//table row data
				if (rownum >= 1)//keep right
				{
					int i = 0;
					while (rownum--)
					{
						std::string physicalfilepath = row_value(row[i++]);
						if (is_existfile(physicalfilepath.c_str()))
							remove(physicalfilepath.c_str());
					}
				}
				mysql_free_result(result);				//free result
				_debug_to(loger_mysql, 0, ("[deletetask_taskid]MySQL select filepath before delete task success\n"));
			}
			else
			{
				ret = false;
				std::string error = mysql_error(&mysql);
				_debug_to(loger_mysql, 1, ("[deletetask_taskid]MySQL select filepath before delete task failed: %s\n"), error.c_str());
			}
		}

		snprintf(sql_buff, BUFF_SZ, "delete from sbt_doctask where taskid = %d", taskid);
		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		_debug_to(loger_mysql, 0, ("[deletetask_taskid] sql: %s\n"), sql_buff);
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			_debug_to(loger_mysql, 0, ("[deletetask_taskid]delete task by taskid success, taskid=%d\n"), taskid);
			errmsg = "delete task by taskid success";
		}
		else
		{
			std::string error = mysql_error(&mysql); errmsg = error;
			_debug_to(loger_mysql, 1, ("[deletetask_taskid]delete task by taskid failed, taskid=%d\n"), taskid);
		}

		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
	}

	bool gettaskhistoryinfo(VEC_FILTERINFO& vecfilterinfo, std::string order_key , int order_way, int pagesize, int pagenum, int& tasktotal, VEC_TASKINFO& vectaskhistory)
	{
		if (simulation) return true;

		//filter
		std::string str_where = "";
		//1
		VEC_FILTERINFO vecusedfilter;
		for (VEC_FILTERINFO::iterator it = vecfilterinfo.begin(); it != vecfilterinfo.end(); ++it)
		{
			std::string filterfield = it->filterfield;
			std::string filtervalue = it->filtervalue;
			if (!filterfield.empty() && !filtervalue.empty())
			{
				filterinfo usedfilter;
				std::string filterfield_utf8; ansi_to_utf8(filterfield.c_str(), filterfield.length(), filterfield_utf8);
				std::string filtervalue_utf8; ansi_to_utf8(filtervalue.c_str(), filtervalue.length(), filtervalue_utf8);

				usedfilter.filterfield = filterfield_utf8;
				usedfilter.filtervalue = filtervalue_utf8;
				vecusedfilter.push_back(usedfilter);
			}
		}
		vecfilterinfo.clear(); vecfilterinfo.assign(vecusedfilter.begin(), vecusedfilter.end());
		//2
		for (VEC_FILTERINFO::iterator it = vecfilterinfo.begin(); it != vecfilterinfo.end(); ++it)
		{
			if(it == vecfilterinfo.begin())
				str_where += " where ";

			std::string field = it->filterfield;
			std::string value = it->filtervalue;
			if (!field.empty() && !value.empty())
			{
				std::string temp;
				char tempbuff[256] = { 0 };
				snprintf(tempbuff, 256, "cast(%s as char) like '%%%s%%'", field.c_str(), value.c_str());//CAST统一转换字段类型为字符
				temp = tempbuff;

				str_where += temp;
			}
			if (it != vecfilterinfo.end() - 1)
				str_where += " and ";
		}

		//order
		std::string str_order = "";
		if (!order_key.empty())
		{
			char temp[256] = { 0 };
			std::string orderway = (order_way == 0) ? ("asc") : ("desc");
			snprintf(temp, 256, " order by %s %s", order_key.c_str(), orderway.c_str());
			str_order = temp;
		}

		//limit
		std::string str_limit = "";
		if (pagesize > 0 && pagenum > 0)
		{
			int throw_count = (pagenum-1)* pagesize;
			int need_count  = pagesize;

			char temp[256] = { 0 };
			snprintf(temp, 256, " limit %d,%d", throw_count, need_count);
			str_limit = temp;
		}

		bool ret = true;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[gettaskhistoryinfo]MySQL database connect failed: %s\n"), error.c_str());
			return false;
		}

		std::string str_sqltotal = "select count(taskid) from sbt_doctask";
		str_sqltotal += str_where;
		_debug_to(loger_mysql, 0, ("[gettaskhistoryinfo] sql: %s\n"), str_sqltotal.c_str());
		mysql_query(&mysql, "SET NAMES UTF8");			//support chinese text
		if (!mysql_query(&mysql, str_sqltotal.c_str()))	//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			_debug_to(loger_mysql, 0, ("[gettaskhistoryinfo] rownum=%d,colnum=%d\n"), rownum, colnum);

			MYSQL_ROW row = mysql_fetch_row(result);//table row data
			if (rownum >= 1 && colnum >= 1)//keep right
			{
				int row_tasktotal = atoi(row_value(row[0]).c_str());
				tasktotal = (row_tasktotal > 0) ? (row_tasktotal) : (0);
			}
			mysql_free_result(result);				//free result
			_debug_to(loger_mysql, 0, ("[gettaskhistoryinfo]MySQL select tasktotal success,totaltask=%d\n"), tasktotal);
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[gettaskhistoryinfo]MySQL select tasktotaltotal failed: %s\n"), error.c_str());
		}

		std::string str_sqlselect = "select taskid,tasktype,moodtype,speakspeed,taskname,state,progress,createtime,humanid,humanname,ssmltext,audiofile,audioformat,audiolength,videofile,finalvideo,videoformat,videolength,videowidth,videoheight,videofps,background,front_xpos,front_ypos,front_scale,front_rotation from sbt_doctask";
		str_sqlselect += str_where;
		str_sqlselect += str_order;
		str_sqlselect += str_limit;
		_debug_to(loger_mysql, 0, ("[gettaskhistoryinfo] sql: %s\n"), str_sqlselect.c_str());
		mysql_query(&mysql, "SET NAMES UTF8");				//support chinese text
		if (!mysql_query(&mysql, str_sqlselect.c_str()))	//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			_debug_to(loger_mysql, 0, ("[gettaskhistoryinfo] rownum=%d,colnum=%d\n"), rownum, colnum);

			MYSQL_ROW row;							//table row data
			while (row = mysql_fetch_row(result))
			{
				if (colnum >= 25)//keep right
				{
					int i = 0;
					int row_taskid = atoi(row_value(row[i++]).c_str());
					int row_tasktype = atoi(row_value(row[i++]).c_str());
					int row_moodtype = atoi(row_value(row[i++]).c_str());
					double row_speakspeed = atof(row_value(row[i++]).c_str());
					std::string row_taskname = row_value(row[i++]);   std::string taskname_ansi; utf8_to_ansi(row_taskname.c_str(), row_taskname.length(), taskname_ansi);
					int row_taskstate = atoi(row_value(row[i++]).c_str());
					int row_taskprogress = atoi(row_value(row[i++]).c_str());
					std::string row_createtime = row_value(row[i++]);
					std::string row_humanid = row_value(row[i++]);
					std::string row_humanname = row_value(row[i++]);  std::string humanname_ansi; utf8_to_ansi(row_humanname.c_str(), row_humanname.length(), humanname_ansi);
					std::string row_ssmltext = row_value(row[i++]);   std::string ssmltext_ansi; utf8_to_ansi(row_ssmltext.c_str(), row_ssmltext.length(), ssmltext_ansi); ssmltext_ansi = str_replace(ssmltext_ansi, "\n", "");
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
					std::string row_background = row_value(row[i++]).c_str();
					int			row_front_xpos = atoi(row_value(row[i++]).c_str());
					int			row_front_ypos = atoi(row_value(row[i++]).c_str());
					int			row_front_scale = atoi(row_value(row[i++]).c_str());
					int			row_front_rotation = atoi(row_value(row[i++]).c_str());
					
					taskinfo historyitem;
					historyitem.taskid = row_taskid;
					historyitem.tasktype = row_tasktype;
					historyitem.moodtype = row_moodtype;
					historyitem.speakspeed = row_speakspeed;
					historyitem.taskname = taskname_ansi;//a=coding convert
					historyitem.taskstate = row_taskstate;
					historyitem.taskprogress = row_taskprogress;
					historyitem.createtime = row_createtime;
					historyitem.humanid = row_humanid;
					historyitem.humanname = humanname_ansi;//b=coding convert
					historyitem.ssmltext = ssmltext_ansi;//c=coding convert
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
					historyitem.background_path = row_background;
					historyitem.front_XPos = row_front_xpos;
					historyitem.front_YPos = row_front_ypos;
					historyitem.front_Scale = row_front_scale;
					historyitem.front_Rotation = row_front_rotation;

					vectaskhistory.push_back(historyitem);
				}
			}
			mysql_free_result(result);				//free result

			_debug_to(loger_mysql, 0, ("[gettaskhistoryinfo]MySQL select taskhistory success\n"));
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[gettaskhistoryinfo]MySQL select taskhistory failed: %s\n"), error.c_str());
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
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[isexisttask]MySQL database connect failed: %s\n"), error.c_str());
			return false;
		}

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "select * from sbt_doctask where taskid = %d", taskid);
		_debug_to(loger_mysql, 0, ("[isexisttask] sql: %s\n"), sql_buff);
		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			_debug_to(loger_mysql, 0, ("[isexisttask] rownum=%d,colnum=%d\n"), rownum, colnum);

			if (rownum >= 1)//keep right
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
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[isexisttask]MySQL database connect failed: %s\n"), error.c_str());
			return false;
		}

		char sql_buff[BUFF_SZ] = { 0 };
		//check textguid
		snprintf(sql_buff, BUFF_SZ, "select taskid,progress,audiofile from sbt_doctask where textguid = '%s'", textguid.c_str());
		_debug_to(loger_mysql, 0, ("[isexisttask] sql: %s\n"), sql_buff);
		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			_debug_to(loger_mysql, 0, ("[isexisttask]sql=%s, row=%d, col=%d\n"), sql_buff, rownum, colnum);

			MYSQL_ROW row = mysql_fetch_row(result);//table row data
			if (rownum >= 1 && colnum >= 3)//keep right
			{
				int i = 0;
				taskid = atoi(row[i++]);
				progress = atoi(row[i++]);
				audiofile = row_value(row[i++]);
				if (progress == 100 && is_existfile(audiofile.c_str()))
					ret = true;
			}
			mysql_free_result(result);				//free result
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[isexisttask]MySQL select textguid failed: %s\n"), error.c_str());
		}

		//update taskname
		if (ret)
		{
			std::string taskname_utf8 = "test taskname";
			ansi_to_utf8(taskname.c_str(), taskname.length(), taskname_utf8);
			snprintf(sql_buff, BUFF_SZ, "update sbt_doctask set taskname='%s' where textguid = '%s'", taskname_utf8.c_str(), textguid.c_str());//update
			_debug_to(loger_mysql, 0, ("[isexisttask] sql: %s\n"), sql_buff);
			mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
			if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
			{
				_debug_to(loger_mysql, 0, ("[isexisttask]task %d, update taskname success\n"), taskid);
			}
			else
			{
				ret = false;
				std::string error = mysql_error(&mysql);
				_debug_to(loger_mysql, 1, ("[isexisttask]task %d, update taskname failed: %s\n"), taskid, error.c_str());
			}
		}

		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
	}

	//
	bool addtasksource(tasksourceinfo tasksourceitem, bool update)
	{
		if (simulation) return true;

		bool ret = true;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[addtasksource]MySQL database connect failed: %s\n"), error.c_str());
			return false;
		}

		//a=coding convert
		std::string sourcepath_utf8 = tasksourceitem.sourcepath;
		ansi_to_utf8(tasksourceitem.sourcepath.c_str(), tasksourceitem.sourcepath.length(), sourcepath_utf8);
		tasksourceitem.sourcepath = sourcepath_utf8;

		std::string sourcekeyframe_utf8 = tasksourceitem.sourcekeyframe;
		ansi_to_utf8(tasksourceitem.sourcekeyframe.c_str(), tasksourceitem.sourcekeyframe.length(), sourcekeyframe_utf8);
		tasksourceitem.sourcekeyframe = sourcekeyframe_utf8;

		//
		std::string OPERATION = "";
		char sql_buff[BUFF_SZ] = { 0 };
		if (update)
		{
			//update
			OPERATION = "UPDATE";
			snprintf(sql_buff, BUFF_SZ, "update sbt_doctasksource set sourcetype=%d,sourcekeyframe='%s',createtime='%s'where sourcepath='%s'",
				tasksourceitem.sourcetype, tasksourceitem.sourcekeyframe.c_str(), tasksourceitem.createtime.c_str(),
				tasksourceitem.sourcepath.c_str());
		}
		else
		{
			//insert
			OPERATION = "INSERT";
			int next_id = newgetsequencenextvalue("sbt_doctasksource", &mysql);
			snprintf(sql_buff, BUFF_SZ, "insert into sbt_doctasksource (id,sourcetype,sourcepath,sourcekeyframe,createtime) values(%d, %d, '%s', '%s', '%s')",
				next_id, 
				tasksourceitem.sourcetype, tasksourceitem.sourcepath.c_str(), tasksourceitem.sourcekeyframe.c_str(), tasksourceitem.createtime.c_str());
		}
		
		//run sql
		_debug_to(loger_mysql, 0, ("[addtasksource] sql: %s\n"), sql_buff);
		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			_debug_to(loger_mysql, 0, ("[addtasksource]%s source[%s] success\n"), OPERATION.c_str(), tasksourceitem.sourcepath.c_str());
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[addtasksource]%s source[%s] failed: %s\n"), OPERATION.c_str(), tasksourceitem.sourcepath.c_str(), error.c_str());
		}

		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
	}
	bool gettasksourcelist(VEC_TASKSOURCEINFO& vectasksource)
	{
		if (simulation) return 0;

		bool ret = true;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[gettasksourcelist]MySQL database connect failed: %s\n"), error.c_str());
			return false;
		}

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "select id,sourcetype,sourcepath,sourcekeyframe,createtime from sbt_doctasksource");//select	

		_debug_to(loger_mysql, 0, ("[gettasksourcelist] sql: %s\n"), sql_buff);
		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			_debug_to(loger_mysql, 0, ("[gettasksourcelist] rownum=%d,colnum=%d\n"), rownum, colnum);

			MYSQL_ROW row;							//table row data
			while (row = mysql_fetch_row(result))
			{
				if (colnum >= 5) //keep right
				{
					int i = 0;
					int row_id = atoi(row_value(row[i++]).c_str());
					int row_sourcetype = atoi(row_value(row[i++]).c_str());
					std::string row_sourcepath = row_value(row[i++]); std::string sourcepath_ansi; utf8_to_ansi(row_sourcepath.c_str(), row_sourcepath.length(), sourcepath_ansi);
					std::string row_sourcekeyframe = row_value(row[i++]); std::string sourcekeyframe_ansi; utf8_to_ansi(row_sourcekeyframe.c_str(), row_sourcekeyframe.length(), sourcekeyframe_ansi);
					std::string row_createtime = row_value(row[i++]);

					tasksourceinfo tasksourceitem;
					tasksourceitem.id = row_id;
					tasksourceitem.sourcetype = row_sourcetype;
					tasksourceitem.sourcepath = sourcepath_ansi;
					tasksourceitem.sourcekeyframe = sourcekeyframe_ansi;
					tasksourceitem.createtime = row_createtime;
					vectasksource.push_back(tasksourceitem);
				}
			}
			mysql_free_result(result);				//free result

			_debug_to(loger_mysql, 0, ("[gettasksourcelist]MySQL select tasksourcelist success\n"));
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[gettasksourcelist]MySQL select tasksourcelist failed: %s\n"), error.c_str());
		}
		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
	}

	bool isexisttasksource_path(std::string sourcepath)
	{
		if (simulation) return true;
		if (sourcepath.empty()) return false;

		bool ret = false;
		MYSQL mysql;
		mysql_init(&mysql);		//inti MYSQL

		//=====================
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[isexisttasksource_path]MySQL database connect failed: %s\n"), error.c_str());
			return false;
		}

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "select * from sbt_doctasksource where sourcepath = '%s'", sourcepath.c_str());
		_debug_to(loger_mysql, 0, ("[isexisttasksource_path] sql: %s\n"), sql_buff);
		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			_debug_to(loger_mysql, 0, ("[isexisttasksource_path] rownum=%d,colnum=%d\n"), rownum, colnum);

			if (rownum >= 1)//keep right
				ret = true;

			mysql_free_result(result);				//free result
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
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[getmergeprogress]MySQL database connect failed: %s\n"), error.c_str());
			return 0;
		}

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "select sbt_doctask.progress from sbt_doctask where taskid = %d", taskid);
		_debug_to(loger_mysql, 0, ("[getmergeprogress] sql: %s\n"), sql_buff);
		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			_debug_to(loger_mysql, 0, ("[getmergeprogress] rownum=%d,colnum=%d\n"), rownum, colnum);

			MYSQL_ROW row = mysql_fetch_row(result);//table row data
			if (rownum >= 1 && colnum >= 1)//keep right
			{
				nprogress = atoi(row[0]);
			}
			else
			{
				ret = false;
				_debug_to(loger_mysql, 1, ("[getmergeprogress]task %d, select mergetask count not only one\n"), taskid);
			}
			mysql_free_result(result);				//free result

			_debug_to(loger_mysql, 0, ("[getmergeprogress]task %d, select mergetask progress success, progress=%d\n"), taskid, nprogress);
		}
		else 
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[getmergeprogress]task %d, select progress failed: %s\n"), taskid, error.c_str());
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
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[setmergeprogress]MySQL database connect failed: %s\n"), error.c_str());
			return false;
		}

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "select sbt_doctask.progress from sbt_doctask where taskid = %d", taskid);
		_debug_to(loger_mysql, 0, ("[setmergeprogress] sql: %s\n"), sql_buff);
		mysql_query(&mysql, "SET NAMES UTF8");			//support chinese text
		if (!mysql_query(&mysql, sql_buff))				//success return 0,failed return random number
		{
			MYSQL_RES* result;							//table data struct
			result = mysql_store_result(&mysql);		//sava dada to result

			snprintf(sql_buff, BUFF_SZ, "update sbt_doctask set progress=%d where taskid = %d", nprogress, taskid);	//update
			_debug_to(loger_mysql, 0, ("[setmergeprogress] sql: %s\n"), sql_buff);
			mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
			if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
			{
				_debug_to(loger_mysql, 0, ("[setmergeprogress]task %d, update progress success, progress=%d\n"), taskid, nprogress);
			}
			else
			{
				ret = false;
				std::string error = mysql_error(&mysql);
				_debug_to(loger_mysql, 1, ("[setmergeprogress]task %d, update progress failed: %s\n"), taskid, error.c_str());
			}
			mysql_free_result(result);					//free result
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[setmergeprogress]task %d, select progress failed: %s\n"), taskid, error.c_str());
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
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[getmergestate]MySQL database connect failed: %s\n"), error.c_str());
			return 0;
		}

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "select sbt_doctask.state from sbt_doctask where taskid = %d", taskid);
		_debug_to(loger_mysql, 0, ("[getmergestate] sql: %s\n"), sql_buff);
		mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
		if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
		{
			MYSQL_RES* result;						//table data struct
			result = mysql_store_result(&mysql);    //sava dada to result
			int rownum = mysql_num_rows(result);	//get row number
			int colnum = mysql_num_fields(result);  //get col number
			_debug_to(loger_mysql, 0, ("[getmergestate] rownum=%d,colnum=%d\n"), rownum, colnum);

			MYSQL_ROW row = mysql_fetch_row(result);//table row data
			if (rownum >= 1 && colnum >= 1)//keep right
			{
				nstate = atoi(row[0]);
			}
			mysql_free_result(result);				//free result

			_debug_to(loger_mysql, 0, ("[getmergestate]task %d, select mergetask state success, state=%d\n"), taskid, nstate);
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[getmergestate]task %d, select mergetask state failed: %s\n"), taskid, error.c_str());
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
		if (!mysql_real_connect(&mysql, g_database_ip.c_str(), g_database_username.c_str(), g_database_password.c_str(), g_database_dbname.c_str(), g_database_port, NULL, 0)) //connect mysql
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[setmergestate]MySQL database connect failed: %s\n"), error.c_str());
			return false;
		}

		char sql_buff[BUFF_SZ] = { 0 };
		snprintf(sql_buff, BUFF_SZ, "select sbt_doctask.state from sbt_doctask where taskid = %d", taskid);
		_debug_to(loger_mysql, 0, ("[setmergestate] sql: %s\n"), sql_buff);
		mysql_query(&mysql, "SET NAMES UTF8");			//support chinese text
		if (!mysql_query(&mysql, sql_buff))				//success return 0,failed return random number
		{
			MYSQL_RES* result;							//table data struct
			result = mysql_store_result(&mysql);		//sava dada to result

			snprintf(sql_buff, BUFF_SZ, "update sbt_doctask set state=%d where taskid = %d", nstate, taskid);	//update
			_debug_to(loger_mysql, 0, ("[setmergestate] sql: %s\n"), sql_buff);
			mysql_query(&mysql, "SET NAMES UTF8");		//support chinese text
			if (!mysql_query(&mysql, sql_buff))			//success return 0,failed return random number
			{
				_debug_to(loger_mysql, 0, ("[setmergestate]task %d, update taskstate success, state=%d\n"), taskid, nstate);
			}
			else
			{
				ret = false;
				std::string error = mysql_error(&mysql);
				_debug_to(loger_mysql, 1, ("[setmergestate]task %d, update taskstate failed: %s\n"), taskid, error.c_str());
			}
			mysql_free_result(result);					//free result
		}
		else
		{
			ret = false;
			std::string error = mysql_error(&mysql);
			_debug_to(loger_mysql, 1, ("[setmergestate]task %d, select state failed: %s\n"), taskid, error.c_str());
		}

		//=====================
		mysql_close(&mysql);	//close connect

		return ret;
	}
};