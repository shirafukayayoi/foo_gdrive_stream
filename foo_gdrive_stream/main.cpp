#include "stdafx.h"
#include "guids.h"
#include "ui_actions.h"

DECLARE_COMPONENT_VERSION(
    "Google Drive Stream",
    "0.1.0",
    "Streams audio files from a configured Google Drive folder through foobar2000 with local caching.");

VALIDATE_COMPONENT_FILENAME("foo_gdrive_stream.dll");

namespace gds {
namespace {

static mainmenu_group_popup_factory g_gdrive_menu_group(
    guid_mainmenu_group,
    mainmenu_groups::file,
    mainmenu_commands::sort_priority_dontcare,
    "Google Drive Stream");

class GDriveMainMenu : public mainmenu_commands {
public:
    enum { cmd_load_folder = 0, cmd_add_feed, cmd_add_all, cmd_auth, cmd_total };

    t_uint32 get_command_count() override { return cmd_total; }
    GUID get_command(t_uint32 index) override {
        if (index == cmd_load_folder) return guid_mainmenu_load_folder;
        if (index == cmd_add_feed) return guid_mainmenu_add_feed;
        if (index == cmd_add_all) return guid_mainmenu_add;
        return guid_mainmenu_auth;
    }
    void get_name(t_uint32 index, pfc::string_base &out) override {
        if (index == cmd_load_folder) out = "Load Google Drive folder...";
        else if (index == cmd_add_feed) out = "Add Google Drive folder feed";
        else if (index == cmd_add_all) out = "Add configured folder files to playlist";
        else out = "Authenticate Google Drive";
    }
    bool get_description(t_uint32 index, pfc::string_base &out) override {
        if (index == cmd_load_folder) out = "Create a new playlist from a Google Drive folder ID or URL.";
        else if (index == cmd_add_feed) out = "Add the configured Google Drive folder as a playlist/feed track.";
        else if (index == cmd_add_all) out = "Load configured Google Drive folder and add all audio files to the active playlist.";
        else out = "Authenticate foo_gdrive_stream with Google Drive.";
        return true;
    }
    GUID get_parent() override { return guid_mainmenu_group; }
    void execute(t_uint32 index, mainmenu_commands::ctx_t) override {
        if (index == cmd_load_folder) loadFolderAsNewPlaylist(core_api::get_main_window());
        else if (index == cmd_add_feed) addConfiguredFolderFeedToPlaylist(core_api::get_main_window());
        else if (index == cmd_add_all) addAllFolderFilesToPlaylist(core_api::get_main_window());
        else authenticateNow(core_api::get_main_window());
    }
};

static mainmenu_commands_factory_t<GDriveMainMenu> g_menu_factory;
}
}
