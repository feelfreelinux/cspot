#ifndef FILEHELPER_H
#define FILEHELPER_H

#include <iostream>
#include <fstream>

class FileHelper
{
public:
    FileHelper() {}
    virtual ~FileHelper() {}
    virtual std::string readFile(std::string filename) = 0;
    virtual bool writeFile(std::string filename, std::string fileContent) = 0;
};

#endif
