#define WIN32_LEAN_AND_MEAN
#include <SDKDDKVer.h>
#include <windows.h>
#include "resource.h"

HINSTANCE hMainWindowInstance;

BITMAP dedBitmap;
HBITMAP hDedBitmap;
HBITMAP hInitialDedBitmap;
HDC hdcMem;

void SetupBitmap( HWND hWnd ) {
	hDedBitmap = ( HBITMAP ) LoadImage( hMainWindowInstance, MAKEINTRESOURCE( BITMAP_DED ), IMAGE_BITMAP, 0, 0, 0 );
	GetObject( hDedBitmap, sizeof( BITMAP ), &dedBitmap );
	auto hdc = GetDC( hWnd ); {
		hdcMem = CreateCompatibleDC( hdc );
		hInitialDedBitmap = ( HBITMAP ) SelectObject( hdcMem, hDedBitmap );
	} ReleaseDC( hWnd, hdc );
}

LRESULT CALLBACK MainWindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	switch ( uMsg ) {
		case WM_CREATE:
		{
			SetupBitmap( hWnd );
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
			DeleteDC( hdcMem );
			DeleteObject( hDedBitmap );
			DeleteObject( hInitialDedBitmap );
			return 0;
		}

		default: {
			break;
		}
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow ) {
	hMainWindowInstance = hInstance;
	auto MAIN_CLASS_NAME = L"MainClass";

	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = MAIN_CLASS_NAME;
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );

	RegisterClass( &wc );

	HWND hwnd = CreateWindowEx(
		0,
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
