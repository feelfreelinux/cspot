#ifndef CLIFILE_H
#define CLIFILE_H

#include "FileHelper.h"

class CliFile : public FileHelper
{
public:
    CliFile();
    ~CliFile();
    std::string readFile(std::string filename);
    bool writeFile(std::string filename, std::string fileContent);
};
#endif
