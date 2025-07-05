#include "Injector.h"
#include <commdlg.h>
#include <fstream>
#include <Psapi.h>
#include <iostream>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Psapi.lib")

void Injector::SetConfig(const std::wstring& path) {
    m_sConfigPath = path + L"\\Config.cfg";
    LoadConfig();
}

bool Injector::HasTarget() const {
    return !m_sExePath.empty();
}

bool Injector::SelectExe() {
    OPENFILENAMEW ofn = { sizeof(ofn) };
    wchar_t szFile[MAX_PATH] = { 0 };

    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Executables\0*.exe\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        m_sExePath = szFile;
        SaveConfig();
        return true;
    }
    m_sError = "Failed to select executable";
    return false;
}

bool Injector::LoadConfig() {
    if (m_sConfigPath.empty()) return false;

    std::wifstream cfgFile(m_sConfigPath);
    if (!cfgFile) return false;

    std::wstring sLine;
    while (std::getline(cfgFile, sLine)) {
        size_t nPos = sLine.find(L'=');
        if (nPos != std::wstring::npos) {
            if (sLine.substr(0, nPos).find(L"Executable") != std::wstring::npos) {
                m_sExePath = sLine.substr(nPos + 1);
                m_sExePath.erase(0, m_sExePath.find_first_not_of(L" \t"));
                m_sExePath.erase(m_sExePath.find_last_not_of(L" \t") + 1);
            }
            else if (sLine.substr(0, nPos).find(L"Delay") != std::wstring::npos) {
                m_nDelay = std::stoi(sLine.substr(nPos + 1));
            }
        }
    }
    return !m_sExePath.empty();
}

void Injector::SaveConfig() {
    if (m_sConfigPath.empty()) return;

    std::wofstream cfgFile(m_sConfigPath);
    if (cfgFile) {
        cfgFile << L"Executable = " << m_sExePath << std::endl;
        cfgFile << L"Delay = " << m_nDelay << std::endl;
    }
}

bool Injector::InjectDLL(const std::wstring& dllPath) {
    if (m_sExePath.empty() && !LoadConfig()) {
        m_sError = "No executable selected";
        return false;
    }

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };

    if (!CreateProcessW(m_sExePath.c_str(), NULL, NULL, NULL, FALSE,
        0, NULL, NULL, &si, &pi)) {
        m_sError = "Failed to create process";
        return false;
    }

    std::wcout << L"Waiting " << m_nDelay << L"ms before injection...\n";
    Sleep(m_nDelay);

    auto hKernel32 = GetModuleHandleW(L"kernel32.dll");
    auto pLoadLib = GetProcAddress(hKernel32, "LoadLibraryW");

    size_t nPathSize = (dllPath.size() + 1) * sizeof(wchar_t);
    auto pRemotePath = VirtualAllocEx(pi.hProcess, NULL, nPathSize, MEM_COMMIT, PAGE_READWRITE);

    if (!pRemotePath || !WriteProcessMemory(pi.hProcess, pRemotePath, dllPath.c_str(), nPathSize, NULL)) {
        m_sError = "Memory allocation failed";
        TerminateProcess(pi.hProcess, 0);
        return false;
    }

    auto hThread = CreateRemoteThread(pi.hProcess, NULL, 0,
        (LPTHREAD_START_ROUTINE)pLoadLib, pRemotePath, 0, NULL);
    if (!hThread) {
        m_sError = "Thread creation failed";
        TerminateProcess(pi.hProcess, 0);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    VirtualFreeEx(pi.hProcess, pRemotePath, 0, MEM_RELEASE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return true;
}

std::string Injector::GetError() const {
    return m_sError;
}