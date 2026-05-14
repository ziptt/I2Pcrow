# I2Pcrow

`I2Pcrow` is a TUI torrent client built on libtorrent and FTXUI. It only works over I2P - all tracker and peer traffic goes through I2P, nothing touches the clearnet.

<img width="1541" height="851" alt="Screenshot" src="https://github.com/user-attachments/assets/1d7e6979-8eb8-46d6-848c-6afe34e6f639" />

## Features

- Download and upload multiple I2P torrents simultaneously.
- Support for `.torrent` files and magnet links.
- Partial download.
- Restore torrents across restarts.
- Terminal User Interface with mouse support.
- Select torrents in the list and pause/resume, remove, or inspect them.
- Show progress, state, peers, seeds, download/upload rates.
- Cross-platform C++ code.
- All peer and tracker connections are made exclusively through the I2P network via the SAM bridge.

## Requirements

`i2pcrow` requires a running I2P router with the SAM bridge enabled. The SAM bridge is a standard component of both the Java I2P and i2pd. By default, the SAM bridge listens on `127.0.0.1:7656`.

### Setting up the SAM bridge

**Java I2P:** In the router console, navigate to `http://127.0.0.1:7657/configclients` and ensure the SAM application bridge is started.

**i2pd:** Add or confirm the following in your `i2pd.conf`:

```ini
[sam]
enabled = true
address = 127.0.0.1
port = 7656
```

Without an active SAM bridge, `i2pcrow` will fail to connect to the I2P network and no torrents will make progress.

## Dependencies

- [libtorrent](https://github.com/arvidn/libtorrent)
- [FTXUI](https://github.com/ArthurSonzogni/FTXUI)
- [simpleini](https://github.com/brofield/simpleini)

Linux packages vary by distribution. On Debian/Ubuntu systems, install CMake, Ninja, a C++17 compiler, libtorrent-rasterbar development files, and FTXUI development files. SimpleIni is used as a local header-only file at `src/SimpleIni.h`.

On Windows, install vcpkg, Visual Studio 2022 or Build Tools for Visual Studio with the "Desktop development with C++" workload.

## Build

Linux:

```sh
cmake --preset linux-release
cmake --build --preset linux-release
./build/linux-release/i2pcrow
```

Windows with vcpkg and MSVC from a regular PowerShell:

```powershell
cmake --preset windows-vs-vcpkg
cmake --build --preset windows-vs-vcpkg
.\build\windows-vs\Release\i2pcrow.exe
```

## Configuration

`i2pcrow` reads an INI configuration file at startup. By default it looks for `%APPDATA%\i2pcrow\config.ini` on Windows, `~/.config/i2pcrow/config.ini` on Linux, or `$XDG_CONFIG_HOME/i2pcrow/config.ini`.  
`--config PATH` can point to a different file.

Example:

```ini
[paths]
save = /path/to/downloads
state = /path/to/i2pcrow-state

[libtorrent]
user_agent = I2Pcrow
i2p_hostname = 127.0.0.1
i2p_port = 7656
i2p_inbound_quantity = 3
i2p_outbound_quantity = 3
i2p_inbound_length = 3
i2p_outbound_length = 3
tracker_completion_timeout = 120
tracker_receive_timeout = 40
stop_tracker_timeout = 10
peer_connect_timeout = 30
request_timeout = 120
# Comma-separated. When omitted, i2pcrow adds its built-in default trackers.
# Set to an empty value to disable built-in additional trackers.
additional_trackers = http://tracker.example/announce, udp://tracker.example:1337/announce
# Rate limits are in Kb/s; 0 means unlimited.
upload_rate_limit = 0
download_rate_limit = 0
```

Command-line `--save` and `--state` values override the INI file for that launch.

Print the application version:

```sh
i2pcrow --version
```

By default, i2pcrow stores resume data under `%APPDATA%\i2pcrow` on Windows, `~/.local/state/i2pcrow` on Linux, or `$XDG_STATE_HOME/i2pcrow`

## Controls

In main list use the arrow keys to select a torrent, `Enter` or `i` to open its information window, `t` to open partial download file selection, `a` to open the Add torrent window, `p` to pause or resume, `d` to delete, and `q` for exit. In the information window, use `1`-`4` to switch tabs. Confirm dialogs with `Enter`, or cancel with `Esc`. In the partial download screen, use `Space` or `Enter` to toggle the selected file, `a` to select all files, and `n` to select none. The same actions are also exposed as clickable on-screen buttons.
