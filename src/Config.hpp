#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace i2pcrow {

struct LibtorrentConfig {
  std::optional<std::string> user_agent;
  std::optional<std::string> i2p_hostname;
  std::optional<int> i2p_port;
  std::optional<int> i2p_inbound_quantity;
  std::optional<int> i2p_outbound_quantity;
  std::optional<int> i2p_inbound_length;
  std::optional<int> i2p_outbound_length;
  std::optional<int> tracker_completion_timeout;
  std::optional<int> tracker_receive_timeout;
  std::optional<int> stop_tracker_timeout;
  std::optional<int> peer_connect_timeout;
  std::optional<int> request_timeout;
  std::optional<int> upload_rate_limit;
  std::optional<int> download_rate_limit;
  std::optional<std::vector<std::string>> additional_trackers;
};

struct ClientConfig {
  std::filesystem::path default_save_path;
  std::filesystem::path state_path;
  LibtorrentConfig libtorrent;
};

std::filesystem::path defaultConfigPath();
ClientConfig loadConfig(std::filesystem::path const& config_path, bool require_exists);

}  // namespace i2pcrow
