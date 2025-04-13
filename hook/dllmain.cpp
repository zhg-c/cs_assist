// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <Windows.h>
#include <fstream>

// 全局变量
HINSTANCE g_hInstance = NULL;
HHOOK g_hHook = NULL;
HWND g_hwndTarget = NULL; // 可选：指定目标窗口

// 日志文件路径
const wchar_t* LOG_FILE = L"C:\\hook_log.txt";

// 钩子过程函数
LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        CWPSTRUCT* pMsg = (CWPSTRUCT*)lParam;

        // 可选：只监控特定窗口
        if (g_hwndTarget == NULL || pMsg->hwnd == g_hwndTarget)
        {
            // 获取当前时间
            SYSTEMTIME st;
            GetLocalTime(&st);
            if (pMsg->message == WM_CREATE) {

				// 记录到日志文件
				std::wofstream log(L"C:\\messagebox_log.txt", std::ios::app);
				if (log.is_open()) {
					static int i;
					if (!(i % 50)) {
						log << "\n";
					}
					log << i++ << " ";
					log.close();
				}
            }

            // 示例：拦截WM_CLOSE消息
            if (pMsg->message == WM_CLOSE)
            {
                // 可以在这里阻止窗口关闭
                // return 1; // 阻止消息传递
            }
        }
    }

    // 传递给下一个钩子
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

// 安装钩子
extern "C" __declspec(dllexport) BOOL InstallHook(DWORD threadId, HWND hwndTarget = NULL)
{
    g_hwndTarget = hwndTarget;
    g_hHook = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, g_hInstance, threadId);
    return g_hHook != NULL;
}

// 卸载钩子
extern "C" __declspec(dllexport) void UninstallHook()
{
    if (g_hHook)
    {
        UnhookWindowsHookEx(g_hHook);
        g_hHook = NULL;
    }
}

// DLL入口点
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hModule;
        DisableThreadLibraryCalls(hModule); // 优化：禁用线程通知
        break;
    case DLL_PROCESS_DETACH:
        UninstallHook(); // 确保卸载钩子
        break;
    }
    return TRUE;
}
