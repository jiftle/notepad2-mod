// ***************************************************************
//  win32   version:  1.0    date: 11/12/2006
//  -------------------------------------------------------------
//     .︵∧ ∧∧ ∧︵.
//  ～(灬ミ^ō^ミ^ō^ミ灬)～ 
//  Author: bluenet
//  Summary: 要使用该文件必须在单独的 CPP 里实现 HINSTANCE wpf::GetInstance 函数
//  -------------------------------------------------------------
//  Copyright (C) 2006 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************
#ifndef WINPLATFORM__H__
#define WINPLATFORM__H__

//Win32 platforms

#ifdef _MSC_VER
#pragma warning(disable:4786)
#pragma warning(disable:4996) // Message: 'You have used a std:: construct that is not safe.
#endif
#define NOMINMAX
#include <Windows.h>
#include <WindowsX.h>
#include <commdlg.h>
#include <commctrl.h>
#include <tchar.h>
#include <assert.h>
#include <process.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>

#include "utils.h"
#include "utf8.h"
#include "gdi.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x20A
#endif

// 比较安全的用户自定义消息
#define WM_USERMESSAGE (WM_USER + 100)

#ifndef COLOR_HOTLIGHT
	#define COLOR_HOTLIGHT 26
#endif

// must implement in a cpp
HINSTANCE WpfGetInstance();



#ifndef WM_GETISHELLBROWSER
   #define WM_GETISHELLBROWSER (WM_USER+7)
#endif
#define GetPIDLFolder(pida) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define GetPIDLItem(pida, i) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])


#ifdef _DEBUG
#define EXCLUDE_MESSAGE(msg, return_value) do\
						{\
							switch (msg)\
							{\
							case WM_GETDLGCODE:\
							case WM_PAINT:\
							case WM_NCPAINT:\
							case WM_ERASEBKGND:\
							case WM_CTLCOLORMSGBOX:\
							case WM_CTLCOLOREDIT:\
							case WM_CTLCOLORLISTBOX:\
							case WM_CTLCOLORBTN:\
							case WM_CTLCOLORDLG:\
							case WM_CTLCOLORSCROLLBAR:\
							case WM_CTLCOLORSTATIC:\
							case WM_NCHITTEST:\
							case WM_SETCURSOR:\
							case WM_MOUSEMOVE:\
							case WM_PARENTNOTIFY:\
							case WM_MOUSEACTIVATE:\
								return return_value;\
							}\
						} while (false)
#else
#define EXCLUDE_MESSAGE(msg, return_value) ((void)0)
#endif



namespace wpf {


namespace CONST_VALUE {

// 缓冲区递增步长
const size_t BUFFER_STEP_SIZE = 256;
// 缓冲区最大值 (128 MB)
const size_t MAX_BUFFER_SIZE = 128 * 1024 * 1024;
// 存储路径缓冲区最大值
const size_t MAX_PATH_BUFFER_SIZE = MAX_PATH * 4;

};




template<typename T>
inline bool IsBadPathChar(T c)
{
	return '|' == c || '*' == c || '?' == c || 
		'<' == c || '>' == c || '\"' == c;
}

template<typename T>
inline bool IsBadFileChar(T c)
{
	return '\\' == c || '/' == c || ':' == c || IsBadPathChar(c);
}

inline void RemoveFileSpec(tchar * path)
{
	tchar * p = _tcsrchr(path, '\\');
	if (p && p > path)
		*p = 0;
}

inline void AddStyle(HWND hwnd, DWORD dwStyle)
{
    DWORD tmp = GetWindowLongPtr(hwnd, GWL_STYLE);
    tmp |= dwStyle;
    SetWindowLongPtr(hwnd, GWL_STYLE, tmp);
}

inline void RemoveStyle(HWND hwnd, DWORD dwStyle)
{
    DWORD tmp = GetWindowLongPtr(hwnd, GWL_STYLE);
    tmp &= ~dwStyle;
    SetWindowLongPtr(hwnd, GWL_STYLE, tmp);
}

#ifdef GetWindowStyle
#undef GetWindowStyle
#endif
inline DWORD GetWindowStyle(HWND hWnd)
{
	return (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
}

inline bool HasWindowStyle(HWND hWnd, DWORD dwStyle)
{
	return (GetWindowStyle(hWnd) & dwStyle) != 0;
}

#ifdef SubclassWindow
#undef SubclassWindow
#endif
inline WNDPROC SubclassWindow(HWND hWnd, WNDPROC pNewProc)
{
	return (WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)pNewProc);
}

inline bool EnableDlgItem(HWND hWnd, int nID, bool bEnable)
{ return ::EnableWindow(::GetDlgItem(hWnd, nID), bEnable) != FALSE; }

inline bool ShowDlgItem(HWND hWnd, int nID, int CmdShow)
{ return ::ShowWindow(::GetDlgItem(hWnd, nID), CmdShow) != FALSE; }

inline bool MoveDlgItem(HWND hWnd, int nID, int X, int Y, int nWidth, int nHeight, bool bRepaint = true)
{
	return ::MoveWindow(::GetDlgItem(hWnd, nID), X, Y, nWidth, nHeight, bRepaint) != FALSE;
}

inline bool IsWndTextEmpty(HWND hWnd, int CtrlID = -1)
{
	TCHAR tmp[2];
	int len = (int)::SendMessage(CtrlID > 0 ? ::GetDlgItem(hWnd, CtrlID) : hWnd, 
		WM_GETTEXT, 2, (LPARAM)tmp);
	return len <= 0 || tmp[0] == 0;
}

inline void GetChildRect(HWND wnd, UINT id, RECT * child)
{
	GetWindowRect(GetDlgItem(wnd, id), child);
	MapWindowPoints(0, wnd, (POINT*)child, 2);
}

inline const LOGFONT & GetDefaultFont(LOGFONT & out)
{
	GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(out), &out);
	return out;
}

inline bool IsBadRect(const RECT * r)
{
	return r->right <= r->left || r->bottom <= r->top;
}

inline void BeginWaitCursor()
{
    DestroyCursor(SetCursor(LoadCursor(NULL, IDC_WAIT)));
}


inline void EndWaitCursor()
{
    DestroyCursor(SetCursor(LoadCursor(NULL, IDC_ARROW)));
}

// 从已被关联的窗口获取指针
inline LONG_PTR GetWindowPointer(HWND wnd)
{
	return ::GetWindowLongPtr(wnd, GWLP_USERDATA);
}

// 将一个指针关联到窗口上
template<typename T>
inline void SetWindowPointer(HWND wnd, T ptr)
{
	::SetWindowLongPtr(wnd, GWLP_USERDATA, (LONG_PTR)ptr);
}



/**************************************************************************
 *   Helper class of Member function callback mechanism
 *   可以把类的非静态函数作为回调函数
 **************************************************************************/
static __declspec( naked ) int StdDynamicJmpProc()
{
	_asm
	{
		POP ECX
		MOV EAX, DWORD PTR [ECX + 4] // func
		MOV ECX, [ECX]     // this
		JMP EAX
	}
}

/* 
 * NOTE: TStdcallType: a type of function pointer to API or Callbacks, *MUST* be _stdcall
         TMemberType: a type of function pointer to class member function,
         *MUST* be the *DEFAULT* calling conversation, *NO* prefix should be added,
          that is, using ECX for "this" pointer, pushing parameters from right to left,
          and the callee cleans the stack.
   LIFE TIME:  It is important to keep the ACCallback object alive until the CALLBACK is not required!!!
   CDynamicCallback 已被重载为 TStdcallType 类型, 测试代码如下:

	class CTestCodepage
	{
	public:
		BOOL OnCodePagesProc(LPTSTR lpCodePageString)
		{
			std::cout << lpCodePageString << std::endl;
			return true;
		}
	};

	int main()
	{
 		CTestCodepage asdf;
		CDynamicCallback<CODEPAGE_ENUMPROC> bb(&asdf, &CTestCodepage::OnCodePagesProc);
		EnumSystemCodePages(bb, CP_INSTALLED);
		return 0;
	}
*/
template <typename TStdcallType>
class CDynamicCallback
{
private:
#pragma pack(push, 1)
	struct DynamicCallbackOpCodes
	{
		unsigned char tag;  // CALL e8
		LONG_PTR offset;  // offset (dest - src - 5, 5=sizeof(tag + offset))
		LONG_PTR _this;   // a this pointer
		LONG_PTR _func;   // pointer to real member function address
	};
#pragma pack(pop)

	LONG_PTR m_pThis;
	LONG_PTR m_pFunc;
	TStdcallType m_pStdFunc;

	static LONG_PTR CalcJmpOffset(LONG_PTR Src, LONG_PTR Dest)
	{
		return Dest - (Src + 5);
	}

	void MakeCode()
	{
		if (m_pStdFunc) ::VirtualFree(m_pStdFunc, 0, MEM_RELEASE);
		m_pStdFunc = (TStdcallType)::VirtualAlloc(NULL, sizeof(DynamicCallbackOpCodes), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		DynamicCallbackOpCodes * p = (DynamicCallbackOpCodes *)m_pStdFunc;
		p->_func = *(LONG_PTR *)&m_pFunc;
		p->_this = (LONG_PTR)m_pThis;
		p->tag = 0xE8;
		p->offset = CalcJmpOffset((LONG_PTR)p, (LONG_PTR)StdDynamicJmpProc);
	}

public:
	CDynamicCallback() {}

	template<typename T1, typename T2>
	CDynamicCallback(T1 pClassAddress, T2 pClassMemberFunctionAddress)
	{
		Assign(pClassAddress, pClassMemberFunctionAddress);
	}

	template<typename T1, typename T2>
	void Assign(T1 pClassAddress, T2 pClassMemberFunctionAddress)
	{
		// pClassAddress 必须是类指针
		STATIC_ASSERT(util::type_trait::is_pointer<T1>::value);
		// pClassMemberFunctionAddress 必须是类成员函数指针
		STATIC_ASSERT(util::type_trait::is_member_function_pointer<T2>::value);
		m_pFunc = *(LONG_PTR *)&pClassMemberFunctionAddress;
		m_pThis = (LONG_PTR)pClassAddress;
		m_pStdFunc = NULL;
		MakeCode();
	}

	~CDynamicCallback()
	{
		::VirtualFree(m_pStdFunc, 0, MEM_RELEASE);
	}

	inline operator TStdcallType()
	{
		return m_pStdFunc;
	}

	inline TStdcallType operator()()
	{
		return m_pStdFunc;
	}
};






//namespace api
namespace api {


// voerload functions
UINT DragQueryFileT(tstring & out, HDROP hDrop, UINT idx);
DWORD GetLongPathNameT(tstring & out, const tchar * path);
DWORD GetShortPathNameT(tstring & out, const tchar * path);
DWORD GetFullPathNameT(tstring & io);
int SearchPathT(tstring & out, const tchar * path);
int ExpandEnvironmentStringsT(tstring & out, const tchar * path);
bool LoadResourceT(std::vector<BYTE> & out, int id, const TCHAR * type);
bool IsAppThemedT();
bool ChooseFontT(HWND parent, LOGFONTA & out);
bool ChooseFontT(HWND parent, LOGFONTW & out);
bool ChooseColorT(HWND wnd, COLORREF & out, COLORREF custom = 0);
int LoadStringT(tstring & out, UINT nID, HINSTANCE hInstance = NULL);
int GetMenuStringT(tstring & out, HMENU hMenu, UINT uIDItem, UINT uFlag);
int GetDlgItemTextT(tstring & out, HWND hWnd, UINT nID);
tstring GetListControlTextT(HWND hWnd, int iItem, int iSubItem);
int GetCurrentDirectoryT(tstring & out);
UINT GetWindowsDirectoryT(tstring & out);
int GetWindowTextT(tstring & out, HWND hWnd);
DWORD GetModuleFileNameT(tstring & out, HMODULE hModule = NULL);
DWORD GetTempPathT(tstring & out);
//create a temp file in PathName
uint GetTempFileNameT(tstring & out, const tchar * PathName, const tchar * PrefixString, uint uUnique = 0U);
//create a temp file in temp path
uint GetTempFileNameT(tstring & out, const tchar * PrefixString, uint uUnique = 0U);
void SetLayeredWindowAttributeT(HWND hwnd, BOOL bTransparentMode, BYTE bAlpha);
//if pExitCode > 0 than wait for cmd finish.
bool CreateProcessT(const TCHAR * cmd, DWORD * pExitCode = NULL);
bool SHBrowseForFolderT(tstring & out, HWND hwndParent, const TCHAR * Title, const TCHAR * BaseDir = NULL);
void GetLastErrorT(tstring & out);
// 显示打开文件对话框, 参数
// out: 如果用户打开了文件, 则该变量包含了文件的全路径
// ParentWnd: 父窗口句柄, 无父窗口是可以留空
// Filter: 扩展名过滤列表, 如 "可执行文件\0*.exe;*.dll\0所有文件\0*.*\0\0"
// DefFilename: 文件存在时则打开文件窗口会自动定位到该文件的目录
// DefExt: 默认的文件扩展名
bool GetOpenFileNameT(tstring & out, HWND ParentWnd, const TCHAR * Filter, const TCHAR * DefFilename = NULL, const TCHAR * DefExt = NULL);
// 显示保存文件对话框, 参数
// out: 如果用户打开了文件, 则该变量包含了文件的全路径
// ParentWnd: 父窗口句柄, 无父窗口是可以留空
// Filter: 扩展名过滤列表, 如 "可执行文件\0*.exe;*.dll\0所有文件\0*.*\0\0"
// DefFilename: 文件存在时则打开文件窗口会自动定位到该文件的目录
// DefExt: 默认的文件扩展名
bool GetSaveFileNameT(tstring & out, HWND ParentWnd, const TCHAR * Filter, const TCHAR * DefFilename = NULL, const TCHAR * DefExt = NULL);




namespace inner {

template<typename T1, typename T2>
inline LRESULT SendMessageImpl(HWND hWnd, UINT uMsg, T1 wParam, T2 lParam, util::int2type<0> /*summy*/)
{ return ::SendMessage(hWnd, uMsg, (WPARAM)wParam, (LPARAM)lParam); }

template<typename T1, typename T2>
inline LRESULT SendMessageImpl(HWND hWnd, UINT uMsg, T1 wParam, T2 lParam, util::int2type<1> /*summy*/)
{ return ::SendMessage(hWnd, uMsg, wParam, (LPARAM)lParam); }

template<typename T1, typename T2>
inline LRESULT SendMessageImpl(HWND hWnd, UINT uMsg, T1 wParam, T2 lParam, util::int2type<2> /*summy*/)
{ return ::SendMessage(hWnd, uMsg, (WPARAM)wParam, lParam); }

template<typename T1, typename T2>
inline LRESULT SendMessageImpl(HWND hWnd, UINT uMsg, T1 wParam, T2 lParam, util::int2type<3> /*summy*/)
{ return ::SendMessage(hWnd, uMsg, wParam, lParam); }

template<typename T1, typename T2>
inline LRESULT PostMessageImpl(HWND hWnd, UINT uMsg, T1 wParam, T2 lParam, util::int2type<0> /*summy*/)
{ return ::PostMessage(hWnd, uMsg, (WPARAM)wParam, (LPARAM)lParam); }

template<typename T1, typename T2>
inline LRESULT PostMessageImpl(HWND hWnd, UINT uMsg, T1 wParam, T2 lParam, util::int2type<1> /*summy*/)
{ return ::PostMessage(hWnd, uMsg, wParam, (LPARAM)lParam); }

template<typename T1, typename T2>
inline LRESULT PostMessageImpl(HWND hWnd, UINT uMsg, T1 wParam, T2 lParam, util::int2type<2> /*summy*/)
{ return ::PostMessage(hWnd, uMsg, (WPARAM)wParam, lParam); }

template<typename T1, typename T2>
inline LRESULT PostMessageImpl(HWND hWnd, UINT uMsg, T1 wParam, T2 lParam, util::int2type<3> /*summy*/)
{ return ::PostMessage(hWnd, uMsg, wParam, lParam); }

template<typename T1, typename T2>
inline LRESULT SendDlgItemMessageImpl(HWND hWnd, int nID, UINT uMsg, T1 wParam, T2 lParam, util::int2type<0> /*summy*/)
{ return ::SendDlgItemMessage(hWnd, nID, uMsg, (WPARAM)wParam, (LPARAM)lParam); }

template<typename T1, typename T2>
inline LRESULT SendDlgItemMessageImpl(HWND hWnd, int nID, UINT uMsg, T1 wParam, T2 lParam, util::int2type<1> /*summy*/)
{ return ::SendDlgItemMessage(hWnd, nID, uMsg, wParam, (LPARAM)lParam); }

template<typename T1, typename T2>
inline LRESULT SendDlgItemMessageImpl(HWND hWnd, int nID, UINT uMsg, T1 wParam, T2 lParam, util::int2type<2> /*summy*/)
{ return ::SendDlgItemMessage(hWnd, nID, uMsg, (WPARAM)wParam, lParam); }

template<typename T1, typename T2>
inline LRESULT SendDlgItemMessageImpl(HWND hWnd, int nID, UINT uMsg, T1 wParam, T2 lParam, util::int2type<3> /*summy*/)
{ return ::SendDlgItemMessage(hWnd, nID, uMsg, wParam, lParam); }

};

template<typename T1, typename T2>
inline LRESULT SendMessageT(HWND hWnd, UINT uMsg, T1 wParam, T2 lParam)
{ return inner::SendMessageImpl(hWnd, uMsg, wParam, lParam, util::int2type<util::can_safe_convert<T1, WPARAM>::value | (util::can_safe_convert<T2, LPARAM>::value << 1)>()); }

template<typename T1, typename T2>
inline LRESULT PostMessageT(HWND hWnd, UINT uMsg, T1 wParam, T2 lParam)
{ return inner::PostMessageImpl(hWnd, uMsg, wParam, lParam, util::int2type<util::can_safe_convert<T1, WPARAM>::value | (util::can_safe_convert<T2, LPARAM>::value << 1)>()); }

template<typename T1, typename T2>
inline LRESULT SendDlgItemMessageT(HWND hWnd, int nID, UINT uMsg, T1 wParam, T2 lParam)
{ return inner::SendDlgItemMessageImpl(hWnd, nID, uMsg, wParam, lParam, util::int2type<util::can_safe_convert<T1, WPARAM>::value | (util::can_safe_convert<T2, LPARAM>::value << 1)>()); }



}; //end of namespace api

namespace com {


template <typename T>
class auto_ptr
{
public:
	inline auto_ptr() : ptr(NULL) {}
	inline ~auto_ptr() { release(); }

	inline auto_ptr(auto_ptr & rhs) { _copy(rhs); }
	inline auto_ptr & operator= (auto_ptr & rhs) { return _copy(rhs); }

	inline T & operator* () { return *ptr; }
	inline T * operator-> () { return ptr; }

public:
	inline T ** address() { return &ptr; }
	inline T * detach() { T * tmp = ptr; ptr = 0; return tmp; }
	inline void attach(T * _ptr) { ptr = _ptr; }
	inline void release() { if (ptr) ptr->Release(); ptr = NULL; }

private:
	auto_ptr & _copy(auto_ptr & rhs) { if (ptr != rhs.ptr) attach(rhs.detach()); return *this; }
	T * ptr;
};


// COM functions
bool GetLinkPath(tstring & out, const tchar * linkfile);
bool CreateLink(const tchar * linkfile,
				const tchar * target,
				const tchar * args = NULL,
				const tchar * iconpath = NULL,
				uint icon = 0,
				const tchar * workdir = NULL,
				const tchar * description = NULL);
bool ILIsFile(LPCITEMIDLIST pidl);
void FreePIDL(void * idl);
bool GetNameFromPIDL(tstring & out, const LPSHELLFOLDER & lpsf, const LPITEMIDLIST & pidl, bool bShowExt);


}; //namespace com



#ifdef _SEH
namespace crash {


void DumpCrashInfo(LPEXCEPTION_POINTERS param, const char * CallPath, const char * ver_string = NULL);


}; //namespace crash
#endif




bool IsWindowsNT();
void CenterWindow(HWND hWnd, HWND hWndCenter = NULL);
void MinimizeWndToTray(HWND hWnd);
void RestoreWndFromTray(HWND hWnd);
HMENU FindMenu(HMENU h, int id, int & pos);
bool HasBadFileChar(const tchar * file);
bool HasBadPathChar(const tchar * path);
bool IsCmdEnabled(HWND hwnd, UINT id);
tstring GetMyFullPath(const TCHAR * path);
tstring GetDefaultOpenDir();
const TCHAR * FormatNumberStr(TCHAR * lpNumberStr);
void CopyToClipboard(const char * Text, size_t Len);
void CopyToClipboard(const wchar * Text, size_t Len);
inline void CopyToClipboard(const char * Text) { CopyToClipboard(Text, strlen(Text)); }
inline void CopyToClipboard(const wchar * Text) { CopyToClipboard(Text, wcslen(Text)); }
bool FileExists(const TCHAR * DestFile, WIN32_FIND_DATA * pfd_out = NULL);
tstring GetSpecialFolderDir(int nFolder);
int SimpToTrad(const wchar * src, wstring & dest, size_t src_len);
int TradToSimp(const wchar * src, wstring & dest, size_t src_len);
void FixFileChars(tstring & Io, char CharToFix = '_');
BYTE CharsetFromCodepage(uint cp);
void EnableShutDownPrivilege(bool bEnable);


//GDI





class CFindFile
{
public:
	typedef WIN32_FIND_DATA FIND_DATA;

	CFindFile() : h(NULL) {}
	~CFindFile() { FindClose(); }

	inline bool FindFirst(const tchar * filespec)
	{
		return (h = ::FindFirstFile(filespec, &fd)) != INVALID_HANDLE_VALUE;
	}

	inline bool FindFirst(const tstring & filespec)
	{
		return (h = ::FindFirstFile(filespec.c_str(), &fd)) != INVALID_HANDLE_VALUE;
	}

	inline bool FindNext() { return ::FindNextFile(h, &fd) != FALSE; }

	inline bool FindClose()
	{
		bool ret = false;
		if (h && INVALID_HANDLE_VALUE != h)
		{
			ret = ::FindClose(h) != FALSE;
			h = NULL;
		}
		return ret;
	}

	inline const FIND_DATA & GetRawFindData() const { return fd; }
	inline const tchar * GetFileName() const { return fd.cFileName; }
	inline bool IsDirectory() const { return (FILE_ATTRIBUTE_DIRECTORY & fd.dwFileAttributes) != 0; }

private:
	HANDLE h;
	FIND_DATA fd;
};

class ResStr
{
public:
	ResStr(UINT nID, HINSTANCE hInstance = NULL);
	~ResStr() { delete [] buf; }
	inline operator const TCHAR * () const { return buf; }
	inline const TCHAR * GetPtr() const { return buf; }

private:
	ResStr(const ResStr & rhs) {} //disabled
	inline ResStr & operator = (const ResStr & rhs) {} //disabled
	TCHAR * buf;
};



//namespace gui {


class CThemeHelper
{
public:
	CThemeHelper();

	enum TEXTURE_FLAG {
		FLAG_ETDT_DISABLE        = 0x00000001,
		FLAG_ETDT_ENABLE         = 0x00000002,
		FLAG_ETDT_USETABTEXTURE  = 0x00000004,
		FLAG_ETDT_ENABLETAB      = (FLAG_ETDT_ENABLE | FLAG_ETDT_USETABTEXTURE),
		FLAG_ETDT_ALL			 = FLAG_ETDT_ENABLETAB,
	};

	bool Load();
	bool Unload();

	LRESULT EnableThemeDialogTexture(HWND wnd, DWORD flags = FLAG_ETDT_ENABLETAB);

private:
	typedef HRESULT WINAPI EnableThemeDialogTextureProc(HWND hwnd, DWORD dwFlags);

	unsigned int RefCount;
	HINSTANCE ModUxtheme;
	EnableThemeDialogTextureProc * pEnableThemeDialogTexture;
};

class CWndBase
{
	NONCOPYABLE(CWndBase);

public:
	CWndBase() : m_hWnd(NULL) {}
	virtual ~CWndBase() {}

	inline HWND GetHandle() const { ASSERT(m_hWnd); return m_hWnd; }
	inline HWND operator()() const { return GetHandle(); }

	inline bool DestroyWindow() const
	{ ASSERT(m_hWnd); return ::DestroyWindow(m_hWnd) != FALSE; }

	inline HWND GetDlgItem(int nIDDlgItem)
	{ ASSERT(m_hWnd); return ::GetDlgItem(m_hWnd, nIDDlgItem); }

	inline bool SetWindowPos(HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) const
	{ ASSERT(m_hWnd); return ::SetWindowPos(m_hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags) != FALSE; }

	template<typename T1, typename T2>
	inline LRESULT SendMessage(UINT uMsg, T1 wParam, T2 lParam) const
	{ ASSERT(m_hWnd); return api::SendMessageT(m_hWnd, uMsg, wParam, lParam); }

	template<typename T1>
	inline LRESULT SendMessage(UINT uMsg, T1 wParam) const
	{ ASSERT(m_hWnd); return api::SendMessageT(m_hWnd, uMsg, wParam, 0); }

	inline LRESULT SendMessage(UINT uMsg) const
	{ ASSERT(m_hWnd); return ::SendMessage(m_hWnd, uMsg, 0, 0); }

	template<typename T1, typename T2>
	inline bool PostMessage(UINT uMsg, T1 wParam, T2 lParam) const
	{ ASSERT(m_hWnd); return api::PostMessageT(m_hWnd, uMsg, wParam, lParam) != FALSE; }

	template<typename T1>
	inline bool PostMessage(UINT uMsg, T1 wParam) const
	{ ASSERT(m_hWnd); return api::PostMessageT(m_hWnd, uMsg, wParam, 0) != FALSE; }

	inline bool PostMessage(UINT uMsg) const
	{ ASSERT(m_hWnd); return ::PostMessage(m_hWnd, uMsg, 0, 0) != FALSE; }

	template<typename T1, typename T2>
	inline LRESULT SendDlgItemMessage(int nIDDlgItem, UINT Msg, T1 wParam, T2 lParam) const
	{ ASSERT(m_hWnd); return api::SendDlgItemMessageT(m_hWnd, nIDDlgItem, Msg, wParam, lParam); }

	template<typename T1>
	inline LRESULT SendDlgItemMessage(int nIDDlgItem, UINT Msg, T1 wParam) const
	{ ASSERT(m_hWnd); return api::SendDlgItemMessageT(m_hWnd, nIDDlgItem, Msg, wParam, 0); }

	inline LRESULT SendDlgItemMessage(int nIDDlgItem, UINT Msg) const
	{ ASSERT(m_hWnd); return ::SendDlgItemMessage(m_hWnd, nIDDlgItem, Msg, 0, 0); }

	inline int MessageBox(LPCTSTR lpText, LPCTSTR lpCaption, UINT uType = MB_OK) const
	{ ASSERT(m_hWnd); return ::MessageBox(m_hWnd, lpText, lpCaption, uType); }

	inline bool SetDlgItemText(int nID, const tchar * lpString) const
	{ ASSERT(m_hWnd); return ::SetDlgItemText(m_hWnd, nID, lpString) != 0; }

	inline uint GetDlgItemText(int nID, tchar * lpBuffer, int ccMax) const
	{ ASSERT(m_hWnd); return ::GetDlgItemText(m_hWnd, nID, lpBuffer, ccMax); }

	inline uint GetDlgItemText(int nID, tstring & out) const
	{ ASSERT(m_hWnd); return api::GetDlgItemTextT(out, m_hWnd, nID); }

	inline bool SetWindowText(const tchar * lpString) const
	{ ASSERT(m_hWnd); return ::SetWindowText(m_hWnd, lpString) != 0; }

	inline uint GetWindowText(tchar * lpBuffer, int ccMax) const
	{ ASSERT(m_hWnd); return ::GetWindowText(m_hWnd, lpBuffer, ccMax); }

	inline uint GetWindowText(tstring & out) const
	{ ASSERT(m_hWnd); return api::GetWindowTextT(out, m_hWnd); }

	inline bool EnableDlgItem(int nID, bool bEnable)
	{ ASSERT(m_hWnd); return wpf::EnableDlgItem(m_hWnd, nID, bEnable); }

protected:
	HWND m_hWnd;
};

class CDialogBase : public CWndBase
{
	NONCOPYABLE(CDialogBase);

public:
	CDialogBase(UINT nID)
		: m_nID(nID)
	{}

private:
	static INT_PTR CALLBACK DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
	{
		CDialogBase * _this = NULL;
		INT_PTR rv;
		if (msg == WM_INITDIALOG)
		{
			_this = reinterpret_cast<CDialogBase *>(lp);
			SetWindowPointer(wnd, _this);
			_this->m_hWnd = wnd;
		}
		else _this = (CDialogBase *)GetWindowPointer(wnd);

		rv = _this ? _this->OnMessage(msg, wp, lp) : FALSE;

		if (msg == WM_DESTROY && _this)
		{
			// _this may have been deleted after processing the WM_DESTROY message
			SetWindowPointer(wnd, NULL);
		}

		return rv;
	}

protected:
	unsigned int m_nID;

	virtual INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

	inline bool EndDialog(INT_PTR nResult) const { return ::EndDialog(m_hWnd, nResult) != FALSE; }
	inline void CenterWindow() const { wpf::CenterWindow(m_hWnd, GetParent(m_hWnd)); }

public:
	inline INT_PTR DoModal(HWND hWndParent) const
	{
		return ::DialogBoxParam(WpfGetInstance(), MAKEINTRESOURCE(m_nID), hWndParent, DialogProc, (long)this);
	}

	inline HWND Create(HWND hWndParent) const
	{
		return ::CreateDialogParam(WpfGetInstance(), MAKEINTRESOURCE(m_nID), hWndParent, DialogProc, (long)this);
	}
};

template<unsigned int IDD>
class CDialogSimple : public CWndBase
{
	NONCOPYABLE(CDialogSimple);

public:
	CDialogSimple() {}

private:
	typedef CDialogSimple<IDD> my_type;

	static INT_PTR CALLBACK DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
	{
		my_type * _this = NULL;
		INT_PTR rv;
		if (msg == WM_INITDIALOG)
		{
			_this = reinterpret_cast<my_type *>(lp);
			SetWindowPointer(wnd, _this);
			_this->m_hWnd = wnd;
		}
		else _this = (my_type *)GetWindowPointer(wnd);

		rv = _this ? _this->OnMessage(msg, wp, lp) : FALSE;

		if (msg == WM_DESTROY && _this)
		{
			// _this may have been deleted after processing the WM_DESTROY message
			SetWindowPointer(wnd, NULL);
		}

		return rv;
	}

protected:
	virtual INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

	inline bool EndDialog(INT_PTR nResult) const { return ::EndDialog(m_hWnd, nResult) != FALSE; }
	inline void CenterWindow() const { wpf::CenterWindow(m_hWnd, GetParent(m_hWnd)); }

public:
	inline INT_PTR DoModal(HWND hWndParent) const
	{
		return ::DialogBoxParam(WpfGetInstance(), MAKEINTRESOURCE(IDD), hWndParent, DialogProc, (long)this);
	}

	inline HWND Create(HWND hWndParent) const
	{
		return ::CreateDialogParam(WpfGetInstance(), MAKEINTRESOURCE(IDD), hWndParent, DialogProc, (long)this);
	}
};

// 必须用 new 来创建对象，在窗口销毁时会自动删除对象
// 如果需要显式删除对象，请使用 DestroyWindow
class CModelessBase : public CWndBase
{
	NONCOPYABLE(CModelessBase);

public:
	CModelessBase(unsigned int nID) : m_nID(nID) {}

private:
	static INT_PTR CALLBACK DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
	{
		CModelessBase * _this;
		INT_PTR rv;
		if (msg == WM_INITDIALOG)
		{
			_this = reinterpret_cast<CModelessBase *>(lp);
			SetWindowPointer(wnd, _this);
			_this->m_hWnd = wnd;
		}
		else _this = (CModelessBase *)GetWindowPointer(wnd);

		rv = _this ? _this->OnMessage(msg, wp, lp) : FALSE;

		if (msg == WM_DESTROY && _this)
		{
			// _this may have been deleted after processing the WM_DESTROY message
			SetWindowPointer(wnd, NULL);
			delete _this;
		}

		return rv;
	}

protected:
	unsigned int m_nID;

	virtual INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	inline void CenterWindow() const { wpf::CenterWindow(m_hWnd, GetParent(m_hWnd)); }

public:
	inline HWND Create(HWND hWndParent) const
	{
		HWND hwnd = ::CreateDialogParam(WpfGetInstance(), MAKEINTRESOURCE(m_nID), hWndParent, DialogProc, (long)this);
		if (hwnd) {
			::ShowWindow(hwnd, SW_SHOW);
		}
		return hwnd;
	}
};

// 必须用 new 来创建对象，在窗口销毁时会自动删除对象
// 如果需要显式删除对象，请使用 DestroyWindow
template<unsigned int IDD>
class CModelessSimple : public CWndBase
{
	NONCOPYABLE(CModelessSimple);

public:
	CModelessSimple() {}

private:
	typedef CModelessSimple<IDD> my_type;

	static INT_PTR CALLBACK DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
	{
		my_type * _this;
		INT_PTR rv;
		if (msg == WM_INITDIALOG)
		{
			_this = reinterpret_cast<my_type *>(lp);
			SetWindowPointer(wnd, _this);
			_this->m_hWnd = wnd;
		}
		else _this = (my_type *)GetWindowPointer(wnd);

		rv = _this ? _this->OnMessage(msg, wp, lp) : FALSE;

		if (msg == WM_DESTROY && _this)
		{
			// _this may have been deleted after processing the WM_DESTROY message
			SetWindowPointer(wnd, NULL);
			delete _this;
		}

		return rv;
	}

protected:
	virtual INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	inline void CenterWindow() const { wpf::CenterWindow(m_hWnd, GetParent(m_hWnd)); }

public:
	inline HWND Create(HWND hWndParent) const
	{
		HWND hwnd = ::CreateDialogParam(WpfGetInstance(), MAKEINTRESOURCE(IDD), hWndParent, DialogProc, (long)this);
		if (hwnd) {
			::ShowWindow(hwnd, SW_SHOW);
		}
		return hwnd;
	}
};

class WPF_NO_VTABLE CControlBase : public CWndBase
{
	NONCOPYABLE(CControlBase);

public:
	CControlBase() {}
	CControlBase(HWND hWndControl) { Attach(hWndControl); }
	CControlBase(HWND hWndDialog, UINT nCtlID) { Attach(hWndDialog, nCtlID); }

	virtual void Attach(HWND hWndControl) { ASSERT(NULL == m_hWnd); m_hWnd = hWndControl; }
	inline void Attach(HWND hWndDialog, int nCtlID) { Attach(::GetDlgItem(hWndDialog, nCtlID)); }
	virtual HWND Detach()
	{
		HWND hWnd = m_hWnd;
		m_hWnd = NULL;
		return hWnd;
	}

	inline void EnableWindow(bool bState)
	{
		::EnableWindow(m_hWnd, bState);
	}
};


class CFileDialog : public CWndBase
{
public:
	enum {
		ID_FILENAME = 1090,
		ID_TYPENAME = 1089,
		ID_CB_FILENAME = 1148,
		ID_CB_TYPENAME = 1136,
		ID_LAST = 1200,
	};

	CFileDialog(bool bOpenFile,
		HWND ParentWnd,
		const TCHAR * Filter,
		const TCHAR * DefFilename = NULL,
		const TCHAR * DefExt = NULL);

	~CFileDialog()
	{
		if (m_buf)
			delete [] m_buf;
	}

	inline OPENFILENAME & GetRefOFN() { return m_ofn; }
	inline void SetTitle(const TCHAR * Title) { m_ofn.lpstrTitle = Title; }
	inline void SetInitDir(const TCHAR * InitDir) { m_ofn.lpstrInitialDir = InitDir; }
	inline void AddFlags(DWORD Flags) { m_ofn.Flags |= Flags; }
	inline void RemoveFlags(DWORD Flags) { m_ofn.Flags &= ~Flags; }
	inline bool DoModal()
	{
		CheckBuffer();
		bool ret = m_bOpen ? (::GetOpenFileName(&m_ofn) != FALSE)
			: (::GetSaveFileName(&m_ofn) != FALSE);
		return ret;
	}
	inline void GetFileName(tstring & out) const { out = m_ofn.lpstrFile; }
	//inline tstring GetFileName() const { return tstring(m_ofn.lpstrFile); }
	inline const TCHAR * GetFileName() const { return m_ofn.lpstrFile; }
	inline const std::list<tstring> & GetMultiFileRawData() const
	{ ASSERT(m_bOpen && (OFN_ALLOWMULTISELECT & m_ofn.Flags)); return m_list; }

	virtual LRESULT OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	OPENFILENAME m_ofn;
	TCHAR * m_buf;
	bool m_bOpen;

	std::list<tstring> m_list;

private:
	void CheckBuffer();

	static DWORD WINAPI DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		CFileDialog * _this = (CFileDialog *)GetWindowPointer(hWnd);
		LRESULT ret = _this->OnMessage(uMsg, wParam, lParam);
		if (ret) {
			switch (uMsg)
			{
			case WM_COMPAREITEM:
			case WM_VKEYTOITEM:
			case WM_CHARTOITEM:
			case WM_INITDIALOG:
			case WM_QUERYDRAGICON:
			case WM_CTLCOLORMSGBOX:
			case WM_CTLCOLOREDIT:
			case WM_CTLCOLORLISTBOX:
			case WM_CTLCOLORBTN:
			case WM_CTLCOLORDLG:
			case WM_CTLCOLORSCROLLBAR:
			case WM_CTLCOLORSTATIC:
				// return directly
				return ret;
			default:
				// return in DWL_MSGRESULT
				::SetWindowLongPtr(hWnd, DWLP_MSGRESULT, ret);
				return ret;
			}
		}
		return ::CallWindowProc((WNDPROC)_this->OldOFNHookProc, hWnd, uMsg, wParam, lParam);
	}

	static UINT_PTR CALLBACK OFNHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	LONG OldOFNHookProc;
};

#define CHECK_STATIC_LINK_MESSAGE(var, hwnd, msg, wparam, lparam) do\
					{\
						LRESULT ret = var.IsProcess(hwnd, msg, wparam, lparam);\
						if (0 != ret)\
						{\
							return ret;\
						}\
					} while (false)

class CStaticLink
{
public:
	//************************************
	// Method:    CStaticLink2
	// Returns:   
	// Qualifier: bluenet
	// Parameter: const TCHAR * URL 要打开的 URL，如果为 NULL 则根据控件上显示的文本来打开
	// Parameter: UINT nCursorResourceID 指定鼠标悬停时的光标资源，如果 nCursorResourceID <= 0 则默认使用系统的小手光标
	//************************************
	CStaticLink(UINT nCtlID, const TCHAR * URL = NULL, UINT nCursorResourceID = -1);
	virtual ~CStaticLink() {}

	// 应该在 WM_INITDIALOG 消息里被调用
	inline void SubclassDlgItem(HWND hWndDlg, UINT nCtl)
	{
		xm_ctl = nCtl;
		SubclassWindow(::GetDlgItem(hWndDlg, nCtl));
	}

	// 应该在 WM_INITDIALOG 消息里被调用
	void SubclassWindow(HWND hWndCtl);

	//************************************
	// Method:    IsProcess 测试父窗口的消息是否可以被字窗口处理
	// Returns:   LRESULT 如果子窗口已经处理则返回非 0，这时候父窗口过程处理函数可以直接返回
	// Qualifier: bluenet
	// Parameter: UINT uMsg
	// Parameter: WPARAM wParam
	// Parameter: LPARAM lParam
	//************************************
	LRESULT IsProcess(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	UINT xm_ctl;
	HWND xm_wnd;
	WNDPROC xm_old;
	UINT xm_id;
	tstring xm_url;
	CDynamicCallback<WNDPROC> xm_proc;
	wpf::CFont xm_font;
	COLORREF xm_color;
	bool xm_bOverControl;
};



//class CStaticLink
//{
//public:
//	//nIDHand = -1 meaning load system hand cursor
//	CStaticLink(int CtrlID, const tchar * Link = NULL, int nIDHand = -1)
//		: wnd(NULL), id(CtrlID), link(Link ? Link : _T("")),
//		hCursorNormal(NULL), hCursorHover(NULL), hFontNormal(NULL),
//		hFontHover(NULL), hover(false), idhand(nIDHand)
//	{ }
//
//	LRESULT Process(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//
//private:
//	inline bool PtInItem(HWND hWnd, const POINT & pt);
//
//private:
//	HWND wnd;
//	int id;
//	tstring link;
//	HCURSOR hCursorNormal;
//	HCURSOR hCursorHover;
//	HFONT hFontNormal;
//	HFONT hFontHover;
//	bool hover;
//	int idhand;
//};

class CProgressBar : public CControlBase
{
	NONCOPYABLE(CProgressBar);

public:
	inline LRESULT DeltaPosition(int nIncrement) const { return SendMessage(PBM_DELTAPOS, nIncrement); }
	inline uint GetPosition() const { return (uint)SendMessage(PBM_GETPOS); }
	inline COLORREF SetBarColor(COLORREF color) const { return (COLORREF)SendMessage(PBM_SETBARCOLOR, 0, color); }
	inline COLORREF SetBKColor(COLORREF color) const { return (COLORREF)SendMessage(PBM_SETBKCOLOR, 0, color); }
	inline uint SetPosition(int nNewPos) const { return (uint)SendMessage(PBM_SETPOS, nNewPos); }
	inline DWORD SetRange(int iLowLim, int iHighLim) const { return (DWORD)SendMessage(PBM_SETRANGE32, iLowLim, iHighLim); }
	inline uint SetStep(int nStepInc) const { return (uint)SendMessage(PBM_SETSTEP, nStepInc); }
};

class CListView : public CControlBase
{
	NONCOPYABLE(CListView);

public:
	CListView() : CControlBase() {}
	CListView(HWND hWndControl) : CControlBase(hWndControl) {}
	CListView(HWND hWndDialog, UINT nCtlID) : CControlBase(hWndDialog, nCtlID) {}

public:
	inline DWORD SetExtendedListViewStyle(DWORD dwExStyle, DWORD dwExMask = 0) const
	{ return (DWORD)SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, dwExMask, dwExStyle); }

	inline void SetItemState(int iItem, UINT State, UINT Mask)
	{
		LV_ITEM lvi;
		lvi.stateMask = Mask;
		lvi.state = State;
		SendMessage(LVM_SETITEMSTATE, iItem, &lvi);
	}

	inline int InsertColumn(int iColumn, const TCHAR * Header, int Width) const
	{
		LVCOLUMN lvc = { LVCF_WIDTH | LVCF_TEXT };
		lvc.cx = Width;
		lvc.pszText = (TCHAR *)Header;
		return (int)SendMessage(LVM_INSERTCOLUMN, iColumn, &lvc);
	}

	inline int InsertItem(const LVITEM & Item) const
	{ return (int)SendMessage(LVM_INSERTITEM, 0, &Item); }

	inline int InsertItem(int iItem, const TCHAR * pszText) const
	{
		ASSERT(pszText);
		LVITEM item = { LVIF_TEXT };
		item.iItem = iItem;
		item.pszText = (TCHAR *)pszText;
		return (int)SendMessage(LVM_INSERTITEM, 0, &item);
	}

	inline int SetItem(const LVITEM & Item) const
	{ return (int)SendMessage(LVM_SETITEM, 0, &Item); }

	inline int SetItem(int iItem, int iSubItem, const TCHAR * pszText) const
	{
		ASSERT(pszText);
		LVITEM item = { LVIF_TEXT };
		item.iItem = iItem;
		item.iSubItem = iSubItem;
		item.pszText = (TCHAR *)pszText;
		return (int)SendMessage(LVM_SETITEM, 0, &item);
	}

	inline int GetItemCount() const
	{ return (int)SendMessage(LVM_GETITEMCOUNT, 0, 0); }

	inline int GetStringWidth(const TCHAR * str) const
	{ return (int)SendMessage(LVM_GETSTRINGWIDTH, 0, str); }

	inline void SetColumnWidth(int iColumn, int cx = LVSCW_AUTOSIZE) const
	{ SendMessage(LVM_SETCOLUMNWIDTH, iColumn, MAKELPARAM(cx, 0)); }

	inline int GetColumnWidth(int iColumn) const
	{ return (int)SendMessage(LVM_GETCOLUMNWIDTH, iColumn); }

	inline void DeleteAllItems() const
	{ SendMessage(LVM_DELETEALLITEMS, 0, 0); }

	inline void DeleteColumn(int iColumn) const
	{ SendMessage(LVM_DELETECOLUMN, (WPARAM)iColumn, 0); }

	inline void DeleteItem(int iItem) const
	{
		ASSERT(iItem < this->GetItemCount());
		SendMessage(LVM_DELETEITEM, (WPARAM)iItem, 0);
	}

	inline int GetSelectedCount() const
	{ return (int)SendMessage(LVM_GETSELECTEDCOUNT, 0, 0); }

	inline int GetNextItem(UINT flags, int iStart = -1) const
	{ return (int)SendMessage(LVM_GETNEXTITEM, iStart, flags); }

	//get the selected item index
	inline int GetSelectedItem() const
	{
		if (GetSelectedCount())
			return GetNextItem(LVNI_ALL | LVNI_SELECTED, -1);
		else
			return -1;
	}

	inline void GetItem(LVITEM * pitem) const
	{ SendMessage(LVM_GETITEM, 0, pitem); }

	inline void GetItemText(int iItem, LVITEM * pitem) const
	{ SendMessage(LVM_GETITEMTEXT, iItem, pitem); }

	inline void GetItemText(int iItem, int iColumn, tchar * buf, int cchMax) const
	{
		ASSERT(iItem < this->GetItemCount());
		ASSERT(buf);
		LVITEM item = { LVIF_TEXT };
		item.iSubItem = iColumn;
		item.pszText = buf;
		item.cchTextMax = cchMax;
		GetItemText(iItem, &item);
		ASSERT(item.pszText);
	}

	inline void GetItemText(int iItem, int iColumn, tstring & out) const
	{
		ASSERT(iItem < this->GetItemCount());
		TCHAR buf[1024] = { 0 };
		GetItemText(iItem, iColumn, buf, countof(buf) - 1);
		out = buf;
	}
};

class CListBox : public CControlBase
{
	NONCOPYABLE(CListBox);

public:
	CListBox() : CControlBase() {}
	CListBox(HWND hWndControl) : CControlBase(hWndControl) {}
	CListBox(HWND hWndDialog, UINT nCtlID) : CControlBase(hWndDialog, nCtlID) {}

public:
	//The return value is the index of the position at which the string was inserted. 
	//If an error occurs, the return value is LB_ERR. 
	//If there is insufficient space to store the new string, the return value is LB_ERRSPACE.
	inline int InsertString(int Idx, const tchar * lpString) const { return (int)SendMessage(LB_INSERTSTRING, Idx, lpString); }

	//The return value is the zero-based index of the string in the list box. 
	//If an error occurs, the return value is LB_ERR. 
	//If there is insufficient space to store the new string, the return value is LB_ERRSPACE.
	inline int AddString(const tchar * lpString) const { return (int)SendMessage(LB_ADDSTRING, 0, lpString); }

	//The return value is a count of the strings remaining in the list. 
	//The return value is LB_ERR if the wParam parameter specifies an index greater than the number of items in the list. 
	inline int DeleteString(int Idx) const { return (int)SendMessage(LB_DELETESTRING, Idx); }

	//The returned count is one greater than the index value of the last item (the index is zero-based). 
	inline int GetCount() const { return (int)SendMessage(LB_GETCOUNT); }

	//In a single-selection list box, the return value is the zero-based index of the currently selected item. 
	//If there is no selection, the return value is LB_ERR.
	inline int GetCurSel() const { return (int)SendMessage(LB_GETCURSEL); }

	//The return value is the value associated with the item, or LB_ERR if an error occurs. 
	//If the item is in an owner-drawn list box and was created without the LBS_HASSTRINGS style, 
	//this value was in the lParam parameter of the LB_ADDSTRING or LB_INSERTSTRING message that added the item to the list box. 
	//Otherwise, it is the value in the lParam of the LB_SETITEMDATA message. 
	inline LRESULT GetItemData(int Idx) const { return SendMessage(LB_GETITEMDATA, Idx); }

	//If an item is selected, the return value is greater than zero; otherwise, it is zero. 
	//If an error occurs, the return value is LB_ERR.
	inline bool GetSel(int Idx) const { return SendMessage(LB_GETSEL, Idx) > 0; }

	//The return value is the count of selected items in the list box. 
	//If the list box is a single-selection list box, the return value is LB_ERR.
	inline int GetSelCount() const { return (int)SendMessage(LB_GETSELCOUNT); }

	//The return value is the length of the string, in TCHARs, excluding the terminating null character. 
	//Under certain conditions, this value may actually be greater than the length of the text. 
	//For more information, see the following Remarks section.
	//If the Idx parameter does not specify a valid index, the return value is LB_ERR.
	inline int GetTextLen(int Idx) const { return (int)SendMessage(LB_GETTEXTLEN, Idx); }

	//The return value is the length of the string, in TCHARs, excluding the terminating null character. 
	//If wParam does not specify a valid index, the return value is LB_ERR.
	inline int GetText(int Idx, tchar * buf) const { return (int)SendMessage(LB_GETTEXT, Idx, buf); }

	//The return value is the length of the string, in TCHARs, excluding the terminating null character. 
	//If wParam does not specify a valid index, the return value is LB_ERR.
	inline int GetText(int Idx, tstring & out) const
	{
		int len = GetTextLen(Idx);
		if (len > 0)
		{
			out.resize(len + 1);
			GetText(Idx, (tchar *)out.c_str());
		}
		else
			out.erase();
		return len;
	}

	inline void ResetContent() const { SendMessage(LB_RESETCONTENT); }

	//If an error occurs, the return value is LB_ERR. 
	//If the wParam parameter is –1, the return value is LB_ERR even though no error occurred.
	inline int SetCurSel(int Idx) const { return (int)SendMessage(LB_SETCURSEL, Idx); }

	//If an error occurs, the return value is LB_ERR.
	template<typename T>
	inline int SetItemData(int Idx, T Data) const
	{
		return (int)SendMessage(LB_SETITEMDATA, Idx, Data);
	}

	//If an error occurs, the return value is LB_ERR.
	inline int SetSel(int Idx, bool bSelectState) const { return (int)SendMessage(LB_SETSEL, bSelectState, Idx); }
};

// 默认去掉了排序的属性 CBS_SORT
class CComboBox : public CControlBase
{
	NONCOPYABLE(CComboBox);

public:
	CComboBox() : CControlBase() {}
	CComboBox(HWND hWndControl) : CControlBase(hWndControl) {}
	CComboBox(HWND hWndDialog, UINT nCtlID) : CControlBase(hWndDialog, nCtlID) {}

	// add a string to the list box of a combo box. 
	// If the combo box does not have the CBS_SORT style, the string is added to the end of the list. Otherwise, the string is inserted into the list, and the list is sorted.
	// @lpString: Pointer to the null-terminated string to be added. 
	// If you create the combo box with an owner-drawn style but without the CBS_HASSTRINGS style, 
	// the value of the lParam parameter is stored as item data rather than the string it would otherwise point to. 
	// The item data can be retrieved or modified by sending the CB_GETITEMDATA or CB_SETITEMDATA message. 
	// @return: The return value is the zero-based index to the string in the list box of the combo box. 
	// If an error occurs, the return value is CB_ERR. If insufficient space is available to store the new string, it is CB_ERRSPACE. 
	inline int AddString(const tchar * lpString) const
	{
		return (int)SendMessage(CB_ADDSTRING, 0, lpString);
	}

	// delete a string in the list box of a combo box.
	// @idx: Specifies the zero-based index of the string to delete.
	// @return: The return value is a count of the strings remaining in the list. 
	// If the wParam parameter specifies an index greater than the number of items in the list, the return value is CB_ERR. 
	inline int DeleteString(uint idx) const
	{
		return (int)SendMessage(CB_DELETESTRING, idx);
	}

	// search the list box of a combo box for an item beginning with the characters in a specified string.
	// @nFirstItem: Specifies the zero-based index of the item preceding the first item to be searched. 
	// When the search reaches the bottom of the list box, it continues from the top of the list box back to the item specified by the wParam parameter. 
	// If wParam is –1, the entire list box is searched from the beginning. 
	// @lpString: Pointer to the null-terminated string that contains the characters for which to search. 
	// The search is not case sensitive, so this string can contain any combination of uppercase and lowercase letters.
	// @return: The return value is the zero-based index of the matching item. If the search is unsuccessful, it is CB_ERR.
	inline int FindString(uint nFirstItem, const tchar * lpString) const
	{
		return (int)SendMessage(CB_FINDSTRING, nFirstItem, lpString);
	}

	// retrieve the number of items in the list box of a combo box. 
	// @return: The return value is the number of items in the list box. If an error occurs, it is CB_ERR.
	inline int GetCount() const
	{
		return (int)SendMessage(CB_GETCOUNT);
	}

	// retrieve the index of the currently selected item, if any, in the list box of a combo box. 
	// @return: The return value is the zero-based index of the currently selected item. If no item is selected, it is CB_ERR.
	inline int GetCurSel() const
	{
		return (int)SendMessage(CB_GETCURSEL);
	}

	// insert a string into the list box of a combo box. Unlike the AddString, the InsertString does not cause a list with the CBS_SORT style to be sorted.
	// @nIdx: Specifies the zero-based index of the position at which to insert the string. 
	// If this parameter is –1, the string is added to the end of the list.
	// @lpString: Pointer to the null-terminated string to be inserted. 
	// If you create the combo box with an owner-drawn style but without the CBS_HASSTRINGS style, 
	// the value of the lParam parameter is stored rather than the string to which it would otherwise point.
	// @return: The return value is the index of the position at which the string was inserted. 
	// If an error occurs, the return value is CB_ERR. 
	// If there is insufficient space available to store the new string, it is CB_ERRSPACE.
	inline int InsertString(uint nIdx, const tchar * lpString) const
	{
		return (int)SendMessage(CB_INSERTSTRING, nIdx, lpString);
	}

	// limit the length of the text the user may type into the edit control of a combo box. 
	// @nMax: Specifies the maximum number of characters the user can enter, not including the terminating null character. 
	// If this parameter is zero, the text length is limited to 0x7FFFFFFE characters.
	inline void LimitText(int nMax) const
	{
		SendMessage(CB_LIMITTEXT, nMax);
	}

	// remove all items from the list box and edit control of a combo box.
	inline void ResetContent() const
	{
		SendMessage(CB_RESETCONTENT);
	}

	// to search the list of a combo box for an item that begins with the characters in a specified string. 
	// If a matching item is found, it is selected and copied to the edit control.
	// @nIdx: Specifies the zero-based index of the item preceding the first item to be searched. 
	// When the search reaches the bottom of the list, it continues from the top of the list back to the item specified by the wParam parameter. 
	// If wParam is –1, the entire list is searched from the beginning.
	inline int SelectString(const tchar * lpString, uint nIdx = -1) const
	{
		return (int)SendMessage(CB_SELECTSTRING, nIdx, lpString);
	}

	// select a string in the list of a combo box. If necessary, the list scrolls the string into view. 
	// The text in the edit control of the combo box changes to reflect the new selection, and any previous selection in the list is removed. 
	// @idx: Specifies the zero-based index of the string to select. 
	// If this parameter is –1, any current selection in the list is removed and the edit control is cleared.
	// @return: If the message is successful, the return value is the index of the item selected. 
	// If wParam is greater than the number of items in the list or if wParam is –1, the return value is CB_ERR and the selection is cleared.
	inline int SetCurSel(uint idx) const
	{
		return (int)SendMessage(CB_SETCURSEL, idx);
	}

	// select characters in the edit control of a combo box.
	// @nPosition: The low-order word of lParam specifies the starting position. 
	// If the low-order word is –1, the selection, if any, is removed. 
	// The high-order word of lParam specifies the ending position. 
	// If the high-order word is –1, all text from the starting position to the last character in the edit control is selected.
	// @return: If the message succeeds, the return value is true. 
	// If the message is sent to a combo box with the CBS_DROPDOWNLIST style, it is false.
	inline bool SetEditSel(DWORD nPosition = MAKELONG(0, -1)) const
	{
		return TRUE == SendMessage(CB_SETEDITSEL, 0, nPosition);
	}

	// 获取编辑框上的文本
	inline int GetEditText(tstring & out) const
	{
		return api::GetWindowTextT(out, m_hWnd);
	}

	// 设置编辑框上的文本
	inline bool SetEditText(const tchar * lpString) const
	{
		return FALSE != ::SetWindowText(m_hWnd, lpString);
	}
};

class CComboBoxHook : public CComboBox
{
public:
	CComboBoxHook();
	CComboBoxHook(HWND hWndControl);
	CComboBoxHook(HWND hWndDialog, UINT nCtlID);

	virtual void Attach(HWND hWndControl);

	inline void Attach(HWND hWndDialog, int nCtlID)
	{
		Attach(::GetDlgItem(hWndDialog, nCtlID));
	}

	inline void ReSelectedText() const
	{
		ASSERT(!HasWindowStyle(m_hWndEdit, CBS_DROPDOWNLIST));
		api::SendMessageT(m_hWndEdit, EM_SETSEL, m_LastSelStart, m_LastSelEnd);
	}

	inline void ReplaceSelectedText(const tchar * lpString)
	{
		ReSelectedText();
		api::SendMessageT(m_hWndEdit, EM_REPLACESEL, false, lpString);
		m_LastSelStart = m_LastSelEnd = m_LastSelStart + _tcslen(lpString);
	}

	// 安全起见，窗口销毁时请调用该函数
	inline void RestoreProc() const
	{
		SubclassWindow(m_hWndEdit, m_EditOldProc);
	}

private:
	static BOOL CALLBACK x_EnumChildProc(HWND hWnd, LPARAM lParam);
	static LRESULT CALLBACK x_SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	DWORD m_LastSelStart;
	DWORD m_LastSelEnd;
	WNDPROC m_EditOldProc;
	HWND m_hWndEdit;
};

class CMenu
{
public:
	CMenu() : m_hMenu(NULL) {}
	CMenu(CMenu & rhs) { if (this != &rhs) Attach(rhs.Detach()); }
	explicit CMenu(HMENU hMenu) { Attach(hMenu); }
	explicit CMenu(uint nID) { LoadMenu(nID); }
	CMenu(uint nID, int SubID) { m_hMenu = ::GetSubMenu(::LoadMenu(WpfGetInstance(), MAKEINTRESOURCE(nID)), SubID); }
	virtual ~CMenu() { DestroyMenu(); }
	inline CMenu & operator= (CMenu & rhs)
	{
		if (this != &rhs)
		{
			if (m_hMenu != rhs.m_hMenu)
				DestroyMenu();
			Attach(rhs.Detach());
		}
		return *this;
	}

	inline bool DestroyMenu()
	{
		if (!m_hMenu)
			return false;
		bool ret = ::DestroyMenu(m_hMenu) != FALSE;
		if (ret)
			m_hMenu = NULL;
		return ret;
	}

	inline bool IsMenu() { return ::IsMenu(m_hMenu) != FALSE; }
	inline bool Attach(HMENU hMenu) { m_hMenu = hMenu; }
	inline HMENU Detach() { HMENU tmp = m_hMenu; m_hMenu = NULL; return tmp; }

	inline bool CreateMenu() { m_hMenu = ::CreateMenu(); return NULL != m_hMenu; }
	inline bool CreatePopupMenu() { m_hMenu = ::CreatePopupMenu(); return NULL != m_hMenu; }
	inline bool LoadMenu(int nID)
	{ return (m_hMenu = ::LoadMenu(WpfGetInstance(), MAKEINTRESOURCE(nID))) != NULL; }

	inline HMENU GetSubMenu(int nPos) const { return ::GetSubMenu(m_hMenu, nPos); }

	inline HMENU GetHandler() const { return m_hMenu; }
	inline operator HMENU() const { return m_hMenu; }

	inline bool DeleteMenu(UINT nPosition, UINT nFlags) const
	{ return ::DeleteMenu(m_hMenu, nPosition, nFlags) != FALSE; }

	inline bool RemoveMenu(UINT nPosition, UINT nFlags) const
	{ return ::RemoveMenu(m_hMenu, nPosition, nFlags) != FALSE; }

#if(WINVER >= 0x0500)

	bool SetMenuInfo(LPCMENUINFO lpcmi) const
	{ return ::SetMenuInfo(m_hMenu, lpcmi) != FALSE; }

	inline bool GetMenuInfo(LPMENUINFO lpcmi) const
	{ return ::GetMenuInfo(m_hMenu, lpcmi) != FALSE; }

#endif

	inline bool AppendMenu(UINT nFlags, UINT_PTR nIDNewItem = 0,
					LPCTSTR lpszNewItem = NULL) const
	{ return ::AppendMenu(m_hMenu, nFlags, nIDNewItem, lpszNewItem) != FALSE; }

	inline int GetMenuItemCount() const { return ::GetMenuItemCount(m_hMenu); }
	inline int GetMenuItemID(int nPos) const { return ::GetMenuItemID(m_hMenu, nPos); }
	inline int GetMenuString(tstring & out, UINT nIDItem, UINT nFlags) const
	{
		int len = ::GetMenuString(m_hMenu, nIDItem, NULL, 0, nFlags);
		if (len > 0) {
			out.assign(len + 1, 0);
			len = ::GetMenuString(m_hMenu, nIDItem, (TCHAR *)out.c_str(), len + 1, nFlags);
			out.erase(len);
		}
		else
			out.erase();
	}

	inline bool GetMenuItemInfo(UINT uItem, LPMENUITEMINFO lpMenuItemInfo,
					BOOL fByPos = FALSE) const
	{ return ::GetMenuItemInfo(m_hMenu, uItem, fByPos, lpMenuItemInfo) != FALSE; }

	inline bool SetMenuItemInfo(UINT uItem, LPMENUITEMINFO lpMenuItemInfo,
					BOOL fByPos = FALSE) const
	{ return ::SetMenuItemInfo(m_hMenu, uItem, fByPos, lpMenuItemInfo) != FALSE; }

	inline bool InsertMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem = 0,
					LPCTSTR lpszNewItem = NULL) const
	{ return ::InsertMenu(m_hMenu, nPosition, nFlags, nIDNewItem, lpszNewItem) != FALSE; }

	inline bool ModifyMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem = 0,
					LPCTSTR lpszNewItem = NULL) const
	{ return ::ModifyMenu(m_hMenu, nPosition, nFlags, nIDNewItem, lpszNewItem) != FALSE; }

	inline bool EnableMenuItem(UINT uIDEnableItem, UINT uEnable) const
	{ return ::EnableMenuItem(m_hMenu, uIDEnableItem, uEnable) != FALSE; }

	inline bool EnableMenuItem(UINT uIDEnableItem, bool bEnable) const
	{ return ::EnableMenuItem(m_hMenu, uIDEnableItem, MF_BYCOMMAND | (bEnable ? MF_ENABLED : MF_GRAYED)) != FALSE; }

	inline DWORD CheckMenuItem(UINT uIDCheckItem, UINT uCheck) const
	{ return ::CheckMenuItem(m_hMenu, uIDCheckItem, uCheck); }

	inline DWORD CheckMenuItem(UINT uIDCheckItem, bool bCheck) const
	{ return ::CheckMenuItem(m_hMenu, uIDCheckItem, MF_BYCOMMAND | (bCheck ? MF_CHECKED : MF_UNCHECKED)); }

	inline bool CheckMenuRadioItem(UINT idFirst, UINT idLast, UINT idCheck, UINT uFlags) const
	{ return ::CheckMenuRadioItem(m_hMenu, idFirst, idLast, idCheck, uFlags) != FALSE; }

	inline bool TrackPopupMenu(uint uFlags, int x, int y, HWND hWnd) const
	{ return ::TrackPopupMenu(m_hMenu, uFlags, x, y, 0, hWnd, 0) != FALSE; }

	inline bool TrackPopupMenu(uint uFlags, const POINT & pt, HWND hWnd) const
	{ return ::TrackPopupMenu(m_hMenu, uFlags, pt.x, pt.y, 0, hWnd, 0) != FALSE; }


private:
	HMENU m_hMenu;
};


class CPropertySheet : public CWndBase
{
public:
	CPropertySheet();
	virtual ~CPropertySheet() {}

	INT_PTR DoModal(HWND hWndParent, uint width, uint height, const tchar * Caption, const tchar * ok_bn_title = _T("OK"), const tchar * cancel_bn_title = _T("Cancel"));
	HWND Create(HWND hWndParent, uint width, uint height);
	inline void AddFirstPage(CDialogBase * pDlg) { m_pdlg.clear(); m_pdlg.push_back(pDlg); }
	inline void AddPage(CDialogBase * pDlg) { m_pdlg.push_back(pDlg); }

private:
	static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void OnTabChange();

protected:
	enum {
		BUTTON_WIDTH = 35,
		BUTTON_HEIGHT = 12,
		CONTROL_SPACE = 4,
		TAB_PADDING = 10,

		ID_TAB = 1200,
		ID_EMPTY,
	};

	virtual BOOL OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	// align ptr to nearest DWORD
	inline WORD * AlignDWORD(WORD * ptr) const
	{
		ptr++;									 // round up to nearest DWORD
		LPARAM lp = (LPARAM)ptr;			 // convert to long
		lp &= 0xFFFFFFFC;						 // make sure on DWORD boundary
		return (WORD *)lp;
	}

	inline void SetCurPage(unsigned int nLastPage)
	{
		SendDlgItemMessage(ID_TAB, TCM_SETCURSEL, nLastPage);
		OnTabChange();
	}

	inline unsigned int GetCurPage() const { return (uint)SendDlgItemMessage(ID_TAB, TCM_GETCURSEL); }

protected:
	HWND m_hWndCurPage;

	typedef std::vector<CDialogBase *> PageList;
	PageList m_pdlg;
	//CThemeHelper m_theme;
};





//}; //namespace gui


/////////////////////////////////////////////////////////////////////////////
// CReg

#define HKCR	HKEY_CLASSES_ROOT
#define HKCC	HKEY_CURRENT_CONFIG
#define HKCU	HKEY_CURRENT_USER
#define HKLM	HKEY_LOCAL_MACHINE
#define HKU		HKEY_USERS

class CReg
{
public:
	enum {
		SUCCESS = ERROR_SUCCESS,
	};

public:
    CReg();
    CReg(CReg & key);
    explicit CReg(HKEY hKey);
    ~CReg();

    CReg & operator=( CReg & key );

    // Attributes
public:
    operator HKEY() const;
    HKEY GetKey() const;

private:
    HKEY m_hKey;

    // Operations
public:
    LONG SetValue(DWORD dwValue, LPCTSTR lpszValueName);
    LONG SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL, bool bMulti = false, int nValueLen = -1);
    LONG SetValue(LPCTSTR pszValueName, DWORD dwType, const void* pValue, ULONG nBytes);
    LONG SetBinaryValue(LPCTSTR pszValueName, const void* pValue, ULONG nBytes);
    LONG SetDWORDValue(LPCTSTR pszValueName, DWORD dwValue);
    LONG SetQWORDValue(LPCTSTR pszValueName, ULONGLONG qwValue);
    LONG SetStringValue(LPCTSTR pszValueName, LPCTSTR pszValue);
    LONG SetMultiStringValue(LPCTSTR pszValueName, LPCTSTR pszValue);

    LONG QueryInfoKey(LPTSTR lpClass,
					  LPDWORD lpcClass,
					  LPDWORD lpcSubKeys,
					  LPDWORD lpcMaxSubKeyLen,
					  LPDWORD lpcMaxClassLen,
					  LPDWORD lpcValues,
					  LPDWORD lpcMaxValueNameLen,
					  LPDWORD lpcMaxValueLen,
					  LPDWORD lpcbSecurityDescriptor = NULL,
					  PFILETIME lpftLastWriteTime = NULL);
    LONG QueryValue(DWORD& dwValue, LPCTSTR lpszValueName);
    LONG QueryValue(LPTSTR szValue, LPCTSTR lpszValueName, DWORD* pdwCount);
    LONG QueryValue(LPCTSTR pszValueName, DWORD* pdwType, void* pData, ULONG* pnBytes);
    LONG QueryBinaryValue(LPCTSTR pszValueName, void * pValue, DWORD * pnBytes);
	bool QueryBinaryValue(const tchar * name, std::vector<byte> & out);
	template<typename T>
    LONG QueryDWORDValue(LPCTSTR pszValueName, T & out);
    DWORD QueryDWORDValue(LPCTSTR pszValueName);
    static DWORD WINAPI QueryDWORDValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR pszValueName);
    LONG QueryQWORDValue(LPCTSTR pszValueName, ULONGLONG & qwValue);
    LONG QueryStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars = 0);
    LONG QueryMultiStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars);

	bool QueryStringValue(LPCTSTR pszValueName, tstring & out);
    // Get the key's security attributes.
    LONG GetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd, LPDWORD pnBytes);
    // Set the key's security attributes.
    LONG SetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd);

    LONG SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);
    static LONG WINAPI SetStringValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR lpszValueName, LPCTSTR lpszValue);
    static LONG WINAPI SetDWORDValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR lpszValueName, DWORD dwValue);

    // Create a new registry key (or open an existing one).
    LONG Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
                LPTSTR lpszClass = REG_NONE, DWORD dwOptions = REG_OPTION_NON_VOLATILE,
                REGSAM samDesired = KEY_READ | KEY_WRITE,
                LPSECURITY_ATTRIBUTES lpSecAttr = NULL,
                LPDWORD lpdwDisposition = NULL);
    // Open an existing registry key.
    LONG Open(HKEY hKeyParent, LPCTSTR lpszKeyName,
              REGSAM samDesired = KEY_READ);
    // Close the registry key.
    LONG Close();
    // Flush the key's data to disk.
    LONG Flush();

    // Detach the CReg object from its HKEY.  Releases ownership.
    HKEY Detach();
    // Attach the CReg object to an existing HKEY.  Takes ownership.
    void Attach(HKEY hKey);

    // Enumerate the subkeys of the key.
    LONG EnumKey(DWORD iIndex, LPTSTR pszName, LPDWORD pnNameLength, FILETIME* pftLastWriteTime = NULL);
	static bool WINAPI CReg::EnumKey(HKEY hKeyRoot, LPCTSTR lpszKeyName, DWORD dwIndex, LPTSTR pszName, LPDWORD pnNameLength, FILETIME * pftLastWriteTime = NULL);
    static bool WINAPI EnumValue(HKEY hKeyRoot,
								LPCTSTR lpszKeyName,
								DWORD dwIndex,
								LPTSTR lpValueName,
								LPDWORD lpcValueName,
								LPBYTE lpData = NULL,
								LPDWORD lpcbData = NULL,
								LPDWORD lpType = NULL);
    LONG NotifyChangeKeyValue(BOOL bWatchSubtree, DWORD dwNotifyFilter, HANDLE hEvent, BOOL bAsync = TRUE);

    LONG DeleteSubKey(LPCTSTR lpszSubKey);
    LONG RecurseDeleteKey(LPCTSTR lpszKey);
	static LONG WINAPI DeleteKey(HKEY hKey, LPCTSTR lpszKey);
    LONG DeleteValue(LPCTSTR lpszValue);
};

inline CReg::CReg() :
    m_hKey( NULL )
{}

inline CReg::CReg( CReg& key ) :
        m_hKey( NULL )
{
    Attach( key.Detach() );
}

inline CReg::CReg(HKEY hKey) :
        m_hKey(hKey)
{}

inline CReg::~CReg()
{
    Close();
}

inline CReg& CReg::operator=( CReg& key )
{
    if(m_hKey!=key.m_hKey)
    {
        Close();
        Attach( key.Detach() );
    }
    return( *this );
}

inline CReg::operator HKEY() const
{
    return m_hKey;
}

inline HKEY CReg::GetKey() const
{
    return m_hKey;
}

inline HKEY CReg::Detach()
{
    HKEY hKey = m_hKey;
    m_hKey = NULL;
    return hKey;
}

inline void CReg::Attach(HKEY hKey)
{
    m_hKey = hKey;
}

inline LONG CReg::DeleteSubKey(LPCTSTR lpszSubKey)
{
    return RegDeleteKey(m_hKey, lpszSubKey);
}

inline LONG CReg::DeleteValue(LPCTSTR lpszValue)
{
    return RegDeleteValue(m_hKey, (LPTSTR)lpszValue);
}

inline LONG CReg::Close()
{
    LONG lRes = ERROR_SUCCESS;
    if (m_hKey != NULL)
    {
        lRes = RegCloseKey(m_hKey);
        m_hKey = NULL;
    }
    return lRes;
}

inline LONG CReg::Flush()
{
    return ::RegFlushKey(m_hKey);
}

inline LONG CReg::EnumKey(DWORD iIndex, LPTSTR pszName, LPDWORD pnNameLength, FILETIME* pftLastWriteTime)
{
    FILETIME ftLastWriteTime;

    if (pftLastWriteTime == NULL)
    {
        pftLastWriteTime = &ftLastWriteTime;
    }

    return ::RegEnumKeyEx(m_hKey, iIndex, pszName, pnNameLength, NULL, NULL, NULL, pftLastWriteTime);
}

inline bool WINAPI CReg::EnumKey(HKEY hKeyRoot, LPCTSTR lpszKeyName, DWORD dwIndex, LPTSTR pszName, LPDWORD pnNameLength, FILETIME * pftLastWriteTime)
{
	HKEY hKey;
	LONG lRes = ERROR_SUCCESS;

	lRes = ::RegOpenKeyEx(hKeyRoot, lpszKeyName, 0, KEY_READ, &hKey);
	if (ERROR_SUCCESS != lRes) return false;

    FILETIME ftLastWriteTime;

    if (pftLastWriteTime == NULL)
    {
        pftLastWriteTime = &ftLastWriteTime;
    }
	lRes = ::RegEnumKeyEx(hKey, dwIndex, pszName, pnNameLength, NULL, NULL, NULL, pftLastWriteTime);
	if (ERROR_SUCCESS != lRes)
	{
		RegCloseKey(hKey);
		return false;
	}

	RegCloseKey(hKey);
	return true;
}

inline bool WINAPI CReg::EnumValue(HKEY hKeyRoot, LPCTSTR lpszKeyName, DWORD dwIndex, LPTSTR lpValueName, LPDWORD lpcValueName, LPBYTE lpData, LPDWORD lpcbData, LPDWORD lpType)
{
	HKEY hKey;
	LONG lRes = ERROR_SUCCESS;

	lRes = ::RegOpenKeyEx(hKeyRoot, lpszKeyName, 0, KEY_READ, &hKey);
	if (ERROR_SUCCESS != lRes) return false;

	lRes = ::RegEnumValue(hKey, dwIndex, lpValueName, lpcValueName, NULL, lpType, lpData, lpcbData);
	if (ERROR_SUCCESS != lRes)
	{
		RegCloseKey(hKey);
		return false;
	}

	RegCloseKey(hKey);
	return true;
}

inline LONG CReg::NotifyChangeKeyValue(BOOL bWatchSubtree, DWORD dwNotifyFilter, HANDLE hEvent, BOOL bAsync)
{
    return ::RegNotifyChangeKeyValue(m_hKey, bWatchSubtree, dwNotifyFilter, hEvent, bAsync);
}

inline LONG CReg::Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
                         LPTSTR lpszClass, DWORD dwOptions, REGSAM samDesired,
                         LPSECURITY_ATTRIBUTES lpSecAttr, LPDWORD lpdwDisposition)
{
    DWORD dw;
    HKEY hKey = NULL;
    LONG lRes = RegCreateKeyEx(hKeyParent, lpszKeyName, 0,
                               lpszClass, dwOptions, samDesired, lpSecAttr, &hKey, &dw);
    if (lpdwDisposition != NULL)
        *lpdwDisposition = dw;
    if (lRes == ERROR_SUCCESS)
    {
        lRes = Close();
        m_hKey = hKey;
    }
    return lRes;
}

inline LONG CReg::Open(HKEY hKeyParent, LPCTSTR lpszKeyName, REGSAM samDesired)
{
    HKEY hKey = NULL;
    LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, samDesired, &hKey);
    if (lRes == ERROR_SUCCESS)
    {
        lRes = Close();
        m_hKey = hKey;
    }
    return lRes;
}

inline LONG CReg::QueryInfoKey(LPTSTR lpClass,
							  LPDWORD lpcClass,
							  LPDWORD lpcSubKeys,
							  LPDWORD lpcMaxSubKeyLen,
							  LPDWORD lpcMaxClassLen,
							  LPDWORD lpcValues,
							  LPDWORD lpcMaxValueNameLen,
							  LPDWORD lpcMaxValueLen,
							  LPDWORD lpcbSecurityDescriptor,
							  PFILETIME lpftLastWriteTime)
{
	return ::RegQueryInfoKey(m_hKey, lpClass, lpcClass, NULL, lpcSubKeys, lpcMaxSubKeyLen, lpcMaxClassLen, lpcValues, lpcMaxValueNameLen, lpcMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime);
}

inline LONG CReg::QueryValue(DWORD& dwValue, LPCTSTR lpszValueName)
{
    DWORD dwType = NULL;
    DWORD dwCount = sizeof(DWORD);
    LONG lRes = RegQueryValueEx(m_hKey, lpszValueName, NULL, &dwType,
                                (LPBYTE)&dwValue, &dwCount);
    if (dwType != REG_DWORD)
        return ERROR_INVALID_DATA;
    return lRes;
}

inline LONG CReg::QueryValue(LPCTSTR pszValueName, DWORD* pdwType, void* pData, ULONG* pnBytes)
{
    return( ::RegQueryValueEx(m_hKey, pszValueName, NULL, pdwType, static_cast< LPBYTE >( pData ), pnBytes) );
}

template<typename T>
inline LONG CReg::QueryDWORDValue(LPCTSTR pszValueName, T & out)
{
    LONG lRes;
    ULONG nBytes;
    DWORD dwType;
	DWORD dwValue;

    nBytes = sizeof(DWORD);
    lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(&dwValue),
                             &nBytes);
    if (lRes != ERROR_SUCCESS)
        return lRes;
    if (dwType != REG_DWORD)
        return ERROR_INVALID_DATA;

	util::assign_cast(out, dwValue);
    return ERROR_SUCCESS;
}

inline DWORD CReg::QueryDWORDValue(LPCTSTR pszValueName)
{
    LONG lRes;
    ULONG nBytes;
    DWORD dwType;
	DWORD dwValue = 0;

    nBytes = sizeof(DWORD);
    lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(&dwValue),
                             &nBytes);
    return dwValue;
}

inline DWORD WINAPI CReg::QueryDWORDValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR pszValueName)
{
	CReg reg;
	if (ERROR_SUCCESS == reg.Open(hKeyParent, lpszKeyName))
		return reg.QueryDWORDValue(pszValueName);
	return 0;
}

inline LONG CReg::QueryQWORDValue(LPCTSTR pszValueName, ULONGLONG& qwValue)
{
    LONG lRes;
    ULONG nBytes;
    DWORD dwType;

    nBytes = sizeof(ULONGLONG);
    lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(&qwValue),
                             &nBytes);
    if (lRes != ERROR_SUCCESS)
        return lRes;
    if (dwType != REG_QWORD)
        return ERROR_INVALID_DATA;

    return ERROR_SUCCESS;
}

inline LONG CReg::QueryBinaryValue(LPCTSTR pszValueName, void * pValue, DWORD * pnBytes)
{
    LONG lRes;
    DWORD dwType;

    lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(pValue),
                             pnBytes);
    if (lRes != ERROR_SUCCESS)
        return lRes;
    if (dwType != REG_BINARY)
        return ERROR_INVALID_DATA;

    return ERROR_SUCCESS;
}

inline bool CReg::QueryBinaryValue(const tchar * name, std::vector<byte> & out)
{
	DWORD size = 0;
	LONG lRes = QueryBinaryValue(name, NULL, &size);
	if (ERROR_SUCCESS != lRes || 0 == size)
	{
		return false;
	}

	out.resize(size);
    return ERROR_SUCCESS == QueryBinaryValue(name, &out[0], &size);
}

inline LONG WINAPI CReg::SetStringValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR lpszValueName, LPCTSTR lpszValue)
{
    CReg key;
    LONG lRes = key.Create(hKeyParent, lpszKeyName);
    if (lRes == ERROR_SUCCESS)
        lRes = key.SetStringValue(lpszValueName, lpszValue);
    return lRes;
}

inline LONG WINAPI CReg::SetDWORDValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR lpszValueName, DWORD dwValue)
{
    CReg key;
    LONG lRes = key.Create(hKeyParent, lpszKeyName);
    if (lRes == ERROR_SUCCESS)
		lRes = key.SetDWORDValue(lpszValueName, dwValue);
    return lRes;
}

inline LONG CReg::SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
    CReg key;
    LONG lRes = key.Create(m_hKey, lpszKeyName);
    if (lRes == ERROR_SUCCESS)
        lRes = key.SetStringValue(lpszValueName, lpszValue);
    return lRes;
}

inline LONG CReg::SetValue(DWORD dwValue, LPCTSTR pszValueName)
{
    return SetDWORDValue(pszValueName, dwValue);
}

inline LONG CReg::SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName, bool bMulti, int nValueLen)
{
    if (bMulti && nValueLen == -1)
        return ERROR_INVALID_PARAMETER;

    if (nValueLen == -1)
        nValueLen = _tcslen(lpszValue) + 1;

    DWORD dwType = bMulti ? REG_MULTI_SZ : REG_SZ;

    return ::RegSetValueEx(m_hKey, lpszValueName, NULL, dwType,
                           reinterpret_cast<const BYTE*>(lpszValue), nValueLen*sizeof(TCHAR));
}

inline LONG CReg::SetValue(LPCTSTR pszValueName, DWORD dwType, const void* pValue, ULONG nBytes)
{
    return ::RegSetValueEx(m_hKey, pszValueName, NULL, dwType, static_cast<const BYTE*>(pValue), nBytes);
}

inline LONG CReg::SetBinaryValue(LPCTSTR pszValueName, const void* pData, ULONG nBytes)
{
    return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_BINARY, reinterpret_cast<const BYTE*>(pData), nBytes);
}

inline LONG CReg::SetDWORDValue(LPCTSTR pszValueName, DWORD dwValue)
{
    return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_DWORD, reinterpret_cast<const BYTE*>(&dwValue), sizeof(DWORD));
}

inline LONG CReg::SetQWORDValue(LPCTSTR pszValueName, ULONGLONG qwValue)
{
    return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_QWORD, reinterpret_cast<const BYTE*>(&qwValue), sizeof(ULONGLONG));
}

inline LONG CReg::SetStringValue(LPCTSTR pszValueName, LPCTSTR pszValue)
{
	return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_SZ, reinterpret_cast<const BYTE*>(pszValue), (_tcslen(pszValue)+1)*sizeof(TCHAR));
}

inline LONG CReg::GetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd, LPDWORD pnBytes)
{
    return ::RegGetKeySecurity(m_hKey, si, psd, pnBytes);
}

inline LONG CReg::SetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd)
{
    return ::RegSetKeySecurity(m_hKey, si, psd);
}

inline LONG WINAPI CReg::DeleteKey(HKEY hKey, LPCTSTR lpszKey)
{
	CReg reg(hKey);
	return reg.RecurseDeleteKey(lpszKey);
}


class CCriticalSection
{
	NONCOPYABLE(CCriticalSection);

public:
	CCriticalSection() { InitializeCriticalSection(&xm_sec); }
	virtual ~CCriticalSection() { DeleteCriticalSection(&xm_sec); }

public:
	inline void Enter() { EnterCriticalSection(&xm_sec); }
	inline void Leave() { LeaveCriticalSection(&xm_sec); }

private:
	CRITICAL_SECTION xm_sec;
};

class CPerformanceCounter
{
	NONCOPYABLE(CPerformanceCounter);

public:
	CPerformanceCounter()
	{
		LARGE_INTEGER timer_freq;
		if (QueryPerformanceFrequency(&timer_freq))
			xm_timer_freq_div = 1.0 / (double)timer_freq.QuadPart;
		else
			xm_timer_freq_div = 0.0;
	}

	inline void Start()
	{
		VERIFY(QueryPerformanceCounter(&xm_t));
	}

	inline double GetCounter() const
	{
		double time = 0.0;
		LARGE_INTEGER end;
		VERIFY(QueryPerformanceCounter(&end));
		return (double)((end.QuadPart - xm_t.QuadPart) * xm_timer_freq_div * 1000);
	}

private:
	double xm_timer_freq_div;
	LARGE_INTEGER xm_t;
};

class MostRecentlyUsed
{
// Constructors
public:
	MostRecentlyUsed(unsigned int nMaxItem, HKEY hKey, LPCTSTR lpSubKey, bool bReadListImmediately = false);

// Attributes
	inline unsigned int Size() const
			{ return m_Data.size(); }
	inline tstring EnumString(unsigned int nIndex) const
			{ assert(nIndex < m_Data.size()); return m_Data[nIndex]; }
	inline const TCHAR * operator[](unsigned int nIndex) const
			{ assert(nIndex < m_Data.size()); return m_Data[nIndex].c_str(); }

// Operations
	virtual void Remove(unsigned int nIndex);
	virtual void Add(LPCTSTR lpszPathName);
	virtual bool ReadList();    // reads from registry or ini file
	virtual bool WriteList();   // writes to registry or ini file

// Implementation
	virtual ~MostRecentlyUsed();

private:
	void DeleteDuplicate(LPCTSTR lpszPathName);

private:
// contents of the MRU list
	typedef vector<tstring> data_type;
	unsigned int m_nMaxItem;
	HKEY m_hKey;
	tstring m_SubKey;
	data_type m_Data;
};

typedef MostRecentlyUsed CMostRecentlyUsed;


class Reader
{
public:
	typedef __int64 fpos_t;
	typedef unsigned __int64 ufpos_t;
	typedef HANDLE handle_type;
	enum CONST_VALUE {
		CONST_INVALID_FILE_POINTER	= -1,
		CONST_INVALID_FILE_SIZE		= -1,
	};

	enum POSITION {
		POS_BEGIN			= FILE_BEGIN,
		POS_CURRENT			= FILE_CURRENT,
		POS_END				= FILE_END,
	};

	enum ACCESS {
		OPEN_READ				= GENERIC_READ,
		OPEN_WRITE				= GENERIC_WRITE,
		OPEN_READ_WRITE			= GENERIC_WRITE|GENERIC_READ,
	};

	enum SHARE {
		SHARE_NONE,
		SHARE_DELETE		= FILE_SHARE_DELETE,
		SHARE_READ			= FILE_SHARE_READ,
		SHARE_WRITE			= FILE_SHARE_WRITE,
	};


public:
	Reader() : hFile(NULL) {}
	Reader(const Reader & rhs) { if(&rhs != this) hFile = rhs.hFile; }
	inline Reader & operator=(const Reader & rhs) { if(&rhs != this) hFile = rhs.hFile; return *this; }

	virtual ~Reader()
	{
		close();
	}

	inline handle_type detach() { handle_type tmp = hFile; hFile = NULL; return tmp; }
	inline void attach(const handle_type & data) { if(hFile) CloseHandle(hFile); hFile = data; }

	bool open(const TCHAR * filename, Reader::ACCESS DesiredAccess, Reader::SHARE ShareMode = SHARE_READ);
	inline bool is_open() const { return (NULL != hFile) && (INVALID_HANDLE_VALUE != hFile); }
	inline void close() { if(hFile) CloseHandle(hFile); hFile = NULL; }

	inline unsigned long read(void * buffer, unsigned long count) const
	{
		unsigned long NumberOfBytesRead = 0;
		ReadFile(hFile, buffer, count, &NumberOfBytesRead, NULL);
		return NumberOfBytesRead;
	}

	inline fpos_t seek(fpos_t offset, unsigned long method = FILE_BEGIN) const //FILE_BEGIN FILE_CURRENT FILE_END
	{
		long high = static_cast<long>(offset>>32);
		unsigned long low = SetFilePointer(hFile, static_cast<long>(offset), &high, method);
		if(INVALID_SET_FILE_POINTER == low) return -1;
		return (static_cast<fpos_t>(high)<<32) | static_cast<fpos_t>(low);
	}

	inline unsigned long seek32(long offset, unsigned long method = FILE_BEGIN) const
	{
		return SetFilePointer(hFile, offset, NULL, method);
	}

	inline ufpos_t get_pos() const { return seek(0, FILE_CURRENT); }

	inline unsigned long write(const void * buffer, unsigned long nbytes) const
	{
		unsigned long NumberOfBytesWritten = 0;
		WriteFile(hFile, buffer, nbytes, &NumberOfBytesWritten, NULL);
		return NumberOfBytesWritten;
	}

	inline ufpos_t get_size() const
	{
		unsigned long high;
		unsigned long low = GetFileSize(hFile, &high);
		if(INVALID_FILE_SIZE == low) return -1;
		return (static_cast<ufpos_t>(high)<<32) | static_cast<ufpos_t>(low);
	}

	inline unsigned long get_size32() const
	{
		return GetFileSize(hFile, NULL);
	}

	inline void set_eof() const { SetEndOfFile(hFile); }
	bool move(long offset) const;

	inline bool get_filetime(__out_opt LPFILETIME lpCreationTime, __out_opt LPFILETIME lpLastAccessTime, __out_opt LPFILETIME lpLastWriteTime)
	{
		return FALSE != ::GetFileTime(hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
	}

private:
	handle_type hFile;

};

typedef Reader CReader;


class CDlgResizer
{
public:
	struct stParam
	{
		unsigned short id;
		unsigned short flags;
	};

	enum {
		X_MOVE = 1, X_SIZE = 2, Y_MOVE = 4, Y_SIZE = 8,
		XY_MOVE = X_MOVE|Y_MOVE, XY_SIZE = X_SIZE|Y_SIZE,
		X_MOVE_Y_SIZE = X_MOVE|Y_SIZE, X_SIZE_Y_MOVE = X_SIZE|Y_MOVE,
	};

	explicit CDlgResizer(unsigned p_min_x, unsigned p_min_y, unsigned p_max_x, unsigned p_max_y);

public:
	inline CDlgResizer & operator()(unsigned short id, unsigned short flags)
	{
		return Add(id, flags);
	}

	inline CDlgResizer & Add(unsigned short id, unsigned short flags)
	{
		stParam st = { id, flags };
		m_table.push_back(st);
		RECT r = { 0 };
		m_rects.push_back(r);
		return *this;
	}

	inline void SetMinSize(unsigned x, unsigned y) { min_x = x; min_y = y; }
	inline void SetMaxSize(unsigned x, unsigned y) { max_x = x; max_y = y; }

	// 在窗口过程调用，如果返回 true 则表示消息已被 CDlgResizer 对象处理了，可直接返回
	bool IsProcess(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

private:
	std::vector<stParam> m_table;
	std::vector<RECT> m_rects;
	RECT orig_client;
	HWND parent;
	unsigned min_x, min_y, max_x, max_y;

	void SetParent(HWND wnd);
	void Reset();
	void OnSize();
};



} //namespace wpf








#endif