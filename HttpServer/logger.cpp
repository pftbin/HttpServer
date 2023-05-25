#include "logger.h"

#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>

//
#include <direct.h>
#include <sys/stat.h>
static bool _createdirectory(const char* path)
{
    struct stat st = { 0 };
    if (stat(path, &st) == -1)
    {
#if defined WIN32
        if (mkdir(path) != 0)
        {
            return false;
        }
#else
        if (mkdir(path, 0700) != 0)
        {
            return false;
        }
#endif
    }

    return true;
}

static std::string localtime()
{
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    char buf[100] = { 0 };
    //std::strftime(buf, sizeof(buf), "%Y-%m-%d %I:%M:%S", std::localtime(&now));//12
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));//24
    return buf;
}

static std::string timefolder()
{
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    char buf[100] = { 0 };
    std::strftime(buf, sizeof(buf), "%Y%m%d", std::localtime(&now));//24
    return buf;
}

static std::string timeprefix()
{
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    char buf[100] = { 0 };
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H_%M_%S", std::localtime(&now));//24
    return buf;
}

//
pthread_mutex_t mutex_list;
pthread_t threadid_writelog;
void* pthread_writelog(void* arg)
{
    FileWriter* pWriter = (FileWriter*)arg;
    if (pWriter)
    {
        while (true)
        {
            _sleep(10);
            if (!pWriter->list_.empty())
            {
                pthread_mutex_lock(&mutex_list);
                std::string log_str = pWriter->list_.front();
                pWriter->list_.pop_front();
                pthread_mutex_unlock(&mutex_list);

                pWriter->writelog(log_str);
            }
        }
    }

    return nullptr;
}

FileWriter::FileWriter(const std::string& filename, int level, bool async) 
    : log_level(level)
    , log_async(async)
    , filename_(timeprefix()+filename)
{
    std::string folder = timefolder();
    if (_createdirectory(folder.c_str()))
        filename_ = folder + "/" + filename_;

    pthread_mutex_init(&mutex_list, NULL);
    int ret = pthread_create(&threadid_writelog, nullptr, pthread_writelog, this);
    if (ret != 0)
        log_async = false;

    file_.open(filename_, std::ios::out | std::ios::app);
}

FileWriter::~FileWriter() 
{
    file_.close();
}

void FileWriter::writelog(const std::string& log_str)
{
    std::lock_guard<std::mutex> lock(mutex_);//局部变量，退出函数自动释放
    
    file_ << log_str;
    file_.flush();
}

void FileWriter::writelog_async(const std::string& log_str)
{
    pthread_mutex_lock(&mutex_list);
    list_.push_back(log_str);
    pthread_mutex_unlock(&mutex_list);
}

void writeToFile(FileWriter& writer, int level, const std::string& str) 
{
    std::string format_str;
    format_str += localtime();
    format_str += "\t";
    format_str += str;
    
    char backchar = str[strlen(str.c_str()) - 1];
    if(backchar!='\n') format_str += "\n";

    if (level >= writer.log_level)
    {
        if(writer.log_async)
            writer.writelog_async(format_str);
        else
            writer.writelog(format_str);
    }   
}

