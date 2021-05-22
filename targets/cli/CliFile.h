#ifndef CLIFILE_H
#define CLIFILE_H

#include "FileHelper.h"

class CliFile : public FileHelper
{
public:
    CliFile();
    ~CliFile();
    bool readFile(std::string filename, std::string &fileContent);
    bool writeFile(std::string filename, std::string fileContent);
};
#endif
