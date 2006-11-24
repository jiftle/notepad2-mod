#include <windows.h>
#include <tchar.h>

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	int len = _tcslen(lpCmdLine);
	TCHAR * pbuf = new TCHAR[len + 10];
	_tcscpy(pbuf, _T("/h "));
	_tcscat(pbuf, lpCmdLine);
	ShellExecute(NULL, NULL, _T("Notepad2.exe"), pbuf, NULL, SW_SHOWNORMAL);
	delete [] pbuf;
	return 0;
}


