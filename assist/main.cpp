#include "assistTools.h"
#include <Windows.h>
#include <iostream>
#include <iomanip>
using namespace std;

int main()
{
    vector<DWORD_PTR> vRes;
    assistTools atools(TEXT("zhg_test.exe"));
    int val = 0;
    cin >> val;
    vRes = atools.scanMemory(val);
    
    int index = 1;
    cout << index++ << ":\n";
    for (const auto& v : vRes) {
        cout << std::hex << v << endl;
    }
    cout << "*******************\n";
    while (vRes.size() > 1) {
        cin >> val;
        vRes = atools.scanMemory(vRes, val);
        cout << index++ << ":\n";
        for (const auto& v : vRes) {
            cout << std::hex << v << endl;
        }
        cout << "*******************\n";
    }
    
    return 0;
}
