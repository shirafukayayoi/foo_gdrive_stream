#pragma once

namespace gds {

extern cfg_var_modern::cfg_string g_cfg_client_json;
extern cfg_var_modern::cfg_string g_cfg_folder;
extern cfg_var_modern::cfg_string g_cfg_cache_dir;
extern cfg_var_modern::cfg_int g_cfg_cache_limit_mb;

std::string clientJsonPath();
std::string folderSetting();
std::string folderIdSetting();
std::wstring cacheDirSetting();
int cacheLimitMb();
void clearSavedToken();

}
