#pragma once
#include <windows.h>
#include <string>

class Injector {
public:
    Injector() = default;
    ~Injector() = default;

    bool SelectExe();
    bool InjectDLL(const std::wstring& dllPath);
    std::string GetError() const;
    void SetConfig(const std::wstring& path);
    bool HasTarget() const;
    bool LoadConfig();
    void SaveConfig();

    std::wstring m_sExePath;
    std::wstring m_sConfigPath;
    std::string m_sError;
    int m_nDelay = 0;
};