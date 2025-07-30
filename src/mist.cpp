#include "mist/mist.hpp"
#include <chrono>
#include <cstring>
#include <curl/curl.h>
#include <iostream>

// Define the static member
MistConnection::CurlGlobalIniter MistConnection::s_curl_initer;

MistConnection::MistConnection(const std::string &project_id, const std::string &username, const std::string &user_agent_contact_info) : project_id_(project_id), username_(username), user_agent_contact_info_(user_agent_contact_info), ws_url_("wss://clouddata.turbowarp.org/"), curl_easy_(nullptr), curl_multi_(nullptr), running_(false), connected_(false) {}

MistConnection::MistConnection(const std::string &url, const std::string &project_id, const std::string &username, const std::string &user_agent_contact_info) : project_id_(project_id), username_(username), user_agent_contact_info_(user_agent_contact_info), ws_url_(url), curl_easy_(nullptr), curl_multi_(nullptr), running_(false), connected_(false) {}

MistConnection::~MistConnection() {
  disconnect();
  std::lock_guard<std::mutex> lock(curl_mutex_);
  if (curl_easy_) {
    curl_easy_cleanup(curl_easy_);
    curl_easy_ = nullptr;
  }
  if (curl_multi_) {
    curl_multi_cleanup(curl_multi_);
    curl_multi_ = nullptr;
  }
}

bool MistConnection::connect(bool secure) {
  std::lock_guard<std::mutex> lock(curl_mutex_);
  if (running_) {
    std::cerr << "[ERROR] Already connected or attempting to connect." << std::endl;
    return false;
  }

  curl_easy_ = curl_easy_init();
  if (!curl_easy_) {
    std::cerr << "[ERROR] Failed to initialize CURL easy handle." << std::endl;
    if (conn_status_callback_) conn_status_callback_(false, "Failed to initialize CURL.");
    return false;
  }

  curl_easy_setopt(curl_easy_, CURLOPT_URL, ws_url_.c_str());
  curl_easy_setopt(curl_easy_, CURLOPT_CONNECT_ONLY, 2L);
  curl_easy_setopt(curl_easy_, CURLOPT_USERAGENT, ("Mist++/0.3.0 " + user_agent_contact_info_).c_str());
  if (!secure) {
    curl_easy_setopt(curl_easy_, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_easy_, CURLOPT_SSL_VERIFYHOST, 0L);
  }

  curl_multi_ = curl_multi_init();
  if (!curl_multi_) {
    std::cerr << "[ERROR] Failed to initialize CURL multi handle." << std::endl;
    curl_easy_cleanup(curl_easy_);
    curl_easy_ = nullptr;
    if (conn_status_callback_) conn_status_callback_(false, "Failed to initialize CURL multi.");
    return false;
  }

  CURLMcode mc = curl_multi_add_handle(curl_multi_, curl_easy_);
  if (mc != CURLM_OK) {
    std::cerr << "[ERROR] curl_multi_add_handle failed: " << curl_multi_strerror(mc) << std::endl;
    curl_easy_cleanup(curl_easy_);
    curl_easy_ = nullptr;
    curl_multi_cleanup(curl_multi_);
    curl_multi_ = nullptr;
    if (conn_status_callback_) conn_status_callback_(false, "Failed to add easy handle to multi: " + std::string(curl_multi_strerror(mc)));
    return false;
  }

  running_ = true;
  curl_thread_ = std::thread([this]() {
    try {
      this->processMessages();
    } catch (const std::exception &e) {
      std::cerr << "[ERROR] Error in CURL processing thread: " << e.what() << std::endl;
      if (conn_status_callback_) conn_status_callback_(false, "Internal thread error: " + std::string(e.what()));
    }
  });

  return true;
}

bool MistConnection::connect() {
  return connect(true);
}

void MistConnection::disconnect() {
  if (!running_) return;
  running_ = false;

  std::lock_guard<std::mutex> lock(curl_mutex_);
  if (connected_) {
    size_t sent;
    CURLcode res = curl_ws_send(curl_easy_, "", 0, &sent, 0, CURLWS_CLOSE);
    if (res != CURLE_OK) std::cerr << "[ERROR] Error sending WebSocket close frame: " << curl_easy_strerror(res) << std::endl;
  }

  if (curl_thread_.joinable()) curl_thread_.join();

  if (curl_easy_) {
    curl_multi_remove_handle(curl_multi_, curl_easy_);
    curl_easy_cleanup(curl_easy_);
    curl_easy_ = nullptr;
  }
  if (curl_multi_) {
    curl_multi_cleanup(curl_multi_);
    curl_multi_ = nullptr;
  }

  connected_ = false;
  if (conn_status_callback_) conn_status_callback_(false, "Disconnected by client request.");
}

bool MistConnection::set(const std::string &name, const std::string &value) {
  std::lock_guard<std::mutex> lock(curl_mutex_);
  if (!connected_) {
    std::cerr << "[ERROR] Not connected to send variables." << std::endl;
    return false;
  }

  nlohmann::json msg;
  msg["method"] = "set";
  msg["name"] = name;
  msg["value"] = value;

  send_json_message(msg);
  std::lock_guard<std::mutex> var_lock(variables_mutex_);
  cloud_variables_[name] = value;
  return true;
}

std::string MistConnection::get(const std::string &name) const {
  std::lock_guard<std::mutex> lock(variables_mutex_);
  auto it = cloud_variables_.find(name);
  if (it != cloud_variables_.end()) return it->second;
  return "";
}

void MistConnection::onVariableUpdate(VariableUpdateCallback callback) {
  var_update_callback_ = std::move(callback);
}

void MistConnection::onConnectionStatus(ConnectionStatusCallback callback) {
  conn_status_callback_ = std::move(callback);
}

size_t MistConnection::write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
  return size * nmemb;
}

size_t MistConnection::header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
  return size * nitems;
}

void MistConnection::processMessages() {
  int still_running = 0;
  char buffer[4096];

  while (running_) {
    {
      std::lock_guard<std::mutex> lock(curl_mutex_);

      CURLMcode perform_code = CURLM_CALL_MULTI_PERFORM;
      while (perform_code == CURLM_CALL_MULTI_PERFORM) perform_code = curl_multi_perform(curl_multi_, &still_running);

      if (perform_code != CURLM_OK) {
        std::cerr << "[ERROR] curl_multi_perform failed: " << curl_multi_strerror(perform_code) << std::endl;
        if (conn_status_callback_) conn_status_callback_(false, "Multi perform error: " + std::string(curl_multi_strerror(perform_code)));
        running_ = false;
      }

      CURLMsg *msg;
      int msgs_left;
      while ((msg = curl_multi_info_read(curl_multi_, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE && msg->easy_handle == curl_easy_) {
          if (msg->data.result == CURLE_OK) {
            if (!connected_) {
              connected_ = true;
              if (conn_status_callback_) conn_status_callback_(true, "Connected successfully.");

              nlohmann::json handshake_msg;
              handshake_msg["method"] = "handshake";
              handshake_msg["user"] = username_;
              handshake_msg["project_id"] = project_id_;

              send_json_message(handshake_msg);

              int still_running_after_send;
              CURLMcode perform_code_after_send = CURLM_CALL_MULTI_PERFORM;
              while (perform_code_after_send == CURLM_CALL_MULTI_PERFORM) perform_code_after_send = curl_multi_perform(curl_multi_, &still_running_after_send);
              if (perform_code_after_send != CURLM_OK) std::cerr << "[ERROR] curl_multi_perform after handshake send failed: " << curl_multi_strerror(perform_code_after_send) << std::endl; // TODO: Improve error handling
            }
          } else {
            std::cerr << "[ERROR] Connection closed unexpectedly: " << curl_easy_strerror(msg->data.result) << std::endl;
            running_ = false;
            connected_ = false;
            if (conn_status_callback_) conn_status_callback_(false, "Connection terminated: " + std::string(curl_easy_strerror(msg->data.result)));
          }
        }
      }

      if (!running_) break;

      if (connected_) {
        size_t rlen;
        const struct curl_ws_frame *ws_meta;
        CURLcode recv_res = curl_ws_recv(curl_easy_, buffer, sizeof(buffer), &rlen, &ws_meta);

        if (recv_res == CURLE_OK) {
          if (ws_meta->flags & CURLWS_TEXT) handle_recv_message(buffer, rlen);
          else if (ws_meta->flags & CURLWS_CLOSE) {
            running_ = false;
            connected_ = false;
            if (conn_status_callback_) conn_status_callback_(false, "Server closed connection.");
          }
        } else if (recv_res != CURLE_AGAIN) {
          std::cerr << "[ERROR] curl_ws_recv failed: " << curl_easy_strerror(recv_res) << std::endl;
          running_ = false;
          connected_ = false;
          if (conn_status_callback_) conn_status_callback_(false, "Receive error: " + std::string(curl_easy_strerror(recv_res)));
        }
      }
    }

    if (!running_) break;

    long curl_timeout_ms = -1;
    if (still_running > 0) {
      std::lock_guard<std::mutex> lock_timeout(curl_mutex_);
      curl_multi_timeout(curl_multi_, &curl_timeout_ms);
    }

    if (still_running == 0) std::this_thread::sleep_for(std::chrono::milliseconds(50));
    else if (curl_timeout_ms < 0) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    else if (curl_timeout_ms != 0) std::this_thread::sleep_for(std::chrono::milliseconds(curl_timeout_ms));
  }
}

void MistConnection::send_json_message(const nlohmann::json &json_msg) {
  if (!curl_easy_ || !connected_) {
    std::cerr << "[ERROR] Cannot send message: Not connected or CURL handle not initialized." << std::endl;
    return;
  }

  std::string payload = json_msg.dump();
  size_t sent;
  CURLcode res = curl_ws_send(curl_easy_, payload.c_str(), payload.length(), &sent, 0, CURLWS_TEXT);

  if (res != CURLE_OK) std::cerr << "[ERROR] Error sending WebSocket message: " << curl_easy_strerror(res) << std::endl;
}

void MistConnection::handle_recv_message(const char *data, size_t len) {
  try {
    std::string payload_str(data, len);
    nlohmann::json json_msg = nlohmann::json::parse(payload_str);

    if (json_msg.contains("method") && json_msg["method"] == "set") {
      std::string name = json_msg["name"].get<std::string>();

      std::string value_str;
      if (json_msg.contains("value")) {
        const auto &value_json = json_msg["value"];
        if (value_json.is_string()) value_str = value_json.get<std::string>();
        else if (value_json.is_number()) value_str = std::to_string(value_json.get<double>());
        else if (value_json.is_boolean()) value_str = value_json.get<bool>() ? "true" : "false";
        else if (value_json.is_null()) value_str = "null";
        else value_str = value_json.dump();
      } else value_str = "";

      std::lock_guard<std::mutex> lock(variables_mutex_);
      cloud_variables_[name] = value_str;

      if (var_update_callback_) var_update_callback_(name, value_str);
    }
  } catch (const nlohmann::json::exception &e) {
    std::cerr << "[ERROR] JSON parsing error: " << e.what() << " Raw message: " << std::string(data, len) << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "[ERROR] Error processing message: " << e.what() << " Raw message: " << std::string(data, len) << std::endl;
  }
}
