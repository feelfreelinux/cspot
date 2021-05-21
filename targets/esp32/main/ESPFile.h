#ifndef CLIFILE_H
#define CLIFILE_H

#include "FileHelper.h"

class ESPFile : public FileHelper
{
public:
    ESPFile();
    ~ESPFile();
    std::string readFile(std::string filename);
    bool writeFile(std::string filename, std::string fileContent);
};
#endif
