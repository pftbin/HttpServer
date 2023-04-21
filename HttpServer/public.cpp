#include "public.h"

//
void unicode_to_utf8(const wchar_t* in, size_t len, std::string& out)
{
#if defined WIN32
    int out_len = ::WideCharToMultiByte(CP_UTF8, 0, in, static_cast<int>(len), nullptr, 0, nullptr, nullptr);
    if (out_len > 0) {
        char* lpBuf = new char[static_cast<unsigned int>(out_len + 1)];
        if (lpBuf) {
            memset(lpBuf, 0, static_cast<unsigned int>(out_len + 1));
            int return_len = ::WideCharToMultiByte(CP_UTF8, 0, in, static_cast<int>(len), lpBuf, out_len, nullptr, nullptr);
            if (return_len > 0) out.assign(lpBuf, static_cast<unsigned int>(return_len));
            delete[]lpBuf;
        }
    }
#else
    /*if (wcslen(in) <= 0 || len <= 0) return;
    std::lock_guard<std::mutex> guard(g_ConvertCodeMutex);
    size_t w_len = len * 4 + 1;
    setlocale(LC_CTYPE, "en_US.UTF-8");
    std::unique_ptr<char[]> p(new char[w_len]);
    size_t return_len = wcstombs(p.get(), in, w_len);
    if (return_len > 0) out.assign(p.get(), return_len);
    setlocale(LC_CTYPE, "C");*/

    size_t w_len = len * (sizeof(wchar_t) / sizeof(char)) + 1U;
    char* save = new char[w_len];
    memset(save, '\0', w_len);
    iconv_convert("UTF-32", "UTF-8//IGNORE", save, w_len, (char*)(in), w_len);
    out = save;
    delete[]save;
#endif
}

void utf8_to_unicode(const char* in, size_t len, std::wstring& out)
{
#if defined WIN32
    int out_len = ::MultiByteToWideChar(CP_UTF8, 0, in, static_cast<int>(len), nullptr, 0);
    if (out_len > 0) {
        wchar_t* lpBuf = new wchar_t[static_cast<unsigned int>(out_len + 1)];
        if (lpBuf) {
            memset(lpBuf, 0, (static_cast<unsigned int>(out_len + 1)) * sizeof(wchar_t));
            int return_len = ::MultiByteToWideChar(CP_UTF8, 0, in, static_cast<int>(len), lpBuf, out_len);
            if (return_len > 0) out.assign(lpBuf, static_cast<unsigned int>(return_len));
            delete[]lpBuf;
        }
    }
#else
    /*if (strlen(in) <= 0 || len <= 0) return;
    std::lock_guard<std::mutex> guard(g_ConvertCodeMutex);
    setlocale(LC_CTYPE, "en_US.UTF-8");
    std::unique_ptr<wchar_t[]> p(new wchar_t[sizeof(wchar_t) * (len + 1)]);
    size_t return_len = mbstowcs(p.get(), in, len + 1);
    if (return_len > 0) out.assign(p.get(), return_len);
    setlocale(LC_CTYPE, "C");*/

    size_t w_len = len * (sizeof(wchar_t) / sizeof(char)) + 1U;
    char* save = new char[w_len];
    memset(save, '\0', w_len);
    iconv_convert("UTF-8", "UTF-32//IGNORE", save, w_len, (char*)(in), len);
    out = (wchar_t*)save + 1;
    delete[]save;
#endif
}

void ansi_to_unicode(const char* in, size_t len, std::wstring& out)
{
#if defined WIN32
    int out_len = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, in, static_cast<int>(len), nullptr, 0);
    if (out_len > 0) {
        wchar_t* lpBuf = new wchar_t[static_cast<unsigned int>(out_len + 1)];
        if (lpBuf) {
            memset(lpBuf, 0, (static_cast<unsigned int>(out_len + 1)) * sizeof(wchar_t));
            int return_len = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, in, static_cast<int>(len), lpBuf, out_len);
            if (return_len > 0) out.assign(lpBuf, static_cast<unsigned int>(return_len));
            delete[]lpBuf;
        }
    }
#else
    /*mbstate_t state;
    memset (&state, '\0', sizeof (state));
    std::lock_guard<std::mutex> guard(g_ConvertCodeMutex);
    //setlocale(LC_CTYPE, "zh_CN.UTF-8");
    setlocale(LC_CTYPE, "en_US.UTF-8");
    size_t out_len= mbsrtowcs(nullptr, &in, 0, &state);
    if (out_len > 0 &&  out_len < UINT_MAX && len > 0)
    {
        std::unique_ptr<wchar_t[]> lpBuf(new wchar_t[sizeof(wchar_t) * (out_len + 1)]);
        size_t return_len = mbsrtowcs(lpBuf.get(), &in, out_len+1, &state);
        if (return_len > 0) out.assign(lpBuf.get(), return_len);
    }
    setlocale(LC_CTYPE, "C");*/

    size_t w_len = len * (sizeof(wchar_t) / sizeof(char)) + 1U;
    char* save = new char[w_len];
    memset(save, '\0', w_len);

    iconv_convert("GBK", "UTF-32//IGNORE", save, w_len, (char*)(in), len);
    out = (wchar_t*)save + 1;
    delete[]save;
#endif
}

void unicode_to_ansi(const wchar_t* in, size_t len, std::string& out)
{
#if defined WIN32
    int out_len = ::WideCharToMultiByte(CP_ACP, 0, in, static_cast<int>(len), nullptr, 0, nullptr, nullptr);
    if (out_len > 0) {
        char* lpBuf = new char[static_cast<unsigned int>(out_len + 1)];
        if (lpBuf) {
            memset(lpBuf, 0, static_cast<unsigned int>(out_len + 1));
            int return_len = ::WideCharToMultiByte(CP_ACP, 0, in, static_cast<int>(len), lpBuf, out_len, nullptr, nullptr);
            if (return_len > 0) out.assign(lpBuf, static_cast<unsigned int>(return_len));
            delete[]lpBuf;
        }
    }
#else
    /*mbstate_t state;
    memset(&state, '\0', sizeof(state));
    std::lock_guard<std::mutex> guard(g_ConvertCodeMutex);
    setlocale(LC_CTYPE, "en_US.UTF-8");
    size_t out_len = wcsrtombs(nullptr, &in, 0, &state);
    if (out_len > 0 && out_len < UINT_MAX && len > 0)
    {
        std::unique_ptr<char[]> lpBuf(new char[sizeof(char) *(out_len + 1)]);
        size_t return_len = wcsrtombs(lpBuf.get(), &in, out_len+1, &state);//wcstombs(lpBuf.get(), in, len + 1);
        if (return_len > 0) out.assign(lpBuf.get(), return_len);
    }
    setlocale(LC_CTYPE, "C");*/

    size_t w_len = len * (sizeof(wchar_t) / sizeof(char)) + 1U;
    char* save = new char[w_len];
    memset(save, '\0', w_len);
    iconv_convert("UTF-32", "GBK//IGNORE", save, w_len, (char*)(in), w_len - 1U);
    out = save;
    delete[]save;
#endif
}

void ansi_to_utf8(const char* in, size_t len, std::string& out)
{
#if defined WIN32
    std::wstring strUnicode;
    ansi_to_unicode(in, len, strUnicode);
    return unicode_to_utf8(strUnicode.c_str(), strUnicode.length(), out);
#else
    /*std::wstring strUnicode;
    ansi_to_unicode(in, len, strUnicode);
    return unicode_to_utf8(strUnicode.c_str(), strUnicode.length(), out);*/

    size_t w_len = len * (sizeof(wchar_t) / sizeof(char));
    char* save = new char[w_len];
    memset(save, '\0', w_len);
    iconv_convert("GBK", "UTF-8//IGNORE", save, w_len, (char*)(in), len);
    out = save;
    delete[]save;
#endif
}

void utf8_to_ansi(const char* in, size_t len, std::string& out)
{
#if defined WIN32
    std::wstring strUnicode;
    utf8_to_unicode(in, len, strUnicode);
    return unicode_to_ansi(strUnicode.c_str(), strUnicode.length(), out);
#else
    /*std::wstring strUnicode;
    utf8_to_unicode(in, len, strUnicode);
    return unicode_to_ansi(strUnicode.c_str(), strUnicode.length(), out);*/

    size_t w_len = len * (sizeof(wchar_t) / sizeof(char));
    char* save = new char[w_len];
    memset(save, '\0', w_len);
    iconv_convert("UTF-8", "GBK//IGNORE", save, w_len, (char*)(in), len);
    out = save;
    delete[]save;
#endif
}

//
int globalStrToIntDef(const LPTSTR valuechar, int& value, int defaultvalue, int base)
{
    if (base < 2 || base > 16) base = 10;
    value = defaultvalue;
#if defined(UNICODE) || defined(_UNICODE)
    wchar_t* endptr;
    errno = 0;
    long val = wcstol(valuechar, &endptr, base);
    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0))
    {
        return -1;
    }

    if (endptr == valuechar)//没有找到有效的字符
    {
        return -1;
    }
    if (*endptr != '\0')
    {
        //value = (int)val;
        //return 0;
        return -1;
    }
#else
    char* endptr;
    errno = 0;
    long val = strtol(valuechar, &endptr, base);
    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0))
    {
        return -1;
    }

    if (endptr == const_cast<char*>(valuechar))//(char*)valuechar)//没有找到有效的字符
    {
        return -1;
    }
    if (*endptr != '\0')
    {
        //value = (int)val;
        //return 0;
        return -1;
    }
#endif
    value = static_cast<int>(val);//(int)val;
    return 1;
}

int globalStrToInt(const LPTSTR valuechar, int defaultvalue, int base)
{
    if (base < 2 || base > 16) base = 10;
#if defined(UNICODE) || defined(_UNICODE)
    wchar_t* endptr;
    errno = 0;
    long val = wcstol(valuechar, &endptr, base);
    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0))
    {
        return defaultvalue;
    }

    if (endptr == valuechar)//没有找到有效的字符
    {
        return defaultvalue;
    }
    if (*endptr != '\0')
    {
        return defaultvalue;
    }
#else
    char* endptr;
    errno = 0;
    long val = strtol(valuechar, &endptr, base);
    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0))
    {
        return defaultvalue;
    }

    if (endptr == const_cast<char*>(valuechar))//(char*)valuechar)//没有找到有效的字符
    {
        return defaultvalue;
    }
    if (*endptr != '\0')
    {
        return -1;
    }
#endif
    return static_cast<int>(val);
}

void globalSpliteString(const std::string& str, std::vector<std::string>& strVector, std::string splitStr, int maxCount)
{
    std::string::size_type pos = 0U;
    std::string::size_type pre = 0U;
    std::string::size_type len = str.length();
    if (splitStr.empty()) splitStr = ("|");
    std::string::size_type splitLen = splitStr.length();
    std::string curStr;
    if (maxCount < 1) maxCount = 1;
    do {
        if (static_cast<int>(strVector.size()) >= maxCount - 1) {//如果超过最大个数则不解析最后的分隔字符串，直接将其放在最后一个里面
            curStr = str.substr(pre, len - pre);
            strVector.push_back(curStr);
            break;
        }
        pos = str.find(splitStr, pre);
        if (pos == 0U) {
            pre = pos + splitLen;
            continue;
        }
        else if (pos == std::string::npos) {
            curStr = str.substr(pre, len - pre);
        }
        else {
            curStr = str.substr(pre, pos - pre);
            pre = pos + splitLen;
        }
        strVector.push_back(curStr);
    } while (pos != std::string::npos && pos != (len - splitLen));
}

void globalCreateGUID(std::string& GuidStr)
{
    std::string result = md5::getStringMD5(gettimecode());
    GuidStr = result;
}

//
unsigned short Checksum(const unsigned short* buf, int size)
{
    unsigned long sum = 0;

    if (buf != (unsigned short*)NULL)
    {
        while (size > 1)
        {
            sum += *(buf++);
            if (sum & 0x80000000)
            {
                sum = (sum & 0xffff) + (sum >> 16);
            }
            size -= sizeof(unsigned short);
        }

        if (size)
        {
            sum += (unsigned short)*(const unsigned char*)buf;
        }
    }

    while (sum >> 16)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return (unsigned short)~sum;
}

std::string str_replace(std::string str, std::string old, std::string now)
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

std::string getnodevalue(std::string info, std::string nodename)
{
    info.erase(std::remove(info.begin(), info.end(), '\n'), info.end());
    std::string result = "";
    std::string start = "", end = "";
    char buff[256] = { 0 };
    snprintf(buff, 256, "<%s>", nodename.c_str()); start = buff;
    snprintf(buff, 256, "</%s>", nodename.c_str()); end = buff;

    int ns = info.find(start);
    int ne = info.find(end);
    if (ns != std::string::npos && ne != std::string::npos && ne > ns)
    {
        int offset = nodename.length();
        result = info.substr(ns + offset + 2, ne - ns - offset - 2);
    }

    return result;
}

std::string gettimecode()
{
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    char buf[100] = { 0 };
    //std::strftime(buf, sizeof(buf), "%Y-%m-%d %I:%M:%S", std::localtime(&now));//12
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));//24
    return buf;
}

long long   gettimecount()
{
    long long set = 0;
    time_t timep;
    set = time(&timep);

    return set;
}

std::string getmessageid()
{
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    char buf[100] = { 0 };
    std::strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", std::localtime(&now));//24
    return buf;
}

bool is_existfile(std::string& name)
{
#if defined WIN32
    return (_taccess(name.c_str(), 0) == 0);
#else
    return (access(name.c_str(), 0) == 0);
#endif
}

std::string get_file_extension(char* filename)
{
    std::string ret_str;
    char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) 
        return ret_str;
    ret_str = dot + 1;

    return ret_str;
}

char* read_file(const char* filename, long& file_size)
{
    FILE* fp;
    char* buffer = nullptr;

    fp = fopen(filename, "rb");
    if (fp == nullptr) {
        _debug_to(loger_public, 1, ("Error opening file %s\n"), filename);
        return nullptr;
    }

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    buffer = (char*)malloc(file_size + 1);
    if (buffer == nullptr) {
        _debug_to(loger_public, 1, ("Error allocating memory for file %s\n"), filename);
        return nullptr;
    }

    fread(buffer, file_size, 1, fp);
    fclose(fp);

    buffer[file_size] = '\0';
    return buffer;
}




