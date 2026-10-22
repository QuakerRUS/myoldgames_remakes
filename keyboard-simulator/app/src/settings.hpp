#pragma once

namespace ks {

// User settings, stored in vs::user_data_dir("Keyboard Simulator")/settings.json: normally
// %APPDATA%\V Software\Keyboard Simulator; --data-dir <path> puts it there instead; smoke and
// scripted (screenshot) runs use a temporary folder (see main.cpp).
struct Settings {
    bool sound = true;

    static Settings load();  // defaults if the file is missing or invalid
    void save() const;       // errors are ignored (settings are not critical)
};

}  // namespace ks
