#pragma once

#include <curl/curl.h>
#include <string>
#include <vector>

namespace mist {
void init();
void quit();

class Connection {
private:
  std::string cloudHost;
  std::string projectID;
  std::string username;
  std::string userAgent;

  CURL *curl;
  CURLcode res;

  /**
   * Requires all arguments unlike TurboWarp/mist
   */
  Connection(std::string cloudHost, std::string projectID, std::string username, std::string userAgent);
  ~Connection();
};
} // namespace mist
