#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <string>

class CspotLogger
{
public:
    // static bool enableColors = true;
    static void debug(std::string filename, int line, const char *format, ...)
    {

        printf(colorRed);
        printf("D ");
        printFilename(filename);
        printf(":%d: ", line);
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n");
    };

    static void error(std::string filename, int line, const char *format, ...)
    {

        printf(colorRed);
        printf("E ");
        printFilename(filename);
        printf(":%d: ", line);
        printf(colorRed);
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n");
    };

    static void info(std::string filename, int line, const char *format, ...)
    {

        printf(colorBlue);
        printf("I ");
        printFilename(filename);
        printf(":%d: ", line);
        printf(colorReset);
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n");
    };

    static void printFilename(std::string filename)
    {
        std::string basenameStr(filename.substr(filename.rfind("/") + 1));
        unsigned long hash = 5381;
        int c;

        for (char const &c : basenameStr)
        {
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        }

        printf("\e[0;%dm", allColors[hash % NColors]);

        printf("%s", basenameStr.c_str());
        printf(colorReset);
    }

private:
    static constexpr const char *colorReset = "\e[0m";
    static constexpr const char *colorRed = "\e[0;31m";
    static constexpr const char *colorBlue = "\e[0;34m";
    static constexpr const int NColors = 16;
    static constexpr int allColors[NColors] = {30, 31, 32, 33, 34, 35, 36, 37, 90, 91, 92, 93, 94, 95, 96, 97};
};

#define CSPOT_LOG(type, ...)                                \
    do                                                      \
    {                                                       \
        CspotLogger::type(__FILE__, __LINE__, __VA_ARGS__); \
    } while (0)

#endif
