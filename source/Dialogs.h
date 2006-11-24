#ifndef DIALOG__H__
#define DIALOG__H__

//#include "Notepad2.h"
//#include "utils.h"
#include "edit.h"
#include "styles.h"
#include <astyle_interface.h>


namespace wnd
{



inline void DisplayCmdLineHelp()
{
	MessageBox(NULL, ResStr(IDS_CMDLINEHELP), ResStr(IDS_APPTITLE), MB_OK|MB_ICONINFORMATION);
}



class About : public wpf::CDialogSimple<IDD_ABOUT>
{
public:
	About()
		: m_web1(IDC_WEBPAGE, NULL, IDC_HOVER)
		, m_web2(IDC_WEBPAGE2, NULL, IDC_HOVER)
		, m_web3(IDC_WEBPAGE3, NULL, IDC_HOVER)
		, m_web4(IDC_WEBPAGE4, NULL, IDC_HOVER)
		, m_web5(IDC_WEBPAGE5, NULL, IDC_HOVER)
		, m_web6(IDC_WEBPAGE6, NULL, IDC_HOVER)
	{}

private:
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	//BOOL PtInItem(int id, const POINT & pt);
	//int IdFromPt(const POINT & pt);

private:
	wpf::CStaticLink m_web1;
	wpf::CStaticLink m_web2;
	wpf::CStaticLink m_web3;
	wpf::CStaticLink m_web4;
	wpf::CStaticLink m_web5;
	wpf::CStaticLink m_web6;
};


class FindReplace : public wpf::CModelessSimple<IDD_REPLACE>
{
	NONCOPYABLE(FindReplace);
public:
	enum METHOD { MT_FIND, MT_REPLACE, MT_COLLECT };

	FindReplace(METHOD Method, util::memory<TCHAR> & InputText);
	static HWND GetWndStatic() { return hWndStatic; }

private:
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void DoMRU(const TCHAR * FindText);
	void OnSwitch();
	// 界面初始化
	void OnInitDialog();
	// 创建查找弹出菜单
	void OnFindMenu();
	// 创建替换弹出菜单
	void OnReplaceMenu();
	void CheckControl(bool bState);

	inline void MoveDlgItem(int nID, int YMove)
	{
		if (YMove)
		{
			RECT r;
			GetChildRect(m_hWnd, nID, &r);
			wpf::MoveDlgItem(m_hWnd, nID, r.left, r.top + YMove, 
				r.right - r.left, r.bottom - r.top);
		}
	}

	inline void MoveTo(int nID, int ptY)
	{
		RECT r;
		GetChildRect(m_hWnd, nID, &r);
		wpf::MoveDlgItem(m_hWnd, nID, r.left, ptY, 
			r.right - r.left, r.bottom - r.top);
	}

private:
	METHOD xm_method;
	util::memory<TCHAR> & m_text;
	wpf::CThemeHelper xm_theme;
	static HWND hWndStatic;
	wpf::CComboBoxHook x_cbFind;
	wpf::CComboBoxHook x_cbReplace;
};


class Format : public wpf::CDialogSimple<IDD_FORMAT>
{
public:
	inline const FMTDATA & GetFmtData() { return m_data; }

private:
	FMTDATA m_data;

private:
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void MakeData(FMTDATA & data);
	void ResetControl();
	void SetControl(const FMTDATA & data);
};


class FormatSave : public wpf::CDialogSimple<IDD_SAVE_METHOD>
{
public:
	FormatSave();
	inline const tstring & GetName() { return m_name; }
	inline const tstring & GetDescription() { return m_description; }

private:
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	tstring m_name;
	tstring m_description;
	CDlgResizer m_resizer;
};


class CompileNSIS : public wpf::CDialogSimple<IDD_COMPILE_NSIS>
{
	enum USERMESSAGE {
		UM_FIRST = WM_USERMESSAGE,
		UM_GET_COMLIPE_CMD,
		UM_INFO,
		UM_COMPILE_COMPLETE,
	};

	enum MESSAGE {
		MSG_ERR_CREATE_PIPE,
		MSG_ERR_RUN_CMD,
		MSG_EXIT,
	};

	enum MAKENSIS_NOTIFY {
	  MAKENSIS_NOTIFY_SCRIPT,
	  MAKENSIS_NOTIFY_WARNING,
	  MAKENSIS_NOTIFY_ERROR,
	  MAKENSIS_NOTIFY_OUTPUT
	};


public:
	CompileNSIS(const TCHAR * makensis);

private:
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	static unsigned int _stdcall ThreadMakeNSIS(void * p);

private:
	tstring m_cmd;
	CDlgResizer m_resizer;
	string m_output;
};



class BatchFormat : public wpf::CDialogSimple<IDD_CPP_BATCH_FORMAT>
{
public:
	BatchFormat();

private:
	enum {
		UM_FORMAT_FINISH = WM_USERMESSAGE,
	};

private:
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	static unsigned int _stdcall ThreadFormatFile(void * param);
	bool FormatFile(const TCHAR * File, astyle::ASFormatter & formatter);
	void ShowMessage(HWND hWnd, const TCHAR * Text, bool bUpdate = false);

	CDlgResizer m_resizer;
};


class DirList
{
public:
	DirList() :
	  _lpsf(NULL), _hWndLV(NULL), _hThread(NULL), _bQuitFlag(FALSE)
	  {}

	virtual ~DirList()
	{
		if (_lpsf)
			_lpsf->Release();
		_WaitForThread();
	}

private:
	struct PathSort : public std::greater<LPITEMIDLIST>
	{
		PathSort(LPSHELLFOLDER & lpsf) :
			_lpsf(lpsf)		{}

		inline bool operator() (const LPITEMIDLIST & lhs, const LPITEMIDLIST & rhs) const
		{
			return (short)HRESULT_CODE(_lpsf->CompareIDs(0, lhs, rhs)) < 0;
		}

		LPSHELLFOLDER & _lpsf;
	};

	static unsigned int _stdcall _ThreadAddIcon(void * param);

protected:
	void ListViewInit(HWND hWndLV);
	void ListViewAddPath(const TCHAR * path, bool bShowExt = false);
	void ListViewGetSelString(tstring & out);
	inline void ListViewAddIcon()
	{
		_WaitForThread();
		unsigned int uThreadID;
		_hThread = (HANDLE)_beginthreadex(NULL, 0, _ThreadAddIcon, this, 0, &uThreadID);
	}

	template<class T>
	inline void ListViewSetFilter(const T & filter)
	{
		_filter = filter;
		std::replace(_filter.begin(), _filter.end(), ';', '\0');
	}


private:
	inline void _WaitForThread()
	{
		if (_hThread) {
			InterlockedExchange(&_bQuitFlag, TRUE);
			if (WAIT_OBJECT_0 == WaitForSingleObject(_hThread, INFINITE)) {
				CloseHandle(_hThread);
				_hThread = NULL;
			}
			InterlockedExchange(&_bQuitFlag, FALSE);
		}
	}

private:
	LPSHELLFOLDER _lpsf;
	HWND _hWndLV;
	HANDLE _hThread;
	LONG _bQuitFlag;
	tstring _filter;
};

class Favorites : public wpf::CDialogSimple<IDD_FAVORITES>, DirList
{
public:
	Favorites();
	inline tstring GetSelFile() { return SelFile; }

private:
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void ResetListView(const TCHAR * dir);

	void OnFilterChange();
	void OnOK();

private:
	HWND hWndList;
	tstring SelFile;
	tstring CurDir;
	WindowPos pos;
	CDlgResizer m_resizer;
};

class OpenWith : public wpf::CDialogSimple<IDD_OPENWITH>
{
public:
	OpenWith();

	enum { IDD = IDD_OPENWITH };

private:
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	inline void ResetColumnWidth()
	{
		lv.SetColumnWidth(0, iColumnWidth + 10);
		RECT r;
		::GetClientRect(GetDlgItem(IDC_INFO), &r);
		lv.SetColumnWidth(1, r.right - GetSystemMetrics(SM_CXHSCROLL) - (iColumnWidth + 10));
	}

private:
	WindowPos pos;
	wpf::CListView lv;
	int iColumnWidth;
	CDlgResizer m_resizer;
};


class StyleConfig : public wpf::CDialogSimple<IDD_STYLECONFIG>
{
public:
	StyleConfig()
		: pCurrentLexer(NULL)
		, pCurrentStyle(NULL)
		, hwndTV(NULL)
		, fDragging(FALSE)
		, pos(_T("StyleConfig"))
	{}

private:
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void EnableControl(BOOL bEnable);
	void RefreshStyleInfo();
	void AddLexer();

private:
	EDITLEXER * pCurrentLexer;
	EDITSTYLE * pCurrentStyle;
	CBrush fg;
	CBrush bg;
	HWND hwndTV;
	BOOL fDragging;
	WindowPos pos;
};


class SelectLexer : public wpf::CDialogSimple<IDD_STYLESELECT>
{
public:
	SelectLexer();

private:
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void AddLexer(EDITLEXER * plex);

private:
	WindowPos m_pos;
	HWND m_hwndLV;
	int m_cur_lexer;
	CDlgResizer m_resizer;
};


class FileAssoc : public wpf::CDialogSimple<IDD_FILEASSOC>
{
public:
	FileAssoc();

	~FileAssoc() { if (hImageList) VERIFY(ImageList_Destroy(hImageList)); }

private:
	enum SELFLAG { SF_NONE, SF_UNSEL, SF_SEL, SF_PSSEL };
	struct TREEPARAM {
		HTREEITEM hTreeItem;
		unsigned int SelFlag;
	};

private:
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void AddFile();
	void RefreshComponents();
	HTREEITEM TreeHitTest(HWND tree);
	SELFLAG CheckAssoc(const TCHAR * ext, bool bContextMenu);
	SELFLAG CheckGroupAssoc(tstring exts, bool bContextMenu);
	void Restore();
	void DirectAssoc(const tstring & module, const TCHAR * ext, const TCHAR * desc, int icon);
	void AddToContextMenu(const tstring & module, const TCHAR * ext);

private:
	WindowPos pos;
	HIMAGELIST hImageList;
	HWND hWndTree;
	vector<TREEPARAM> hTreeItem;
	TREEPARAM hTreePar1;
	TREEPARAM hTreePar2;
	CDlgResizer m_resizer;
};


class SaveFile : public wpf::CFileDialog
{
public:
	enum {
		ID_EOLMODE = ID_LAST,
		ID_CB_EOLMODE,
		ID_ENCODING,
		ID_CB_ENCODING,

		WS_DEFAULT = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
		WS_STATIC = WS_DEFAULT | SS_LEFT,
		WSEX_STATIC = 0,
		WS_COMBOBOX = WS_DEFAULT | WS_TABSTOP | WS_VSCROLL | WS_CLIPCHILDREN | CBS_AUTOHSCROLL | CBS_HASSTRINGS | CBS_DROPDOWNLIST,
		WSEX_COMBOBOX = WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE,
	};

	SaveFile(HWND ParentWnd,
		const TCHAR * Filter,
		const TCHAR * DefFilename,
		const TCHAR * DefExt) :
	wpf::CFileDialog(false, ParentWnd, Filter, DefFilename, DefExt)
	{
		this->AddFlags(OFN_ENABLESIZING);
	}

private:
	LRESULT OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam); 
};

class Run : public wpf::CDialogSimple<IDD_RUN>
{
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam); 
};

class AddToFav : public wpf::CDialogSimple<IDD_ADDTOFAV>
{
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam); 
};

class Goto : public wpf::CDialogSimple<IDD_LINENUM>
{
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam); 
};

class PropGeneral : public wpf::CDialogBase //wpf::CDialogSimple<IDD_PROP_GENERAL>
{
public:
	PropGeneral(uint ID) : wpf::CDialogBase(ID) {}
private:
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam); 

	wpf::CThemeHelper theme;
};

class PropEditor : public wpf::CDialogBase //wpf::CDialogSimple<IDD_PROP_EDITOR>
{
public:
	PropEditor(uint ID) : wpf::CDialogBase(ID) {}
private:
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam); 

	wpf::CThemeHelper theme;
};

class CustomRegxp : public wpf::CDialogSimple<IDD_CUSTOM_REGXP>
{
public:
	CustomRegxp();

private:
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam); 

private:
	CDlgResizer m_resizer;
	wpf::CListView xm_lv;
};

class ImportExport : public wpf::CDialogSimple<IDD_IMPORT_EXPORT>
{
public:
	ImportExport(bool b_is_export) : xm_is_export(b_is_export) {}

private:
	INT_PTR OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Import(const string & file);
	void Export(const string & file);
	void ReadRegistry(CMarkupSTL & xml, const tstring & key, bool recursive = true);
	void WriteRegistry(CMarkupSTL & xml, const tstring & key, bool recursive = true);

private:
	bool xm_is_export;
};



void FindAndReplace(wnd::FindReplace::METHOD method, int caller);


} // end of namespace dialog






#endif 
// End of Dialogs.h