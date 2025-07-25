#include <chrono>
#include <iostream>
#include <mist/mist.hpp>
#include <random>
#include <string>
#include <thread>

std::string generate_random_username() {
  std::random_device rd;
  std::ostringstream usernameStream;
  usernameStream << "player" << std::setw(7) << std::setfill('0') << rd() % 10000000;
  return usernameStream.str();
}

int main() {
  MistConnection client("152385113", generate_random_username(), "contact@grady.link");

  client.onConnectionStatus([](bool connected, const std::string &message) {
    if (connected) {
      std::cout << "[INFO] Connected: " << message << std::endl;
      return;
    }
    std::cout << "[INFO] Disconnected: " << message << std::endl;
  });
  client.onVariableUpdate([](const std::string &name, const std::string &value) {
    std::cout << "[INFO] Variable \"" << name << "\" changed to: " << value << std::endl;
  });

  if (client.connect()) {
    std::cout << "[INFO] Client started. Waiting for connection and handshake..." << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(5000)); // TODO: remove arbitrary time.

    int counter = 0;
    while (counter < 20) {
      counter++;
      client.set("☁ High Score", std::to_string(counter));
      std::cout << "Set \"☁ High Score\" to: " << counter << std::endl;

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "[INFO] Disconnecting..." << std::endl;
    client.disconnect();
  } else {
    std::cerr << "[ERROR] Failed to connect to TurboWarp Cloud Variables." << std::endl;
  }
}
