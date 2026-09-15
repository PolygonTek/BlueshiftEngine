#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <tchar.h>

class DIB {
public:
    DIB() = default;
    DIB(const DIB &) = delete;
    DIB &                           operator=(const DIB &) = delete;
    ~DIB() { Destroy(); }

    void                            Create(int w, int h);
    void                            Destroy();

    BYTE *                          GetPtr(int x, int y) const;
    DWORD                           GetPixel(int x, int y) const;
    void                            SetPixel(int x, int y, COLORREF rgb);
    void                            GetColor(int x, int y, float *color) const;
    void                            SetColor(int x, int y, const float *color);

    void                            Fill(const float *color);
    void                            DrawLine(int x1, int y1, int x2, int y2, COLORREF rgb);
    void                            Blt(HDC hDC, int destX, int destY, const RECT *srcRect = nullptr) const;

    int                             WriteBMP(const TCHAR *filename) const;
    int                             WritePNG(const TCHAR *filename) const;

private:
    int                             width = 0;
    int                             height = 0;
    int                             pitch = 0;
    BYTE *                          data = nullptr;
    HBITMAP                         bitmapHandle = nullptr;
};
