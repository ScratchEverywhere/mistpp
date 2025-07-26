#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <sys/select.h>
#include <thread>

#include <curl/curl.h>

class MistConnection {
public:
  using VariableUpdateCallback = std::function<void(const std::string &name, const std::string &value)>;
  using ConnectionStatusCallback = std::function<void(bool connected, const std::string &message)>;

  MistConnection(
      const std::string &project_id,
      const std::string &username,
      const std::string &user_agent_contact_info);

  ~MistConnection();

  bool connect();
  void disconnect();

  bool set(const std::string &name, const std::string &value);
  std::string get(const std::string &name) const;

  void onVariableUpdate(VariableUpdateCallback callback);
  void onConnectionStatus(ConnectionStatusCallback callback);

private:
  std::string project_id_;
  std::string username_;
  std::string user_agent_contact_info_;
  std::string ws_url_;

  std::map<std::string, std::string> cloud_variables_;
  mutable std::mutex variables_mutex_;

  CURL *curl_easy_;
  CURLM *curl_multi_;
  std::thread curl_thread_;
  std::atomic<bool> running_;
  std::atomic<bool> connected_;

  VariableUpdateCallback var_update_callback_;
  ConnectionStatusCallback conn_status_callback_;

  std::mutex curl_mutex_;

  static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);
  static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata);
  static int sock_callback(CURL *e, curl_socket_t s, int what, void *userp, void *sockp);
  static int timer_callback(CURLM *multi, long timeout_ms, void *userp);

  void processMessages();
  void send_json_message(const nlohmann::json &json_msg);
  std::string generateUserAgent() const;
  void handle_recv_message(const char *data, size_t len);

  struct CurlGlobalIniter {
    CurlGlobalIniter() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobalIniter() { curl_global_cleanup(); }
  };
  static CurlGlobalIniter s_curl_initer;
};
