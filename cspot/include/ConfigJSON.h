#ifndef CONFIGJSON_H
#define CONFIGJSON_H

#include <memory>
#include <iostream>
#include "FileHelper.h"
#include "protobuf/metadata.pb.h"

class ConfigJSON
{
private:
    std::shared_ptr<FileHelper> _file;
    std::string _jsonFileName;
public:
    ConfigJSON(std::string jsonFileName, std::shared_ptr<FileHelper> file);
    bool load();
    bool save();

    uint16_t volume;
    std::string deviceName;
    std::string apOverride;
    AudioFormat format;
};

extern std::shared_ptr<ConfigJSON> configMan;

#endif
