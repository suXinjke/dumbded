#define WIN32_LEAN_AND_MEAN
#include <SDKDDKVer.h>
#include <windows.h>
#include <ShellAPI.h>
#include "resource.h"

HINSTANCE hMainWindowInstance;

BITMAP dedBitmap;
HBITMAP hDedBitmap;
HBITMAP hInitialDedBitmap;
HDC hdcMem;
NOTIFYICONDATA dedTrayIcon = {};

bool debugMode = false;

void SetupBitmap( HWND hWnd ) {
	hDedBitmap = ( HBITMAP ) LoadImage( hMainWindowInstance, MAKEINTRESOURCE( BITMAP_DED ), IMAGE_BITMAP, 0, 0, 0 );
	GetObject( hDedBitmap, sizeof( BITMAP ), &dedBitmap );
	auto hdc = GetDC( hWnd ); {
		hdcMem = CreateCompatibleDC( hdc );
		hInitialDedBitmap = ( HBITMAP ) SelectObject( hdcMem, hDedBitmap );
	} ReleaseDC( hWnd, hdc );
}

void SetupTray( HWND hWnd ) {
	dedTrayIcon.cbSize = sizeof( NOTIFYICONDATA );
	dedTrayIcon.hWnd = hWnd;
	dedTrayIcon.uID = 1;
	dedTrayIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	dedTrayIcon.hIcon = LoadIcon( hMainWindowInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
	lstrcpyW( dedTrayIcon.szTip, L"ded" );
	Shell_NotifyIcon( NIM_ADD, &dedTrayIcon );
}

LRESULT CALLBACK MainWindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	switch ( uMsg ) {
		case WM_CREATE:
		{
			SetupBitmap( hWnd );
			SetupTray( hWnd );
			SetWindowPos( hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
			SetTimer( hWnd, NULL, 1000, []( HWND hWnd, UINT, UINT_PTR, DWORD ) {
				// do something every second
			} );
			break;
		}

		case WM_LBUTTONDOWN: {
			// HACK: pretend title bar is being dragged
			ReleaseCapture();
			SendMessage( hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0 );
			break;
		}

		case WM_PAINT: {
			PAINTSTRUCT ps;
			RECT rect;

			auto hdc = BeginPaint( hWnd, &ps ); {
				GetClientRect( hWnd, &rect );
				SetStretchBltMode( hdc, STRETCH_HALFTONE );
				StretchBlt(
					hdc, 0, 0, rect.right, rect.bottom,
					hdcMem, 0, 0, dedBitmap.bmWidth, dedBitmap.bmHeight,
					SRCCOPY
				);
			} EndPaint( hWnd, &ps );
			break;
		}

		case WM_DESTROY: {
			PostQuitMessage( 0 );
			Shell_NotifyIcon( NIM_DELETE, &dedTrayIcon );
			DeleteDC( hdcMem );
			DeleteObject( hDedBitmap );
			DeleteObject( hInitialDedBitmap );
			return 0;
		}

		case WM_CLOSE: {
			if ( debugMode ) {
				break;
			} else {
				return 0;
			}
		}

		default: {
			break;
		}
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow ) {
	hMainWindowInstance = hInstance;
	debugMode = lstrcmpW( pCmdLine, L"debug-mode" ) == 0;
	auto MAIN_CLASS_NAME = L"MainClass";

	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = MAIN_CLASS_NAME;
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );

	RegisterClass( &wc );

	HWND hwnd = CreateWindowEx(
		WS_EX_TOOLWINDOW,
		MAIN_CLASS_NAME,
		L"ded",
		WS_POPUP,
		64, 64,
		256, 256,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if ( !hwnd ) {
		return 0;
	}

	ShowWindow( hwnd, nCmdShow );

	MSG msg = { };
	while ( GetMessage( &msg, NULL, 0, 0 ) ) {
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	return 0;
}
