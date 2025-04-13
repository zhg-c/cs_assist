#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <map>

class assistTools
{
public:
	assistTools(const std::wstring& processName);
	~assistTools() = default;
public:
	DWORD getPid() { return m_pid; }
	std::vector<DWORD_PTR> scanMemory(int val);
	std::vector<DWORD_PTR> scanMemory(const std::vector<DWORD_PTR>& preRes, int val);
	BOOL writeMemory(const std::vector<DWORD_PTR>& vRes, int val);
	std::vector<std::wstring> getModPaths();
	BOOL injectDll(const std::wstring &dllPath);
	BOOL installHook(const std::wstring& dllPath,bool bUninstallHook = false);
	std::vector<DWORD> getTids();
private:
	DWORD getPid(const std::wstring& processName);
	void initm_SysFun();
private:
	DWORD m_pid, m_tid;
	HANDLE m_hProcess;
	typedef struct {
		LPCWSTR fun;
		HMODULE mod;
	} sysFun_t;
	std::map<std::wstring, sysFun_t> m_mSysFun;
public:
	static HHOOK m_hHook;
};
