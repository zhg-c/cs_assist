#pragma once
#include <Windows.h>
#include <string>
#include <vector>

class assistTools
{
public:
	assistTools(const std::wstring& processName);
	~assistTools() = default;
public:
	DWORD getPid() { return m_pid; }
	std::vector<DWORD_PTR> scanMemory(int val);
	std::vector<DWORD_PTR> scanMemory(const std::vector<DWORD_PTR>& preRes,int val);
	BOOL writeMemory(const std::vector<DWORD_PTR> &vRes,int val);
	std::vector<std::wstring> getModPaths();
	BOOL injectDll(const std::wstring &dllPath);
private:
	DWORD getPid(const std::wstring& processName);
private:
	DWORD m_pid;
	HANDLE m_hProcess;
};

