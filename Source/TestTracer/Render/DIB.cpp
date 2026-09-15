#include "Precompiled.h"
#include "Render/DIB.h"

void DIB::Create(int w, int h) {
    Destroy();
    BITMAPINFO bi;
    memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC hDC = GetDC(GetDesktopWindow());
    bitmapHandle = CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, (void **)&data, nullptr, 0);
    ReleaseDC(GetDesktopWindow(), hDC);

    width = w;
    height = h;
    pitch = ((w * 24 + 31) / 32) << 2;
}

void DIB::Destroy() {
    if (bitmapHandle) {
        DeleteObject(bitmapHandle);
        bitmapHandle = nullptr;
    }
    data = nullptr;
    width = height = pitch = 0;
}

BYTE *DIB::GetPtr(int x, int y) const {
    return data + y * pitch + x * 3;
}

COLORREF DIB::GetPixel(int x, int y) const {
    BYTE *ptr = GetPtr(x, y);
    RGBTRIPLE *rgbt = (RGBTRIPLE *)ptr;
    return (COLORREF)((((DWORD)rgbt->rgbtBlue) << 16) | (((DWORD)rgbt->rgbtGreen) << 8) | ((DWORD)rgbt->rgbtRed));
}

void DIB::GetColor(int x, int y, float *color) const {
    DWORD rgb = GetPixel(x, y);
    color[0] = (rgb & 255) / 255.0f;
    color[1] = ((rgb >> 8) & 255) / 255.0f;
    color[2] = ((rgb >> 16) & 255) / 255.0f;
}

void DIB::SetPixel(int x, int y, COLORREF rgb) {
    BYTE *ptr = GetPtr(x, y);
    RGBTRIPLE rgbt = {static_cast<BYTE>((rgb >> 16) & 255), static_cast<BYTE>((rgb >> 8) & 255), static_cast<BYTE>(rgb & 255)};
    *(RGBTRIPLE *)ptr = rgbt;
}

void DIB::SetColor(int x, int y, const float *color) {
    DWORD rgb = RGB(BE1::Min(color[0], 1.0f) * 255, BE1::Min(color[1], 1.0f) * 255, BE1::Min(color[2], 1.0f) * 255);
    SetPixel(x, y, rgb);
}

void DIB::Fill(const float *color) {
    DWORD rgb = RGB(BE1::Min(color[0], 1.0f) * 255, BE1::Min(color[1], 1.0f) * 255, BE1::Min(color[2], 1.0f) * 255);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            SetPixel(x, y, rgb);
        }
    }
}

void DIB::DrawLine(int x1, int y1, int x2, int y2, COLORREF rgb) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int adx = abs(dx);
    int ady = abs(dy);
    int step = adx > ady ? adx : ady;
    if (step == 0) {
        return;
    }

    int x = (x1 << 8) + 127;
    int y = (y1 << 8) + 127;
    int xInc = (dx << 8) / step;
    int yInc = (dy << 8) / step;

    for (int i = 0; i < step; i++) {
        SetPixel(x >> 8, y >> 8, rgb);
        x += xInc;
        y += yInc;
    }
}

void DIB::Blt(HDC hDC, int destX, int destY, const RECT *srcRect) const {
    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP hOld = (HBITMAP)SelectObject(hMemDC, bitmapHandle);

    if (srcRect) {
        BitBlt(hDC, destX, destY, srcRect->right - srcRect->left, srcRect->bottom - srcRect->top, hMemDC, srcRect->left, srcRect->top, SRCCOPY);
    } else {
        BitBlt(hDC, destX, destY, width, height, hMemDC, 0, 0, SRCCOPY);
    }

    SelectObject(hMemDC, hOld);
    DeleteDC(hMemDC);
}

int DIB::WriteBMP(const TCHAR *filename) const {
    DWORD dwBytesWritten;
    HANDLE hFile;

    hFile = CreateFile(filename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        return 0;
    }

    int headerSize = sizeof(BITMAPFILEHEADER);
    int infoSize = sizeof(BITMAPINFOHEADER);
    int dataSize = pitch * height;

    BITMAPFILEHEADER bmfh;
    bmfh.bfType = MAKEWORD('B', 'M');
    bmfh.bfSize = headerSize + infoSize + dataSize;
    bmfh.bfReserved1 = 0;
    bmfh.bfReserved2 = 0;
    bmfh.bfOffBits = headerSize + infoSize;
    WriteFile(hFile, &bmfh, headerSize, &dwBytesWritten, nullptr);

    BITMAPINFO bi;
    memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24;
    bi.bmiHeader.biCompression = BI_RGB;
    WriteFile(hFile, &bi, infoSize, &dwBytesWritten, nullptr);

    WriteFile(hFile, data, dataSize, &dwBytesWritten, nullptr);

    CloseHandle(hFile);
    return 1;
}

int DIB::WritePNG(const TCHAR *filename) const {
    // GDI stores top-down BGR rows with 4-byte padding; Image expects packed rows.
    BE1::Image image;
    image.Create2D(width, height, 1, BE1::Image::Format::B8G8R8, BE1::Image::GammaSpace::sRGB, nullptr, BE1::Image::Flag::None);
    for (int y = 0; y < height; ++y) {
        memcpy(image.GetPixels() + y * width * 3, data + y * pitch, width * 3);
    }
    char utf8Filename[4096];
    if (!WideCharToMultiByte(CP_UTF8, 0, filename, -1, utf8Filename, sizeof(utf8Filename), nullptr, nullptr)) {
        return 0;
    }
    return image.WritePNG(utf8Filename) ? 1 : 0;
}
