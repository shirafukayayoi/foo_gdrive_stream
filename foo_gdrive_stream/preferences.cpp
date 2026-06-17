#include "stdafx.h"
#include "guids.h"
#include "core_utils.h"
#include "config.h"
#include "resource.h"

namespace gds {
namespace {

class PreferencesDialog : public CDialogImpl<PreferencesDialog>, public preferences_page_instance {
public:
    enum { IDD = IDD_PREFERENCES };

    explicit PreferencesDialog(preferences_page_callback::ptr cb) : m_callback(std::move(cb)) {}

    fb2k::hwnd_t get_wnd() override { return m_hWnd; }

    t_uint32 get_state() override {
        t_uint32 state = preferences_state::resettable | preferences_state::dark_mode_supported;
        if (hasChanged()) state |= preferences_state::changed;
        return state;
    }

    void apply() override {
        CString value;
        GetDlgItemText(IDC_EDIT_CLIENT_JSON, value);
        g_cfg_client_json = pfc::stringcvt::string_utf8_from_os(value);
        GetDlgItemText(IDC_EDIT_FOLDER, value);
        g_cfg_folder = pfc::stringcvt::string_utf8_from_os(value);
        GetDlgItemText(IDC_EDIT_CACHE_DIR, value);
        g_cfg_cache_dir = pfc::stringcvt::string_utf8_from_os(value);
        GetDlgItemText(IDC_EDIT_CACHE_LIMIT, value);
        g_cfg_cache_limit_mb = _wtoi(value);
        m_callback->on_state_changed();
    }

    void reset() override {
        SetDlgItemText(IDC_EDIT_CLIENT_JSON, L"");
        SetDlgItemText(IDC_EDIT_FOLDER, L"");
        SetDlgItemText(IDC_EDIT_CACHE_DIR, utf8ToWide(wideToUtf8(defaultCacheDir())).c_str());
        SetDlgItemInt(IDC_EDIT_CACHE_LIMIT, 4096, FALSE);
        m_callback->on_state_changed();
    }

    BEGIN_MSG_MAP_EX(PreferencesDialog)
        MSG_WM_INITDIALOG(onInitDialog)
        COMMAND_HANDLER_EX(IDC_EDIT_CLIENT_JSON, EN_CHANGE, onChange)
        COMMAND_HANDLER_EX(IDC_EDIT_FOLDER, EN_CHANGE, onChange)
        COMMAND_HANDLER_EX(IDC_EDIT_CACHE_DIR, EN_CHANGE, onChange)
        COMMAND_HANDLER_EX(IDC_EDIT_CACHE_LIMIT, EN_CHANGE, onChange)
        COMMAND_HANDLER_EX(IDC_BTN_BROWSE_CLIENT, BN_CLICKED, onBrowseClient)
        COMMAND_HANDLER_EX(IDC_BTN_BROWSE_CACHE, BN_CLICKED, onBrowseCache)
        COMMAND_HANDLER_EX(IDC_BTN_CLEAR_TOKEN, BN_CLICKED, onClearToken)
    END_MSG_MAP()

private:
    BOOL onInitDialog(CWindow, LPARAM) {
        m_dark.AddDialogWithControls(*this);
        SetDlgItemText(IDC_EDIT_CLIENT_JSON, pfc::stringcvt::string_os_from_utf8(g_cfg_client_json.get()));
        SetDlgItemText(IDC_EDIT_FOLDER, pfc::stringcvt::string_os_from_utf8(g_cfg_folder.get()));
        const auto cache = g_cfg_cache_dir.get();
        SetDlgItemText(IDC_EDIT_CACHE_DIR, cache.is_empty() ? defaultCacheDir().c_str() : pfc::stringcvt::string_os_from_utf8(cache));
        SetDlgItemInt(IDC_EDIT_CACHE_LIMIT, (UINT)cacheLimitMb(), FALSE);
        return FALSE;
    }

    void onChange(UINT, int, CWindow) { m_callback->on_state_changed(); }

    void onBrowseClient(UINT, int, CWindow) {
        OPENFILENAMEW ofn{};
        wchar_t buf[MAX_PATH]{};
        GetDlgItemText(IDC_EDIT_CLIENT_JSON, buf, MAX_PATH);
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = m_hWnd;
        ofn.lpstrFilter = L"JSON Files\0*.json\0All Files\0*.*\0";
        ofn.lpstrFile = buf;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        if (GetOpenFileNameW(&ofn)) SetDlgItemText(IDC_EDIT_CLIENT_JSON, buf);
        m_callback->on_state_changed();
    }

    void onBrowseCache(UINT, int, CWindow) {
        wchar_t buf[MAX_PATH]{};
        GetDlgItemText(IDC_EDIT_CACHE_DIR, buf, MAX_PATH);
        BROWSEINFOW bi{};
        bi.hwndOwner = m_hWnd;
        bi.lpszTitle = L"Select cache folder";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
        if (pidl) {
            SHGetPathFromIDListW(pidl, buf);
            CoTaskMemFree(pidl);
            SetDlgItemText(IDC_EDIT_CACHE_DIR, buf);
        }
        m_callback->on_state_changed();
    }

    void onClearToken(UINT, int, CWindow) {
        clearSavedToken();
        MessageBoxW(L"Saved Google Drive token was deleted.", L"foo_gdrive_stream", MB_OK | MB_ICONINFORMATION);
    }

    bool hasChanged() {
        CString value;
        GetDlgItemText(IDC_EDIT_CLIENT_JSON, value);
        if (CString(pfc::stringcvt::string_os_from_utf8(g_cfg_client_json.get())) != value) return true;
        GetDlgItemText(IDC_EDIT_FOLDER, value);
        if (CString(pfc::stringcvt::string_os_from_utf8(g_cfg_folder.get())) != value) return true;
        GetDlgItemText(IDC_EDIT_CACHE_DIR, value);
        const CString currentCache = g_cfg_cache_dir.get().is_empty() ? CString(defaultCacheDir().c_str()) : CString(pfc::stringcvt::string_os_from_utf8(g_cfg_cache_dir.get()));
        if (currentCache != value) return true;
        GetDlgItemText(IDC_EDIT_CACHE_LIMIT, value);
        if (_wtoi(value) != cacheLimitMb()) return true;
        return false;
    }

    preferences_page_callback::ptr m_callback;
    fb2k::CDarkModeHooks m_dark;
};

class PreferencesPage : public preferences_page_impl<PreferencesDialog> {
public:
    const char *get_name() override { return "Google Drive Stream"; }
    GUID get_guid() override { return guid_preferences_page; }
    GUID get_parent_guid() override { return preferences_page::guid_tools; }
};

static preferences_page_factory_t<PreferencesPage> g_preferences_factory;

}
}
