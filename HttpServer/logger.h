#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>

class FileWriter
{
public:
    FileWriter(const std::string& filename);
    ~FileWriter();

    void setlevel(int level);
    void write(const std::string& str);

public:
    int log_level;

private:
    std::string filename_;
    std::ofstream file_;
    std::string buffer_;
    std::mutex mutex_;
};

void writeToFile(FileWriter& writer, int level, const std::string& str);


