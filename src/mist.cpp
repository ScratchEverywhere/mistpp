#include "mist/mist.hpp"
#include <curl/curl.h>
#include <curl/easy.h>

namespace mist {
void init() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
}
void quit() {
  curl_global_cleanup();
}

Connection::Connection(std::string cloudHost, std::string projectID, std::string username, std::string userAgent) : cloudHost(cloudHost), projectID(projectID), username(username), userAgent(userAgent) {
  curl = curl_easy_init();

  curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);
  curl_easy_setopt(curl, CURLOPT_URL, cloudHost.c_str());
  curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());
}

Connection::~Connection() {
  curl_easy_cleanup(curl);
}
} // namespace mist
