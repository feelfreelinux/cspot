#include "ESPFile.h"
#include "Logger.h"
#include <sys/stat.h>

ESPFile::ESPFile(){}
ESPFile::~ESPFile(){}

bool ESPFile::readFile(std::string filename, std::string &fileContent)
{
  struct stat file_stat;
  FILE *fd;

  // File not present
  if(stat(filename.c_str(), &file_stat) != 0)
  {
      return false;
  }

  fd = fopen(filename.c_str(), "r");
  if(!fd)
  {
      return false;
  }

  char buffer[file_stat.st_size];

  fread(buffer, 1, file_stat.st_size, fd);
  fclose(fd);

  fileContent = buffer;

  return true;
}

bool ESPFile::writeFile(std::string filename, std::string fileContent)
{
    FILE *fd = fopen(filename.c_str(), "w");
    if(fd == NULL)
    {
        return false;
    }
    fwrite(fileContent.c_str() , sizeof(char), fileContent.length(), fd);
    fclose(fd);

    return true;
}
