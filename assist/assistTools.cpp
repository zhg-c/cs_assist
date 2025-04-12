#include "assistTools.h"

#include <TlHelp32.h>
#include <iostream>
#define MAX_SIZE 4096

assistTools::assistTools(const std::wstring& processName) :
	m_pid(0),
	m_hProcess(NULL)
{
	m_pid = getPid(processName);
	m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_pid);
}

DWORD assistTools::getPid(const std::wstring& processName) {
	HANDLE hsnapshot = INVALID_HANDLE_VALUE;
	DWORD pid = 0;
	do
	{
		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);
		hsnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hsnapshot == INVALID_HANDLE_VALUE) {
			break;
		}
		if (!Process32First(hsnapshot, &pe32)) {
			break;
		}
		do
		{
			if (processName == pe32.szExeFile) {
				pid = pe32.th32ProcessID;
				break;
			}
		} while (Process32Next(hsnapshot, &pe32));
	} while (false);
	if (hsnapshot != INVALID_HANDLE_VALUE) {
		CloseHandle(hsnapshot);
	}
	return pid;
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