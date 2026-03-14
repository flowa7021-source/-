#include "utils/file_utils.h"
#include "utils/string_utils.h"
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <fstream>
#include <filesystem>
#include <wincrypt.h>

#pragma comment(lib, "crypt32.lib")

namespace docvision {
namespace utils {

bool fileExists(const std::wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool directoryExists(const std::wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool createDirectories(const std::wstring& path) {
    try {
        std::filesystem::create_directories(path);
        return true;
    } catch (...) {
        return false;
    }
}

int64_t getFileSize(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad)) {
        return -1;
    }
    LARGE_INTEGER size;
    size.HighPart = fad.nFileSizeHigh;
    size.LowPart = fad.nFileSizeLow;
    return size.QuadPart;
}

std::wstring getLastModified(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad)) {
        return {};
    }
    SYSTEMTIME st;
    FileTimeToSystemTime(&fad.ftLastWriteTime, &st);
    wchar_t buf[64];
    swprintf_s(buf, L"%04d-%02d-%02d %02d:%02d:%02d",
               st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return buf;
}

std::vector<uint8_t> readFileBytes(const std::wstring& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return {};
    auto size = file.tellg();
    if (size <= 0) return {};
    std::vector<uint8_t> data(static_cast<size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

bool writeFileBytes(const std::wstring& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

std::string readFileText(const std::wstring& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return {};
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

bool writeFileText(const std::wstring& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    file.write(text.data(), text.size());
    return file.good();
}

std::wstring getExeDirectory() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring fullPath(path);
    auto pos = fullPath.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        return fullPath.substr(0, pos);
    }
    return fullPath;
}

std::wstring getTempDirectory() {
    wchar_t path[MAX_PATH];
    GetTempPathW(MAX_PATH, path);
    std::wstring result(path);
    result += L"DocVision";
    createDirectories(result);
    return result;
}

std::string computeFileHash(const std::wstring& path) {
    auto data = readFileBytes(path);
    if (data.empty()) return {};

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    std::string result;

    if (CryptAcquireContextW(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
            if (CryptHashData(hHash, data.data(), static_cast<DWORD>(data.size()), 0)) {
                DWORD hashLen = 32;
                uint8_t hash[32];
                if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
                    char hex[65];
                    for (DWORD i = 0; i < hashLen; ++i) {
                        sprintf_s(hex + i * 2, 3, "%02x", hash[i]);
                    }
                    result = hex;
                }
            }
            CryptDestroyHash(hHash);
        }
        CryptReleaseContext(hProv, 0);
    }
    return result;
}

std::wstring showOpenFileDialog(void* parentHwnd,
                                 const std::wstring& filter,
                                 const std::wstring& title) {
    wchar_t filename[MAX_PATH] = {};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = static_cast<HWND>(parentHwnd);
    ofn.lpstrFilter = filter.c_str();
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title.c_str();
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        return filename;
    }
    return {};
}

std::wstring showSaveFileDialog(void* parentHwnd,
                                 const std::wstring& filter,
                                 const std::wstring& defaultName,
                                 const std::wstring& title) {
    wchar_t filename[MAX_PATH] = {};
    wcscpy_s(filename, defaultName.c_str());

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = static_cast<HWND>(parentHwnd);
    ofn.lpstrFilter = filter.c_str();
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title.c_str();
    ofn.Flags = OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameW(&ofn)) {
        return filename;
    }
    return {};
}

std::wstring showFolderDialog(void* parentHwnd, const std::wstring& title) {
    IFileDialog* pDialog = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pDialog));
    if (FAILED(hr)) return {};

    DWORD options;
    pDialog->GetOptions(&options);
    pDialog->SetOptions(options | FOS_PICKFOLDERS);
    pDialog->SetTitle(title.c_str());

    std::wstring result;
    if (SUCCEEDED(pDialog->Show(static_cast<HWND>(parentHwnd)))) {
        IShellItem* pItem = nullptr;
        if (SUCCEEDED(pDialog->GetResult(&pItem))) {
            PWSTR pszPath = nullptr;
            if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                result = pszPath;
                CoTaskMemFree(pszPath);
            }
            pItem->Release();
        }
    }
    pDialog->Release();
    return result;
}

std::wstring getSupportedFileFilter() {
    return L"All Supported Files\0*.pdf;*.djvu;*.djv;*.cbz;*.epub\0"
           L"PDF Files (*.pdf)\0*.pdf\0"
           L"DjVu Files (*.djvu, *.djv)\0*.djvu;*.djv\0"
           L"CBZ Files (*.cbz)\0*.cbz\0"
           L"EPUB Files (*.epub)\0*.epub\0"
           L"All Files (*.*)\0*.*\0\0";
}

} // namespace utils
} // namespace docvision
