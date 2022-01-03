#include "CommandLineArguments.h"

CommandLineArguments::CommandLineArguments(std::string u, std::string p, bool shouldShowHelp) : username(u), password(p), shouldShowHelp(shouldShowHelp) {}

std::shared_ptr<CommandLineArguments> CommandLineArguments::parse(int argc, char **argv)
{

    if (argc == 1)
    {
        return std::make_shared<CommandLineArguments>("", "", false);
    }
    auto result = std::make_shared<CommandLineArguments>("", "", false);
    for (int i = 1; i < argc; i++)
    {
        auto stringVal = std::string(argv[i]);

        // if we encounter -h or --help we just jump to printing the help immediately
        if (stringVal == "-h" || stringVal == "--help")
        {
            result->username = "";
            result->password = "";
            result->shouldShowHelp = true;
            return result;
        }
        else if (stringVal == "-u" || stringVal == "--username")
        {
            // check if we have something more to read from the arguments
            if (i >= argc - 1)
            {
                throw std::invalid_argument("expected path after the username flag");
            }
            result->username = std::string(argv[++i]);
        }
        else if (stringVal == "-p" || stringVal == "--password")
        {
            if (i >= argc - 1)
            {
                throw std::invalid_argument("expected path after the password flag");
            }
            result->password = std::string(argv[++i]);
        }
        else if (stringVal == "-b" || stringVal == "--bitrate")
        {
            if (i >= argc - 1)
            {
                throw std::invalid_argument("expected path after the bitrate flag");
            }
            i++;
            if(std::string(argv[i]) == "320")
            {
              result->bitrate = AudioFormat_OGG_VORBIS_320;
            }
            else if(std::string(argv[i]) == "160")
            {
              result->bitrate = AudioFormat_OGG_VORBIS_160;
            }
            else if(std::string(argv[i]) == "96")
            {
              result->bitrate = AudioFormat_OGG_VORBIS_96;
            }
            else
            {
              throw std::invalid_argument("invalid bitrate argument");
            }
            result->setBitrate = true;
        }
        else
        {
            throw std::invalid_argument(("unknown flag '" + stringVal + "'").c_str());
        }
    }

    // we need to check that we've got everything we need
    if ((result->username == "") != (result->password == ""))
    {
        throw std::invalid_argument("both username and password must be provided when using username authorization");
    }

    return result;
}
