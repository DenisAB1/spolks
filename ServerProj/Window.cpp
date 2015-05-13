#include <Windows.h>

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

int ResWindowX;
int ResWindowY;

HWND CreateNewWindow(int ResolutionScreenClientX, int ResolutionScreenClientY)
{
	//что-то для установки нужного размера экрана и нахождения коэффициента
	ResWindowX = ResolutionScreenClientX;
	ResWindowY = ResolutionScreenClientY;

	static wchar_t szClassName[ ] = L"RemoteComputer";

	HINSTANCE hInstance = GetModuleHandle(NULL); // получение HINSTANCE приложения
    HWND hwnd;
    MSG msg;
    WNDCLASSEX wincl;
 
    wincl.hInstance = hInstance;							/*				*/
    wincl.lpszClassName = szClassName;						//
    wincl.lpfnWndProc = WndProc;							//
    wincl.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;		//
    wincl.cbSize = sizeof (WNDCLASSEX);
 
    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);					// для регистрации класса окна
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;
    wincl.cbClsExtra = 0;									//
    wincl.cbWndExtra = 0;									/*				*/
 
    wincl.hbrBackground = (HBRUSH) (COLOR_WINDOW+1);

	if (!RegisterClassEx (&wincl))							// регистрация класса для создания окна
        return NULL;

	hwnd = CreateWindowEx (								// создаем окно, но не показываем
           NULL,
           szClassName,
           L"",
           WS_POPUP,
           0,
           0,
           ResWindowX,
           ResWindowY,
           HWND_DESKTOP,
           NULL,
           hInstance,
           NULL
           );

	ShowWindow (hwnd, 1);

	return hwnd; 
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
        static HBITMAP hImage;
		static BITMAP bm;
        
        HDC hPaintDC, hMemDC;
        PAINTSTRUCT ps;
        HGDIOBJ hOld;		// дескриптор графического объекта
        
    switch (message)
    {
        case WM_PAINT: 
			
            hImage = (HBITMAP)LoadImage(NULL,L"TempImageServer.bmp", IMAGE_BITMAP,ResWindowX,ResWindowY,LR_LOADFROMFILE); // сделать под любой размер

			//LoadImageW(NULL,L"C:\\2.bmp",IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
			//InvalidateRect(hwnd,0,0);
			//SendMessage(hwnd,STM_SETIMAGE,(WPARAM)IMAGE_BITMAP,(LPARAM) hImage);
			
			GetObject(hImage,sizeof(BITMAP),&bm);

            hPaintDC = BeginPaint (hwnd, &ps);

            hMemDC = CreateCompatibleDC (hPaintDC);
            hOld = SelectObject (hMemDC, hImage);
             
            BitBlt (hPaintDC, 0, 0, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);// после этого появляется изображение
             
            SelectObject (hMemDC, hOld);
			
            EndPaint (hwnd, &ps);
			
			DeleteDC (hMemDC);
			DeleteDC (hPaintDC);
			DeleteObject(hImage);
            break;
        case WM_DESTROY:
            DeleteObject (hImage);
            PostQuitMessage (0);
            break;
        default:
            return DefWindowProc (hwnd, message, wParam, lParam);
    }
    return 0;
}

void CheckWidthScreen(double *kx, int *resScrClientX)
{
	int cxScreen = GetSystemMetrics (SM_CXSCREEN);
	if(cxScreen < *resScrClientX)
	{
		*kx = (double)(*resScrClientX) / cxScreen;
		*resScrClientX = cxScreen;
	}
	return;
}
void ChechHeigthScreen(double *ky, int *resScrClientY)
{
	int cyScreen = GetSystemMetrics (SM_CYSCREEN);
	if(cyScreen < *resScrClientY)
	{
		*ky = (double)(*resScrClientY) / cyScreen;
		*resScrClientY = cyScreen;
	}
	return;
}