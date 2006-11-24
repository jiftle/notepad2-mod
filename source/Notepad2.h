#ifndef NOTEPAD2_H__
#define NOTEPAD2_H__

#include "resource.h"
#include "Common\stdafx.h"

#include <commctrl.h>
#include <commdlg.h>

#include <platform.h>
#include <scintilla.h>
#include <scilexer.h>
#include <astyle_interface.h>

#include "markupstl.h"

#define APP_NAME_ANSI			"Notepad2"
#define APP_NAME				_T(APP_NAME_ANSI)
#define APP_INTERNAL_VERSION	1006
#define APP_VERSION_STRING		_T("1.1.1.0")

#define APP_REG_ROOT			HKEY_CURRENT_USER
#define APP_REG_KEY				_T("Software\\Notepad2")
#define APP_REG_KEY_NODE(x)		APP_REG_KEY _T("\\") _T(x)

#define MAX_RECENT_FILE			10
#define RECENT_FILE_START		1200	//1200 ~ 1220
#define FORMAT_SCHEME_START		(RECENT_FILE_START + MAX_RECENT_FILE + 10)	//1220 ~ 1299
#define CODEPAGE_START			1300		// 1300 ~ 1499
#define OPENWITH_START			1500		// 1500 ~ 1599
#define OPENWITH_END			1599


#define DEFAULT_FONTSIZE	10


//==== Data Type for WM_COPYDATA ==============================================
#define DATA_NOTEPAD2_FILEARG 0xDE70


//==== Toolbar Style ==========================================================
#define WS_TOOLBAR (WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | \
                    TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | TBSTYLE_ALTDRAG | \
                    TBSTYLE_LIST | CCS_NODIVIDER | CCS_NOPARENTALIGN | \
                    CCS_ADJUSTABLE)


//==== ReBar Style ============================================================
#define WS_REBAR (WS_CHILD | WS_CLIPCHILDREN | WS_BORDER | RBS_VARHEIGHT | \
                  RBS_BANDBORDERS | CCS_NODIVIDER | CCS_NOPARENTALIGN)


//==== Ids ====================================================================
#define IDC_STATUSBAR    0xFB00
#define IDC_TOOLBAR      0xFB01
#define IDC_REBAR        0xFB02
#define IDC_EDIT         0xFB03
#define IDC_EDITFRAME    0xFB04



//==== Callback Message from System Tray ======================================
#define WM_TRAYMESSAGE WM_USER


#ifdef _SEH

#ifdef _DEBUG
#define __TRY ((void)0);
#define __CATCH(path) ((void)0);
#else
#define __TRY LPEXCEPTION_POINTERS pExPtr = 0; __try {
#define __CATCH(path) } __except(ExceptionFilter(pExPtr = GetExceptionInformation()))\
					{ ExceptionHandler(pExPtr, path); }
#endif

void ExceptionHandler(LPEXCEPTION_POINTERS p, const char * path);
int ExceptionFilter(LPEXCEPTION_POINTERS param);

#else
#define __TRY ((void)0);
#define __CATCH(path) ((void)0);
#endif //_SEH



enum ICON_ID {
	II_NONE = -1,
	II_MAIN = 0,
	II_ASP  = 4,
	II_C,
	II_CS,
	II_CFG,
	II_CPP,
	II_H,
	II_INI,
	II_JS,
	II_NSIS,
	II_RC,
	II_SQL,
	II_TXT,
	II_VB,
	II_CUE,
	II_MAK,
	II_PY,
	II_CSS,
};

enum SUBMENU {
	SUBM_FILE,
	SUBM_EDIT,
	SUBM_VIEW,
	SUBM_SETTING,
	SUBM_TOOL,
	SUBM_HELP,
};

enum ENCODING {
	ENC_NONE = -1,
	ENC_ANSI,
	ENC_UNICODE_LE,
	ENC_UNICODE = ENC_UNICODE_LE,
	ENC_UNICODE_BE,
	ENC_UTF8,
};

struct WININFO
{
    int x;
    int y;
    int cx;
    int cy;
    int max;
};

struct STYLEPAIR
{
	TCHAR Char;
	TBYTE StyleID;
};

#ifdef  _MSC_VER
#pragma pack(push,1)
#endif
struct STYLE
{
	DWORD cbStructSize;
	TCHAR FontName[LF_FACESIZE];
	DWORD iFontSize; //32 bit for + or -
	BYTE fCharSet;
	COLORREF rForeColor; //24~32 for detect color is set
	COLORREF rBackColor; //24~32 for detect color is set
	bool fBold;
	bool fItalic; 
	bool fUnderline; 
	bool sEOLFilled;
	bool sCaseSensitive;
};
#ifdef  _MSC_VER
#pragma pack(pop)
#endif

#ifdef  _MSC_VER
#pragma warning(disable:4200)
#endif
struct EDITSTYLE
{
    int   iStyle;
	STYLE Value;
};

struct KEYWORDLIST
{
    TCHAR * pszKeyWords[9];
};

struct EDITLEXER
{
    int   iLexer;
	UINT NameID;
    const TCHAR * pszDefExt;
    TCHAR  szExtensions[128];
	int IconID;
    KEYWORDLIST * pKeyWords;
#ifdef _MSC_VER
    EDITSTYLE    Styles[];
#else
    EDITSTYLE    Styles[128];
#endif

};

#ifdef  _MSC_VER
#pragma pack(push,1)
#endif
typedef struct FORMATDATA
{
	unsigned short nStructSize; //len of description in WCHAR
	BYTE Style; // c or java
	unsigned short Indentation; //HIBYTE > 0 == tab, else == space, LOBYTE is number
	bool ClassIndent;
	bool SwitchIndent;
	bool CaseIndent;
	bool BracketIndent;
	bool BlockIndent;
	bool NamespaceIndent;
	bool LabelIndent;
	bool PreprocessorIndent;
	bool TabSpaceConversionMode;
	bool EmptyLineFill;
	unsigned short MaxInStatementIndentLength;
	unsigned short MinConditionalIndentLength;
	unsigned short BracketFormatMode;
	bool BreakBlocksMode;
	bool BreakElseIfsMode;
	bool OperatorPaddingMode;
	bool ParenthesisPaddingMode;
	bool SingleStatementsMode;
	bool BreakOneLineBlocksMode;

	DWORD Reserved[10];
} FMTDATA;
#ifdef  _MSC_VER
#pragma pack(pop)
#endif

struct FMTPARAM
{
	tstring name;
	FORMATDATA data;
	tstring description;
	inline bool operator==(const FMTPARAM & rhs) const { return name == rhs.name; }
};

struct STRPAIR
{
	tstring first;
	tstring second;
	inline bool operator< (const STRPAIR & rhs) const { return _tcsicmp(first.c_str(), rhs.first.c_str()) < 0; }
	inline bool operator== (const STRPAIR & rhs) const { return !_tcscmp(first.c_str(), rhs.first.c_str()); }
};

struct VERSION
{
	WORD Ver1;
	WORD Ver2;
	WORD Ver3;
	WORD Ver4;
};



#include "edit.h"
#include "Styles.h"


class WindowPos
{
public:
	WindowPos(const TCHAR * pname)
	{
		subkey = APP_REG_KEY;
		subkey += _T("\\Window");
		name = pname;
		memset(&rc, 0, sizeof(RECT));
	}

	~WindowPos()
	{
		CReg reg;
		if (CReg::SUCCESS == reg.Create(APP_REG_ROOT, subkey.c_str()))
		{
			reg.SetBinaryValue(name.c_str(), &rc, sizeof(RECT));
		}
	}

	inline bool SetPos(HWND hwnd, bool bRememberSize = false)
	{
		UINT flag = SWP_NOACTIVATE | SWP_NOZORDER;
		if (!bRememberSize)
			flag |= SWP_NOSIZE;

		if (rc.left > 0 && rc.top > 0)
		{
			SetWindowPos(hwnd, NULL, rc.left, rc.top, bRememberSize ? rc.right-rc.left : 0, 
				bRememberSize ? rc.bottom-rc.top : 0, flag);
			return true;
		}

		CReg reg;
		if (CReg::SUCCESS == reg.Open(APP_REG_ROOT, subkey.c_str()))
		{
			DWORD len = 0;
			if (CReg::SUCCESS == reg.QueryBinaryValue(name.c_str(), NULL, &len)
				&& len == sizeof(RECT))
			{
				reg.QueryBinaryValue(name.c_str(), &rc, &len);
				if (rc.right <= rc.left || rc.bottom <= rc.top)
					return false;
				SetWindowPos(hwnd, NULL, rc.left, rc.top, bRememberSize ? rc.right-rc.left : 0, 
					bRememberSize ? rc.bottom-rc.top : 0, flag);
				return true;
			}
		}
		return false;
	}

	inline void GetPos(HWND hwnd)
	{
		GetWindowRect(hwnd, &rc);
	}

private:
	RECT rc;
	tstring subkey;
	tstring name;
};


class CFileInformation
{
public:
	CFileInformation()
	: enc(ENC_ANSI)
	, eol(SC_EOL_CRLF)
	, readonly(false)
	{}

	CFileInformation(const CFileInformation & rhs) { _copy(rhs); }

public:
	inline const tchar * c_str() const { return m_path.c_str(); }
	inline const tstring & str() const { return m_path; }
	inline CFileInformation & operator=(const CFileInformation & rhs) { return _copy(rhs); }

	// _verify: 验证目标文件是否有效，如果无效则不接收文件
	bool attach(const tchar * filepath, bool _verify = false)
	{
		bool ret = update(filepath);
		if ((!_verify || ret) && filepath != m_path.c_str())
			m_path = filepath;

		return ret;
	}

	inline bool attach(const tstring & filepath, bool _verify = false)
	{
		return attach(filepath.c_str(), _verify);
	}

	inline bool operator== (const CFileInformation & rhs) const
	{
		return !_tcsicmp(m_path.c_str(), rhs.m_path.c_str()) &&
			!CompareFileTime(&m_LastWriteTime, &rhs.m_LastWriteTime);
	}

	inline bool operator!= (const CFileInformation & rhs) const { return !(*this == rhs); }

	bool update(const tchar * filepath = NULL)
	{
		Reader reader;
		if (!filepath)
			filepath = m_path.c_str();
		if (filepath[0] && reader.open(filepath, Reader::OPEN_READ))
		{
			DWORD dwFileAttributes = GetFileAttributes(filepath);
			if (dwFileAttributes != INVALID_FILE_ATTRIBUTES)
				readonly = dwFileAttributes & FILE_ATTRIBUTE_READONLY;

			return reader.get_filetime(NULL, NULL, &m_LastWriteTime);
		}
		
		return false;
	}

	CFileInformation & get_fullpath()
	{
		if (FileExists(m_path.c_str()))
		{
			api::GetLongPathNameT(m_path, m_path.c_str());
			api::GetFullPathNameT(m_path);

			if (!_tcsicmp(util::rget_after(m_path, '.').c_str(), _T("lnk")))
			{
				com::GetLinkPath(m_path, m_path.c_str());
			}
		}
		return *this;
	}

	inline void clear()
	{
		m_path.erase();
		memset(&m_LastWriteTime, 0, sizeof(FILETIME));
	}

	inline bool bad() const { return m_path.empty() || !FileExists(m_path.c_str()); }
	inline bool empty() const { return m_path.empty(); }
	inline uint length() const { return m_path.length(); }

private:
	CFileInformation & _copy(const CFileInformation & rhs)
	{
		if (this != &rhs) {
			m_path = rhs.m_path;
			m_LastWriteTime = rhs.m_LastWriteTime;
			enc = rhs.enc;
			eol = rhs.eol;
			readonly = rhs.readonly;
		}
		return *this;
	}

public:
	int enc;
	int eol;
	bool readonly;

private:
	tstring m_path;
	FILETIME m_LastWriteTime;
};


class CView : public wpf::CWndBase
{
public:
	CView();
	~CView();

	int MessageBox(const tchar * lpText,
		UINT uType = MB_OK|MB_ICONINFORMATION,
		const tchar * lpCaption = ResStr(IDS_APPTITLE)) const;

	bool Create(int nCmdShow);
	void ApplyView();
	void UpdateToolbar();
	void UpdateStatusbar();
	void UpdateLineNumberWidth();
	void SetTitle(bool IsShort, bool IsModified, bool IsReadOnly);
	void SetNotifyIconTitle();
	void ShowNotifyIcon(bool bAdd);
	inline LRESULT SendWMSize();

	bool LoadFile(bool bDontSave, bool bNew, int Enc, const TCHAR * lpszFile);
	bool SaveFile(bool bSaveAlways, bool bAsk, bool bSaveAs, bool bSaveCopy);
	void PreFileIO(const tchar * status);
	void FinishFileIO(const tchar * file);
	inline void SetNewFileState();

	template<typename _Bool>
	void EnableCmd(HMENU hMenu, UINT uID, _Bool bEnable);
	template<typename _Bool>
	void CheckCmd(HMENU hMenu, UINT uID, _Bool bEnable);

	void SetOnTop();
	void SetTransparent();

private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL EnumCodePagesProc(LPTSTR lpCodePageString);
	static unsigned int _stdcall ThreadEnumCodepage(void * param);
	LRESULT OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnCreate(WPARAM wParam, LPARAM lParam);
	void OnCharCount();
	LRESULT OnCodepage(WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
	LRESULT OnCompileNSIS(WPARAM wParam, LPARAM lParam);
	LRESULT OnContextMenu(WPARAM wParam, LPARAM lParam);
	void OnInitMenu(WPARAM wParam, LPARAM lParam, bool bContextMenu);
	LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
	LRESULT OnOpenWith(WPARAM wParam, LPARAM lParam);
	void OnSize(WPARAM wParam, LPARAM lParam);
	//Handle WM_THEMECHANGED
	void OnThemeChanged(WPARAM wParam, LPARAM lParam);

	void DoToolBarCmd(int MenuID);

private:
	LONG m_bEnumCodepageThreadFinished;

	wpf::CMenu m_menu_mru;
	wpf::CMenu m_menu_codepage;
	wpf::CMenu m_menu_scheme;

public:
	CDocment m_doc;
	MostRecentlyUsed m_mru;

	CDocmentFrame m_docframe;
	CToolbar m_toolbar;
	CStatusBar m_status;
	CSizeBox m_sizebox;
};

class CAppModule
{
public:
	CAppModule();
	void Init(LPTSTR lpCmdLine);
	int Run(int nCmdShow);
	void AddDialog(HWND hwnd);
	void RemoveDialog(HWND hwnd);
	bool IsDialogMessage(MSG * pMsg);
	void LoadRegxp();
	void LoadFormatting();
	void LoadStyle();
	void LoadSettings();
	void SaveFormatting();
	void SaveStyle();
	void SaveRegxp();
	void SaveSettings();

private:
	void ParseCommandLine(LPTSTR lpCmdLine);
	bool ActivatePrevInst();
	void RegisterFont();

private:
	typedef std::vector<HWND> WndList;
	WndList m_wndmrg;
	DWORD m_InternalVersion;

public:
	bool m_flagNoReuseWindow;
	bool m_flagStartAsTrayIcon;
	bool m_flagUnregProgram;
	bool m_flagPosParam;
	bool m_flagNewFromClipboard;
	bool m_flagPasteBoard;
	bool m_flagDisplayHelp;
	bool m_flagForceHTML;

	bool f_bModified;
	bool f_bReadOnly;
	bool f_bLastCopyFromMe;
	int f_iInitialLine;
	int f_iInitialColumn;
	int f_cxEditFrame;
	int f_cyEditFrame;

	bool d_bAlwaysOnTop;
	bool d_bMinimizeToTray;
	WORD d_Transparent;
	bool d_bShortPathNames;
	bool d_bSaveBeforeRunningTools;
	byte d_EscFunction;
	WORD d_WordWrapIndent;
	byte d_WordWrapSymbols;
	bool d_bShowWordWrapSymbols;
	bool d_bTabsAsSpaces;
	WORD d_iTabWidth;
	WORD d_iLongLinesLimit;
	byte d_LongLineMode;
	bool d_bMarkLongLines;
	bool d_WordWrap;
	bool d_bMatchBraces;
	bool d_bAutoIndent;
	bool d_bAutoCloseTags;
	bool d_bShowIndentGuides;
	bool d_bShowSelectionMargin;
	bool d_bShowLineNumbers;
	bool d_bViewWhiteSpace;
	bool d_bViewEOLs;
	byte d_iDefaultEOL;
	byte d_iPrintHeader;
	byte d_iPrintFooter;
	sbyte d_iPrintZoom;
	bool d_bShowToolbar;
	bool d_bShowStatusbar;
	DWORD d_FindFlag;
	WININFO d_WndInfo;

	struct stCustomRegxpItem
	{
		tstring first;
		tstring second;
		tstring third;
	};

	typedef util::bind_ptr<stCustomRegxpItem> CustomRegxpItemType;
	typedef vector<CustomRegxpItemType> CustomRegxpType;
	CustomRegxpType d_CustomRegxp;

	CView m_view;
	CFileInformation CurFile;
	MyLexerManager Lexer;
	vector<tstring> Codepage;
	vector<FMTPARAM> Formatting;
	vector<STRPAIR> Editor;
	tstring FavDir;
	// base font size used for relative values
	DWORD BaseFontSize;
};

template<typename _Bool>
inline void CView::EnableCmd(HMENU hMenu, UINT uID, _Bool bEnable)
{ ::EnableMenuItem(hMenu, uID, MF_BYCOMMAND | (bEnable ? MF_ENABLED : MF_GRAYED)); }

template<typename _Bool>
inline void CView::CheckCmd(HMENU hMenu, UINT uID, _Bool bEnable)
{ ::CheckMenuItem(hMenu, uID, MF_BYCOMMAND | (bEnable ? MF_CHECKED : MF_UNCHECKED)); }

inline int CView::MessageBox(const tchar * lpText, UINT uType, const tchar * lpCaption) const
{
	return ::MessageBox(m_hWnd, lpText, lpCaption, uType);
}

inline LRESULT CView::SendWMSize()
{
    RECT rc;
	::GetClientRect(m_hWnd, &rc);
    return SendMessage(WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right,rc.bottom));
}

inline void CView::DoToolBarCmd(int MenuID)
{
	if (IsCmdEnabled(m_hWnd, MenuID))
		SendMessage(WM_COMMAND, MenuID, 0);
	else
		MessageBeep(0);
}





void GetVersion(VERSION & out);
tstring FormatBytes(DWORD dwBytes);
//unsigned int ConvertNFO(const char * in_ascii, char * out_utf8);
//unsigned int MakeNFO(const char * in_utf8, char * out_ascii);
void SetFormatter(astyle::ASFormatter & formatter, const FMTDATA & data);
int GetEOLMode(TCHAR * lpData, unsigned int cbData = -1);
const TCHAR * GetEOLModeString(int iMode);


// 把资源字串里的 \n 替换为 \0
inline tstring & FixMutliString(tstring & str)
{
	std::replace(str.begin(), str.end(), '\n', '\0');
	return str;
}

inline void MakeBitmapButton(HWND hwnd, int nCtlId, UINT uBmpId)
{
    HWND hwndCtl = GetDlgItem(hwnd,nCtlId);
	AddStyle(hwndCtl, BS_BITMAP);
	::SendMessage(hwndCtl, BM_SETIMAGE, IMAGE_BITMAP,
		(LPARAM)LoadImage(WpfGetInstance(), MAKEINTRESOURCE(uBmpId),
                                  IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS));
}




namespace sci {


enum MESSAGE {
	GETSELECTIONSTART = SCI_GETSELECTIONSTART,
	GETSELECTIONEND = SCI_GETSELECTIONEND,
	GETTARGETSTART = SCI_GETTARGETSTART,
	GETTARGETEND = SCI_GETTARGETEND,
	GETLENGTH = SCI_GETLENGTH,
	//int length, tchar * buffer
	GETTEXT = SCI_GETTEXT,
	//(<unused>, TextRange *tr)
	GETTEXTRANGE = SCI_GETTEXTRANGE,
	//(<unused>, char *text)
	GETSELTEXT = SCI_GETSELTEXT,
	GETSELECTIONMODE = SCI_GETSELECTIONMODE,

	//(int anchorPos, int currentPos)
	SETSEL = SCI_SETSEL,
	SETCURSOR = SCI_SETCURSOR,
	SETCODEPAGE = SCI_SETCODEPAGE,
	SETTARGETSTART = SCI_SETTARGETSTART,
	SETTARGETEND = SCI_SETTARGETEND,

	//(int length, const char *s)
	ADDTEXT = SCI_ADDTEXT,
	BEGINUNDOACTION = SCI_BEGINUNDOACTION,
	CLEARALL = SCI_CLEARALL,
	ENDUNDOACTION = SCI_ENDUNDOACTION,
	FINDTEXT = SCI_FINDTEXT,
	//(<unused>, const char *text)
	REPLACESEL = SCI_REPLACESEL,

	GETSTYLEBITS = SCI_GETSTYLEBITS,
	GETSTYLEDTEXT = SCI_GETSTYLEDTEXT,
	GETLEXER = SCI_GETLEXER,
};



};




#endif
