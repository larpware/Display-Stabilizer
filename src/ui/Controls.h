#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>

namespace DisplayStabilizer {

class UiHelpers {
public:
    static int scale(int val, int dpi) {
        return MulDiv(val, dpi, 96);
    }

    static HFONT createDefaultFont(int pointSize, bool bold = false, int dpi = 96) {
        int height = -MulDiv(pointSize, dpi, 72);
        return CreateFontW(
            height, 0, 0, 0,
            bold ? FW_BOLD : FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI"
        );
    }

    static HWND createLabel(HWND parent, const wchar_t* text, int x, int y, int w, int h, int id = 0, DWORD style = 0) {
        return CreateWindowExW(
            0, L"STATIC", text,
            WS_CHILD | WS_VISIBLE | SS_LEFT | style,
            x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
            GetModuleHandle(nullptr), nullptr
        );
    }

    static HWND createButton(HWND parent, const wchar_t* text, int x, int y, int w, int h, int id, DWORD style = 0) {
        return CreateWindowExW(
            0, L"BUTTON", text,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | style,
            x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
            GetModuleHandle(nullptr), nullptr
        );
    }

    static HWND createTrackbar(HWND parent, int x, int y, int w, int h, int id, int minVal, int maxVal, int curVal) {
        HWND hTb = CreateWindowExW(
            0, TRACKBAR_CLASSW, L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_HORZ | TBS_AUTOTICKS,
            x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
            GetModuleHandle(nullptr), nullptr
        );
        if (hTb) {
            SendMessageW(hTb, TBM_SETRANGE, TRUE, MAKELPARAM(minVal, maxVal));
            SendMessageW(hTb, TBM_SETPOS, TRUE, curVal);
            SendMessageW(hTb, TBM_SETTICFREQ, (maxVal - minVal) / 10, 0);
        }
        return hTb;
    }

    static HWND createComboBox(HWND parent, int x, int y, int w, int h, int id) {
        return CreateWindowExW(
            0, L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
            x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
            GetModuleHandle(nullptr), nullptr
        );
    }

    static HWND createGroupBox(HWND parent, const wchar_t* title, int x, int y, int w, int h) {
        return CreateWindowExW(
            0, L"BUTTON", title,
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            x, y, w, h, parent, nullptr,
            GetModuleHandle(nullptr), nullptr
        );
    }

    static HWND createListView(HWND parent, int x, int y, int w, int h, int id, DWORD style = 0) {
        return CreateWindowExW(
            WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | style,
            x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
            GetModuleHandle(nullptr), nullptr
        );
    }
};

} // namespace DisplayStabilizer
