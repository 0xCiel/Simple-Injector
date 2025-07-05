#include "Injector.h"
#include <iostream>
#include <filesystem>

int main() {
    wchar_t sInjPath[MAX_PATH];
    GetModuleFileNameW(nullptr, sInjPath, MAX_PATH);
    auto fsExeDir = std::filesystem::path(sInjPath).parent_path();
    auto fsDllPath = fsExeDir / L"Cheat.dll"; //Dll name

    if (!std::filesystem::exists(fsDllPath)) {
        std::wcerr << L"Error: DLL not found in executable directory\n";
        system("pause");
        return 1;
    }

    Injector cInjector;
    cInjector.SetConfig(fsExeDir.wstring());

    if (!cInjector.HasTarget()) {
        std::wcout << L"Select target executable...\n";
        if (!cInjector.SelectExe()) {
            std::cerr << "Error: " << cInjector.GetError() << "\n";
            system("pause");
            return 1;
        }
    }

    std::wcout << L"Target: " << cInjector.m_sExePath << L"\n"
        << L"Injecting " << fsDllPath.filename() << L"...\n";

    if (!cInjector.InjectDLL(fsDllPath.wstring())) {
        std::cerr << "Error: " << cInjector.GetError() << "\n";
        system("pause");
        return 1;
    }

    std::wcout << L"Injection successful!\n";
    Sleep(10000);
    return 0;
}