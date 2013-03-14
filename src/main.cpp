#include <Windows.h>


TCHAR g_szClassName[] = TEXT("WICSample");


HRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_DESTROY:
		{
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
	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon		 = LoadIcon(NULL, IDI_APPLICATION);
	wc.hInstance	 = hInstance;
	wc.lpfnWndProc	 = WndProc;
	wc.lpszClassName = g_szClassName;
	if (!RegisterClassEx(&wc)) {
		MessageBox(NULL, TEXT("Error RegisterClassEx()"), NULL, MB_ICONERROR);
		return 0;
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
		return 0;
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
