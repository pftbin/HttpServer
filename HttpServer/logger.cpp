#include "logger.h"

#include <direct.h>
#include <sys/stat.h>
static bool create_directory(const char* path)
{
    struct stat st = { 0 };
    if (stat(path, &st) == -1)
    {
#ifdef _WIN32
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

FileWriter::FileWriter(const std::string& filename) 
    : log_level(0)
    , filename_(timeprefix()+filename)
{
    std::string folder = timefolder();
    if (create_directory(folder.c_str()))
        filename_ = folder + "/" + filename_;

    file_.open(filename_, std::ios::out | std::ios::app);
}

FileWriter::~FileWriter() 
{
    file_.close();
}

void FileWriter::setlevel(int level)
{
    log_level = (level >= 0) ? (level) : (0);
}

void FileWriter::write(const std::string& str) 
{
    std::lock_guard<std::mutex> lock(mutex_);//局部变量，退出函数自动释放

    buffer_ += localtime();
    buffer_ += "\t";
    buffer_ += str;
    buffer_ += "\n";

    file_ << buffer_;
    file_.flush();
    buffer_.clear();
}

void writeToFile(FileWriter& writer, int level, const std::string& str) 
{
    if(level >= writer.log_level)
        writer.write(str);
}

