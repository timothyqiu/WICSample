#include <Windows.h>
#include <windowsx.h>
#include <wincodec.h>

// Just For AlphaBlend
#pragma comment(lib, "Msimg32.lib")

TCHAR g_szClassName[] = TEXT("WICSample");


template <typename T>
inline void SafeRelease(T *&obj) {
    if (obj) {
        obj->Release();
        obj = NULL;
    }
}


HBITMAP CreateHBitmapFromBitmapSource(IWICBitmapSource *pBitmap)
{
    HRESULT hr = S_OK;

    // Make sure that the pixel format is 32bppBGRA
    WICPixelFormatGUID format;
    hr = pBitmap->GetPixelFormat(&format);
    if (SUCCEEDED(hr)) {
        hr = (format == GUID_WICPixelFormat32bppBGRA) ? S_OK : E_FAIL;
    }

    // Get bitmap size
    UINT cxBitmap = 0;
    UINT cyBitmap = 0;
    if (SUCCEEDED(hr)) {
        hr = pBitmap->GetSize(&cxBitmap, &cyBitmap);
    }

    // Create DIB
    HBITMAP hBitmap = NULL;
    LPVOID pvImageBits = NULL;

    if (SUCCEEDED(hr)) {
        BITMAPINFO bminfo = {};
        bminfo.bmiHeader.biSize         = sizeof(BITMAPINFO);
        bminfo.bmiHeader.biBitCount     = 32;   // FIXME: no magic number
        bminfo.bmiHeader.biCompression  = BI_RGB;
        bminfo.bmiHeader.biWidth        = cxBitmap;
        bminfo.bmiHeader.biHeight       = - static_cast<LONG>(cyBitmap);
        bminfo.bmiHeader.biPlanes       = 1;

        HDC hScreenDC = GetDC(HWND_DESKTOP);
        hr = hScreenDC ? S_OK : E_FAIL;

        if (SUCCEEDED(hr)) {
            hBitmap = CreateDIBSection(hScreenDC,
                                       &bminfo,
                                       DIB_RGB_COLORS,
                                       &pvImageBits,
                                       NULL,
                                       0);
            hr = hBitmap ? S_OK : E_FAIL;
        }
    }

    // Fill in image data
    if (SUCCEEDED(hr)) {
        hr = pBitmap->CopyPixels(NULL,
                                 cxBitmap * 4,
                                 cxBitmap * cyBitmap * 4,
                                 reinterpret_cast<BYTE *>(pvImageBits));
    }

    if (FAILED(hr)) {
        if (hBitmap) {
            DeleteBitmap(hBitmap);
            hBitmap = NULL;
        }
    }

    return hBitmap;
}

HBITMAP LoadBitmapByWIC(LPCWSTR wzFilename)
{
    HRESULT hr = S_OK;
    HBITMAP hBitmap = NULL;

    IWICImagingFactory *pFactory = NULL;
    IWICBitmapDecoder *pDecoder = NULL;
    IWICBitmapFrameDecode *pFrame = NULL;

    hr = CoCreateInstance(CLSID_WICImagingFactory,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&pFactory));

    if (SUCCEEDED(hr)) {
        hr = pFactory->CreateDecoderFromFilename(wzFilename,
                                                 NULL,
                                                 GENERIC_READ,
                                                 WICDecodeMetadataCacheOnDemand,
                                                 &pDecoder);
    }

    if (SUCCEEDED(hr)) {
        hr = pDecoder->GetFrame(0, &pFrame);
    }

    // Convert to 32bppBGRA for future use :)
    if (SUCCEEDED(hr)) {
        IWICFormatConverter *pConverter = NULL;

        hr = pFactory->CreateFormatConverter(&pConverter);

        if (SUCCEEDED(hr)) {
            hr = pConverter->Initialize(pFrame,
                                        GUID_WICPixelFormat32bppBGRA,
                                        WICBitmapDitherTypeNone,
                                        NULL,
                                        0.f,
                                        WICBitmapPaletteTypeCustom);

            if (SUCCEEDED(hr)) {
                IWICBitmapSource *pFinalImage = NULL;

                hr = pConverter->QueryInterface(IID_PPV_ARGS(&pFinalImage));

                if (SUCCEEDED(hr)) {
                    hBitmap = CreateHBitmapFromBitmapSource(pFinalImage);
                }

                SafeRelease(pFinalImage);
            }
        }

        SafeRelease(pConverter);
    }

    SafeRelease(pFrame);
    SafeRelease(pDecoder);
    SafeRelease(pFactory);

    return hBitmap;
}


HRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HBITMAP hBackground = NULL;

    switch (message) {
    case WM_NCCREATE:
        {
            // Initialize COM
            CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
            // Create the bitmap using WIC
            hBackground = LoadBitmapByWIC(L"images/Dirt Block.png");
            // Done
            CoUninitialize();

            if (!hBackground)
                return FALSE;
        }
        return TRUE;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            {
                BITMAP bm = {};
                GetObject(hBackground, sizeof(BITMAP), &bm);

                HDC hMemDC = CreateCompatibleDC(ps.hdc);
                HBITMAP hPrevBitmap = SelectBitmap(hMemDC, hBackground);
                {
                    //BitBlt(ps.hdc, 0, 0, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
                    BLENDFUNCTION blend = {};
                    blend.AlphaFormat           = AC_SRC_ALPHA;
                    blend.BlendOp               = AC_SRC_OVER;
                    blend.SourceConstantAlpha   = 255;

                    AlphaBlend(ps.hdc, 0, 0, bm.bmWidth, bm.bmHeight,
                               hMemDC, 0, 0, bm.bmWidth, bm.bmHeight,
                               blend);
                }
                SelectBitmap(hMemDC, hPrevBitmap);
                DeleteDC(hMemDC);
            }
            EndPaint(hWnd, &ps);
        }
        return 0;

    case WM_DESTROY:
        {
            // Delete the bitmap
            if (hBackground) {
                DeleteBitmap(hBackground);
                hBackground = NULL;
            }
            PostQuitMessage(0);
        }
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR szCmdLine, int iCmdShow)
{
    WNDCLASSEX wc = {};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hInstance     = hInstance;
    wc.lpfnWndProc   = WndProc;
    wc.lpszClassName = g_szClassName;
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, TEXT("Error RegisterClassEx()"), NULL, MB_ICONERROR);
        return 1;
    }

    HWND hWnd = CreateWindowEx(0,
                               g_szClassName,
                               TEXT("Windows Imaging Component"),
                               WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT,
                               CW_USEDEFAULT, CW_USEDEFAULT,
                               HWND_DESKTOP, NULL, hInstance, NULL);
    if (!hWnd) {
        MessageBox(NULL, TEXT("Error CreateWindowEx()"), NULL, MB_ICONERROR);
        return 2;
    }

    ShowWindow(hWnd, iCmdShow);
    UpdateWindow(hWnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
