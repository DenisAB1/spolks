#include <windows.h>
#include <olectl.h>
LPDISPATCH pImage;
void LoadSaveImg(LPCTSTR pszFileName,char outName[MAX_PATH]) 
{ 
 HANDLE hFile=CreateFile(pszFileName,GENERIC_READ,0,NULL, 
   OPEN_EXISTING,0,NULL); 
 DWORD dwFileSize=GetFileSize(hFile,NULL); 
 HGLOBAL hGlobalA=GlobalAlloc(GMEM_MOVEABLE,dwFileSize); 
 LPVOID pvData=GlobalLock(hGlobalA); 
 DWORD dwBytesRead=0; 
 BOOL bReadFile=ReadFile(hFile,pvData,dwFileSize,&dwBytesRead,NULL); 
 GlobalUnlock( hGlobalA ); 
 CloseHandle(hFile); 
 LPSTREAM pst=NULL; 
 HRESULT hr=CreateStreamOnHGlobal(hGlobalA,TRUE,&pst); 
 if(pImage) pImage->Release(); 
 hr=OleLoadPicture(pst,dwFileSize,FALSE,IID_IPicture,(LPVOID*)&pImage); 
 pst->Release(); 
 WCHAR fBuff[MAX_PATH];
 mbstowcs(fBuff,outName,sizeof(fBuff)/sizeof(WCHAR));
 OleSavePictureFile(pImage,fBuff);
}

int ConvertToBMP()
{
	static wchar_t fNamIn[ ] = L"TempImage2.jpeg";
	static char fNamOut[] = "TempImage2.bmp";        
	LoadSaveImg(fNamIn, fNamOut);
	return 0;
}