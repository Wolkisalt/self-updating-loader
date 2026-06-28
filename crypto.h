#pragma once
#include <string>
#include <vector>

// Generates a unique Hardware ID based on CPU and Disk
std::string GetHWID();

// Verifies the Base64 signature of the HWID using the embedded RSA Public Key
bool VerifyLicense(const std::string& hwid, const std::string& base64_signature);

// Copies text to clipboard
void CopyToClipboard(const std::string& text);

// Saves and loads the local license cache
bool SaveLicense(const std::string& signature);
std::string LoadLicense();
