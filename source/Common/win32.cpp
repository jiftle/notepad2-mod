#include "win32.h"

#include <imagehlp.h>

using std::string;
using namespace util::utf8;

#ifdef _MSC_VER
#pragma comment(lib,"shlwapi.lib")
#endif


//namespace wpf
namespace wpf {



BYTE CharsetFromCodepage(uint cp)
{
	switch (cp)
	{
	case 936: return GB2312_CHARSET; //CHINESE_SIMPLIFIED
	case 950: return CHINESEBIG5_CHARSET; //CHINESE_TRADITIONAL
	case 949: return HANGUL_CHARSET; //KOREAN
	case 932: return SHIFTJIS_CHARSET; //JAPANESE
	case 1253: return GREEK_CHARSET; //GREEK
	default: return 0;
	}
}

void FixFileChars(tstring & Io, char CharToFix)
{
	for (tstring::size_type i = 0; i < Io.length(); ++i)
	{
		if (IsBadFileChar(Io[i]))
			Io[i] = CharToFix;
	}
}




int SimpToTrad(const wchar * src, wstring & dest, size_t src_len)
{
	ASSERT(src != dest.c_str());
	int r = LCMapStringW(MAKELCID(MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_SIMPLIFIED),SORT_CHINESE_PRC),
		LCMAP_TRADITIONAL_CHINESE, src, (int)src_len, NULL, 0);

	if(!r)
		return 0;

	dest.resize(r + 1);
	r = LCMapStringW(MAKELCID(MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_SIMPLIFIED),SORT_CHINESE_PRC),
		LCMAP_TRADITIONAL_CHINESE, src, (int)src_len, (wchar *)dest.c_str(), r + 1);
	dest.erase(r);
	return r;
}

int TradToSimp(const wchar * src, wstring & dest, size_t src_len)
{
	ASSERT(src != dest.c_str());
	int r = LCMapStringW(MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
		SORT_CHINESE_PRC), LCMAP_SIMPLIFIED_CHINESE, src, src_len, NULL, 0);

	if(!r)
		return 0;

	dest.resize(r + 1);
	r=LCMapStringW(MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), SORT_CHINESE_PRC),
		LCMAP_SIMPLIFIED_CHINESE, src, (int)src_len, (wchar *)dest.c_str(), r + 1);

	dest.erase(r);
	return r;
}

CDlgResizer::CDlgResizer(unsigned p_min_x, unsigned p_min_y, unsigned p_max_x, unsigned p_max_y) 
: min_x(p_min_x), min_y(p_min_y), max_x(p_max_x), max_y(p_max_y), parent(0)
{
}

void CDlgResizer::SetParent(HWND wnd)
{
	Reset();
	parent = wnd;
	GetClientRect(parent, &orig_client);
}

void CDlgResizer::Reset()
{
	parent = 0;
}

void CDlgResizer::OnSize()
{
	if (parent)
	{
		unsigned n;
		for(n=0; n < m_table.size(); n++)
		{
			stParam & e = m_table[n];
			const RECT & orig_rect = m_rects[n];
			RECT cur_client;
			GetClientRect(parent, &cur_client);
			HWND wnd = GetDlgItem(parent, e.id);

			unsigned dest_x = orig_rect.left, dest_y = orig_rect.top, 
				dest_cx = orig_rect.right - orig_rect.left, dest_cy = orig_rect.bottom - orig_rect.top;

			int delta_x = cur_client.right - orig_client.right,
				delta_y = cur_client.bottom - orig_client.bottom;

			if (e.flags & X_MOVE)
				dest_x += delta_x;
			else if (e.flags & X_SIZE)
				dest_cx += delta_x;

			if (e.flags & Y_MOVE)
				dest_y += delta_y;
			else if (e.flags & Y_SIZE)
				dest_cy += delta_y;
			
			SetWindowPos(wnd, 0, dest_x, dest_y, dest_cx, dest_cy, SWP_NOZORDER);
		}
		RedrawWindow(parent, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
	}
}


bool CDlgResizer::IsProcess(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_SIZE:
		OnSize();
		return true;

	case WM_GETMINMAXINFO:
		{
			RECT r;
			LPMINMAXINFO info = (LPMINMAXINFO)lp;
			if (max_x && max_y)
			{
				r.left = 0; r.right = max_x;
				r.top = 0; r.bottom = max_y;
				MapDialogRect(wnd, &r);
				info->ptMaxTrackSize.x = r.right;
				info->ptMaxTrackSize.y = r.bottom;
			}
			if (min_x && min_y)
			{
				r.left = 0; r.right = min_x;
				r.top = 0; r.bottom = min_y;
				MapDialogRect(wnd, &r);
				info->ptMinTrackSize.x = r.right;
				info->ptMinTrackSize.y = r.bottom;
			}
		}
		return true;

	case WM_INITDIALOG:
		SetParent(wnd);
		{
			unsigned n;
			for(n=0; n < m_table.size(); n++)
			{
				GetChildRect(parent, m_table[n].id, &m_rects[n]);
			}
		}
		break;

	case WM_DESTROY:
		Reset();
		break;
	}
	return false;
}




bool Reader::open(const TCHAR * filename, Reader::ACCESS DesiredAccess, Reader::SHARE ShareMode)
{
	if(hFile)
		CloseHandle(hFile);

	DWORD dwCreationDisposition = OPEN_EXISTING;

	if(GENERIC_READ == DesiredAccess) {
		dwCreationDisposition = OPEN_EXISTING;
	} else if(GENERIC_WRITE == DesiredAccess) {
		dwCreationDisposition = CREATE_ALWAYS;
	} else if((GENERIC_WRITE|GENERIC_READ) == DesiredAccess) {
		dwCreationDisposition = OPEN_ALWAYS;
	}

	int attr = GetFileAttributes(filename);
	hFile = CreateFile(filename, DesiredAccess, ShareMode, NULL,
		dwCreationDisposition, attr == INVALID_FILE_ATTRIBUTES ? 0 : attr, NULL);

	return INVALID_HANDLE_VALUE != hFile;
}

bool Reader::move(long offset) const
{
	if(0 == offset) return true;

	const unsigned int MAX_BUFFER_LEN = 8192;
	unsigned char buf[MAX_BUFFER_LEN];
	ufpos_t size = this->get_size();
	ufpos_t pos = this->get_pos();

	if(offset > 0) {
		fpos_t count = static_cast<fpos_t>(size);
		while(count > 0)
		{
			unsigned int n = util::min(MAX_BUFFER_LEN, static_cast<unsigned int>(count));
			count -= n;

			if(this->seek(count) < 0) return false;
			if(this->read(buf, n) != n) return false;
			if(this->seek(count + static_cast<fpos_t>(offset)) < 0) return false;
			if(this->write(buf, n) != n) return false;
		}
	} else {
		long count = -offset;
		while(count < static_cast<long>(size))
		{
			unsigned int n = util::min(MAX_BUFFER_LEN, static_cast<unsigned int>(size-count));

			if(this->seek(count) < 0) return false;
			if(this->read(buf, n) != n) return false;
			if(this->seek(count + static_cast<long>(offset)) < 0) return false;
			if(this->write(buf, n) != n) return false;

			count += n;
		}
		this->set_eof();
	}

	this->seek(static_cast<fpos_t>(pos));
	return true;
}

MostRecentlyUsed::MostRecentlyUsed(unsigned int nMaxItem, HKEY hKey, LPCTSTR lpSubKey, bool bReadListImmediately)
{
	m_nMaxItem = nMaxItem;
	m_hKey = hKey;
	m_SubKey = lpSubKey;
	if (bReadListImmediately) ReadList();
}

MostRecentlyUsed::~MostRecentlyUsed()
{
	WriteList();
}

void MostRecentlyUsed::DeleteDuplicate(LPCTSTR lpszPathName)
{
	assert(lpszPathName);
	data_type::iterator iter;
	for (iter = m_Data.begin(); iter != m_Data.end(); ++iter)
	{
		if (!_tcscmp(iter->c_str(), lpszPathName)) {
			iter = m_Data.erase(iter);
			break;
		}
	}
}

// Operations
void MostRecentlyUsed::Add(LPCTSTR lpszPathName)
{
	assert(lpszPathName);
	DeleteDuplicate(lpszPathName);
	m_Data.push_back(tstring(lpszPathName));
	if (m_Data.size() > m_nMaxItem) m_Data.erase(m_Data.begin());
}

void MostRecentlyUsed::Remove(unsigned int nIndex)
{
	assert((int)nIndex >= 0 && nIndex < m_Data.size());
	unsigned int i = 0;
	data_type::iterator iter = m_Data.begin();
	for (i=0; i<nIndex; i++)
		++iter;

	m_Data.erase(iter);
}

bool MostRecentlyUsed::WriteList()
{
	HKEY hKey;
	LONG lRes = RegCreateKeyEx(m_hKey, m_SubKey.c_str(), 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL);

	if (lRes == ERROR_SUCCESS)
	{
		TCHAR ci[32];
		unsigned int i = 0;

		while (true)
		{
			wsprintf(ci, _T("%u"), i);
			if (ERROR_SUCCESS != RegDeleteValue(hKey, ci)) break;
			i++;
		}

		for (i=0; i<m_Data.size(); i++)
		{
			wsprintf(ci, _T("%u"), i);
			RegSetValueEx(hKey, ci, 0, REG_SZ, (const BYTE*)m_Data[i].c_str(), m_Data[i].length());
		}
		RegCloseKey(hKey);
		return true;
	}

	return false;
}

bool MostRecentlyUsed::ReadList()
{
	m_Data.clear();
	HKEY hKey;
	LONG lRes = RegOpenKeyEx(m_hKey, m_SubKey.c_str(), 0, KEY_READ, &hKey);
	if (ERROR_SUCCESS != lRes)
		return false;

	unsigned int i = 0;
	TCHAR ci[32];
	DWORD Type = REG_SZ, cbData = 0;
	util::memory<TCHAR> mem;
	do
	{
		_stprintf(ci, _T("%u"), i);
		lRes = RegQueryValueEx(hKey, ci, 0, &Type, NULL, &cbData);
		if (ERROR_SUCCESS != lRes)
			break;
		mem.resize(static_cast<unsigned int>(cbData+1));
		lRes = RegQueryValueEx(hKey, ci, 0, &Type, (LPBYTE)mem.ptr(), &cbData);
		if (ERROR_SUCCESS != lRes || REG_SZ != Type)
			break;
		mem[static_cast<unsigned int>(cbData)] = 0;
		DeleteDuplicate(mem);
		m_Data.push_back(tstring(mem.ptr()));
		i++;
	} while (i < m_nMaxItem);

	RegCloseKey(hKey);
	return true;
}

bool FileExists(const TCHAR * DestFile, WIN32_FIND_DATA * pfd_out)
{
	HANDLE h;
	WIN32_FIND_DATA fd;
	// Avoid a "There is no disk in the drive" error box on empty removable drives
	SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);
	h = FindFirstFile(DestFile, &fd);
	SetErrorMode(0);
	if (h != INVALID_HANDLE_VALUE)
	{
		if (pfd_out)
			memcpy(pfd_out, &fd, sizeof(WIN32_FIND_DATA));
		FindClose(h);
		return true;
	}
	return false;
}

HMENU FindMenu(HMENU h, int id, int & pos)
{
	int count = ::GetMenuItemCount(h);
	for (int i = pos; i < count; i++)
	{
		int nid = ::GetMenuItemID(h, i);
		if (nid == id) {
			pos = i;
			return h;
		}
		else if (-1 == nid)
		{
			int new_pos = 0;
			HMENU new_h = FindMenu(::GetSubMenu(h, i), id, new_pos);
			if (new_h) {
				pos = new_pos;
				return new_h;
			}
		}
	}
	return NULL;
}

bool HasBadFileChar(const tchar * file)
{
	while (*file++)
	{
		if (IsBadFileChar(*file))
			return true;
	}
	return false;
}

bool HasBadPathChar(const tchar * path)
{
	while (*path++)
	{
		if (IsBadPathChar(*path))
			return true;
	}
	return false;
}

bool IsCmdEnabled(HWND hwnd, UINT id)
{
    HMENU hmenu = GetMenu(hwnd);
	::SendMessage(hwnd, WM_INITMENU, (WPARAM)hmenu, 0);
    UINT Flag = GetMenuState(hmenu, id, MF_BYCOMMAND);

    if (-1 == Flag)
        return true;
    else
        return !(Flag & (MF_DISABLED | MF_GRAYED));
}

tstring GetMyFullPath(const TCHAR * path)
{
	ASSERT(path);
	tstring tmp = path;
	api::ExpandEnvironmentStringsT(tmp, tmp.c_str());
	api::GetLongPathNameT(tmp, tmp.c_str());
	api::GetFullPathNameT(tmp);

	if (util::tolower(util::rget_after(tmp, '.')) == _T("lnk"))
	{
		com::GetLinkPath(tmp, tmp.c_str());
	}

	return tmp;
}

tstring GetDefaultOpenDir()
{
    LPITEMIDLIST pidl;
    LPMALLOC     lpMalloc;

    if (NOERROR == SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl))
    {
		TCHAR buf[1024];
		buf[0] = 0;
        SHGetPathFromIDList(pidl, buf);
        if (NOERROR == SHGetMalloc(&lpMalloc))
        {
            lpMalloc->Free(pidl);
            lpMalloc->Release();
        }
		buf[1023] = 0;
		return tstring(buf);
    }
    
	tstring dir;
	api::GetWindowsDirectoryT(dir);
	return dir;
}

void CopyToClipboard(const char * Text, size_t Len)
{
	OpenClipboard(NULL);
	EmptyClipboard();
	HGLOBAL memory = GlobalAlloc(GHND, Len * sizeof(char));
	char * ptr = (char *)GlobalLock(memory);
	memcpy(ptr, Text, Len * sizeof(char));
	GlobalUnlock(memory);
	SetClipboardData(CF_TEXT, memory);
	CloseClipboard();
}

void CopyToClipboard(const wchar * Text, size_t Len)
{
	OpenClipboard(NULL);
	EmptyClipboard();
	HGLOBAL memory = GlobalAlloc(GHND, Len * sizeof(wchar));
	wchar * ptr = (wchar *)GlobalLock(memory);
	memcpy(ptr, Text, Len * sizeof(wchar));
	GlobalUnlock(memory);
	SetClipboardData(CF_UNICODETEXT, memory);
	CloseClipboard();
}

tstring GetSpecialFolderDir(int nFolder)
{
	//CSIDL_PERSONAL
    LPITEMIDLIST pidl;
    LPMALLOC lpMalloc;

    if (NOERROR == SHGetSpecialFolderLocation(NULL, nFolder, &pidl))
    {
		int size = 0;
		int res = 0;
		TCHAR * buf = 0;
		do {
			size += CONST_VALUE::BUFFER_STEP_SIZE;
			if(buf) delete [] buf;
			buf = new TCHAR[size+1];
			buf[0] = 0;
			if (SHGetPathFromIDList(pidl, buf)) {
				res = _tcslen(buf);
			}
		} while(size - res <= sizeof(TCHAR) && static_cast<size_t>(size) < CONST_VALUE::MAX_PATH_BUFFER_SIZE - CONST_VALUE::BUFFER_STEP_SIZE);

		tstring tmp;
		if (res > 0) {
			buf[res] = 0;
			tmp = buf;
		}
		delete [] buf;

        if (NOERROR == SHGetMalloc(&lpMalloc))
        {
            lpMalloc->Free(pidl);
            lpMalloc->Release();
        }

		return tmp;

    }

	tstring dir;
	api::GetWindowsDirectoryT(dir);
	return dir;
}

const TCHAR * FormatNumberStr(TCHAR * lpNumberStr)
{
	ASSERT(lpNumberStr);
    TCHAR * c;
    TCHAR szSep[8];
    int  i = 0;

    if (!lpNumberStr[0])
		return lpNumberStr;

    if (!GetLocaleInfo(LOCALE_USER_DEFAULT,
                       LOCALE_STHOUSAND,
                       szSep,
                       countof(szSep)))
        szSep[0] = '\'';

    c = lpNumberStr + _tcslen(lpNumberStr);

    while ((c = CharPrev(lpNumberStr, c)) != lpNumberStr)
    {
        if (++i == 3)
        {
            i = 0;
            MoveMemory(c+1, c, _tcslen(c)+1);
            *c = szSep[0];
        }
    }

    return lpNumberStr;
}

ResStr::ResStr(UINT nID, HINSTANCE hInstance)
{
	int nSize = 0, res;
	buf = 0;
	do {
		nSize += CONST_VALUE::BUFFER_STEP_SIZE;
		if(buf) delete [] buf;
		buf = new TCHAR[nSize+1];
		res = ::LoadString(hInstance, nID, buf, nSize);
	} while(nSize - res <= sizeof(TCHAR) * 5);

	buf[res] = 0;
}

//namespace api
namespace api {



bool GetOpenFileNameT(tstring & out, HWND ParentWnd, const TCHAR * Filter, const TCHAR * DefFilename, const TCHAR * DefExt)
{
	ASSERT(Filter);
	OPENFILENAME ofn = { 0 };
	tchar buf[CONST_VALUE::MAX_PATH_BUFFER_SIZE] = { 0 };
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = ParentWnd;
	ofn.hInstance = WpfGetInstance();
	ofn.lpstrFilter = Filter;
	ofn.lpstrDefExt = DefExt;
	ofn.lpstrFile = buf;
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST;
	ofn.nMaxFile = countof(buf);
	if (DefFilename && DefFilename[0])
		_tcscpy(buf, DefFilename);
	if (::GetOpenFileName(&ofn))
	{
		out = buf;
		return true;
	}
	return false;
}

bool GetSaveFileNameT(tstring & out, HWND ParentWnd, const TCHAR * Filter, const TCHAR * DefFilename, const TCHAR * DefExt)
{
	ASSERT(Filter);
	OPENFILENAME ofn = { 0 };
	tchar buf[CONST_VALUE::MAX_PATH_BUFFER_SIZE] = { 0 };
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = ParentWnd;
	ofn.hInstance = WpfGetInstance();
	ofn.lpstrFilter = Filter;
	ofn.lpstrDefExt = DefExt;
	ofn.lpstrFile = buf;
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_EXPLORER | OFN_ENABLESIZING | OFN_PATHMUSTEXIST;
	ofn.nMaxFile = countof(buf);
	if (DefFilename && DefFilename[0])
		_tcscpy(buf, DefFilename);
	if (::GetSaveFileName(&ofn))
	{
		out = buf;
		return true;
	}
	return false;
}

bool IsAppThemedT()
{
    bool bIsAppThemed = false;
    HMODULE hDll = LoadLibrary(_T("uxtheme.dll"));
    if (hDll)
    {
        FARPROC fp = GetProcAddress(hDll, "IsAppThemed");
        if (fp)
        {
            bIsAppThemed = fp() == TRUE;
        }
        FreeLibrary(hDll);
    }
    return bIsAppThemed;
}

bool ChooseFontT(HWND parent, LOGFONTA & out)
{
	CHOOSEFONTA cf = { sizeof(cf) };
	cf.hwndOwner=parent;
	cf.lpLogFont=&out;
	cf.Flags=CF_SCREENFONTS|CF_FORCEFONTEXIST|CF_INITTOLOGFONTSTRUCT;
	cf.nFontType=SCREEN_FONTTYPE;				
	return ChooseFontA(&cf) != FALSE;
}

bool ChooseFontT(HWND parent, LOGFONTW & out)
{
	CHOOSEFONTW cf = { sizeof(cf) };
	cf.hwndOwner=parent;
	cf.lpLogFont=&out;
	cf.Flags=CF_SCREENFONTS|CF_FORCEFONTEXIST|CF_INITTOLOGFONTSTRUCT;
	cf.nFontType=SCREEN_FONTTYPE;				
	return ChooseFontW(&cf) != FALSE;
}


bool ChooseColorT(HWND wnd, COLORREF & out, COLORREF custom)
{
	COLORREF COLORS[16] = {custom,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	CHOOSECOLOR cc;
	memset(&cc,0,sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = wnd;
	cc.rgbResult = out;
	cc.lpCustColors = &COLORS[0];
	cc.Flags = CC_ANYCOLOR|CC_FULLOPEN|CC_RGBINIT;
	if (ChooseColor(&cc))
	{
		out = cc.rgbResult;
		return true;
	}
	else return false;
}

int GetMenuStringT(tstring & out, HMENU hMenu, UINT uIDItem, UINT uFlag)
{
	int size = 0;
	int res = 0;
	do {
		size += CONST_VALUE::BUFFER_STEP_SIZE;
		out.assign(size, 0);
		res = GetMenuString(hMenu, uIDItem, (TCHAR *)out.c_str(), size, uFlag);
	} while(size - res <= sizeof(TCHAR) * 5 && static_cast<size_t>(size) < CONST_VALUE::MAX_PATH_BUFFER_SIZE - CONST_VALUE::BUFFER_STEP_SIZE);

	out.erase(res);
	return res;
}

int LoadStringT(tstring & out, UINT nID, HINSTANCE hInstance)
{
	int size = 0;
	int res = 0;
	do {
		size += CONST_VALUE::BUFFER_STEP_SIZE;
		out.assign(size, 0);
		res = LoadString(hInstance, nID, (TCHAR *)out.c_str(), size);
	} while(size - res <= sizeof(TCHAR) * 5 && static_cast<size_t>(size) < CONST_VALUE::MAX_PATH_BUFFER_SIZE - CONST_VALUE::BUFFER_STEP_SIZE);

	out.erase(res);
	return res;
}

tstring GetListControlTextT(HWND hWnd, int iItem, int iSubItem)
{
	int size = 0;
	int res;
	TCHAR * buf = 0;
	LVITEM item;
	item.iSubItem = iSubItem;
	do {
		size += CONST_VALUE::BUFFER_STEP_SIZE;
		if(buf) delete [] buf;
		buf = new TCHAR[size+1];
		item.pszText = buf;
		item.cchTextMax = size;
		res = ::SendMessage(hWnd, LVM_GETITEMTEXT, iItem, (LPARAM)&item);
	} while(size - res <= sizeof(TCHAR) * 5 && static_cast<size_t>(size) < CONST_VALUE::MAX_PATH_BUFFER_SIZE - CONST_VALUE::BUFFER_STEP_SIZE);

	buf[res] = 0;
	tstring tmp(buf);
	delete [] buf;
	return tmp;
}

int GetCurrentDirectoryT(tstring & out)
{
	int size = GetCurrentDirectory(0, NULL);
	if (size > 0)
	{
		out.assign(size, 0);
		size = GetCurrentDirectory(size, (tchar *)out.c_str());
	}
	out.erase(size);

	return size;
}

UINT GetWindowsDirectoryT(tstring & out)
{
	int size = 0;
	int ret = 0;
	do {
		size += CONST_VALUE::BUFFER_STEP_SIZE;
		out.resize(size + 1);
		ret = GetWindowsDirectory((TCHAR *)out.c_str(), size);
	} while(size - ret <= sizeof(TCHAR) * 5 && static_cast<size_t>(size) < CONST_VALUE::MAX_PATH_BUFFER_SIZE - CONST_VALUE::BUFFER_STEP_SIZE);

	out.erase(ret);
	return ret;
}

int GetWindowTextT(tstring & out, HWND hWnd)
{
	int size = 0;
	int res = 0;
	do {
		size += CONST_VALUE::BUFFER_STEP_SIZE;
		out.assign(size, 0);
		res = GetWindowText(hWnd, (TCHAR *)out.c_str(), size);
	} while(size - res <= sizeof(TCHAR) * 5 && static_cast<size_t>(size) < CONST_VALUE::MAX_PATH_BUFFER_SIZE - CONST_VALUE::BUFFER_STEP_SIZE);

	out.erase(res);
	return res;
}

DWORD GetModuleFileNameT(tstring & out, HMODULE hModule)
{
	int size = 0;
	DWORD res = 0;
	do {
		size += CONST_VALUE::BUFFER_STEP_SIZE;
		out.assign(size, 0);
		res = GetModuleFileName(hModule, (TCHAR *)out.c_str(), size);
	} while(size - res <= sizeof(TCHAR) * 5 && static_cast<size_t>(size) < CONST_VALUE::MAX_PATH_BUFFER_SIZE - CONST_VALUE::BUFFER_STEP_SIZE);

	out.erase(res);
	return res;
}

DWORD GetTempPathT(tstring & out)
{
	DWORD size = GetTempPath(0, NULL);
	out.resize(size + 1);
	size = GetTempPath(size + 1, (LPTSTR)out.c_str());
	out.erase(size);
	return size;
}

uint GetTempFileNameT(tstring & out, const tchar * PathName, const tchar * PrefixString, uint uUnique)
{
	out.resize(CONST_VALUE::BUFFER_STEP_SIZE);
	uint ret = GetTempFileName(PathName, PrefixString, uUnique, (LPTSTR)out.c_str());
	if (ret)
		out.erase(_tcslen(out.c_str()));
	return ret;
}

uint GetTempFileNameT(tstring & out, const tchar * PrefixString, uint uUnique)
{
	tstring PathName;
	GetTempPathT(PathName);
	out.resize(CONST_VALUE::BUFFER_STEP_SIZE);
	uint ret = GetTempFileName(PathName.c_str(), PrefixString, uUnique, (LPTSTR)out.c_str());
	if (ret)
		out.erase(_tcslen(out.c_str()));
	return ret;
}

int GetDlgItemTextT(tstring & out, HWND hWnd, UINT nID)
{
	int len = ::SendDlgItemMessage(hWnd, nID, WM_GETTEXTLENGTH, 0, 0);
	if (len > 0) {
		out.assign(len + 2, 0);
		len = ::SendDlgItemMessage(hWnd, nID, WM_GETTEXT, len+1, (LPARAM)out.c_str());
		out.erase(len);
	}
	else
		out.erase();
	return len;
}

void SetLayeredWindowAttributeT(HWND hwnd, BOOL bTransparentMode, BYTE bAlpha)
{
	typedef BOOL (WINAPI *SetLayeredProc)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
    SetLayeredProc fp;

    if (bTransparentMode)
    {
        if (fp = (SetLayeredProc)GetProcAddress(GetModuleHandle(_T("User32")), "SetLayeredWindowAttributes"))
        {
            SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | /*WS_EX_LAYERED*/0x00080000);
            fp(hwnd, 0, bAlpha, /*LWA_ALPHA*/0x00000002);
        }
    }
    else
        SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~/*WS_EX_LAYERED*/0x00080000);
}

bool LoadResourceT(std::vector<BYTE> & out, int id, const TCHAR * type)
{
	HRSRC hr = FindResource(NULL, MAKEINTRESOURCE(id), type);
	if (hr) {
		HGLOBAL hg = LoadResource(NULL, hr);
		if (hg) {
			DWORD size = SizeofResource(NULL, hr);
			BYTE * pdata = (BYTE *)LockResource(hg);
			if (pdata && size > 0) {
				out.reserve(size + 16);
				out.assign(pdata, pdata + size);
				return true;
			}
		}
	}

	return false;
}

int ExpandEnvironmentStringsT(tstring & out, const tchar * path)
{
	int size = 0;
	int res = 0;
	TCHAR * buf = 0;
	do {
		size += CONST_VALUE::BUFFER_STEP_SIZE;
		if(buf) delete [] buf;
		buf = new TCHAR[size+1];
		res = ExpandEnvironmentStrings(path, buf, size);
	} while(size - res <= sizeof(TCHAR) * 5 && static_cast<size_t>(size) < CONST_VALUE::MAX_PATH_BUFFER_SIZE - CONST_VALUE::BUFFER_STEP_SIZE);

	if (res > 0) {
		out = buf;
	}
	delete [] buf;
	return res;
}

int SearchPathT(tstring & out, const tchar * path)
{
	int size = 0;
	int res = 0;
	TCHAR * buf = 0;
	do {
		size += CONST_VALUE::BUFFER_STEP_SIZE;
		if(buf) delete [] buf;
		buf = new TCHAR[size+1];
		res = SearchPath(NULL, path, NULL, size, buf, NULL);
	} while(size - res <= sizeof(TCHAR) * 5 && static_cast<size_t>(size) < CONST_VALUE::MAX_PATH_BUFFER_SIZE - CONST_VALUE::BUFFER_STEP_SIZE);

	if (res > 0) {
		buf[res] = 0;
		out = buf;
	}
	delete [] buf;
	return res;
}

DWORD GetFullPathNameT(tstring & io)
{
	int size = 0;
	int res = 0;
	TCHAR * buf = 0;
	do {
		size += CONST_VALUE::BUFFER_STEP_SIZE;
		if(buf) delete [] buf;
		buf = new TCHAR[size+1];
		res = GetFullPathName(io.c_str(), size, buf, NULL);
	} while(size - res <= sizeof(TCHAR) * 5 && static_cast<size_t>(size) < CONST_VALUE::MAX_PATH_BUFFER_SIZE - CONST_VALUE::BUFFER_STEP_SIZE);

	if (res > 0) {
		buf[res] = 0;
		io = buf;
	}
	delete [] buf;
	return res;
}

DWORD GetLongPathNameT(tstring & out, const tchar * path)
{
	int size = 0;
	int res = 0;
	TCHAR * buf = 0;
	do {
		size += CONST_VALUE::BUFFER_STEP_SIZE;
		if(buf) delete [] buf;
		buf = new TCHAR[size+1];
		res = GetLongPathName(path, buf, size);
	} while(size - res <= sizeof(TCHAR) * 5 && static_cast<size_t>(size) < CONST_VALUE::MAX_PATH_BUFFER_SIZE - CONST_VALUE::BUFFER_STEP_SIZE);

	if (res > 0) {
		buf[res] = 0;
		out = buf;
	}
	delete [] buf;
	return res;
}

DWORD GetShortPathNameT(tstring & out, const tchar * path)
{
	int size = 0;
	int res = 0;
	TCHAR * buf = 0;
	do {
		size += CONST_VALUE::BUFFER_STEP_SIZE;
		if(buf) delete [] buf;
		buf = new TCHAR[size+1];
		res = GetShortPathName(path, buf, size);
	} while(size - res <= sizeof(TCHAR) * 5 && static_cast<size_t>(size) < CONST_VALUE::MAX_PATH_BUFFER_SIZE - CONST_VALUE::BUFFER_STEP_SIZE);

	if (res > 0) {
		buf[res] = 0;
		out = buf;
	}
	delete [] buf;
	return res;
}

bool CreateProcessT(const TCHAR * cmd, DWORD * pExitCode)
{
	PROCESS_INFORMATION ProcInfo;
	STARTUPINFO StartUp = { sizeof(STARTUPINFO) };
	if (!CreateProcess(NULL, (TCHAR *)cmd, NULL, NULL, FALSE, 0, NULL, NULL, &StartUp, &ProcInfo))
		return false;

	CloseHandle(ProcInfo.hThread);
	if (pExitCode) {
		while (WaitForSingleObject(ProcInfo.hProcess, 100) == WAIT_TIMEOUT)
		{
			MSG msg;
			while (PeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE))
				DispatchMessage(&msg);

		}
		GetExitCodeProcess(ProcInfo.hProcess, pExitCode);
	}
	CloseHandle(ProcInfo.hProcess);
	return true;
}

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED)
	{
		if (lpData)
			SendMessage(hwnd, BFFM_SETSELECTION, 1, lpData);
	}
	return 0;
}

bool SHBrowseForFolderT(tstring & out, HWND hwndParent, const TCHAR * Title, const TCHAR * BaseDir)
{
	ASSERT(Title);
	BROWSEINFO bi;
	LPITEMIDLIST pidl;
	TCHAR buf[MAX_PATH * 2];
	buf[0] = 0;

	bi.hwndOwner = hwndParent;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = buf;
	bi.lpszTitle = Title;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = (LPARAM)BaseDir;
	bi.iImage = 0;

	pidl = SHBrowseForFolder(&bi);
	if (pidl)
	{
		SHGetPathFromIDList(pidl, buf);
		com::FreePIDL(pidl);
		out = buf;
		return true;
	}

	return false;
}

UINT DragQueryFileT(tstring & out, HDROP hDrop, UINT idx)
{
	UINT size = ::DragQueryFile(hDrop, (UINT)-1, NULL, 0);
	if ((UINT)-1 == idx || idx >= size)
		return size;

	size = ::DragQueryFile(hDrop, idx, NULL, 0);
	tchar * buf = new tchar[size + 1];
	size = ::DragQueryFile(hDrop, idx, buf, size + 1);
	buf[size] = 0;
	out = buf;
	delete [] buf;
	return size;
}

void GetLastErrorT(tstring & out)
{
	DWORD err = GetLastError();
	LPVOID lpMsgBuf;
	FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

	out = (tchar *)lpMsgBuf;
	LocalFree(lpMsgBuf);
}




}; //end of namespace api


// COM
namespace com {




bool GetLinkPath(tstring & out, const tchar * linkfile)
{
	com::auto_ptr<IShellLink> psl;
    if (FAILED(CoCreateInstance(CLSID_ShellLink,NULL, CLSCTX_INPROC_SERVER,
								   IID_IShellLink, (void **)psl.address())))
		return false;

    com::auto_ptr<IPersistFile> ppf;

	if (FAILED(psl->QueryInterface(IID_IPersistFile, (void **)ppf.address())))
		return false;

    if (FAILED(ppf->Load(string_utf16_from_os(linkfile), STGM_READ)))
		return false;

    tchar buf[CONST_VALUE::MAX_PATH_BUFFER_SIZE];
	buf[0] = 0;
    if (NOERROR != psl->GetPath(buf, CONST_VALUE::MAX_PATH_BUFFER_SIZE, NULL, 0))
        return false;

	out = buf;
    return true;
}

bool CreateLink(const tchar * linkfile,
				const tchar * target,
				const tchar * args,
				const tchar * iconpath,
				uint icon,
				const tchar * workdir,
				const tchar * description)
{
	ASSERT(linkfile && linkfile[0]);
	ASSERT(target && target[0]);
	com::auto_ptr<IShellLink> psl;
    if (FAILED(CoCreateInstance(CLSID_ShellLink,NULL, CLSCTX_INPROC_SERVER,
								   IID_IShellLink, (void **)psl.address())))
		return false;

    com::auto_ptr<IPersistFile> ppf;

	if (FAILED(psl->QueryInterface(IID_IPersistFile, (void **)ppf.address())))
		return false;

	psl->SetPath(target);
	if (args && args[0])
		psl->SetArguments(args);
	if (iconpath && iconpath[0])
		psl->SetIconLocation(iconpath, icon);
	if (workdir && workdir[0])
		psl->SetWorkingDirectory(workdir);
	if (description && description[0])
		psl->SetDescription(description);

	if (FAILED(ppf->Save(string_utf16_from_os(linkfile), TRUE)))
		return false;

    return true;
}

bool ILIsFile(LPCITEMIDLIST pidl)
{
	bool bRet = false;
	LPCITEMIDLIST  pidlChild = NULL;
	IShellFolder * psf = NULL;
	HRESULT	hr = SHBindToParent(pidl, IID_IShellFolder, (LPVOID*)&psf, &pidlChild);
	if (SUCCEEDED(hr)&&psf)
	{
		SFGAOF rgfInOut = SFGAO_FOLDER|SFGAO_FILESYSTEM;
		hr = psf->GetAttributesOf(1, &pidlChild, &rgfInOut);
		if (SUCCEEDED(hr))
		{
			//in file system, but is not a folder
			if( (~rgfInOut & SFGAO_FOLDER) && (rgfInOut & SFGAO_FILESYSTEM) )
			{
				bRet = true;	
			}
		}
		psf->Release();
	}
	return bRet;
}

void FreePIDL(void * idl)
{
	if (!idl)
		return;

	IMalloc * m;
	SHGetMalloc(&m);
	if (m)
	{
		m->Free(idl);
		m->Release();
	}
}

bool GetNameFromPIDL(tstring & out, const LPSHELLFOLDER & lpsf, const LPITEMIDLIST & pidl, bool bShowExt)
{
	STRRET sr;

	if (NOERROR == lpsf->GetDisplayNameOf(pidl, bShowExt ? SHGDN_FORPARSING : SHGDN_INFOLDER, &sr))
	{
		//// StrRetToStr not work for Win98
		//tchar * p = NULL;
		//if (S_OK == StrRetToStr(&sr, pidl, &p))
		//{
		//	ASSERT(p);
		//	out = p;
		//	CoTaskMemFree(p);
		//	return true;
		//}

		switch (sr.uType)
		{
		case STRRET_WSTR:
			assign_os_from_utf16(out, sr.pOleStr);
			//out = string_os_from_utf16(sr.pOleStr);
			FreePIDL(sr.pOleStr);
			break;

		case STRRET_CSTR:
			assign_os_from_ansi(out, sr.cStr);
			//out = string_os_from_ansi(sr.cStr);
			break;

		default:
			return false;
		}
		
		return true;
	}

	out.erase();
	return false;
}



}; //namespace com


#ifdef _SEH
namespace crash {

typedef BOOL (__stdcall * t_SymInitialize)(IN HANDLE hProcess,IN LPSTR UserSearchPath,IN BOOL fInvadeProcess);
typedef BOOL (__stdcall * t_SymCleanup)(IN HANDLE hProcess);
typedef BOOL (__stdcall * t_SymGetModuleInfo)(IN HANDLE hProcess,IN DWORD dwAddr,OUT PIMAGEHLP_MODULE ModuleInfo);
typedef BOOL (__stdcall * t_SymGetSymFromAddr)(IN HANDLE hProcess,IN DWORD dwAddr,OUT PDWORD pdwDisplacement,OUT PIMAGEHLP_SYMBOL Symbol);
typedef BOOL (__stdcall * t_SymEnumerateModules)(IN HANDLE hProcess,IN PSYM_ENUMMODULES_CALLBACK EnumModulesCallback,IN PVOID UserContext);

static t_SymInitialize p_SymInitialize;
static t_SymCleanup p_SymCleanup;
static t_SymGetModuleInfo p_SymGetModuleInfo;
static t_SymGetSymFromAddr p_SymGetSymFromAddr;
static t_SymEnumerateModules p_SymEnumerateModules;

static HMODULE hImageHlp;
static util::CCriticalSection g_cs;

static bool LoadImageHelp()
{
	static bool error;
	if (hImageHlp) return true;
	if (error) return false;
	hImageHlp = LoadLibraryA("imagehlp.dll");
	if (hImageHlp==0) {error = true; return false; }
	p_SymInitialize = (t_SymInitialize)GetProcAddress(hImageHlp,"SymInitialize");
	p_SymCleanup = (t_SymCleanup)GetProcAddress(hImageHlp,"SymCleanup");
	p_SymGetModuleInfo = (t_SymGetModuleInfo)GetProcAddress(hImageHlp,"SymGetModuleInfo");
	p_SymGetSymFromAddr = (t_SymGetSymFromAddr)GetProcAddress(hImageHlp,"SymGetSymFromAddr");
	p_SymEnumerateModules = (t_SymEnumerateModules)GetProcAddress(hImageHlp,"SymEnumerateModules");

	if (!p_SymInitialize || !p_SymCleanup || !p_SymGetModuleInfo || !p_SymGetSymFromAddr || !p_SymEnumerateModules)
	{
		FreeLibrary(hImageHlp);
		hImageHlp = 0;
		error = true;
		return false;
	}

	return true;
}

static char * hexdump8(char * out,unsigned address,const char * msg,int from,int to)
{
	unsigned max = (to-from)*16;
	if (!IsBadReadPtr((const void*)(address+(from*16)),max))
	{
		out += sprintf(out,"%s (%08Xh):",msg,address);
		unsigned n;
		const unsigned char * src = (const unsigned char*)(address)+(from*16);
		
		for(n=0;n<max;n++)
		{
			if (n%16==0)
			{
				out += sprintf(out,"\r\n%08Xh: ",src);
			}

			out += sprintf(out," %02X",*(src++));
		}
		*(out++) = '\r';
		*(out++) = '\n';
		*out=0;
	}
	return out;
}

static char * hexdump32(char * out,unsigned address,const char * msg,int from,int to)
{
	unsigned max = (to-from)*16;
	if (!IsBadReadPtr((const void*)(address+(from*16)),max))
	{
		out += sprintf(out,"%s (%08Xh):",msg,address);
		unsigned n;
		const unsigned char * src = (const unsigned char*)(address)+(from*16);
		
		for(n=0;n<max;n+=4)
		{
			if (n%16==0)
			{
				out += sprintf(out,"\r\n%08Xh: ",src);
			}

			out += sprintf(out," %08X",*(long*)src);
			src += 4;
		}
		*(out++) = '\r';
		*(out++) = '\n';
		*out=0;
	}
	return out;
}

static bool ReadInt(unsigned src,unsigned * out)
{
	__try
	{
		*out = *(unsigned*)src;
		return true;
	}
	__except(1) {return false;}
}

static bool TestAddress(unsigned address)
{
	__try
	{
		unsigned temp = *(unsigned*)address;
		return temp != 0;
	}
	__except(1) {return false;}
}

static void CallStackParse(unsigned address,HANDLE hFile,char * temp,HANDLE hProcess)
{
	bool inited = false;
	unsigned data;
	unsigned count_done = 0;
	int len = 0;
	DWORD Written = 0;
	for (unsigned int count_done = 0; count_done<256 && ReadInt(address, &data);
		address += 4, count_done++)
	{
		unsigned data;
		if (!ReadInt(address, &data))
			continue;

		if (!TestAddress(data))
			continue;

		bool found = false;
		{
			IMAGEHLP_MODULE mod;
			memset(&mod,0,sizeof(mod));
			mod.SizeOfStruct = sizeof(mod);
			if (p_SymGetModuleInfo(hProcess,data,&mod))
			{
				if (!inited)
				{
					strcpy(temp, "\r\nStack dump analysis:\r\n");
					WriteFile(hFile, temp, strlen(temp), &Written, NULL);
					inited = true;
				}
				len = sprintf(temp,"Address: %08Xh, location: \"%s\", loaded at %08Xh - %08Xh\r\n",data,mod.ModuleName,mod.BaseOfImage,mod.BaseOfImage+mod.ImageSize);
				WriteFile(hFile, temp, len, &Written, NULL);
				found = true;
			}
		}


		if (found)
		{
			union
			{
				char buffer[128];
				IMAGEHLP_SYMBOL symbol;
			};
			memset(buffer,0,sizeof(buffer));
			symbol.SizeOfStruct = sizeof(symbol);
			symbol.MaxNameLength = buffer + sizeof(buffer) - symbol.Name;
			DWORD offset = 0;
			if (p_SymGetSymFromAddr(hProcess,data,&offset,&symbol))
			{
				buffer[countof(buffer)-1]=0;
				if (symbol.Name[0])
				{
					len = sprintf(temp,"Symbol: \"%s\" (+%08Xh)\r\n",symbol.Name,offset);
					WriteFile(hFile, temp, len, &Written, NULL);
				}
			}
		}
	}
}

static HANDLE CreateFailureLog(char * path_out)
{
	bool rv = false;
	GetModuleFileNameA(0,path_out,MAX_PATH);
	path_out[MAX_PATH-1]=0;
	{
		char * ptr = path_out + strlen(path_out);
		while(ptr>path_out && *ptr!='\\') ptr--;
		ptr[1]=0;
	}
	char * fn_out = path_out + strlen(path_out);
	HANDLE hFile = INVALID_HANDLE_VALUE;
	unsigned attempts = 0;
	for(;;)
	{
		if (*fn_out==0)
		{
			strcpy(fn_out,"failure.html");
		}
		else
		{
			attempts++;
			sprintf(fn_out,"failure_%08u.html",attempts);
		}
		hFile = CreateFileA(path_out,GENERIC_WRITE,0,0,CREATE_NEW,0,0);
		if (hFile!=INVALID_HANDLE_VALUE) break;
		if (attempts > 1000) break;
	}
	return hFile;
}

static BOOL CALLBACK EnumModulesCallback(LPSTR ModuleName,ULONG BaseOfDll,PVOID UserContext)
{
	IMAGEHLP_MODULE mod;
	memset(&mod,0,sizeof(mod));
	mod.SizeOfStruct = sizeof(mod);
	if (p_SymGetModuleInfo(GetCurrentProcess(),BaseOfDll,&mod))
	{
		char temp[1024];
		char temp2[countof(mod.ModuleName)+1];
		strcpy(temp2,mod.ModuleName);

		{
			unsigned ptr;
			for(ptr=strlen(temp2);ptr<countof(temp2)-1;ptr++)
				temp2[ptr]=' ';
			temp2[ptr]=0;
		}

		int len = sprintf(temp,"%s loaded at %08Xh - %08Xh\r\n",temp2,mod.BaseOfImage,mod.BaseOfImage+mod.ImageSize);
		DWORD Written;
		WriteFile((HANDLE)UserContext, temp, len, &Written, NULL);
	}
	return TRUE;
}

void DumpCrashInfo(LPEXCEPTION_POINTERS param, const char * CallPath, const char * ver_string)
{
	char log[MAX_PATH * 2];
	HANDLE hFile = CreateFailureLog(log);
	if (INVALID_HANDLE_VALUE == hFile)
		return;

	char temp[2048];
	int len = 0;
	DWORD Written = 0;
	unsigned address = (unsigned)param->ExceptionRecord->ExceptionAddress;
	len = sprintf(temp, "Illegal operation:\r\nCode: %08Xh, flags: %08Xh, address: %08Xh\r\n", param->ExceptionRecord->ExceptionCode, param->ExceptionRecord->ExceptionFlags, address);
	WriteFile(hFile, temp, len, &Written, NULL);

	if (param->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && param->ExceptionRecord->NumberParameters >= 2)
	{
		len = sprintf(temp, "Access violation, operation: %s, address: %08Xh\r\n", param->ExceptionRecord->ExceptionInformation[0] ? "write" : "read", param->ExceptionRecord->ExceptionInformation[1]);
		WriteFile(hFile, temp, len, &Written, NULL);
	}

	if (CallPath && CallPath[0]) {
		len = sprintf(temp, "Call path:\r\n%s\r\n", CallPath);
		WriteFile(hFile, temp, len, &Written, NULL);
	}

	hexdump8(temp, address, "Code bytes", -4, + 4);
	WriteFile(hFile, temp, strlen(temp), &Written, NULL);
	hexdump32(temp, param->ContextRecord->Esp, "Stack", -2, + 18);
	WriteFile(hFile, temp, strlen(temp), &Written, NULL);
	len = sprintf(temp, "Registers:\r\n" 
		"EAX: %08X, EBX: %08X, ECX: %08X, EDX: %08X\r\n"
		"ESI: %08X, EDI: %08X, EBP: %08X, ESP: %08X\r\n", 
		param->ContextRecord->Eax, 
		param->ContextRecord->Ebx, 
		param->ContextRecord->Ecx, 
		param->ContextRecord->Edx, 
		param->ContextRecord->Esi, 
		param->ContextRecord->Edi, 
		param->ContextRecord->Ebp, 
		param->ContextRecord->Esp);
	WriteFile(hFile, temp, len, &Written, NULL);

	bool imagehelp_succeeded = false;
	//g_cs.Enter();
	if (LoadImageHelp())
	{
		HANDLE hProcess = GetCurrentProcess();
		char exepath[MAX_PATH];
		exepath[0] = 0;
		GetModuleFileNameA(0, exepath, countof(exepath));
		exepath[countof(exepath) - 1] = 0;
		{
			char * ptr = exepath + strlen(exepath);
			while (ptr > exepath && *ptr != '\\') ptr--;
			*ptr = 0;
		}
		if (p_SymInitialize(hProcess, exepath, TRUE))
		{
			{
				IMAGEHLP_MODULE mod;
				memset(&mod, 0, sizeof(mod));
				mod.SizeOfStruct = sizeof(mod);
				if (p_SymGetModuleInfo(hProcess, address, &mod))
				{
					len = sprintf(temp, "Crash location: \"%s\", loaded at %08Xh - %08Xh\r\n", mod.ModuleName, mod.BaseOfImage, mod.BaseOfImage + mod.ImageSize);
					WriteFile(hFile, temp, len, &Written, NULL);
				}
				else
				{
					len = sprintf(temp, "Unable to identify crash location\r\n");
					WriteFile(hFile, temp, len, &Written, NULL);
				}
			}

			{
				union
				{
					char buffer[128];
					IMAGEHLP_SYMBOL symbol;
				};
				memset(buffer, 0, sizeof(buffer));
				symbol.SizeOfStruct = sizeof(symbol);
				symbol.MaxNameLength = buffer + sizeof(buffer) - symbol.Name;
				DWORD offset = 0;
				if (p_SymGetSymFromAddr(hProcess, address, &offset, &symbol))
				{
					buffer[countof(buffer) - 1] = 0;
					if (symbol.Name[0])
					{
						len = sprintf(temp, "Symbol: \"%s\" (+%08Xh)\r\n", symbol.Name, offset);
						WriteFile(hFile, temp, len, &Written, NULL);
					}
				}
			}

			strcpy(temp, "\r\nLoaded modules:\r\n");
			WriteFile(hFile, temp, strlen(temp), &Written, NULL);
			p_SymEnumerateModules(hProcess, EnumModulesCallback, hFile);


			CallStackParse(param->ContextRecord->Esp, hFile, temp, hProcess);

			p_SymCleanup(hProcess);
			imagehelp_succeeded = true;
		}
	}
	//g_cs.Leave();
	if (!imagehelp_succeeded)
	{
		strcpy(temp, "Failed to get module/symbol info.\r\n");
		WriteFile(hFile, temp, strlen(temp), &Written, NULL);
	}

	strcpy(temp, "\r\nVersion info: \r\n");
	WriteFile(hFile, temp, strlen(temp), &Written, NULL);
	if (ver_string && ver_string[0])
	{
		WriteFile(hFile, ver_string, strlen(ver_string), &Written, NULL);
		WriteFile(hFile, "\r\n", 2, &Written, NULL);
	}
	strcpy(temp, 
#ifdef UNICODE
			"UNICODE"
#else
			"ANSI"
#endif
				   );
	WriteFile(hFile, temp, strlen(temp), &Written, NULL);
	CloseHandle(hFile);

	//ShellExecuteA(NULL, 0, log, NULL, NULL, SW_SHOW);
}


}; //namespace crash
#endif //_SEH




bool IsWindowsNT()
{
	static DWORD ver = 0;
	if (ver) return VER_PLATFORM_WIN32_NT == ver;

    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);
	ver = osvi.dwPlatformId;

    return (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

void CenterWindow(HWND hWnd, HWND hWndCenter)
{
	if(NULL == hWndCenter) {
		hWndCenter = GetDesktopWindow();
	}
	RECT rcDlg;
	GetWindowRect(hWnd, &rcDlg);
	RECT rcCenter;

	GetWindowRect(hWndCenter, &rcCenter);

	int xLeft = (rcCenter.left + rcCenter.right) / 2 - (rcDlg.right-rcDlg.left) / 2;
	int yTop = (rcCenter.top + rcCenter.bottom) / 2 - (rcDlg.bottom-rcDlg.top) / 2;
	SetWindowPos(hWnd, NULL, xLeft, yTop, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

LONG CReg::QueryValue(LPTSTR pszValue, LPCTSTR lpszValueName, DWORD* pdwCount)
{
    DWORD dwType = NULL;
    LONG lRes = RegQueryValueEx(m_hKey, lpszValueName, NULL, &dwType, (LPBYTE)pszValue, pdwCount);
    if (pszValue != NULL)
    {
        if(*pdwCount>0)
        {
            switch(dwType)
            {
            case REG_SZ:
            case REG_EXPAND_SZ:
                if ((*pdwCount) % sizeof(TCHAR) != 0 || pszValue[(*pdwCount) / sizeof(TCHAR) - 1] != 0)
                {
                    pszValue[0]=_T('\0');
                    return ERROR_INVALID_DATA;
                }
                break;
            case REG_MULTI_SZ:
                if ((*pdwCount) % sizeof(TCHAR) != 0 || (*pdwCount) / sizeof(TCHAR) < 1 || pszValue[(*pdwCount) / sizeof(TCHAR) -1] != 0 || (((*pdwCount) / sizeof(TCHAR))>1 && pszValue[(*pdwCount) / sizeof(TCHAR) - 2] != 0) )
                {
                    pszValue[0]=_T('\0');
                    return ERROR_INVALID_DATA;
                }
                break;
            default:
                // Ensure termination
                pszValue[0]=_T('\0');
                return ERROR_INVALID_DATA;
            }
        }
        else
        {
            // this is a blank one with no data yet
            // Ensure termination
            pszValue[0]=_T('\0');
        }
    }
    return lRes;
}

#ifdef  _MSC_VER
#pragma warning(push)
#endif
// prefast noise VSW 496818
LONG CReg::QueryStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars)
{
    LONG lRes;
    DWORD dwType;
    ULONG nBytes;

    if(pnChars)
    {
        nBytes = (*pnChars)*sizeof(TCHAR);
        *pnChars = 0;
	} else {
		::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, NULL, &nBytes);
	}
    lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(pszValue),
                             &nBytes);

    if (lRes != ERROR_SUCCESS)
    {
        return lRes;
    }

    if(dwType != REG_SZ && dwType != REG_EXPAND_SZ)
    {
        return ERROR_INVALID_DATA;
    }

    if (pszValue != NULL)
    {
        if(nBytes!=0)
        {
            if ((nBytes % sizeof(TCHAR) != 0) || (pszValue[nBytes / sizeof(TCHAR) -1] != 0))
            {
                return ERROR_INVALID_DATA;
            }
        }
        else
        {
            pszValue[0]=_T('\0');
        }
    }

    if(pnChars)
    {
        *pnChars = nBytes/sizeof(TCHAR);
    }

    return ERROR_SUCCESS;
}

bool CReg::QueryStringValue(LPCTSTR pszValueName, tstring & out)
{
	out.erase();

	if (!m_hKey)
		return false;
    LONG lRes;
    DWORD dwType;
    DWORD nBytes = 0;
	TCHAR * buf;

	::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, NULL, &nBytes);
	if (nBytes) {
		buf = new TCHAR[nBytes+1];
		lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, 
			reinterpret_cast<LPBYTE>(buf), &nBytes);

		if (lRes == ERROR_SUCCESS && (dwType == REG_SZ || dwType == REG_EXPAND_SZ))
		{
			buf[nBytes] = 0;
			out = buf;
			delete [] buf;
			return true;
		}
		delete [] buf;
	}

    return false;
}

LONG CReg::QueryMultiStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars)
{
    LONG lRes;
    DWORD dwType;
    ULONG nBytes;

    if (pszValue != NULL && *pnChars < 2)
        return ERROR_INSUFFICIENT_BUFFER;

    nBytes = (*pnChars)*sizeof(TCHAR);
    *pnChars = 0;

    lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(pszValue),
                             &nBytes);
    if (lRes != ERROR_SUCCESS)
        return lRes;
    if (dwType != REG_MULTI_SZ)
        return ERROR_INVALID_DATA;
    if (pszValue != NULL && (nBytes % sizeof(TCHAR) != 0 || nBytes / sizeof(TCHAR) < 1 || pszValue[nBytes / sizeof(TCHAR) -1] != 0 || ((nBytes/sizeof(TCHAR))>1 && pszValue[nBytes / sizeof(TCHAR) - 2] != 0)))
        return ERROR_INVALID_DATA;

    *pnChars = nBytes/sizeof(TCHAR);

    return ERROR_SUCCESS;
}
#ifdef  _MSC_VER
#pragma warning(pop)
#endif

LONG CReg::SetMultiStringValue(LPCTSTR pszValueName, LPCTSTR pszValue)
{
    LPCTSTR pszTemp;
    ULONG nBytes;
    ULONG nLength;

    // Find the total length (in bytes) of all of the strings, including the
    // terminating '\0' of each string, and the second '\0' that terminates
    // the list.
    nBytes = 0;
    pszTemp = pszValue;
    do
    {
        nLength = _tcslen(pszTemp)+1;
        pszTemp += nLength;
        nBytes += nLength*sizeof(TCHAR);
    }
    while (nLength != 1);

    return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_MULTI_SZ, reinterpret_cast<const BYTE*>(pszValue),
                           nBytes);
}

LONG CReg::RecurseDeleteKey(LPCTSTR lpszKey)
{
    CReg key;
    LONG lRes = key.Open(m_hKey, lpszKey, KEY_READ | KEY_WRITE);
    if (lRes != ERROR_SUCCESS)
    {
        return lRes;
    }
    FILETIME time;
    DWORD dwSize = 256;
    TCHAR szBuffer[256];
    while (RegEnumKeyEx(key.m_hKey, 0, szBuffer, &dwSize, NULL, NULL, NULL,
                        &time)==ERROR_SUCCESS)
    {
        lRes = key.RecurseDeleteKey(szBuffer);
        if (lRes != ERROR_SUCCESS)
            return lRes;
        dwSize = 256;
    }
    key.Close();
    return DeleteSubKey(lpszKey);
}



//namespace gui
//namespace gui {



CThemeHelper::CThemeHelper()
: RefCount(0)
, ModUxtheme(NULL)
, pEnableThemeDialogTexture(NULL)
{
}

bool CThemeHelper::Load() {
	RefCount++;

	if (RefCount > 1) {
		return (pEnableThemeDialogTexture != NULL);
	}

	ModUxtheme = ::LoadLibrary(_T("uxtheme.dll"));
	if (ModUxtheme == NULL) {
		return false;
	}

	pEnableThemeDialogTexture =
		(EnableThemeDialogTextureProc *)::GetProcAddress(ModUxtheme, "EnableThemeDialogTexture");

	return (pEnableThemeDialogTexture != NULL);
}

bool CThemeHelper::Unload() {
	RefCount--;

	if (RefCount > 0) {
		return false;
	}

	if (pEnableThemeDialogTexture != NULL) {
		::FreeLibrary(ModUxtheme);
		ModUxtheme = NULL;
		pEnableThemeDialogTexture = NULL;
	}

	return true;
}

LRESULT CThemeHelper::EnableThemeDialogTexture(HWND wnd, DWORD flags) {
	if (pEnableThemeDialogTexture != NULL) {
		return pEnableThemeDialogTexture(wnd, flags);
	}
	else {
		// TODO: more appropriate return value
		return S_OK;
	}
}


CFileDialog::CFileDialog(bool bOpenFile,
		HWND ParentWnd,
		const TCHAR * Filter,
		const TCHAR * DefFilename,
		const TCHAR * DefExt) :
		m_buf(0), m_bOpen(bOpenFile)
{
	ASSERT(Filter);
	memset(&m_ofn, 0, sizeof(OPENFILENAME));
	m_ofn.lStructSize = sizeof(OPENFILENAME);
	m_ofn.hwndOwner = ParentWnd;
	m_ofn.hInstance = WpfGetInstance();
	m_ofn.lpstrFilter = Filter;
	m_ofn.lpstrDefExt = DefExt;
	m_ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLESIZING
#if (_WIN32_WINNT >= 0x0500)
		| OFN_ENABLEINCLUDENOTIFY
#endif
		;
	if (m_bOpen)
	{
		m_ofn.Flags |= OFN_FILEMUSTEXIST;
	}
	else
	{
		// 文件类型自动匹配
		if (DefFilename && DefFilename[0])
		{
			const tchar * p = m_ofn.lpstrFilter;
			DWORD idx = 0;
			bool done = false;
			for (; *p; ++idx)
			{
				p += _tcslen(p) + 1;
				if (0 == *p)
				{
					break;
				}
				tstring file = DefFilename;
				util::rmake_after(file, '\\');
				util::vptstring v;
				util::split(p, ';', v);
				for (uint i = 0; i < v.size(); ++i)
				{
					const tstring & spec = *v[i];
					if (PathMatchSpec(file.c_str(), spec.c_str()))
					{
						m_ofn.nFilterIndex = idx + 1;
						done = true;
						break;
					}
				}
				if (done)
				{
					break;
				}
				p += _tcslen(p) + 1;
			}
		}
	}
	m_ofn.lCustData = (LPARAM)this;
	m_ofn.lpfnHook = (LPOFNHOOKPROC)OFNHookProc;
	m_ofn.nMaxFile = CONST_VALUE::MAX_PATH_BUFFER_SIZE;
	if (DefFilename && DefFilename[0]) {
		m_buf = new TCHAR[util::max<size_t>(CONST_VALUE::MAX_PATH_BUFFER_SIZE, _tcslen(DefFilename) + 1)];
		_tcscpy(m_buf, DefFilename);
		m_ofn.lpstrFile = m_buf;
	}
}

void CFileDialog::CheckBuffer()
{
	if (!m_ofn.lpstrFile) {
		if (m_buf)
			delete [] m_buf;
		m_buf = new TCHAR[CONST_VALUE::MAX_PATH_BUFFER_SIZE];
		m_ofn.lpstrFile = m_buf;
		m_ofn.lpstrFile[0] = 0;
	}

	if (0 == m_ofn.nMaxFile)
		m_ofn.nMaxFile = CONST_VALUE::MAX_PATH_BUFFER_SIZE;
	m_ofn.lStructSize = sizeof(OPENFILENAME);
	m_ofn.lCustData = (LPARAM)this;
}

UINT_PTR CALLBACK CFileDialog::OFNHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CFileDialog * _this;
	LRESULT lRes = 0;
	if (WM_INITDIALOG != uMsg)
		_this = (CFileDialog *)GetWindowPointer(hWnd);

	switch (uMsg)
	{

	case WM_INITDIALOG:
		_this = (CFileDialog *)((OPENFILENAME *)lParam)->lCustData;
		ASSERT(_this);
		_this->m_hWnd = GetParent(hWnd);
		_this->OldOFNHookProc = ::SetWindowLongPtr(_this->m_hWnd, GWLP_WNDPROC, (LONG)_this->DialogProc);
		SetWindowPointer(hWnd, _this);
		SetWindowPointer(_this->m_hWnd, _this);
		lRes = _this->OnMessage(uMsg, wParam, lParam);
		break;

#if (_WIN32_WINNT >= 0x0500)
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case CDN_INCLUDEITEM:
			{
				OFNOTIFYEX * pne = (OFNOTIFYEX *)lParam;
				IShellFolder * psf = (IShellFolder *)pne->psf;
				ITEMIDLIST * pidl = (ITEMIDLIST *)pne->pidl;

				if (!com::ILIsFile(pidl))
					break;

				const tchar * p = pne->lpOFN->lpstrFilter;
				if (!p || !p[0])
					break;

				tstring name;
				if (!com::GetNameFromPIDL(name, psf, pidl, true))
					break;
				util::rmake_after(name, '\\');

				for (DWORD i = 0; i < pne->lpOFN->nFilterIndex * 2 - 1; i++)
				{
					p += _tcslen(p) + 1;
				}

				tstring filter = p;
				std::replace(filter.begin(), filter.end(), ';', '\0');
				for (p = filter.c_str(); p < filter.c_str() + filter.length();
					p += _tcslen(p) + 1)
				{
					if (PathMatchSpec(name.c_str(), p))
					{
						lRes = 1;
						break;
					}
				}
			}
			break;

		case CDN_FILEOK:
			if (_this->m_bOpen && (OFN_ALLOWMULTISELECT & _this->m_ofn.Flags))
			{
				IShellBrowser* pSB = (IShellBrowser *)::SendMessage(hWnd, WM_GETISHELLBROWSER, 0, 0);
				IShellView * pIShellView = NULL;
				LPMALLOC pMalloc = NULL;
				IDataObject * pIDataObject = NULL;
				FORMATETC fmte;
				STGMEDIUM stgmedium;
				ZeroMemory( (LPVOID)&fmte, sizeof(STGMEDIUM) );
				ZeroMemory( (LPVOID)&fmte, sizeof(FORMATETC) );

				fmte.tymed = TYMED_HGLOBAL;
				fmte.lindex = -1;
				fmte.dwAspect = DVASPECT_CONTENT;
				fmte.cfFormat = RegisterClipboardFormat(CFSTR_SHELLIDLIST);

				LPITEMIDLIST pidlFull = NULL;
				TCHAR szPath[CONST_VALUE::MAX_PATH_BUFFER_SIZE];
				_this->m_list.clear();
				do {
					HRESULT hr = pSB->QueryActiveShellView(&pIShellView);
					if (FAILED(hr))
						break;
					hr = ::SHGetMalloc(&pMalloc); //Get pointer to shell alloc
					if (FAILED(hr))
						break;
					hr = pIShellView->GetItemObject(SVGIO_SELECTION , IID_IDataObject, (LPVOID*) & pIDataObject);
					if (FAILED(hr))
						break;
					if (pIDataObject == NULL)
						break;
					hr = pIDataObject->GetData(&fmte, &stgmedium);
					if (FAILED(hr))
						break;
					LPIDA pida = (LPIDA) GlobalLock(stgmedium.hGlobal);
					if (pida)
					{
						LPCITEMIDLIST pidlFolder = GetPIDLFolder(pida);
						for (UINT i = 0;i < pida->cidl;i++)
						{
							//filter folders
							LPCITEMIDLIST pidl = GetPIDLItem(pida, i);
							pidlFull = ILCombine(pidlFolder, pidl);
							if (com::ILIsFile(pidlFull))
							{
								szPath[0] = 0;
								hr = SHGetPathFromIDList(pidlFull, szPath);
								if (SUCCEEDED(hr))
								{
									tstring tmp;
									_this->m_list.push_back(tmp);
									_this->m_list.back() = szPath;
								}
							}
							pMalloc->Free(pidlFull);
							pidlFull = NULL;
						}
					}
				} while (false);

				//Clean up
				GlobalUnlock(stgmedium.hGlobal);
				ReleaseStgMedium(&stgmedium);
				if (pIDataObject)
					pIDataObject->Release();
				if (pIShellView)
					pIShellView->Release();
				if (pMalloc)
				{
					if (pidlFull)
						pMalloc->Free(pidlFull);
					pMalloc->Release();
				}
			}
			break;
		}
		break; //WM_NOTIFY
#endif
	}

	if (lRes)
		::SetWindowLongPtr(hWnd, DWLP_MSGRESULT, lRes);
	return lRes;
}

LRESULT CFileDialog::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case MAKELONG(ID_CB_TYPENAME, CBN_SELCHANGE):
			if (!m_bOpen) //only for save file dialog
			{
				tstring name;
				if (api::GetDlgItemTextT(name, m_hWnd, ID_CB_FILENAME) <= 0)
					break;

				HWND hwcb = (HWND)lParam;
				int sel = ::SendMessage(hwcb, CB_GETCURSEL, 0, 0);
				if (CB_ERR == sel)
					break;

				const tchar * p = m_ofn.lpstrFilter;
				for (int i = 0; i < sel * 2 + 1 && *p; i++)
				{
					p += _tcslen(p) + 1;
				}
				if (0 == *p)
					break;

				tstring ext = p;
				util::make_before(ext, ';');
				util::rmake_after(ext, '.');
				if (tstring::npos != ext.find_first_of(_T("*?|\"<>:")))
					break;

				util::rmake_before(name, '.');
				name += '.';
				name += ext;
				SendDlgItemMessage(ID_CB_FILENAME, WM_SETTEXT, 0, name.c_str());
			}
			break;
		}
		break; //WM_COMMAND

	}
	return 0;
}


CStaticLink::CStaticLink(UINT nCtlID, const TCHAR * URL, UINT nCursorResourceID)
: xm_ctl(nCtlID)
, xm_wnd(NULL)
, xm_old(NULL)
, xm_id(nCursorResourceID)
, xm_bOverControl(false)
{
	if (NULL != URL)
	{
		xm_url = URL;
	}

	if (GetSysColorBrush(COLOR_HOTLIGHT))
	{
		xm_color = GetSysColor(COLOR_HOTLIGHT);
	}
	else
	{
		xm_color = RGB(0, 0, 255);
	}

	xm_proc.Assign(this, &CStaticLink::WndProc);
}

void CStaticLink::SubclassWindow(HWND hWndCtl)
{
	xm_wnd = hWndCtl;
	if (xm_url.empty())
	{
		api::GetWindowTextT(xm_url, xm_wnd);
		util::trim(xm_url, _T("\t "));
	}
	xm_old = (WNDPROC)::SetWindowLongPtr(xm_wnd, GWLP_WNDPROC, (LONG_PTR)xm_proc());
}

LRESULT CStaticLink::IsProcess(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CTLCOLORSTATIC:
		if (xm_wnd == (HWND)lParam)
		{
			HDC dc = (HDC)wParam;
			::SetTextColor(dc, xm_color);
			::SetBkMode(dc, TRANSPARENT);

			LOGFONT lf;
			xm_font.GetLogFont(&lf);
			lf.lfUnderline = !xm_bOverControl;
			xm_font.CreateFontIndirect(&lf);
			::SelectObject(dc, xm_font.GetHandle());

			return (LRESULT)CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
		}
		break;

	case WM_INITDIALOG:
		{
			xm_wnd = ::GetDlgItem(hWnd, xm_ctl);
			xm_font.GetFromWindow(xm_wnd);
			::RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
			SubclassWindow(xm_wnd);
		}
		break;

	}
	return 0;
}

LRESULT CStaticLink::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCHITTEST:
		return HTCLIENT;

	case WM_SETCURSOR:
		SetCursor(LoadCursor((int)xm_id <= 0 ? NULL : WpfGetInstance(), MAKEINTRESOURCE(xm_id)));
		return 1;

	case WM_MOUSEMOVE:
		{
			if (!xm_bOverControl)
			{
				xm_bOverControl = true;
				::SetTimer(hWnd, 666, 100, NULL);
				::RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
				return 0;
			}
		}
		return 1;

	case WM_TIMER:
		{
			DWORD pos = GetMessagePos();
			POINT pt = { GET_X_LPARAM(pos), GET_Y_LPARAM(pos) };
			::ScreenToClient(hWnd, &pt);
			RECT r;
			::GetClientRect(hWnd, &r);
			if (!::PtInRect(&r, pt))
			{
				::KillTimer(hWnd, 666);
				xm_bOverControl = false;
				::RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
		xm_color = RGB(128, 0, 128);
		ShellExecute(NULL, _T("open"), xm_url.c_str(), NULL, NULL, SW_SHOWNORMAL);
		::RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
		return 1;

	default:
		return CallWindowProc(xm_old, hWnd, uMsg, wParam, lParam);
	}
	return 0;
}


//inline bool CStaticLink::PtInItem(HWND hWnd, const POINT & pt)
//{
//	RECT rc;
//	GetWindowRect(wnd, &rc);
//	ScreenToClient(hWnd, (LPPOINT)&rc);
//	ScreenToClient(hWnd, (LPPOINT)&rc + 1);
//	return PtInRect(&rc, pt) != FALSE;
//}
//
//LRESULT CStaticLink::Process(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//{
//	switch (uMsg)
//	{
//	case WM_INITDIALOG:
//		{
//			wnd = GetDlgItem(hWnd, id);
//			LOGFONT lf;
//
//			//  Font setup
//			if (NULL == (hFontHover = (HFONT)SendMessage(wnd, WM_GETFONT, 0, 0)))
//				hFontHover = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
//			GetObject(hFontHover, sizeof(LOGFONT), &lf);
//			lf.lfUnderline = TRUE;
//			hFontNormal = CreateFontIndirect(&lf);
//
//			//  Cursor setup
//			hCursorNormal = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
//			hCursorHover = (idhand <= 0) ? LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND))
//				: LoadCursor(WpfGetInstance(), MAKEINTRESOURCE(idhand));
//		}
//		return 0;
//
//	case WM_NCACTIVATE:
//		hover = false;
//		InvalidateRect(wnd, NULL, FALSE);
//		return 0;
//
//	case WM_CTLCOLORSTATIC:
//		if (id == GetWindowLong((HWND)lParam, GWL_ID))
//		{
//			HDC hdc = (HDC)wParam;
//			SetBkMode(hdc, TRANSPARENT);
//			COLORREF color;
//			if (GetSysColorBrush(COLOR_HOTLIGHT))
//			{
//				color = GetSysColor(COLOR_HOTLIGHT);
//			}
//			else
//			{
//				color = RGB(0, 0, 255);
//			}
//			SetTextColor(hdc, color);
//			SelectObject(hdc, hover ? hFontHover : hFontNormal);
//			return (LONG)GetSysColorBrush(COLOR_BTNFACE);
//		}
//		return 0;
//
//	case WM_MOUSEMOVE:
//		{
//			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
//			if (this->PtInItem(hWnd, pt))
//			{
//				hover = true;
//				InvalidateRect(wnd, NULL, FALSE);
//				SetCursor(hCursorHover);
//				return 1;
//			}
//			else if (hover)
//			{
//				InvalidateRect(wnd, NULL, FALSE);
//				SetCursor(hCursorNormal);
//				hover = false;
//			}
//		}
//		return 0;
//
//	case WM_LBUTTONDOWN:
//		{
//			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
//			if (this->PtInItem(hWnd, pt))
//			{
//				if (link.empty())
//					api::GetDlgItemTextT(link, hWnd, id);
//				ShellExecute(hWnd, _T("open"), link.c_str(), NULL, NULL, SW_SHOWNORMAL);
//				return 1;
//			}
//		}
//		break;
//	}
//
//	return 0;
//}

CPropertySheet::CPropertySheet()
: m_hWndCurPage(NULL)
{
}

INT_PTR CPropertySheet::DoModal(HWND hWndParent, uint width, uint height, const tchar * Caption, const tchar * ok_bn_title, const tchar * cancel_bn_title)
{
	ASSERT(Caption);
	wstring mem;
	mem.reserve(160);
	mem.assign(sizeof(DLGTEMPLATE) / sizeof(wchar), 0);
	wstring::size_type pos = 0;
	DLGTEMPLATE * pdt = (DLGTEMPLATE *)mem.c_str();
	pdt->style = DS_CENTER | DS_3DLOOK | WS_POPUP | WS_CAPTION | WS_SYSMENU;
	pdt->dwExtendedStyle = WS_EX_TOOLWINDOW;
	pdt->x = 0;
	pdt->y = 0;
	pdt->cx = width;
	pdt->cy = height;
	pdt->cdit = 0;
	mem += (wchar)0; //Menu
	mem += (wchar)0; //Class name
	mem += t2w(Caption);
	mem += (wchar)0;

	DLGITEMTEMPLATE * pdit = 0;

	pos = mem.length();
	mem.append((sizeof(DWORD) * 2 + sizeof(DLGITEMTEMPLATE)) / sizeof(wchar), 0);
	pdit = (DLGITEMTEMPLATE *)AlignDWORD((WORD *)(mem.c_str() + pos));
	pdit->style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
	pdit->dwExtendedStyle = WS_EX_NOPARENTNOTIFY;
	pdit->x = CONTROL_SPACE;
	pdit->y = CONTROL_SPACE;
	pdit->cx = width - pdit->x * 2;
	pdit->cy = height - BUTTON_HEIGHT - CONTROL_SPACE - pdit->y * 2;
	pdit->id = ID_TAB;
	mem.erase((wchar *)++pdit - mem.c_str());
	mem += L"SysTabControl32";
	mem += (wchar)0;
	mem += L"Tab1";
	mem += (wchar)0; //end of string
	mem += (wchar)0; //no creation data
	((DLGTEMPLATE *)mem.c_str())->cdit++;

	pos = mem.length();
	mem.append((sizeof(DWORD) * 2 + sizeof(DLGITEMTEMPLATE)) / sizeof(wchar), 0);
	pdit = (DLGITEMTEMPLATE *)AlignDWORD((WORD *)(mem.c_str() + pos));
	pdit->style = WS_CHILD | WS_VISIBLE | WS_GROUP | WS_DISABLED | SS_CENTERIMAGE | SS_CENTER;
	pdit->dwExtendedStyle = 0;
	pdit->x = CONTROL_SPACE * 2;
	pdit->y = CONTROL_SPACE * 2 + TAB_PADDING;
	pdit->cx = width - pdit->x * 2;
	pdit->cy = height - BUTTON_HEIGHT - CONTROL_SPACE - TAB_PADDING - CONTROL_SPACE * 4;
	pdit->id = ID_EMPTY;
	mem.erase((wchar *)++pdit - mem.c_str());
	mem += L"STATIC";
	mem += (wchar)0;
	mem += L"<empty>";
	mem += (wchar)0; //end of string
	mem += (wchar)0; //no creation data
	((DLGTEMPLATE *)mem.c_str())->cdit++;

	if (ok_bn_title)
	{
		pos = mem.length();
		mem.append((sizeof(DWORD) * 2 + sizeof(DLGITEMTEMPLATE)) / sizeof(wchar), 0);
		pdit = (DLGITEMTEMPLATE *)AlignDWORD((WORD *)(mem.c_str() + pos));
		pdit->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON;
		pdit->dwExtendedStyle = 0;
		pdit->x = width - (BUTTON_WIDTH + CONTROL_SPACE) * 2;
		pdit->y = height - CONTROL_SPACE - BUTTON_HEIGHT;
		pdit->cx = BUTTON_WIDTH;
		pdit->cy = BUTTON_HEIGHT;
		pdit->id = IDOK;
		mem.erase((wchar *)++pdit - mem.c_str());
		mem += L"BUTTON";
		mem += (wchar)0;
		mem += t2w(ok_bn_title);
		mem += (wchar)0; //end of string
		mem += (wchar)0; //no creation data
		((DLGTEMPLATE *)mem.c_str())->cdit++;
	}

	if (cancel_bn_title)
	{
		pos = mem.length();
		mem.append((sizeof(DWORD) * 2 + sizeof(DLGITEMTEMPLATE)) / sizeof(wchar), 0);
		pdit = (DLGITEMTEMPLATE *)AlignDWORD((WORD *)(mem.c_str() + pos));
		pdit->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON;
		pdit->dwExtendedStyle = 0;
		pdit->x = width - (BUTTON_WIDTH + CONTROL_SPACE);
		pdit->y = height - CONTROL_SPACE - BUTTON_HEIGHT;
		pdit->cx = BUTTON_WIDTH;
		pdit->cy = BUTTON_HEIGHT;
		pdit->id = IDCANCEL;
		mem.erase((wchar *)++pdit - mem.c_str());
		mem += L"BUTTON";
		mem += (wchar)0;
		mem += t2w(cancel_bn_title);
		mem += (wchar)0; //end of string
		mem += (wchar)0; //no creation data
		((DLGTEMPLATE *)mem.c_str())->cdit++;
	}


	mem.append(8, 0);

	return ::DialogBoxIndirectParam(WpfGetInstance(), (DLGTEMPLATE *)mem.c_str(), 
		hWndParent, DialogProc, (LPARAM)this);
}

HWND CPropertySheet::Create(HWND hWndParent, uint width, uint height)
{
	wstring mem;
	mem.reserve(160);
	mem.assign(sizeof(DLGTEMPLATE) / sizeof(wchar), 0);
	wstring::size_type pos = 0;
	DLGTEMPLATE * pdt = (DLGTEMPLATE *)mem.c_str();
	pdt->style = WS_CHILD | WS_CLIPSIBLINGS;
	pdt->dwExtendedStyle = WS_EX_CONTROLPARENT;
	pdt->x = 0;
	pdt->y = 0;
	pdt->cx = width;
	pdt->cy = height;
	pdt->cdit = 0;
	mem += (wchar)0; //Menu
	mem += (wchar)0; //Class name
	mem += (wchar)0; //No caption

	DLGITEMTEMPLATE * pdit = 0;

	pos = mem.length();
	mem.append((sizeof(DWORD) * 2 + sizeof(DLGITEMTEMPLATE)) / sizeof(wchar), 0);
	pdit = (DLGITEMTEMPLATE *)AlignDWORD((WORD *)(mem.c_str() + pos));
	pdit->style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
	pdit->dwExtendedStyle = WS_EX_NOPARENTNOTIFY;
	pdit->x = CONTROL_SPACE;
	pdit->y = CONTROL_SPACE;
	pdit->cx = width - pdit->x * 2;
	pdit->cy = height - BUTTON_HEIGHT - CONTROL_SPACE - pdit->y * 2;
	pdit->id = ID_TAB;
	mem.erase((wchar *)++pdit - mem.c_str());
	mem += L"SysTabControl32";
	mem += (wchar)0;
	mem += L"Tab1";
	mem += (wchar)0; //end of string
	mem += (wchar)0; //no creation data
	((DLGTEMPLATE *)mem.c_str())->cdit++;

	pos = mem.length();
	mem.append((sizeof(DWORD) * 2 + sizeof(DLGITEMTEMPLATE)) / sizeof(wchar), 0);
	pdit = (DLGITEMTEMPLATE *)AlignDWORD((WORD *)(mem.c_str() + pos));
	pdit->style = WS_CHILD | WS_VISIBLE | WS_GROUP | WS_DISABLED | SS_CENTERIMAGE | SS_CENTER;
	pdit->dwExtendedStyle = 0;
	pdit->x = CONTROL_SPACE * 2;
	pdit->y = CONTROL_SPACE * 2 + TAB_PADDING;
	pdit->cx = width - pdit->x * 2;
	pdit->cy = height - BUTTON_HEIGHT - CONTROL_SPACE - TAB_PADDING - CONTROL_SPACE * 4;
	pdit->id = ID_EMPTY;
	mem.erase((wchar *)++pdit - mem.c_str());
	mem += L"STATIC";
	mem += (wchar)0;
	mem += L"<empty>";
	mem += (wchar)0; //end of string
	mem += (wchar)0; //no creation data
	((DLGTEMPLATE *)mem.c_str())->cdit++;


	mem.append(8, 0);

	return ::CreateDialogIndirectParam(WpfGetInstance(), (DLGTEMPLATE *)mem.c_str(), 
		hWndParent, DialogProc, (LPARAM)this);
}

BOOL CPropertySheet::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			//m_theme.Load();
			//m_theme.EnableThemeDialogTexture(m_hWnd);
			RECT r;
			GetChildRect(m_hWnd, ID_EMPTY, &r);
			HWND hWndTab = ::GetDlgItem(m_hWnd, ID_TAB);
			HWND hWndEmpty = ::GetDlgItem(m_hWnd, ID_EMPTY);
			HFONT hf = NULL;
			for (unsigned int i=0; i<m_pdlg.size(); ++i)
			{
				HWND hWndPage = m_pdlg[i]->Create(m_hWnd);
				if (NULL == hf)
					hf = (HFONT)m_pdlg[i]->SendMessage(WM_GETFONT);
				::SetWindowLongPtr(hWndPage, GWL_STYLE, WS_CHILD | WS_DISABLED);
				if (NULL != hWndPage)
				{
					::SetWindowPos(hWndPage, hWndEmpty, r.left, r.top,
						r.right - r.left, r.bottom - r.top, SWP_HIDEWINDOW);

					TCITEM tab;
					tab.mask = TCIF_TEXT;
					tchar title[256];
					::GetWindowText(hWndPage, title, countof(title));
					tab.pszText = title;
					TabCtrl_InsertItem(hWndTab, i, &tab);
				}
			}
			::SendMessage(hWndTab, WM_SETFONT, (WPARAM)hf, TRUE);
			SendDlgItemMessage(IDOK, WM_SETFONT, hf, TRUE);
			SendDlgItemMessage(IDCANCEL, WM_SETFONT, hf, TRUE);
			::SendMessage(hWndTab, TCM_SETCURSEL, 0, 0);
			this->OnTabChange();
		}
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
		case IDCANCEL:
			::EndDialog(m_hWnd, wParam);
			break;
		}
		break; //WM_COMMAND

	case WM_DESTROY:
		for (unsigned int i=0; i<m_pdlg.size(); ++i)
		{
			m_pdlg[i]->DestroyWindow();
		}
		//m_theme.Unload();
		break;

	case WM_NOTIFY:
		if (ID_TAB == ((NMHDR *)lParam)->idFrom && TCN_SELCHANGE == ((NMHDR *)lParam)->code)
		{
			this->OnTabChange();
		}
		break;
	}

	return 0;
}

INT_PTR CALLBACK CPropertySheet::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR rv;
	CPropertySheet * _this = NULL;
	if (uMsg == WM_INITDIALOG)
	{
		_this = reinterpret_cast<CPropertySheet *>(lParam);
		SetWindowPointer(hWnd, _this);
		_this->m_hWnd = hWnd;
	}
	else _this = (CPropertySheet *)GetWindowPointer(hWnd);

	rv = _this ? _this->OnMessage(uMsg, wParam, lParam) : FALSE;

	if (uMsg == WM_DESTROY && _this)
	{
		// _this may have been deleted after processing the WM_DESTROY message
		SetWindowPointer(hWnd, NULL);
	}

	return rv;
}

void CPropertySheet::OnTabChange()
{
	if (NULL != m_hWndCurPage) {
		::ShowWindow(m_hWndCurPage, SW_HIDE);
		::EnableWindow(m_hWndCurPage, false);
		m_hWndCurPage = NULL;
	}

	unsigned int idx = SendDlgItemMessage(ID_TAB, TCM_GETCURSEL, 0, 0);
	if ((unsigned int)-1 != idx)
	{
		m_hWndCurPage = m_pdlg[idx]->GetHandle();
		if (NULL != m_hWndCurPage) {
			::EnableWindow(m_hWndCurPage, true);
			::ShowWindow(m_hWndCurPage, SW_SHOW);
		} else {
			::ShowWindow(m_hWndCurPage, SW_HIDE);
			::EnableWindow(m_hWndCurPage, false);
		}
	}
}



//}; //namespace gui


// MinimizeToTray
//
// A couple of routines to show how to make it produce a custom caption
// animation to make it look like we are minimizing to and maximizing
// from the system tray
//
// These routines are public domain, but it would be nice if you dropped
// me a line if you use them!
//
// 1.0 29.06.2000 Initial version
// 1.1 01.07.2000 The window retains it's place in the Z-order of windows
//     when minimized/hidden. This means that when restored/shown, it doen't
//     always appear as the foreground window unless we call SetForegroundWindow
//
// Copyright 2000 Matthew Ellis <m.t.ellis@bigfoot.com>

// Odd. VC++6 winuser.h has IDANI_CAPTION defined (as well as IDANI_OPEN and
// IDANI_CLOSE), but the Platform SDK only has IDANI_OPEN...

// I don't know what IDANI_OPEN or IDANI_CLOSE do. Trying them in this code
// produces nothing. Perhaps they were intended for window opening and closing
// like the MAC provides...
#ifndef IDANI_OPEN
#define IDANI_OPEN 1
#endif
#ifndef IDANI_CLOSE
#define IDANI_CLOSE 2
#endif
#ifndef IDANI_CAPTION
#define IDANI_CAPTION 3
#endif

#define DEFAULT_RECT_WIDTH 150
#define DEFAULT_RECT_HEIGHT 30

// Returns the rect of where we think the system tray is. This will work for
// all current versions of the shell. If explorer isn't running, we try our
// best to work with a 3rd party shell. If we still can't find anything, we
// return a rect in the lower right hand corner of the screen
static void GetTrayWndRect(LPRECT lpTrayRect)
{
	APPBARDATA appBarData;
	// First, we'll use a quick hack method. We know that the taskbar is a window
	// of class Shell_TrayWnd, and the status tray is a child of this of class
	// TrayNotifyWnd. This provides us a window rect to minimize to. Note, however,
	// that this is not guaranteed to work on future versions of the shell. If we
	// use this method, make sure we have a backup!
	HWND hShellTrayWnd = FindWindowEx(NULL, NULL, TEXT("Shell_TrayWnd"), NULL);
	if (hShellTrayWnd)
	{
		HWND hTrayNotifyWnd = FindWindowEx(hShellTrayWnd, NULL, TEXT("TrayNotifyWnd"), NULL);
		if (hTrayNotifyWnd)
		{
			GetWindowRect(hTrayNotifyWnd, lpTrayRect);
			return ;
		}
	}

	// OK, we failed to get the rect from the quick hack. Either explorer isn't
	// running or it's a new version of the shell with the window class names
	// changed (how dare Microsoft change these undocumented class names!) So, we
	// try to find out what side of the screen the taskbar is connected to. We
	// know that the system tray is either on the right or the bottom of the
	// taskbar, so we can make a good guess at where to minimize to
	/*APPBARDATA appBarData;*/
	appBarData.cbSize = sizeof(appBarData);
	if (SHAppBarMessage(ABM_GETTASKBARPOS, &appBarData))
	{
		// We know the edge the taskbar is connected to, so guess the rect of the
		// system tray. Use various fudge factor to make it look good
		switch (appBarData.uEdge)
		{
		case ABE_LEFT:
		case ABE_RIGHT:
			// We want to minimize to the bottom of the taskbar
			lpTrayRect->top = appBarData.rc.bottom - 100;
			lpTrayRect->bottom = appBarData.rc.bottom - 16;
			lpTrayRect->left = appBarData.rc.left;
			lpTrayRect->right = appBarData.rc.right;
			break;

		case ABE_TOP:
		case ABE_BOTTOM:
			// We want to minimize to the right of the taskbar
			lpTrayRect->top = appBarData.rc.top;
			lpTrayRect->bottom = appBarData.rc.bottom;
			lpTrayRect->left = appBarData.rc.right - 100;
			lpTrayRect->right = appBarData.rc.right - 16;
			break;
		}

		return ;
	}

	// Blimey, we really aren't in luck. It's possible that a third party shell
	// is running instead of explorer. This shell might provide support for the
	// system tray, by providing a Shell_TrayWnd window (which receives the
	// messages for the icons) So, look for a Shell_TrayWnd window and work out
	// the rect from that. Remember that explorer's taskbar is the Shell_TrayWnd,
	// and stretches either the width or the height of the screen. We can't rely
	// on the 3rd party shell's Shell_TrayWnd doing the same, in fact, we can't
	// rely on it being any size. The best we can do is just blindly use the
	// window rect, perhaps limiting the width and height to, say 150 square.
	// Note that if the 3rd party shell supports the same configuraion as
	// explorer (the icons hosted in NotifyTrayWnd, which is a child window of
	// Shell_TrayWnd), we would already have caught it above
	hShellTrayWnd = FindWindowEx(NULL, NULL, TEXT("Shell_TrayWnd"), NULL);
	if (hShellTrayWnd)
	{
		GetWindowRect(hShellTrayWnd, lpTrayRect);
		if (lpTrayRect->right - lpTrayRect->left > DEFAULT_RECT_WIDTH)
			lpTrayRect->left = lpTrayRect->right - DEFAULT_RECT_WIDTH;
		if (lpTrayRect->bottom - lpTrayRect->top > DEFAULT_RECT_HEIGHT)
			lpTrayRect->top = lpTrayRect->bottom - DEFAULT_RECT_HEIGHT;

		return ;
	}

	// OK. Haven't found a thing. Provide a default rect based on the current work
	// area
	SystemParametersInfo(SPI_GETWORKAREA, 0, lpTrayRect, 0);
	lpTrayRect->left = lpTrayRect->right - DEFAULT_RECT_WIDTH;
	lpTrayRect->top = lpTrayRect->bottom - DEFAULT_RECT_HEIGHT;
}

// Check to see if the animation has been disabled
static bool GetDoAnimateMinimize(VOID)
{
	ANIMATIONINFO ai;

	ai.cbSize = sizeof(ai);
	SystemParametersInfo(SPI_GETANIMATION, sizeof(ai), &ai, 0);

	return ai.iMinAnimate ? true : false;
}

void MinimizeWndToTray(HWND hWnd)
{
	if (GetDoAnimateMinimize())
	{
		RECT rcFrom, rcTo;

		// Get the rect of the window. It is safe to use the rect of the whole
		// window - DrawAnimatedRects will only draw the caption
		GetWindowRect(hWnd, &rcFrom);
		GetTrayWndRect(&rcTo);

		// Get the system to draw our animation for us
		DrawAnimatedRects(hWnd, IDANI_CAPTION, &rcFrom, &rcTo);
	}

	// Add the tray icon. If we add it before the call to DrawAnimatedRects,
	// the taskbar gets erased, but doesn't get redrawn until DAR finishes.
	// This looks untidy, so call the functions in this order

	// Hide the window
	ShowWindow(hWnd, SW_HIDE);
}

void RestoreWndFromTray(HWND hWnd)
{
	if (GetDoAnimateMinimize())
	{
		// Get the rect of the tray and the window. Note that the window rect
		// is still valid even though the window is hidden
		RECT rcFrom, rcTo;
		GetTrayWndRect(&rcFrom);
		GetWindowRect(hWnd, &rcTo);

		// Get the system to draw our animation for us
		DrawAnimatedRects(hWnd, IDANI_CAPTION, &rcFrom, &rcTo);
	}

	// Show the window, and make sure we're the foreground window
	ShowWindow(hWnd, SW_SHOW);
	SetActiveWindow(hWnd);
	SetForegroundWindow(hWnd);

	// Remove the tray icon. As described above, remove the icon after the
	// call to DrawAnimatedRects, or the taskbar will not refresh itself
	// properly until DAR finished
}

CComboBoxHook::CComboBoxHook()
	: CComboBox()
{
}

CComboBoxHook::CComboBoxHook(HWND hWndControl)
	: CComboBox(hWndControl)
{
}

CComboBoxHook::CComboBoxHook(HWND hWndDialog, UINT nCtlID)
	: CComboBox(hWndDialog, nCtlID)
{
}

void CComboBoxHook::Attach(HWND hWndControl)
{
	CControlBase::Attach(hWndControl);
	RemoveStyle(m_hWnd, CBS_SORT);
	m_LastSelStart = 0;
	m_LastSelEnd = -1; // 默认选择全部的文本
	::EnumChildWindows(m_hWnd, CComboBoxHook::x_EnumChildProc, (LPARAM)this);
}

BOOL CALLBACK CComboBoxHook::x_EnumChildProc(HWND hWnd, LPARAM lParam)
{
#ifdef _DEBUG
	tchar buf[16];
	::GetClassName(hWnd, buf, COUNTOF(buf));
	ASSERT(!_tcsicmp(buf, _T("Edit")));
#endif

	CComboBoxHook * _this = (CComboBoxHook *)lParam;
	_this->m_hWndEdit = hWnd;
	wpf::SetWindowPointer(hWnd, _this);
	_this->m_EditOldProc = SubclassWindow(hWnd, CComboBoxHook::x_SubclassProc);
	return false;
}

LRESULT CALLBACK CComboBoxHook::x_SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CComboBoxHook * _this = (CComboBoxHook *)wpf::GetWindowPointer(hWnd);
	ASSERT(_this);
	if (0 == _this)
	{
		return 0;
	}

	switch (uMsg)
	{
	case WM_KILLFOCUS:
		{
			api::SendMessageT(hWnd, EM_GETSEL, &_this->m_LastSelStart, &_this->m_LastSelEnd);
		}
		break;
	}

	return CallWindowProc(_this->m_EditOldProc, hWnd, uMsg, wParam, lParam);
}

void EnableShutDownPrivilege(bool bEnable)
{
	// 附给本进程特权，以便访问系统进程
	HANDLE hToken;

	// 打开一个进程的访问令牌
	if(::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		// 取得特权名称为“SetDebugPrivilege”的LUID
		LUID uID;
		::LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &uID);

		// 调整特权级别
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = uID;
		tp.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;
		::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);

		// 关闭访问令牌句柄
		::CloseHandle(hToken);
	}
}

}; //end of namespace wpf




