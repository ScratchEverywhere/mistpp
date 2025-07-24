#pragma once

#include <string>
#include <vector>

namespace mist {
void init();
void quit();

class Connection {
private:
  std::vector<std::string> cloudHosts;
  std::string projectID;
  std::string username;
  std::string userAgent;

  Connection(std::vector<std::string> cloudHosts, std::string projectID, std::string username, std::string userAgent) : cloudHosts(cloudHosts), projectID(projectID), username(username), userAgent(userAgent) {}
};
} // namespace mist
