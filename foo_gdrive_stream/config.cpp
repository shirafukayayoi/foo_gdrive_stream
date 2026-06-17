#include "stdafx.h"
#include "guids.h"
#include "core_utils.h"
#include "config.h"

namespace gds {

cfg_var_modern::cfg_string g_cfg_client_json(guid_cfg_client_json, "");
cfg_var_modern::cfg_string g_cfg_folder(guid_cfg_folder, "");
cfg_var_modern::cfg_string g_cfg_cache_dir(guid_cfg_cache_dir, "");
cfg_var_modern::cfg_int g_cfg_cache_limit_mb(guid_cfg_cache_limit_mb, 4096);

std::string clientJsonPath() {
    pfc::string8 value = g_cfg_client_json.get();
    return value.c_str();
}

std::string folderSetting() {
    pfc::string8 value = g_cfg_folder.get();
    return value.c_str();
}

std::string folderIdSetting() {
    return extractFolderId(folderSetting());
}

std::wstring cacheDirSetting() {
    pfc::string8 value = g_cfg_cache_dir.get();
    if (value.is_empty()) return defaultCacheDir();
    return utf8ToWide(value.c_str());
}

int cacheLimitMb() {
    const auto v = (int)g_cfg_cache_limit_mb.get();
    return v <= 0 ? 4096 : v;
}

void clearSavedToken() {
    DeleteFileW(tokenPath().c_str());
}

}
