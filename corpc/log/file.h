#ifndef FILEMANAGEMENT_H
#define FILEMANAGEMENT_H
#include <iostream>
#include <string>
#include <fstream>
#include <time.h>
#ifdef __linux__
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#elif _WIN32
#include <io.h>
#include <direct.h>
#endif

class FileManagement
{
public:
    FileManagement();

    ~FileManagement();

    bool createFilePath(std::string fileName);

    bool createFile(std::string fileName);

    bool verifyFileExistence(std::string fileName);

    // bool verifyFileValidityDays(string fileName, string logOverlay);废除

    bool fileRename(std::string oldFile, std::string newFile);

    long verifyFileSize(std::string fileName);

    long getCurrentTime();
};

#endif