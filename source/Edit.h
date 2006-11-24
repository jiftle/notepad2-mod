#ifndef EDIT__H__
#define EDIT__H__




class CStatusBar : public wpf::CWndBase
{
public:
	enum {
		ST_DOCPOS,
		ST_DOCSIZE,
		ST_CODEPAGE,
		ST_EOLMODE,
		ST_OVRMODE,
		ST_LEXER ,
		ST_HELP = 255,
	};

	bool Create(HWND hWndParent);
	int CalcPaneWidth(LPCTSTR lpsz);
	void SetText(UINT nPart, LPCTSTR lpszText);
};

class CFileInformation;
class CDocment : public wpf::CWndBase
{
public:
	CDocment();
	~CDocment();

	int MessageBox(const tchar * lpText,
		UINT uType = MB_OK|MB_ICONINFORMATION,
		const tchar * lpCaption = ResStr(IDS_APPTITLE)) const;

	bool DisableRectSel();

	bool Create(HWND hWndParent);
	void CommentBlock(const TCHAR * CommentSymbol, bool bDoComment);
	void OnExportUBB();
	void OnExportHTML();
	void ReloadLexer();
	void SetStyles(int iStyle, const STYLE & Style);
	void SetLexer(EDITLEXER * pLexNew);
	void SetLexerFromFile(const tstring & file);
	void SetHTMLLexer();
	void SetXMLLexer();
	void SetLongLineColors();
	void SetNewText(LPCTSTR lpstrText, DWORD cbText);

	void FormatCPP(astyle::ASFormatter & formatter);
	void RemoveBlankLines();
	void StripFirstCharacter();
	void StripTrailingBlanks();
	void SwapTabsAndSpaces(int nTabWidth, bool bTabsToSpaces);

	void JumpTo(int iNewLine, int iNewCol);
	void JumpToBookmark(bool bNext);
	void ToggleBookmark();
	void ClearAllBookmarks();

	bool FindPrev(const TCHAR * text, DWORD flag);
	bool FindNext(const TCHAR * text, DWORD flag);
	bool Replace(const TCHAR * find, const TCHAR * rep, DWORD flag);
	bool ReplaceAll(const TCHAR * find, const TCHAR * rep, bool bShowInfo, DWORD flag);
	bool ReplaceSelection(const tchar * find, const tchar * rep, bool bShowInfo, DWORD flag);
	bool Collect(const tchar * find, const tchar * rep, bool bmatch_case, bool bregxp);

	bool LoadFile(CFileInformation & fi, int enc);
	bool SaveFile(LPCTSTR pszFile, int iEncoding, BOOL bSaveCopy);

	void PrintSetup();
	bool Print(const tchar * pszDocTitle, const tchar * pszPageFormat, CStatusBar * pStatus);

private:
	//Setup default page margin if no values from registry
	void PrintInit();
	static UINT_PTR CALLBACK PrintSetupHook(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	// Stored objects...
	HGLOBAL m_hDevMode;
	HGLOBAL m_hDevNames;
};

class CDocmentFrame : public wpf::CWndBase
{
public:
	bool Create(HWND hWndParent);
};

class CToolbar : public wpf::CWndBase
{
public:
	CToolbar();
	~CToolbar();

	bool Create(HWND hWndParent);
	void Destroy();
	void SaveToolBar();
	void EnableTool(int id, BOOL bEnable);
	void CheckTool(int id, BOOL bEnable);
	bool GetButtonInfo(NMTOOLBAR * ptb);
	void Reset();

private:
	HIMAGELIST m_himl1;
	HIMAGELIST m_himl2;

	static TBSAVEPARAMS m_tbsp;
	//static TBBUTTON m_tbbtn[];
	static const unsigned int m_bmp_num;
	static const unsigned int m_btn_num;
};

class CSizeBox : public wpf::CWndBase
{
public:
	bool Create(HWND hWndParent, HWND hWndChild, const RECT & rc);
	int GetHeight() const;

private:
	int m_cy;
};



inline void CStatusBar::SetText(UINT nPart, LPCTSTR lpszText)
{
    UINT uFlags = (nPart == 255) ? nPart | SBT_NOBORDERS : nPart;
    SendMessage(SB_SETTEXT, uFlags, lpszText);
}

inline void CToolbar::Destroy()
{
	::DestroyWindow(m_hWnd);
}

inline void CToolbar::SaveToolBar()
{
	SendMessage(TB_SAVERESTORE, TRUE, &m_tbsp);
}

inline void CToolbar::EnableTool(int id, BOOL bEnable)
{
	SendMessage(TB_ENABLEBUTTON, id, MAKELONG(bEnable ? 1 : 0, 0));
}

inline void CToolbar::CheckTool(int id, BOOL bEnable)
{
	SendMessage(TB_CHECKBUTTON, id, MAKELONG(bEnable,0));
}


inline int CSizeBox::GetHeight() const
{
	return m_cy;
}


inline void CDocment::ClearAllBookmarks()
{
    SendMessage(SCI_MARKERDELETEALL, -1, 0);
}

inline bool CDocment::DisableRectSel()
{
	if (SC_SEL_RECTANGLE == SendMessage(SCI_GETSELECTIONMODE, 0, 0))
	{
		MessageBox(ResStr(IDS_SELRECT), MB_OK|MB_ICONINFORMATION);
		return true;
	}
	return false;
}








#endif
