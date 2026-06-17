#include "stdafx.h"
#include "guids.h"
#include "core_utils.h"
#include "ui_actions.h"

namespace gds {
namespace {

bool selectionHasFolderFeed(metadb_handle_list_cref data) {
    for (t_size i = 0; i < data.get_count(); ++i) {
        if (isGdriveFolderPath(data[i]->get_path())) return true;
    }
    return false;
}

class GDriveContextMenu : public contextmenu_item_simple {
public:
    enum { cmd_expand_feed = 0, cmd_refresh_feed, cmd_total };

    unsigned get_num_items() override { return cmd_total; }

    void get_item_name(unsigned index, pfc::string_base &out) override {
        out = index == cmd_refresh_feed ? "Refresh Google Drive folder feed" : "Expand Google Drive folder feed";
    }

    bool context_get_display(unsigned index, metadb_handle_list_cref data, pfc::string_base &out, unsigned &displayFlags, const GUID &) override {
        if (!selectionHasFolderFeed(data)) return false;
        displayFlags = 0;
        get_item_name(index, out);
        return true;
    }

    void context_command(unsigned index, metadb_handle_list_cref data, const GUID &) override {
        for (t_size i = 0; i < data.get_count(); ++i) {
            const char *path = data[i]->get_path();
            if (isGdriveFolderPath(path)) {
                if (index == cmd_refresh_feed) refreshFolderFeedInActivePlaylist(path, core_api::get_main_window());
                else expandFolderFeedToActivePlaylist(path, core_api::get_main_window());
            }
        }
    }

    GUID get_item_guid(unsigned index) override {
        return index == cmd_refresh_feed ? guid_context_refresh_feed : guid_context_expand_feed;
    }

    bool get_item_description(unsigned index, pfc::string_base &out) override {
        out = index == cmd_refresh_feed
            ? "Replaces existing expanded Google Drive tracks after the feed with the current folder contents."
            : "Loads the selected Google Drive folder feed and inserts its audio files after the feed track.";
        return true;
    }

    GUID get_parent() override { return contextmenu_groups::utilities; }
};

static contextmenu_item_factory_t<GDriveContextMenu> g_context_menu_factory;

}
}
