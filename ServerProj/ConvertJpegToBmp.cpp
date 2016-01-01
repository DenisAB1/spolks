#include <windows.h>
#include <iostream>
#include <olectl.h>
#include <conio.h>
LPDISPATCH pImage;
bool LoadSaveImg(HANDLE hFile,char outName[MAX_PATH]) 
{ 
	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

	DWORD dwFileSize = GetFileSize(hFile, NULL);
	if(dwFileSize == INVALID_FILE_SIZE)
	{
		std::cout << "GetFileSize() failed. Error: " << GetLastError() << std::endl;
		_getch();
		return false;
	}
	HGLOBAL hGlobalA = GlobalAlloc(GMEM_MOVEABLE, dwFileSize); 
	LPVOID pvData = GlobalLock(hGlobalA);
	DWORD dwBytesRead = 0; 
	BOOL bReadFile = ReadFile(hFile, pvData, dwFileSize, &dwBytesRead, NULL); 
	GlobalUnlock(hGlobalA); 


	LPSTREAM pst = NULL; 
	HRESULT hr = CreateStreamOnHGlobal(hGlobalA, TRUE, &pst); 
	if(pImage) 
		pImage->Release(); 
	hr = OleLoadPicture(pst, dwFileSize, FALSE, IID_IPicture, (LPVOID*)&pImage); 
	pst -> Release(); 
	WCHAR fBuff[MAX_PATH];
	mbstowcs(fBuff, outName, sizeof(fBuff)/sizeof(WCHAR));
	OleSavePictureFile(pImage, fBuff);

	GlobalFree(hGlobalA); //add
	return true;
}

bool ConvertToBMP(HANDLE hFile)
{
	static char fNamOut[] = "TempImageServer.bmp";    

	if(!LoadSaveImg(hFile, fNamOut))
		return false;
	return true;
}