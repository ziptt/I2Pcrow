#include "Config.hpp"

#include "PathUtil.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include "SimpleIni.h"

namespace i2pcrow {
namespace {

std::filesystem::path envPath(char const* name) {
  if (auto const* value = std::getenv(name); value != nullptr && *value != '\0') {
    return std::filesystem::path(value);
  }
  return {};
}

std::filesystem::path readPath(CSimpleIniA const& ini, char const* section, char const* key) {
  auto const* value = ini.GetValue(section, key, nullptr);
  if (value == nullptr || *value == '\0') {
    return {};
  }
  return std::filesystem::path(value);
}

std::optional<std::string> readOptionalText(CSimpleIniA const& ini,
                                            char const* section,
                                            char const* key) {
  auto const* value = ini.GetValue(section, key, nullptr);
  if (value == nullptr) {
    return std::nullopt;
  }
  return std::string(value);
}

std::string trim(std::string value) {
  auto const first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
    return std::isspace(ch) != 0;
  });
  auto const last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
                      return std::isspace(ch) != 0;
                    }).base();
  if (first >= last) {
    return {};
  }
  return std::string(first, last);
}

int parseNonNegativeInt(std::string value, char const* section, char const* key) {
  value = trim(std::move(value));
  int parsed = 0;
  auto const* begin = value.data();
  auto const* end = value.data() + value.size();
  auto const result = std::from_chars(begin, end, parsed);
  if (value.empty() || result.ec != std::errc{} || result.ptr != end || parsed < 0) {
    throw std::runtime_error(std::string("Invalid value for [") + section + "] " + key +
                             ": expected a non-negative integer");
  }
  return parsed;
}

int parseIntInRange(std::string value, char const* section, char const* key, int min, int max) {
  auto const parsed = parseNonNegativeInt(std::move(value), section, key);
  if (parsed < min || parsed > max) {
    throw std::runtime_error(std::string("Invalid value for [") + section + "] " + key +
                             ": expected an integer from " + std::to_string(min) + " to " +
                             std::to_string(max));
  }
  return parsed;
}

int parseRateLimitKilobytes(std::string value, char const* section, char const* key) {
  constexpr int kBytesPerKilobyte = 1024;
  auto const parsed = parseNonNegativeInt(std::move(value), section, key);
  if (parsed > std::numeric_limits<int>::max() / kBytesPerKilobyte) {
    throw std::runtime_error(std::string("Invalid value for [") + section + "] " + key +
                             ": kilobytes-per-second value is too large");
  }
  return parsed * kBytesPerKilobyte;
}

std::vector<std::string> parseTrackerList(std::string value) {
  std::vector<std::string> trackers;

  std::size_t start = 0;
  while (start <= value.size()) {
    auto const end = value.find_first_of(",\r\n", start);
    auto tracker = trim(value.substr(start, end == std::string::npos ? std::string::npos
                                                                      : end - start));
    if (!tracker.empty()) {
      trackers.push_back(std::move(tracker));
    }
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }

  return trackers;
}

std::string configFileText(std::filesystem::path const& path) {
  return path.empty() ? std::string("<none>") : pathToUtf8(path);
}

}  // namespace

std::filesystem::path defaultConfigPath() {

#ifdef _WIN32
  if (auto app_data = envPath("APPDATA"); !app_data.empty()) {
    return app_data / "i2pcrow" / "config.ini";
  }
  if (auto local_app_data = envPath("LOCALAPPDATA"); !local_app_data.empty()) {
    return local_app_data / "i2pcrow" / "config.ini";
  }
  if (auto home = envPath("USERPROFILE"); !home.empty()) {
    return home / "AppData" / "Roaming" / "i2pcrow" / "config.ini";
  }
#else
  if (auto config_home = envPath("XDG_CONFIG_HOME"); !config_home.empty()) {
    return config_home / "i2pcrow" / "config.ini";
  }
  if (auto home = envPath("HOME"); !home.empty()) {
    return home / ".config" / "i2pcrow" / "config.ini";
  }
#endif

  return std::filesystem::current_path() / "i2pcrow.ini";
}

ClientConfig loadConfig(std::filesystem::path const& config_path, bool require_exists) {
  ClientConfig config;
  if (config_path.empty()) {
    return config;
  }

  std::error_code fs_error;
  auto const exists = std::filesystem::exists(config_path, fs_error);
  if (fs_error) {
    throw std::runtime_error("Could not inspect config file " + configFileText(config_path) +
                             ": " + fs_error.message());
  }

  if (!exists) {
    if (require_exists) {
      throw std::runtime_error("Config file does not exist: " + configFileText(config_path));
    }
    return config;
  }

  if (!std::filesystem::is_regular_file(config_path, fs_error)) {
    if (fs_error) {
      throw std::runtime_error("Could not inspect config file " + configFileText(config_path) +
                               ": " + fs_error.message());
    }
    throw std::runtime_error("Config path is not a file: " + configFileText(config_path));
  }

  CSimpleIniA ini;
  ini.SetUnicode();
  auto const rc = ini.LoadFile(pathToUtf8(config_path).c_str());
  if (rc < 0) {
    throw std::runtime_error("Could not read config file: " + configFileText(config_path));
  }

  config.default_save_path = readPath(ini, "paths", "save");
  config.state_path = readPath(ini, "paths", "state");

  if (auto value = readOptionalText(ini, "libtorrent", "user_agent")) {
    config.libtorrent.user_agent = trim(std::move(*value));
  }
  if (auto value = readOptionalText(ini, "libtorrent", "i2p_hostname")) {
    config.libtorrent.i2p_hostname = trim(std::move(*value));
  }
  if (auto value = readOptionalText(ini, "libtorrent", "i2p_port")) {
    config.libtorrent.i2p_port = parseNonNegativeInt(*value, "libtorrent", "i2p_port");
  }
  if (auto value = readOptionalText(ini, "libtorrent", "i2p_inbound_quantity")) {
    config.libtorrent.i2p_inbound_quantity =
        parseIntInRange(*value, "libtorrent", "i2p_inbound_quantity", 1, 16);
  }
  if (auto value = readOptionalText(ini, "libtorrent", "i2p_outbound_quantity")) {
    config.libtorrent.i2p_outbound_quantity =
        parseIntInRange(*value, "libtorrent", "i2p_outbound_quantity", 1, 16);
  }
  if (auto value = readOptionalText(ini, "libtorrent", "i2p_inbound_length")) {
    config.libtorrent.i2p_inbound_length =
        parseIntInRange(*value, "libtorrent", "i2p_inbound_length", 0, 7);
  }
  if (auto value = readOptionalText(ini, "libtorrent", "i2p_outbound_length")) {
    config.libtorrent.i2p_outbound_length =
        parseIntInRange(*value, "libtorrent", "i2p_outbound_length", 0, 7);
  }
  if (auto value = readOptionalText(ini, "libtorrent", "tracker_completion_timeout")) {
    config.libtorrent.tracker_completion_timeout =
        parseNonNegativeInt(*value, "libtorrent", "tracker_completion_timeout");
  }
  if (auto value = readOptionalText(ini, "libtorrent", "tracker_receive_timeout")) {
    config.libtorrent.tracker_receive_timeout =
        parseNonNegativeInt(*value, "libtorrent", "tracker_receive_timeout");
  }
  if (auto value = readOptionalText(ini, "libtorrent", "stop_tracker_timeout")) {
    config.libtorrent.stop_tracker_timeout =
        parseNonNegativeInt(*value, "libtorrent", "stop_tracker_timeout");
  }
  if (auto value = readOptionalText(ini, "libtorrent", "peer_connect_timeout")) {
    config.libtorrent.peer_connect_timeout =
        parseNonNegativeInt(*value, "libtorrent", "peer_connect_timeout");
  }
  if (auto value = readOptionalText(ini, "libtorrent", "request_timeout")) {
    config.libtorrent.request_timeout =
        parseNonNegativeInt(*value, "libtorrent", "request_timeout");
  }
  if (auto value = readOptionalText(ini, "libtorrent", "upload_rate_limit")) {
    config.libtorrent.upload_rate_limit =
        parseRateLimitKilobytes(*value, "libtorrent", "upload_rate_limit");
  }
  if (auto value = readOptionalText(ini, "libtorrent", "download_rate_limit")) {
    config.libtorrent.download_rate_limit =
        parseRateLimitKilobytes(*value, "libtorrent", "download_rate_limit");
  }
  if (auto value = readOptionalText(ini, "libtorrent", "additional_trackers")) {
    config.libtorrent.additional_trackers = parseTrackerList(std::move(*value));
  }
  return config;
}

}  // namespace i2pcrow

