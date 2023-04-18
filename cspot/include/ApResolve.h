#pragma once

#include <string>  // for string
#ifdef BELL_ONLY_CJSON
#include "cJSON.h"
#else
#endif

namespace cspot {
class ApResolve {
 private:
  std::string apOverride;

 public:
  ApResolve(std::string apOverride);

  /**
     * @brief Connects to spotify's servers and returns first valid ap address
     * 
     * @return std::string Address in form of url:port
     */
  std::string fetchFirstApAddress();
};
}  // namespace cspot
