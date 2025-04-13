#include "assistTools.h"
#include <Windows.h>
#include <iostream>
#include <iomanip>
using namespace std;

int main()
{
    vector<DWORD_PTR> vRes;
    //assistTools atools(TEXT("PlantsVsZombies.exe"));
    assistTools atools(TEXT("zhg_test.exe"));
#if 0
    int val = 0;
    cout << "input: \n";
    cin >> val;
    vRes = atools.scanMemory(val);
    
    int index = 1;
    cout << index++ << ":\n";
    for (const auto& v : vRes) {
        cout << std::hex << v << endl;
    }
    cout << "*******************\n";
    while (vRes.size() > 1) {
        cout << "input: \n";
        cin >> val;
        vRes = atools.scanMemory(vRes, val);
        cout << index++ << ":\n";
        for (const auto& v : vRes) {
            cout << std::hex << v << endl;
        }
        cout << "*******************\n";
    }
    if (vRes.size()) {
        cout << "input: \n";
        cin >> val;
        atools.writeMemory(vRes, val);
    }
    
    atools.injectDll(TEXT("C:\\Users\\Administrator\\Documents\\GitHub\\cs_assist\\Debug\\zhg_dll.dll"));
    auto vPath = atools.getModPaths();
    cout << "all mod:\n";
    for (const auto& v : vPath) {
        wcout << v << endl;
    }
    atools.getTids();
#endif
    atools.installHook(TEXT("C:\\Users\\Administrator\\Documents\\GitHub\\cs_assist\\Debug\\hook.dll"));
    atools.installHook(TEXT("C:\\Users\\Administrator\\Documents\\GitHub\\cs_assist\\Debug\\hook.dll"),true);
    return 0;
}
