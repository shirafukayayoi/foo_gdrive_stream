#pragma once

#define _WIN32_WINNT 0x0A00
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <mmsystem.h>
#include <winhttp.h>
#include <wincrypt.h>
#include <bcrypt.h>
#include <shellapi.h>
#include <shlobj.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <map>
#include <mutex>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <helpers/foobar2000+atl.h>
#include <SDK/foobar2000.h>
#include <helpers/atl-misc.h>
#include <helpers/DarkMode.h>
#include <foobar2000/helpers/helpers.h>
#include <foobar2000/helpers/input_helpers.h>
#include <libPPUI/wtl-pp.h>

#include "third_party/nlohmann/json.hpp"
