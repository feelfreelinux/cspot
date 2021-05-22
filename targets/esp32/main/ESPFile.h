#ifndef CLIFILE_H
#define CLIFILE_H

#include "FileHelper.h"

class ESPFile : public FileHelper
{
public:
    ESPFile();
    ~ESPFile();
    bool readFile(std::string filename, std::string &fileContent);
    bool writeFile(std::string filename, std::string fileContent);
};
#endif
