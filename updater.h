#pragma once
#include <string>
#include <vector>
#include <atomic>

struct DownloadProgress {
    std::atomic<uint64_t> bytes_downloaded{0};
    std::atomic<uint64_t> total_bytes{1};
    std::atomic<bool> is_finished{false};
    std::atomic<bool> is_error{false};
    std::string error_message;
};

// Starts an asynchronous download. Updates the progress object.
void StartDownloadAsync(const std::string& url, const std::string& dest_path, DownloadProgress* progress);

// Synchronously fetch a string (like version.json) from a URL
bool FetchString(const std::string& url, std::string& out_content);
