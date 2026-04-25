#include <windows.h>
#include <iostream>
#include <string>
#include "ScreenMetric.h"

ScreenMetric screenMetric{};
int boundaryLoopThreshold = 5;
bool debugMode = false;

LRESULT CALLBACK hookMouseLoop(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0)
    {
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    if (wParam != WM_MOUSEMOVE)
    {
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    MSLLHOOKSTRUCT *pMouseStruct = reinterpret_cast<MSLLHOOKSTRUCT *>(lParam);
    int x = pMouseStruct->pt.x;
    int y = pMouseStruct->pt.y;

    bool cross = false;
    int crossedX = x;
    int crossedY = y;

    if (x < boundaryLoopThreshold)
    {
        cross = true;
        crossedX = screenMetric.xDeviceSize() - boundaryLoopThreshold * 2;
    }
    else if (x > screenMetric.xDeviceSize() - boundaryLoopThreshold)
    {
        cross = true;
        crossedX = boundaryLoopThreshold * 2;
    }

    if (y < boundaryLoopThreshold)
    {
        cross = true;
        crossedY = screenMetric.yDeviceSize() - boundaryLoopThreshold * 2;
    }
    else if (y > screenMetric.yDeviceSize() - boundaryLoopThreshold)
    {
        cross = true;
        crossedY = boundaryLoopThreshold * 2;
    }

    if (cross)
    {
        int sysX = screenMetric.xDeviceToSystem(crossedX);
        int sysY = screenMetric.yDeviceToSystem(crossedY);
        SetCursorPos(sysX, sysY);
        if (debugMode)
        {
            std::cout << "Teleported cursor: device(" << crossedX << "," << crossedY << ") -> system(" << sysX << "," << sysY << ")" << std::endl;
        }
        // Block the original mouse move, otherwise the cursor will be moved back
        return 1;
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main(int argc, char *argv[])
{
    // Parse command-line arguments
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--debug" || arg == "-d")
        {
            debugMode = true;
            AllocConsole();
            FILE *dummy;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            std::cout << "MouseLoop debug mode enabled." << std::endl;
            std::cout << "Screen device size: " << screenMetric.xDeviceSize() << "x" << screenMetric.yDeviceSize() << std::endl;
            std::cout << "Screen system size: " << screenMetric.xSystemSize() << "x" << screenMetric.ySystemSize() << std::endl;
        }
    }

    HHOOK hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, hookMouseLoop, NULL, 0);

    if (hMouseHook == NULL)
    {
        MessageBox(NULL, "Failed to install mouse hook", "Error", MB_ICONERROR);
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hMouseHook);

    return 0;
}