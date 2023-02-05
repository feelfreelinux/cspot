#pragma once

#include <memory>
#include <string>

#include "HTTPClient.h"
#include "nlohmann/json.hpp"

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
