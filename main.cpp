#define WIN32_LEAN_AND_MEAN
#include <SDKDDKVer.h>
#include <windows.h>
#include <ShellAPI.h>
#include <random>
#include "resource.h"

HINSTANCE hMainWindowInstance;

struct DedBitmap {
	BITMAP bitmap;
	HBITMAP hBitmap;
	HBITMAP hInitialBitmap;
	HDC hdcMem;

	DedBitmap() {};

	DedBitmap( HWND hWnd, UINT flags ) {
		hBitmap = ( HBITMAP ) LoadImage( hMainWindowInstance, MAKEINTRESOURCE( BITMAP_DED ), IMAGE_BITMAP, 0, 0, flags );
		GetObject( hBitmap, sizeof( BITMAP ), &bitmap );
		auto hdc = GetDC( hWnd ); {
			hdcMem = CreateCompatibleDC( hdc );
			hInitialBitmap = ( HBITMAP ) SelectObject( hdcMem, hBitmap );
		} ReleaseDC( hWnd, hdc );
	}

	void Delete() {
		DeleteDC( hdcMem );
		DeleteObject( hBitmap );
		DeleteObject( hInitialBitmap );
	}
};

DedBitmap dedBitmap;
DedBitmap dedBitmapMonochrome;
NOTIFYICONDATA dedTrayIcon = {};

bool debugMode = false;
bool dedMonochrome = false;
bool dedUpsideDown = false;

void SetupTray( HWND hWnd ) {
	dedTrayIcon.cbSize = sizeof( NOTIFYICONDATA );
	dedTrayIcon.hWnd = hWnd;
	dedTrayIcon.uID = 1;
	dedTrayIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	dedTrayIcon.hIcon = LoadIcon( hMainWindowInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
	lstrcpyW( dedTrayIcon.szTip, L"ded" );
	Shell_NotifyIcon( NIM_ADD, &dedTrayIcon );
}

template <typename T>
bool inRange( T value, T min, T max ) {
	return value >= min && value <= max;
}

void ShowDedRandomly( HWND hWnd ) {
	static std::random_device rd;
	static std::mt19937 gen( rd() );

	static std::uniform_int_distribution<int> dedScaleDistribution( 78, 208 );
	static std::uniform_int_distribution<int> dedScaleErrorDistribution( -48, 48 );
	static std::uniform_real_distribution<float> unlikelyDistribution( 0, 100 );
	int dedWidth = dedScaleDistribution( gen );
	int dedHeight = dedWidth;
	dedWidth += dedScaleErrorDistribution( gen );
	dedHeight += dedScaleErrorDistribution( gen );

	int screenWidth = GetSystemMetrics( SM_CXSCREEN );
	int screenHeight = GetSystemMetrics( SM_CYSCREEN );
	std::uniform_int_distribution<int> xPosDistribution( 24, screenWidth - dedWidth - 24  );
	std::uniform_int_distribution<int> yPosDistribution( 24, screenHeight - dedHeight - 24 );

	auto dedX = xPosDistribution( gen );
	auto dedY = yPosDistribution( gen );
	dedMonochrome = inRange( unlikelyDistribution( gen ), debugMode ? 0.0f : 42.0f, 42.2f );
	dedUpsideDown = inRange( unlikelyDistribution( gen ), debugMode ? 0.0f : 42.0f, 42.2f );

	SetWindowPos( hWnd, HWND_TOPMOST, dedX, dedY, dedWidth, dedHeight, NULL );
}

LRESULT CALLBACK MainWindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	switch ( uMsg ) {
		case WM_CREATE:
		{
			dedBitmap = DedBitmap( hWnd, 0 );
			dedBitmapMonochrome = DedBitmap( hWnd, LR_MONOCHROME );
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

		case WM_CHAR: {
			if ( debugMode && wParam == L'q' ) {
				ShowDedRandomly( hWnd );
			}
			break;
		}

		case WM_PAINT: {
			PAINTSTRUCT ps;
			RECT rect;

			auto hdc = BeginPaint( hWnd, &ps ); {
				GetClientRect( hWnd, &rect );
				SetStretchBltMode( hdc, STRETCH_HALFTONE );

				const auto &dedBitmapInstance = dedMonochrome ? dedBitmapMonochrome : dedBitmap;
				const auto &bitmap = dedBitmapInstance.bitmap;

				if ( dedUpsideDown ) {
					StretchBlt(
						hdc, 0, 0, rect.right, rect.bottom,
						dedBitmapInstance.hdcMem, bitmap.bmWidth, bitmap.bmHeight, -bitmap.bmWidth, -bitmap.bmHeight,
						SRCCOPY
					);
				} else {
					StretchBlt(
						hdc, 0, 0, rect.right, rect.bottom,
						dedBitmapInstance.hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight,
						SRCCOPY
					);
				}
			} EndPaint( hWnd, &ps );
			break;
		}

		case WM_DESTROY: {
			PostQuitMessage( 0 );
			Shell_NotifyIcon( NIM_DELETE, &dedTrayIcon );
			dedBitmap.Delete();
			dedBitmapMonochrome.Delete();
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
