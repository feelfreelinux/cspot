#ifndef FILEHELPER_H
#define FILEHELPER_H

#include <iostream>
#include <fstream>

class FileHelper
{
public:
    FileHelper() {}
    virtual ~FileHelper() {}
    virtual bool readFile(std::string filename, std::string &fileContent) = 0;
    virtual bool writeFile(std::string filename, std::string fileContent) = 0;
};

#endif
