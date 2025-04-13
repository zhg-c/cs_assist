#include "assistTools.h"
#include <TlHelp32.h>
#include <iostream>
#include <Psapi.h>
#include <DbgHelp.h>
#include <fstream>

#pragma comment(lib, "DbgHelp.lib")

#define MAX_SIZE 4096
HHOOK assistTools::m_hHook = NULL;
// 窗口和消息相关
typedef LRESULT(CALLBACK* WNDPROC_TYPE)(HWND, UINT, WPARAM, LPARAM);
typedef BOOL(WINAPI* POSTMESSAGE_TYPE)(HWND, UINT, WPARAM, LPARAM);
typedef LRESULT(WINAPI* SENDMESSAGE_TYPE)(HWND, UINT, WPARAM, LPARAM);

// 进程和线程相关
typedef BOOL(WINAPI* CREATEPROCESS_TYPE)(
	LPCWSTR, LPWSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES,
	BOOL, DWORD, LPVOID, LPCWSTR, LPSTARTUPINFOW, LPPROCESS_INFORMATION);
typedef HMODULE(WINAPI* LOADLIBRARY_TYPE)(LPCWSTR);
typedef FARPROC(WINAPI* GETPROCADDRESS_TYPE)(HMODULE, LPCSTR);

// 文件操作相关
typedef HANDLE(WINAPI* CREATEFILE_TYPE)(
	LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES,
	DWORD, DWORD, HANDLE);
typedef BOOL(WINAPI* DELETEFILE_TYPE)(LPCWSTR);
typedef BOOL(WINAPI* MOVEFILE_TYPE)(LPCWSTR, LPCWSTR);

// 注册表操作相关
typedef LSTATUS(WINAPI* REGOPENKEY_TYPE)(HKEY, LPCWSTR, PHKEY);
typedef LSTATUS(WINAPI* REGSETVALUE_TYPE)(HKEY, LPCWSTR, DWORD, const BYTE*, DWORD);


// 输入相关
typedef LRESULT(CALLBACK* KEYBOARDPROC_TYPE)(int, WPARAM, LPARAM);
typedef LRESULT(CALLBACK* MOUSEPROC_TYPE)(int, WPARAM, LPARAM);

assistTools::assistTools(const std::wstring& processName) :
	m_pid(0),
	m_tid(0),
	m_hProcess(NULL)
{
	m_pid = getPid(processName);
	m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_pid);
	//initm_SysFun();
}

DWORD assistTools::getPid(const std::wstring& processName) {
	HANDLE hSnapshot = INVALID_HANDLE_VALUE;
	DWORD pid = 0;
	do
	{
		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);
		hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) {
			break;
		}
		if (!Process32First(hSnapshot, &pe32)) {
			break;
		}
		do
		{
			if (processName == pe32.szExeFile) {
				pid = pe32.th32ProcessID;
				break;
			}
		} while (Process32Next(hSnapshot, &pe32));
	} while (false);
	if (hSnapshot != INVALID_HANDLE_VALUE) {
		CloseHandle(hSnapshot);
	}
	return pid;
}
std::vector<DWORD> assistTools::getTids() {
	std::vector<DWORD> vTid;
	HANDLE hSnapshot = INVALID_HANDLE_VALUE;
	do
	{
		hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) {
			break;
		}
		THREADENTRY32 te32{sizeof(THREADENTRY32)};
		if (!Thread32First(hSnapshot, &te32)) {
			break;
		}
		do
		{
			if (te32.th32OwnerProcessID == m_pid) {
				vTid.push_back(te32.th32ThreadID);
			}
		} while (Thread32Next(hSnapshot,&te32));
	} while (false);
	if (hSnapshot != INVALID_HANDLE_VALUE) {
		CloseHandle(hSnapshot);
	}
	return vTid;
}
std::vector<DWORD_PTR> assistTools::scanMemory(int val) {
	std::vector<DWORD_PTR> vRes;
	do
	{
		if (!m_hProcess) {
			break;
		}
		SYSTEM_INFO sysInfo{};
		GetSystemInfo(&sysInfo);
		MEMORY_BASIC_INFORMATION memInfo{};
		BYTE* address = 0;
		BYTE* maxAddress = (BYTE*)sysInfo.lpMaximumApplicationAddress;
		while (address < maxAddress) {
			if (VirtualQueryEx(m_hProcess, address, &memInfo, sizeof(memInfo))) {
				SIZE_T regionSize = memInfo.RegionSize;
				if (memInfo.State == MEM_COMMIT && memInfo.Protect != PAGE_NOACCESS) {
					for (SIZE_T i = 0; i < regionSize; i += MAX_SIZE)
					{
						BYTE buf[MAX_SIZE]{};
						SIZE_T byteRead = 0;
						SIZE_T toRead = min(MAX_SIZE, regionSize - i);
						if (!ReadProcessMemory(m_hProcess, (BYTE*)memInfo.BaseAddress + i, buf, toRead, &byteRead)) {
							continue;
						}
						for (SIZE_T j = 0; j < byteRead - sizeof(int); j++)
						{
							if (*(int*)(buf + j) == val) {
								vRes.push_back((DWORD_PTR)address + i + j);
							}
						}
					}
				}
				address += regionSize;
			}
			else {
				address += sysInfo.dwPageSize;
			}
		}
	} while (false);
	return vRes;
}
std::vector<DWORD_PTR> assistTools::scanMemory(const std::vector<DWORD_PTR>& preRes, int val) {
	std::vector<DWORD_PTR> vRes;
	do
	{
		if (!m_hProcess) {
			break;
		}
		for (const auto& v : preRes) {
			int num = 0;
			if (ReadProcessMemory(m_hProcess, (LPCVOID)v, &num, sizeof(num),NULL)) {
				if (num == val) {
					vRes.push_back(v);
				}
			}
		}
	} while (false);
	return vRes;
}
BOOL assistTools::writeMemory(const std::vector<DWORD_PTR>& vRes, int val) {
	BOOL bRet = TRUE;
	do
	{
		if (!m_hProcess) {
			break;
		}
		for (const auto& v : vRes) {
			if (!WriteProcessMemory(m_hProcess, (LPVOID)v, &val, sizeof(val), NULL)) {
				bRet = FALSE;
			}
		}
	} while (false);
	return bRet;
}
std::vector<std::wstring> assistTools::getModPaths() {
	std::vector<std::wstring> vPath;
	do
	{
		if (!m_hProcess) {
			break;
		}
		HMODULE hMods[1024]{};
		DWORD cbNeeded = 0;
		if (EnumProcessModulesEx(m_hProcess, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL)) {
			for (size_t i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
				TCHAR modName[MAX_PATH]{};
				if (GetModuleFileNameEx(m_hProcess, hMods[i], modName, sizeof(modName) / sizeof(TCHAR))) {
					vPath.push_back(modName);
				}
			}
		}
	} while (false);
	return vPath;
}
BOOL assistTools::injectDll(const std::wstring& dllPath) {
	BOOL bRet = FALSE;
	HANDLE hThread = NULL;
	LPVOID pRemoteMem = NULL;
	DWORD exitCode = 0;
	do
	{
		if (!m_hProcess) {
			break;
		}
		pRemoteMem = VirtualAllocEx(m_hProcess, NULL, (dllPath.size() + 1) * sizeof(dllPath[0]), MEM_COMMIT, PAGE_READWRITE);
		if (!pRemoteMem) {
			break;
		}
		if (!WriteProcessMemory(m_hProcess, pRemoteMem, dllPath.data(), (dllPath.size() + 1) * sizeof(dllPath[0]), NULL)) {
			break;
		}
		HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32.dll"));
		if (!hKernel32) {
			break;
		}
		LPVOID pLoadLib = (LPVOID)GetProcAddress(hKernel32, "LoadLibraryW");
		if (!pLoadLib) {
			break;
		}
		hThread = CreateRemoteThread(m_hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLib, pRemoteMem, 0, NULL);
		if (!hThread) {
			break;
		}
		WaitForSingleObject(hThread, INFINITE);
		GetExitCodeThread(hThread, &exitCode);
		if (!exitCode) {
			break;
		}
		bRet = TRUE;
	} while (false);
	if (pRemoteMem) {
		VirtualFreeEx(m_hProcess, pRemoteMem, 0, MEM_RELEASE);
	}
	if (hThread) {
		CloseHandle(hThread);
	}
	return bRet;
}
BOOL assistTools::installHook(const std::wstring& dllPath, bool bUninstallHook) {
	BOOL bRet = FALSE;
	HMODULE hDll = NULL;
	do
	{
		if (!injectDll(dllPath)) {
			break;
		}
		hDll = LoadLibrary(dllPath.data());
		if (!hDll) {
			break;
		}
		typedef BOOL(*InstallHookFunc)(DWORD, HWND);
		InstallHookFunc pInstallHook = (InstallHookFunc)GetProcAddress(hDll, "InstallHook");
		typedef void (*UninstallHookFunc)();
		UninstallHookFunc pUninstallHook = (UninstallHookFunc)GetProcAddress(hDll, "UninstallHook");
		if (!pInstallHook || !pUninstallHook) {
			break;
		}
		auto vTid = getTids();
		if (vTid.empty()) {
			break;
		}
		if (bUninstallHook) {
			pUninstallHook();
			if (hDll) {
				FreeLibrary(hDll);
			}
		}
		else {
			if (!pInstallHook(vTid[0], 0)) {
				break;
			}
		}
		bRet = TRUE;
	} while (false);
	return bRet;
}
void assistTools::initm_SysFun() {
	HMODULE hUser32 = GetModuleHandle(TEXT("user32.dll"));
	HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32.dll"));
	HMODULE hAdvapi32 = GetModuleHandle(TEXT("advapi32.dll"));
	HMODULE hWs2_32 = GetModuleHandle(TEXT("ws2_32.dll"));
	sysFun_t pSysFun[] = {
		{TEXT("CreateWindowEx"),hUser32},
		{TEXT("PostMessage"),hUser32},
		{TEXT("SendMessage"),hUser32},

		{TEXT("CreateProcess"),hKernel32},
		{TEXT("LoadLibrary"),hKernel32},
		{TEXT("GetProcAddress"),hKernel32},
		{TEXT("CreateFile"),hKernel32},
		{TEXT("DeleteFile"),hKernel32},
		{TEXT("MoveFile"),hKernel32},
		
		{TEXT("RegOpenKeyEx"),hAdvapi32},
		{TEXT("RegSetValueEx"),hAdvapi32},
		NULL
	};
	for (int i = 0; pSysFun[i].mod; i++)
	{
		m_mSysFun[pSysFun[i].fun] = pSysFun[i];
	}
}