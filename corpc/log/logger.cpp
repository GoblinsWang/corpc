#ifdef _WIN32
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "logger.h"

using namespace corpc;

Logger *Logger::singleObject = nullptr;
std::mutex Logger::mutex_log;
std::mutex Logger::mutex_file;
std::mutex Logger::mutex_queue;
std::mutex Logger::mutex_terminal;

Logger::Logger()
{
    initLogConfig();
}

Logger::~Logger()
{
}

Logger *Logger::getInstance()
{
    // mutex_log.lock();
    if (singleObject == nullptr)
    {
        singleObject = new Logger();
    }
    // mutex_log.unlock();
    return singleObject;
}

void Logger::initLogConfig()
{
#ifdef __linux__
    coutColor["Error"] = "\e[1;31m";
    coutColor["Warn"] = "\e[1;35m";
    coutColor["Info"] = "\e[1;34m";
    coutColor["Debug"] = "\e[1;32m";
    coutColor["Trace"] = "\e[1;37m";
#elif _WIN32
    coutColor["Error"] = "";
    coutColor["Warn"] = "";
    coutColor["Info"] = "";
    coutColor["Debug"] = "";
    coutColor["Trace"] = "";
#endif

    std::map<std::string, std::string *> flogConfInfo;
    flogConfInfo["logSwitch"] = &this->settings.logSwitch;
    flogConfInfo["logFileSwitch"] = &this->settings.logFileSwitch;
    flogConfInfo["logTerminalSwitch"] = &this->settings.logTerminalSwitch;
    flogConfInfo["logFileQueueSwitch"] = &this->settings.logFileQueueSwitch;
    flogConfInfo["logName"] = &this->settings.logName;
    flogConfInfo["logFilePath"] = &this->settings.logFilePath;
    flogConfInfo["logMixSize"] = &this->settings.logMixSize;
    flogConfInfo["logBehavior"] = &this->settings.logBehavior;
    flogConfInfo["logOutputLevelFile"] = &this->settings.logOutputLevelFile;
    flogConfInfo["logOutputLevelTerminal"] = &this->settings.logOutputLevelTerminal;

    bool isOpen = true;
    std::string str;
    std::ifstream file;
    char str_c[100] = {0};
#ifdef __linux__
    file.open("../log.conf");
#elif _WIN32
    file.open("log.conf");
#endif
    if (!file.is_open())
    {
        isOpen = false;
        std::cout << "File open failed" << std::endl;
    }
    while (getline(file, str))
    {
        if (!str.length())
        {
            continue;
        }
        std::string str_copy = str;
        int j = 0;
        for (unsigned int i = 0; i < str.length(); i++)
        {
            if (str[i] == ' ')
                continue;
            str_copy[j] = str[i];
            j++;
        }
        str_copy.erase(j);
        if (str_copy[0] != '#')
        {
            sscanf(str_copy.data(), "%[^=]", str_c);
            auto iter = flogConfInfo.find(str_c);
            if (iter != flogConfInfo.end())
            {
                sscanf(str_copy.data(), "%*[^=]=%s", str_c);
                *iter->second = str_c;
            }
            else
            {
            }
        }
    }
    file.close();

    bindFileCoutMap("5", fileType::Error);
    bindFileCoutMap("4", fileType::Warn);
    bindFileCoutMap("3", fileType::Info);
    bindFileCoutMap("2", fileType::Debug);
    bindFileCoutMap("1", fileType::Trace);

    bindTerminalCoutMap("5", terminalType::Error);
    bindTerminalCoutMap("4", terminalType::Warn);
    bindTerminalCoutMap("3", terminalType::Info);
    bindTerminalCoutMap("2", terminalType::Debug);
    bindTerminalCoutMap("1", terminalType::Trace);

    std::string filePathAndName = getFilePathAndName();
    std::string filePath = getFilePath();
    std::cout << filePathAndName << " : " << filePath << std::endl;
    if (settings.logFileSwitch == SWITCH_ON)
    {
        // 检查路径
        filemanagement.createFilePath(filePath);
        // 检测文件有效性
        if (!filemanagement.verifyFileExistence(filePathAndName))
        {
            filemanagement.createFile(filePathAndName);
        }
        else
        {
            long fileSize = filemanagement.verifyFileSize(filePathAndName);
            if (fileSize > (long)atoi(settings.logMixSize.data()) * MEGABYTES && settings.logBehavior == "1")
            {
                std::string newFileName = getFilePathAndNameAndTime();
                filemanagement.fileRename(filePathAndName, newFileName);
                filemanagement.createFile(filePathAndName);
            }
        }
    }
    if (isOpen)
    {
        ConfInfoPrint();
    }
    return;
}

void Logger::releaseConfig()
{
}

void Logger::ConfInfoPrint()
{
#ifdef __linux__
    for (unsigned int i = 0; i < settings.logFilePath.size() + 15; i++)
    {
        std::cout << GREEN << "-";
        if (i == (settings.logFilePath.size() + 15) / 2)
        {
            std::cout << "Logger";
        }
    }
    std::cout << DEFA << std::endl;
    std::cout << GREEN << std::left << std::setw(25) << "  日志开关　　" << settings.logSwitch << DEFA << std::endl;
    std::cout << GREEN << std::left << std::setw(25) << "  文件输出　　" << settings.logFileSwitch << DEFA << std::endl;
    std::cout << GREEN << std::left << std::setw(25) << "  终端输出开关" << settings.logTerminalSwitch << DEFA << std::endl;
    std::cout << GREEN << std::left << std::setw(25) << "  文件输出等级" << settings.logOutputLevelFile << DEFA << std::endl;
    std::cout << GREEN << std::left << std::setw(25) << "  终端输出等级" << settings.logOutputLevelTerminal << DEFA << std::endl;
    std::cout << GREEN << std::left << std::setw(25) << "  日志队列策略" << settings.logFileQueueSwitch << DEFA << std::endl;
    std::cout << GREEN << std::left << std::setw(25) << "  日志文件名称" << settings.logName << ".log" << DEFA << std::endl;
    std::cout << GREEN << std::left << std::setw(25) << "  日志保存路径" << settings.logFilePath << DEFA << std::endl;
    std::cout << GREEN << std::left << std::setw(25) << "  日志文件大小" << settings.logMixSize << "M" << DEFA << std::endl;
    for (unsigned int i = 0; i < settings.logFilePath.size() + 25; i++)
    {
        std::cout << GREEN << "-" << DEFA;
    }
    std::cout << std::endl;
#elif _WIN32
    for (unsigned int i = 0; i < settings.logFilePath.size() + 15; i++)
    {
        std::cout << "-";
        if (i == (settings.logFilePath.size() + 15) / 2)
        {
            std::cout << "Logger";
        }
    }
    std::cout << std::endl;
    std::cout << std::left << std::setw(25) << "  日志开关　　" << logger.logSwitch << std::endl;
    std::cout << std::left << std::setw(25) << "  文件输出　　" << logger.logFileSwitch << std::endl;
    std::cout << std::left << std::setw(25) << "  终端输出开关" << logger.logTerminalSwitch << std::endl;
    std::cout << std::left << std::setw(25) << "  文件输出等级" << logger.logOutputLevelFile << std::endl;
    std::cout << std::left << std::setw(25) << "  终端输出等级" << logger.logOutputLevelTerminal << std::endl;
    std::cout << std::left << std::setw(25) << "  日志队列策略" << logger.logFileQueueSwitch << std::endl;
    std::cout << std::left << std::setw(25) << "  日志文件名称" << logger.logName << ".log" << std::endl;
    std::cout << std::left << std::setw(25) << "  日志保存路径" << logger.logFilePath << std::endl;
    std::cout << std::left << std::setw(25) << "  日志文件大小" << logger.logMixSize << "M" << std::endl;
    for (int i = 0; i < logger.logFilePath.size() + 25; i++)
    {
        std::cout << "-";
    }
    std::cout << std::endl;
#endif
}

std::string Logger::getCoutType(coutType coutType)
{
    return singleObject->coutTypeMap[coutType];
}

bool Logger::getFileType(fileType fileCoutBool)
{
    return singleObject->fileCoutMap[fileCoutBool];
}

bool Logger::getTerminalType(terminalType terminalCoutTyle)
{
    return singleObject->terminalCoutMap[terminalCoutTyle];
}

std::string Logger::getLogCoutTime()
{
    time_t timep;
    time(&timep);
    char tmp[64];
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&timep));
    std::string tmp_str = tmp;
    return SQUARE_BRACKETS_LEFT + tmp_str + SQUARE_BRACKETS_RIGHT;
}

std::string Logger::getLogNameTime()
{
    time_t timep;
    time(&timep);
    char tmp[64];
    strftime(tmp, sizeof(tmp), "%Y-%m-%d-%H:%M:%S", localtime(&timep));
    return tmp;
}

std::string Logger::getSourceFilePash()
{
#ifdef __linux__
    getcwd(sourceFilePath, sizeof(sourceFilePath) - 1);
#elif _WIN32
    getcwd(sourceFilePash, sizeof(sourceFilePash) - 1);
#endif
    std::string sourceFilePash_str = sourceFilePath;
    return sourceFilePash_str + SLASH;
}
std::string Logger::getFilePath()
{
    return settings.logFilePath + SLASH;
}

std::string Logger::getFilePathAndName()
{
#ifdef __linux__
    return settings.logFilePath + SLASH + settings.logName + ".log";
#elif _WIN32
    return settings.logFilePath + settings.logName + ".log";
#endif
}

std::string Logger::getFilePathAndNameAndTime()
{
    return settings.logFilePath + settings.logName + getLogNameTime() + ".log";
}

std::string Logger::getLogCoutProcessId()
{
#ifdef __linux__
    return std::to_string(getpid());
#elif _WIN32
    return to_string(getpid());
//  return GetCurrentProcessId();
#endif
}

std::string Logger::getLogCoutThreadId()
{
#ifdef __linux__
    return std::to_string(syscall(__NR_gettid));
#elif _WIN32
    return std::to_string(GetCurrentThreadId());
#endif
}

std::string Logger::getLogCoutUserName()
{
#ifdef __linux__
    struct passwd *my_info;
    my_info = getpwuid(getuid());
    std::string name = my_info->pw_name;
    return SPACE + name + SPACE;
#elif _WIN32
    const int MAX_LEN = 100;
    TCHAR szBuffer[MAX_LEN];
    DWORD len = MAX_LEN;
    GetUserName(szBuffer, &len);

    int iLen = WideCharToMultiByte(CP_ACP, 0, szBuffer, -1, NULL, 0, NULL, NULL);
    char *chRtn = new char[iLen * sizeof(char)];
    WideCharToMultiByte(CP_ACP, 0, szBuffer, -1, chRtn, iLen, NULL, NULL);
    std::string str(chRtn);
    return " " + str + " ";
#endif
}

bool Logger::logFileWrite(std::string messages, std::string message, std::string line_effd)
{
    std::string filePathAndName = getFilePathAndName();

    long fileSize = filemanagement.verifyFileSize(filePathAndName);
    if (fileSize > (long)atoi(settings.logMixSize.data()) * MEGABYTES && settings.logBehavior == "1")
    {
        std::string newFileName = getFilePathAndNameAndTime();
        filemanagement.fileRename(filePathAndName, newFileName);
        filemanagement.createFile(filePathAndName);
    }
    if (settings.logFileQueueSwitch == SWITCH_OFF)
    {
        mutex_file.lock();
        std::ofstream file;
        file.open(filePathAndName, std::ios::app | std::ios::out);
        file << messages << message << line_effd;
        file.close();
        mutex_file.unlock();
    }
    else
    {
        insertQueue(messages + message + line_effd, filePathAndName);
    }
    return 1;
}

bool Logger::insertQueue(std::string messages, std::string filePathAndName)
{
    mutex_queue.lock();
    messageQueue.push(messages);
    if (messageQueue.size() >= 5000)
    {
        mutex_file.lock();
        std::ofstream file;
        file.open(filePathAndName, std::ios::app | std::ios::out);
        while (!messageQueue.empty())
        {
            file << messageQueue.front();
            messageQueue.pop();
        }
        file.close();
        mutex_file.unlock();
    }
    mutex_queue.unlock();
    return true;
}

std::string Logger::getLogSwitch()
{
    return settings.logSwitch;
}

std::string Logger::getLogFileSwitch()
{
    return settings.logFileSwitch;
}

std::string Logger::getLogTerminalSwitch()
{
    return settings.logTerminalSwitch;
}
std::string Logger::getCoutTypeColor(std::string colorType)
{
#ifdef __linux__
    return coutColor[colorType];
#elif _WIN32
    return "";
#endif
}

bool Logger::bindFileCoutMap(std::string value1, fileType value2)
{
    if (settings.logOutputLevelFile.find(value1) != std::string::npos)
    {
        fileCoutMap[value2] = true;
    }
    else
    {
        fileCoutMap[value2] = false;
    }
    return true;
}

bool Logger::bindTerminalCoutMap(std::string value1, terminalType value2)
{
    if (settings.logOutputLevelTerminal.find(value1) != std::string::npos)
    {
        terminalCoutMap[value2] = true;
    }
    else
    {
        terminalCoutMap[value2] = false;
    }
    return true;
}
