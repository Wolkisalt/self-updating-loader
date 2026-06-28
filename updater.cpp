#include "updater.h"
#include <windows.h>
#include <winhttp.h>
#include <thread>
#include <fstream>
#include <iostream>

#pragma comment(lib, "winhttp.lib")

// Helper function to crack URL
bool CrackUrl(const std::string& url, std::wstring& host, std::wstring& path, bool& is_https) {
    std::wstring wurl(url.begin(), url.end());
    URL_COMPONENTS urlComp = { 0 };
    urlComp.dwStructSize = sizeof(urlComp);
    
    wchar_t hostName[256];
    wchar_t urlPath[1024];
    
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = 1024;
    
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &urlComp)) return false;
    
    host = hostName;
    path = urlPath;
    is_https = (urlComp.nScheme == INTERNET_SCHEME_HTTPS);
    return true;
}

bool FetchString(const std::string& url, std::string& out_content) {
    std::wstring host, path;
    bool is_https;
    if (!CrackUrl(url, host, path, is_https)) return false;

    HINTERNET hSession = WinHttpOpen(L"ImGui Loader/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), is_https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, is_https ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    bool success = false;
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(hRequest, NULL)) {
        DWORD size = 0;
        DWORD downloaded = 0;
        do {
            size = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &size)) break;
            if (size == 0) break;

            char* buffer = new char[size + 1];
            if (WinHttpReadData(hRequest, (LPVOID)buffer, size, &downloaded)) {
                out_content.append(buffer, downloaded);
            }
            delete[] buffer;
        } while (size > 0);
        success = true;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return success;
}

void StartDownloadAsync(const std::string& url, const std::string& dest_path, DownloadProgress* progress) {
    std::thread([url, dest_path, progress]() {
        std::wstring host, path;
        bool is_https;
        if (!CrackUrl(url, host, path, is_https)) {
            progress->error_message = "Invalid URL";
            progress->is_error = true;
            progress->is_finished = true;
            return;
        }

        HINTERNET hSession = WinHttpOpen(L"ImGui Loader/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), is_https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, is_https ? WINHTTP_FLAG_SECURE : 0);

        if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(hRequest, NULL)) {
            // Get content length
            wchar_t contentLengthBuf[32];
            DWORD dwSize = sizeof(contentLengthBuf);
            uint64_t total = 1;
            if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, &contentLengthBuf, &dwSize, WINHTTP_NO_HEADER_INDEX)) {
                total = std::wcstoull(contentLengthBuf, nullptr, 10);
                if (total == 0) total = 1;
            }
            progress->total_bytes = total;

            std::ofstream outFile(dest_path, std::ios::binary);
            if (!outFile.is_open()) {
                progress->error_message = "Cannot open file for writing";
                progress->is_error = true;
            } else {
                DWORD size = 0;
                DWORD downloaded = 0;
                uint64_t current = 0;
                do {
                    size = 0;
                    if (!WinHttpQueryDataAvailable(hRequest, &size)) break;
                    if (size == 0) break;

                    char* buffer = new char[size];
                    if (WinHttpReadData(hRequest, (LPVOID)buffer, size, &downloaded)) {
                        outFile.write(buffer, downloaded);
                        current += downloaded;
                        progress->bytes_downloaded = current;
                    }
                    delete[] buffer;
                } while (size > 0);
                outFile.close();
            }
        } else {
            progress->error_message = "HTTP request failed";
            progress->is_error = true;
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        
        progress->is_finished = true;
    }).detach();
}
