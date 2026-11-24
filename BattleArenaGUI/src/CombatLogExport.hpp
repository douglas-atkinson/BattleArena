#pragma once

// Save the original combat log entries, not the wrapped rows used for display.
// This belongs to the GUI: the Arena never writes files.

#include "ViewState.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>

#ifdef _WIN32
#include <objbase.h>
#include <shlobj.h>  // Resolve Documents correctly, including redirected folders.
#ifdef _MSC_VER
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#endif
#endif

namespace logexport
{
struct Result
{
    bool success;
    std::string message;
};

inline Result save(const std::vector<LogEntry>& log)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path folder;
#ifdef _WIN32
    // Use the Windows-known Documents folder (works with OneDrive redirection).
    PWSTR documents = nullptr;
    const HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, nullptr, &documents);
    if (FAILED(hr) || documents == nullptr)
    {
        if (documents)
            CoTaskMemFree(documents);
        return {false, "Log export failed: cannot locate Documents folder"};
    }
    folder = fs::path(documents) / "BattleArena" / "BattleLogs";
    CoTaskMemFree(documents);
#else
    // Retain the existing behaviour on non-Windows platforms.
    folder = fs::current_path(ec) / "BattleLogs";
    if (ec)
        return {false, "Log export failed: working directory unavailable"};
#endif
    fs::create_directories(folder, ec);
    if (ec)
        return {false, "Log export failed: cannot create BattleLogs folder"};

    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#ifdef _WIN32
    if (localtime_s(&localTime, &time) != 0)
#else
    if (localtime_r(&time, &localTime) == nullptr)
#endif
        return {false, "Log export failed: cannot determine local time"};

    std::ostringstream timestamp;
    timestamp << std::put_time(&localTime, "%Y%m%d_%H%M%S");
    const std::string baseName = "BattleLog_" + timestamp.str();

    // If F8 is pressed twice within a second, preserve both transcripts.
    fs::path path;
    for (unsigned suffix = 0;; ++suffix)
    {
        path = folder / (baseName + (suffix == 0 ? "" : "_" + std::to_string(suffix)) + ".txt");
        if (!fs::exists(path, ec))
            break;
    }
    if (ec)
        return {false, "Log export failed: cannot check existing files"};

    std::ofstream file(path);
    if (!file)
        return {false, "Log export failed: cannot open output file"};

    file << "BattleArena - Combat Log\n";
    file << "Exported: " << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "\n\n";
    for (const LogEntry& entry : log)
        file << "Round " << entry.round << ": " << entry.text << '\n';
    file.close();
    if (!file)
    {
        fs::remove(path, ec);  // Remove an incomplete export if possible.
        return {false, "Log export failed: writing the file was unsuccessful"};
    }

#ifdef _WIN32
    return {true, "Saved log in Documents/BattleArena/BattleLogs"};
#else
    return {true, "Saved " + path.filename().string() + " in BattleLogs"};
#endif
}
} // namespace logexport
