#include <windows.h>
#include <bcrypt.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

#pragma comment(lib, "Bcrypt.lib")

void PrintHex(const std::vector<BYTE>& data, const char* name) {
    std::cout << "const BYTE " << name << "[] = {";
    for (size_t i = 0; i < data.size(); ++i) {
        if (i % 16 == 0) std::cout << "\n    ";
        std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)data[i] << ", ";
    }
    std::cout << "\n};\n" << std::dec;
}

int main() {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RSA_ALGORITHM, NULL, 0) != 0) {
        std::cerr << "Failed to open algorithm provider\n";
        return 1;
    }

    if (BCryptGenerateKeyPair(hAlg, &hKey, 2048, 0) != 0) {
        std::cerr << "Failed to generate key pair\n";
        return 1;
    }

    if (BCryptFinalizeKeyPair(hKey, 0) != 0) {
        std::cerr << "Failed to finalize key pair\n";
        return 1;
    }

    // Export Public Key
    DWORD cbPublicKey = 0;
    BCryptExportKey(hKey, NULL, BCRYPT_RSAPUBLIC_BLOB, NULL, 0, &cbPublicKey, 0);
    std::vector<BYTE> publicKey(cbPublicKey);
    BCryptExportKey(hKey, NULL, BCRYPT_RSAPUBLIC_BLOB, publicKey.data(), cbPublicKey, &cbPublicKey, 0);

    // Export Private Key
    DWORD cbPrivateKey = 0;
    BCryptExportKey(hKey, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, NULL, 0, &cbPrivateKey, 0);
    std::vector<BYTE> privateKey(cbPrivateKey);
    BCryptExportKey(hKey, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, privateKey.data(), cbPrivateKey, &cbPrivateKey, 0);

    // Write to files
    std::ofstream pubFile("public_key.bin", std::ios::binary);
    pubFile.write((char*)publicKey.data(), publicKey.size());
    pubFile.close();

    std::ofstream privFile("private_key.bin", std::ios::binary);
    privFile.write((char*)privateKey.data(), privateKey.size());
    privFile.close();

    std::cout << "Keys generated successfully!\n\n";
    std::cout << "// Embed this in your loader.exe:\n";
    PrintHex(publicKey, "RSA_PUBLIC_KEY");

    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return 0;
}
