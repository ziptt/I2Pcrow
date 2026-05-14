#include "Config.hpp"
#include "PathUtil.hpp"
#include "TuiApp.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifndef I2PCROW_VERSION
#define I2PCROW_VERSION "unknown"
#endif

namespace {

struct Options {
  std::filesystem::path config_path = i2pcrow::defaultConfigPath();
  std::filesystem::path save_path;
  std::filesystem::path state_path;
  std::vector<std::string> sources;
  bool config_path_set = false;
  bool save_path_set = false;
  bool state_path_set = false;
  bool help = false;
  bool version = false;
};

void printHelp(char const* executable) {
  std::cout << "Usage: " << executable
            << " [--config PATH] [--save PATH] [--state PATH] [MAGNET_OR_TORRENT ...]\n"
            << "\n"
            << "Start the i2pcrow terminal torrent client.\n"
            << "\n"
            << "Options:\n"
            << "  --config PATH  INI config file. Default: "
            << i2pcrow::pathToUtf8(i2pcrow::defaultConfigPath()) << '\n'
            << "  --save PATH    Default download directory. Overrides config [paths] save.\n"
            << "  --state PATH   Directory for i2pcrow session state and resume data. "
               "Overrides config [paths] state.\n"
            << "  --version      Show app name and version.\n"
            << "  --help         Show this help text.\n";
}

void printVersion() {
  std::cout << "i2pcrow " << I2PCROW_VERSION << '\n';
}

Options parseOptions(int argc, char** argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    std::string arg = argv[index];
    if (arg == "--help" || arg == "-h") {
      options.help = true;
    } else if (arg == "--version") {
      options.version = true;
    } else if (arg == "--config") {
      if (index + 1 >= argc) {
        throw std::runtime_error("--config requires a path");
      }
      options.config_path = argv[++index];
      options.config_path_set = true;
    } else if (arg == "--save") {
      if (index + 1 >= argc) {
        throw std::runtime_error("--save requires a path");
      }
      options.save_path = argv[++index];
      options.save_path_set = true;
    } else if (arg == "--state") {
      if (index + 1 >= argc) {
        throw std::runtime_error("--state requires a path");
      }
      options.state_path = argv[++index];
      options.state_path_set = true;
    } else {
      options.sources.push_back(std::move(arg));
    }
  }
  return options;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    auto options = parseOptions(argc, argv);
    if (options.help) {
      printHelp(argv[0]);
      return 0;
    }
    if (options.version) {
      printVersion();
      return 0;
    }

    auto config = i2pcrow::loadConfig(options.config_path, options.config_path_set);
    if (options.save_path_set) {
      config.default_save_path = std::move(options.save_path);
    }
    if (options.state_path_set) {
      config.state_path = std::move(options.state_path);
    }

    i2pcrow::TuiApp app(std::move(config));
    for (auto& source : options.sources) {
      app.addStartupTorrent(std::move(source));
    }

    return app.run();
  } catch (std::exception const& ex) {
    std::cerr << "i2pcrow: " << ex.what() << '\n';
    return 1;
  }
}
