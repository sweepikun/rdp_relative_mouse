#include <windows.h>
#include <iostream>
#include <string>
#include "ScreenMetric.h"

// Unique signature to identify our own SendInput calls
static constexpr ULONG_PTR OWN_INPUT_SIGNATURE = 0x52444D55; // "RDMU"

void sendRelativeMouseMove(int dx, int dy)
{
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.mouseData = 0;
    input.mi.dwExtraInfo = OWN_INPUT_SIGNATURE;
    input.mi.time = 0;
    SendInput(1, &input, sizeof(INPUT));
}

ScreenMetric screenMetric{};
int lastX = 0;
int lastY = 0;
int lastDx = 0;
int lastDy = 0;
bool isFirstMove = true;
int crossXBoundaryJudgeThreshold;
int crossYBoundaryJudgeThreshold;
bool debugMode = false;

LRESULT CALLBACK hookRelativeMove(int nCode, WPARAM wParam, LPARAM lParam)
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

    // Block our own SendInput calls precisely by signature
    if (pMouseStruct->dwExtraInfo == OWN_INPUT_SIGNATURE)
    {
        return 1;
    }

    int x = pMouseStruct->pt.x;
    int y = pMouseStruct->pt.y;

    // Initialize on first real mouse move to avoid a huge initial jump
    if (isFirstMove)
    {
        lastX = x;
        lastY = y;
        isFirstMove = false;
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    int dx = x - lastX;
    int dy = y - lastY;

    // Cross-boundary correction for MouseLoop teleportation
    if (dx < -crossXBoundaryJudgeThreshold && lastDx > 0)
    {
        dx += screenMetric.xDeviceSize();
    }
    else if (dx > crossXBoundaryJudgeThreshold && lastDx < 0)
    {
        dx -= screenMetric.xDeviceSize();
    }
    if (dy < -crossYBoundaryJudgeThreshold && lastDy > 0)
    {
        dy += screenMetric.yDeviceSize();
    }
    else if (dy > crossYBoundaryJudgeThreshold && lastDy < 0)
    {
        dy -= screenMetric.yDeviceSize();
    }

    // Ignore giant jumps that are likely caused by games resetting cursor position
    // (e.g. Minecraft calling SetCursorPos to warp cursor to screen center)
    int maxReasonableDelta = screenMetric.xDeviceSize() / 4;
    if (abs(dx) > maxReasonableDelta || abs(dy) > maxReasonableDelta)
    {
        if (debugMode)
        {
            std::cout << "Ignored jump: dx=" << dx << " dy=" << dy << std::endl;
        }
        lastX = x;
        lastY = y;
        lastDx = 0;
        lastDy = 0;
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    if (dx != 0 || dy != 0)
    {
        if (debugMode)
        {
            std::cout << "Sending relative move: dx=" << dx << " dy=" << dy << std::endl;
        }
        sendRelativeMouseMove(dx, dy);
    }

    lastX = x;
    lastY = y;
    lastDx = dx;
    lastDy = dy;

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
            std::cout << "Debug mode enabled." << std::endl;
        }
    }

    crossXBoundaryJudgeThreshold = screenMetric.xDeviceSize() / 2;
    crossYBoundaryJudgeThreshold = screenMetric.yDeviceSize() / 2;

    if (debugMode)
    {
        std::cout << "Screen device size: " << screenMetric.xDeviceSize() << "x" << screenMetric.yDeviceSize() << std::endl;
        std::cout << "Screen system size: " << screenMetric.xSystemSize() << "x" << screenMetric.ySystemSize() << std::endl;
    }

    HHOOK hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, hookRelativeMove, NULL, 0);

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