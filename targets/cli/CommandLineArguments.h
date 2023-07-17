#ifndef COMMANDLINEARGUMENTS_H
#define COMMANDLINEARGUMENTS_H
#include <protobuf/metadata.pb.h>  // for AudioFormat
#include <memory>                  // for shared_ptr
#include <string>                  // for string, basic_string

/**
 * Represents the command line arguments passed to the program.
 *
 */
class CommandLineArguments {
 public:
  /**
     * The username used to log in to spotify without using mDNS.
     */
  std::string username;
  /**
     * The spotify password.
    */
  std::string password;
  /**
     * A file to store/read reusable credentials from
    */
  std::string credentials;
  /**
     * Bitrate setting.
    */
  bool setBitrate = false;
  AudioFormat bitrate;
  /**
     * Determines whether the help text should be printed.
     */
  bool shouldShowHelp;
  /**
     * This is a constructor which initializez all the fields of CommandLineArguments
     * @param shouldShowHelp determines whether the help text should be printed.
     */
  CommandLineArguments(std::string username, std::string password, 
                       std::string credentials, bool shouldShowHelp);

  /**
     * Parses command line arguments, as they are passed to main().
     * @param argc the number of arguments, including the executable path
     * @param argv a pointer to an array of pointers to c strings which are the contents of the arguments
     */
  static std::shared_ptr<CommandLineArguments> parse(int argc, char** argv);
};

#endif
