#include <vs/engine/json_file.hpp>

#include <algorithm>
#include <fstream>
#include <system_error>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <vs/engine/error.hpp>

namespace vs {

namespace fs = std::filesystem;

std::string path_to_utf8(const fs::path& path) {
    const std::u8string s = path.u8string();
    return {reinterpret_cast<const char*>(s.data()), s.size()};
}

fs::path utf8_to_path(std::string_view utf8) {
    return fs::path(std::u8string(reinterpret_cast<const char8_t*>(utf8.data()), utf8.size()));
}

std::string read_text_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw Error("cannot open " + path_to_utf8(path));
    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (in.bad()) throw Error("cannot read " + path_to_utf8(path));
    return data;
}

std::vector<unsigned char> read_binary_file(const fs::path& path) {
    const std::string text = read_text_file(path);
    return {text.begin(), text.end()};
}

namespace {

#ifdef _WIN32
std::string win_error_text(DWORD code) { return std::system_category().message(static_cast<int>(code)); }

// Writes `data` to `tmp` and flushes it to disk (FlushFileBuffers) before the rename, so the
// renamed file is never empty after a power loss. Returns an error text, empty on success.
std::string write_and_flush(const fs::path& tmp, std::string_view data) {
    HANDLE h = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return "cannot create " + path_to_utf8(tmp) + ": " + win_error_text(GetLastError());
    std::string err;
    std::size_t done = 0;
    while (done < data.size() && err.empty()) {
        const DWORD chunk = static_cast<DWORD>(std::min<std::size_t>(data.size() - done, 1u << 30));
        DWORD written = 0;
        if (!WriteFile(h, data.data() + done, chunk, &written, nullptr) || written == 0)
            err = "cannot write " + path_to_utf8(tmp) + ": " + win_error_text(GetLastError());
        done += written;
    }
    if (err.empty() && !FlushFileBuffers(h)) err = "cannot flush " + path_to_utf8(tmp) + ": " + win_error_text(GetLastError());
    if (!CloseHandle(h) && err.empty()) err = "cannot close " + path_to_utf8(tmp) + ": " + win_error_text(GetLastError());
    return err;
}

// Renames tmp over path. Antivirus scanners and indexers briefly open fresh files without
// FILE_SHARE_DELETE, so sharing/access errors are retried a few times.
std::string replace_file(const fs::path& tmp, const fs::path& path) {
    DWORD code = 0;
    for (int attempt = 0; attempt < 5; ++attempt) {
        if (attempt > 0) Sleep(20u * static_cast<DWORD>(attempt));
        if (MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return {};
        code = GetLastError();
        if (code != ERROR_SHARING_VIOLATION && code != ERROR_ACCESS_DENIED && code != ERROR_LOCK_VIOLATION) break;
    }
    return "cannot replace " + path_to_utf8(path) + ": " + win_error_text(code);
}
#else
std::string write_and_flush(const fs::path& tmp, std::string_view data) {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) return "cannot create " + path_to_utf8(tmp);
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
    out.flush();
    if (!out) return "cannot write " + path_to_utf8(tmp);
    return {};
}

std::string replace_file(const fs::path& tmp, const fs::path& path) {
    std::error_code ec;
    fs::rename(tmp, path, ec);  // replaces an existing target
    return ec ? "cannot replace " + path_to_utf8(path) + ": " + ec.message() : std::string();
}
#endif

}  // namespace

void write_file_atomic(const fs::path& path, std::string_view data) {
    std::error_code ec;
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path(), ec);
        if (ec && !fs::is_directory(path.parent_path()))
            throw Error("cannot create folder " + path_to_utf8(path.parent_path()) + ": " + ec.message());
    }
    fs::path tmp = path;
    tmp += ".tmp";
    std::string err = write_and_flush(tmp, data);
    if (err.empty()) err = replace_file(tmp, path);
    if (!err.empty()) {
        fs::remove(tmp, ec);  // never leave the temporary file behind
        throw Error(err);
    }
}

std::optional<nlohmann::json> read_json_file(const fs::path& path) {
    std::error_code ec;
    if (!fs::exists(path, ec)) return std::nullopt;
    const std::string text = read_text_file(path);
    try {
        return nlohmann::json::parse(text);
    } catch (const nlohmann::json::exception& e) {
        throw Error(path_to_utf8(path) + ": " + e.what());
    }
}

void write_json_file(const fs::path& path, const nlohmann::json& value, int indent) {
    std::string text = value.dump(indent, ' ', false, nlohmann::json::error_handler_t::replace);
    text += '\n';
    write_file_atomic(path, text);
}

}  // namespace vs
