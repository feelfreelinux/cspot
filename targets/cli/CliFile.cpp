#include "CliFile.h"

CliFile::CliFile(){}
CliFile::~CliFile(){}

bool CliFile::readFile(std::string filename, std::string &fileContent)
{

    std::ifstream configFile(filename);
    std::string jsonConfig((std::istreambuf_iterator<char>(configFile)),
                                std::istreambuf_iterator<char>());

    fileContent = jsonConfig;
 
    return true;
}

bool CliFile::writeFile(std::string filename, std::string fileContent)
{
  std::ofstream configFile(filename);

  if(!configFile.good())
  {
    return false;
  }

  if(!(configFile << fileContent))
  {
    return false;
  }

  configFile.close();
  return true;
}
