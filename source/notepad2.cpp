#include "notepad2.h"
#include "edit.h"
#include "styles.h"
#include "dialogs.h"

using namespace astyle;



HWND hwndNextCBChain = NULL;

static tstring g_OpenWithDir;
RECT pagesetupMargin;

#ifdef _SEH
VERSION g_ver;
#endif


struct WRAPSYMBOLS
{
	int flags;
	int location;
}
WrapSymbols[4] = { { 1, 1 },
				   { 2, 2 },
				   { 3, 3 },
				   { 3, 0 } };

CView * pView = NULL;
CAppModule theApp;
CAppModule * pApp = &theApp;
HINSTANCE g_hInstance;


HINSTANCE WpfGetInstance() { return g_hInstance; }


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInst,
				   LPTSTR lpCmdLine, int nCmdShow)
{
	g_hInstance = hInstance;
	theApp.Init(lpCmdLine);
	int ret = theApp.Run(nCmdShow);
	return ret;
}

CAppModule::CAppModule()
: m_flagNoReuseWindow(false)
, m_flagStartAsTrayIcon(false)
, m_flagUnregProgram(false)
, m_flagPosParam(false)
, m_flagNewFromClipboard(false)
, m_flagPasteBoard(false)
, m_flagDisplayHelp(false)
, m_flagForceHTML(false)

, f_bModified(false)
, f_bReadOnly(false)
, f_bLastCopyFromMe(false)
, f_iInitialLine(0)
, f_iInitialColumn(0)

, d_bAlwaysOnTop(false)
, d_bMinimizeToTray(false)
, d_Transparent(0x00BF)
, d_bShortPathNames(false)
, d_bSaveBeforeRunningTools(false)
, d_EscFunction(0)
, d_WordWrapIndent(0)
, d_WordWrapSymbols(0)
, d_bShowWordWrapSymbols(false)
, d_bTabsAsSpaces(false)
, d_iTabWidth(4)
, d_iLongLinesLimit(72)
, d_LongLineMode(EDGE_LINE)
, d_bMarkLongLines(false)
, d_WordWrap(false)
, d_bMatchBraces(true)
, d_bAutoIndent(true)
, d_bAutoCloseTags(true)
, d_bShowIndentGuides(true)
, d_bShowSelectionMargin(true)
, d_bShowLineNumbers(false)
, d_bViewWhiteSpace(false)
, d_bViewEOLs(false)
, d_iDefaultEOL(SC_EOL_CRLF)
, d_iPrintHeader(1)
, d_iPrintFooter(0)
, d_iPrintZoom(10)
, d_bShowToolbar(true)
, d_bShowStatusbar(true)
, d_FindFlag(0)

, BaseFontSize(DEFAULT_FONTSIZE)
{
	pView = &m_view;

	for (uint i = 0; i < this->Lexer.GetSize(); ++i)
	{
		_tcscpy(this->Lexer[i]->szExtensions, this->Lexer[i]->pszDefExt);
	}

}

bool CAppModule::IsDialogMessage(MSG * pMsg)
{
	for (WndList::iterator iter = m_wndmrg.begin(); iter != m_wndmrg.end(); ++iter)
	{
		if (::IsDialogMessage(*iter, pMsg))
			return true;
	}
	return false;
}

void CAppModule::AddDialog(HWND hwnd)
{
	ASSERT(::IsWindow(hwnd));
	if (m_wndmrg.end() == std::find(m_wndmrg.begin(), m_wndmrg.end(), hwnd))
		m_wndmrg.push_back(hwnd);
}

void CAppModule::RemoveDialog(HWND hwnd)
{
	WndList::iterator iter = std::find(m_wndmrg.begin(), m_wndmrg.end(), hwnd);
	if (m_wndmrg.end() != iter)
		m_wndmrg.erase(iter);
}

void CAppModule::Init(LPTSTR lpCmdLine)
{
	ParseCommandLine(lpCmdLine);
}

int CAppModule::Run(int nCmdShow)
{
	RegisterFont();

	// Command Line Help Dialog
	if (m_flagDisplayHelp)
	{
		wnd::DisplayCmdLineHelp();
		return 0;
	}

	// Unregister Dialog
	if (m_flagUnregProgram)
	{
		CReg::DeleteKey(APP_REG_ROOT, APP_REG_KEY);
		return 0;
	}

	// Try to activate another window
	if (ActivatePrevInst())
		return 0;

	// Init OLE and Common Controls
	::OleInitialize(NULL);

	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_WIN95_CLASSES | ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES;
	::InitCommonControlsEx(&icex);

	// Error mode set to SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX
	// from here throughout the whole program
	::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	LoadSettings();

	if (!m_view.Create(nCmdShow))
		return 1;

	HACCEL hAccel = ::LoadAccelerators(WpfGetInstance(), MAKEINTRESOURCE(IDR_MAINWND));

	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		__TRY
			if (!::TranslateAccelerator(m_view.GetHandle(), hAccel, &msg) &&
				!this->IsDialogMessage(&msg))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
		__CATCH("main_loop")
	}

	for (WndList::iterator iter = m_wndmrg.begin(); iter != m_wndmrg.end(); ++iter)
	{
		if (::IsWindow(*iter))
			::SendMessage(*iter, WM_CLOSE, 0, 0);
	}
	SaveSettings();
	::OleUninitialize();
	return msg.wParam;
}

void CAppModule::ParseCommandLine(LPTSTR lpCmdLine)
{
	CReg reg;
	if (ERROR_SUCCESS == reg.Open(APP_REG_ROOT, APP_REG_KEY _T("\\Settings")))
	{
		m_flagNoReuseWindow = reg.QueryDWORDValue(_T("bReuseWindow")) == 0;
		reg.Close();
	}
	if ('\0' == lpCmdLine[0])
		return;

	tstring cmd = lpCmdLine;
	tstring::size_type p1 = 0;

	while (p1 < cmd.length())
	{
		p1 = cmd.find_first_not_of(_T(" \t"), p1);
		if (tstring::npos == p1)
			return;

		if (cmd[p1] == '/' && ++p1 < cmd.length())
		{
			switch (util::tolower(cmd[p1]))
			{
			case 'h': //edit a HTML file
				m_flagForceHTML = true;
				break;

			case 'n':
				m_flagNoReuseWindow = true;
				break;

			case 'i':
				m_flagStartAsTrayIcon = true;
				break;

			case 'u':
				m_flagUnregProgram = true;
				return;

			case 'c':
				m_flagNewFromClipboard = true;
				break;

			case 'b':
				m_flagPasteBoard = true;
				break;

			case 'p':
				if ((p1 = cmd.find_first_not_of(_T(" \t"), p1 + 1)) != tstring::npos)
				{
					int itok = _stscanf(cmd.c_str() + p1, _T("%i,%i,%i,%i,%i"), 
						&pApp->d_WndInfo.x, &pApp->d_WndInfo.y, &pApp->d_WndInfo.cx, &pApp->d_WndInfo.cy, &pApp->d_WndInfo.max);
					if (itok == 4 || itok == 5)
					{ // scan successful
						m_flagPosParam = true;
						if (pApp->d_WndInfo.cx < 1)
							pApp->d_WndInfo.cx = CW_USEDEFAULT;
						if (pApp->d_WndInfo.cy < 1)
							pApp->d_WndInfo.cy = CW_USEDEFAULT;
						if (pApp->d_WndInfo.max)
							pApp->d_WndInfo.max = 1;
						if (itok == 4)
							pApp->d_WndInfo.max = 0;
					}

					if ((p1 = cmd.find_first_of(_T(" \t"), p1 + 1)) == tstring::npos)
						return;
				}
				else
					return;
				break;

			case 'g':
				if ((p1 = cmd.find_first_not_of(_T(" \t"), p1)) != tstring::npos)
				{
					int itok = _stscanf(cmd.c_str() + p1, _T("%i,%i"), 
						&f_iInitialLine, &f_iInitialColumn);
					if (itok != 1 && itok != 2)
					{ // scan successful
						f_iInitialLine = f_iInitialColumn = 0;
					}

					if ((p1 = cmd.find_first_of(_T(" \t"), p1)) == tstring::npos)
						return;
				}
				else
					return;
				break;

			case '?':
				m_flagDisplayHelp = true;
				return;
			}

			if (' ' != cmd[p1] && '\t' != cmd[p1])
				p1++;
		}
		else // is file
		{
			tstring file = cmd.c_str() + p1;
			util::trim(file, _T("\" \t"));
			if (!HasBadPathChar(file.c_str()))
				pApp->CurFile.attach(file.c_str());
			return;
		}
	}
}

bool CAppModule::ActivatePrevInst()
{
	COPYDATASTRUCT cds;

	if (m_flagNoReuseWindow || m_flagStartAsTrayIcon || m_flagNewFromClipboard || m_flagPasteBoard)
		return false;

	// Found a window
	HWND hwnd = FindWindowEx(NULL, NULL, APP_NAME, NULL);
	if (hwnd)
	{
		// Enabled
		if (IsWindowEnabled(hwnd))
		{
			// Make sure the previous window won't pop up a change notification message
			//SendMessage(hwnd,WM_CHANGENOTIFYCLEAR,0,0);

			if (IsIconic(hwnd))
				ShowWindowAsync(hwnd, SW_RESTORE);

			if (!IsWindowVisible(hwnd))
			{
				::SendMessage(hwnd, WM_TRAYMESSAGE, 0, WM_LBUTTONDBLCLK);
				::SendMessage(hwnd, WM_TRAYMESSAGE, 0, WM_LBUTTONUP);
			}

			SetForegroundWindow(hwnd);

			if (!pApp->CurFile.empty())
			{
				cds.dwData = DATA_NOTEPAD2_FILEARG;
				cds.cbData = pApp->CurFile.length();
				cds.lpData = (void *)pApp->CurFile.c_str();

				::SendMessage(hwnd, WM_COPYDATA, NULL, (LPARAM)&cds);
			}
			return true;
		}

		else // IsWindowEnabled()
		{
			// Ask...
			if (IDYES == MessageBox(NULL, ResStr(IDS_ERR_PREVWINDISABLED), APP_NAME, MB_YESNO|MB_ICONWARNING))
				return false;
			else
				return true;

		}
	}

	return false;
}

void CAppModule::LoadSettings()
{
	CReg reg;
	// Scintilla Styles
	reg.Open(APP_REG_ROOT, APP_REG_KEY);
	m_InternalVersion = reg.QueryDWORDValue(_T("InternalVersion"));
	if (m_InternalVersion > APP_INTERNAL_VERSION)
		return;

	reg.Open(APP_REG_ROOT, APP_REG_KEY _T("\\Settings"));

	reg.QueryDWORDValue(_T("FindFlag"), d_FindFlag);

	if (!reg.QueryStringValue(_T("OpenWithDir"), g_OpenWithDir))
		g_OpenWithDir = GetDefaultOpenDir();

	if (!reg.QueryStringValue(_T("Favorites"), pApp->FavDir))
		pApp->FavDir = GetSpecialFolderDir(CSIDL_PERSONAL);

	reg.QueryDWORDValue(_T("bShortPathNames"), d_bShortPathNames);
	reg.QueryDWORDValue(_T("bWordWrap"), d_WordWrap);

	if (ERROR_SUCCESS == reg.QueryDWORDValue(_T("iWordWrapIndent"), d_WordWrapIndent))
		d_WordWrapIndent = util::range<WORD>(d_WordWrapIndent, 0, 1024);
	if (ERROR_SUCCESS == reg.QueryDWORDValue(_T("iWordWrapSymbols"), d_WordWrapSymbols))
		d_WordWrapSymbols = util::range<byte>(d_WordWrapSymbols, 0, 3);

	reg.QueryDWORDValue(_T("bShowWordWrapSymbols"), d_bShowWordWrapSymbols);
	reg.QueryDWORDValue(_T("bMatchBraces"), d_bMatchBraces);
	reg.QueryDWORDValue(_T("bAutoIndent"), d_bAutoIndent);
	reg.QueryDWORDValue(_T("bAutoCloseTags"), d_bAutoCloseTags);
	reg.QueryDWORDValue(_T("bShowIndentGuides"), d_bShowIndentGuides);
	reg.QueryDWORDValue(_T("bTabsAsSpaces"), d_bTabsAsSpaces);

	if (ERROR_SUCCESS == reg.QueryDWORDValue(_T("iTabWidth"), d_iTabWidth))
		d_iTabWidth = util::range<WORD>(d_iTabWidth, 1, 24);

	reg.QueryDWORDValue(_T("bMarkLongLines"), d_bMarkLongLines);

	if (ERROR_SUCCESS == reg.QueryDWORDValue(_T("iLongLinesLimit"), d_iLongLinesLimit))
		d_iLongLinesLimit = util::range<WORD>(d_iLongLinesLimit, 0, 1024);

	if (ERROR_SUCCESS == reg.QueryDWORDValue(_T("iLongLineMode"), d_LongLineMode))
		d_LongLineMode = util::range<byte>(d_LongLineMode, EDGE_LINE, EDGE_BACKGROUND);

	reg.QueryDWORDValue(_T("bShowSelectionMargin"), d_bShowSelectionMargin);
	reg.QueryDWORDValue(_T("bShowLineNumbers"), d_bShowLineNumbers);
	reg.QueryDWORDValue(_T("bViewWhiteSpace"), d_bViewWhiteSpace);
	reg.QueryDWORDValue(_T("bViewEOLs"), d_bViewEOLs);

	if (ERROR_SUCCESS == reg.QueryDWORDValue(_T("iDefaultEOLMode"), d_iDefaultEOL))
		d_iDefaultEOL = util::range<byte>(d_iDefaultEOL, SC_EOL_CRLF, SC_EOL_LF);

	if (ERROR_SUCCESS == reg.QueryDWORDValue(_T("iPrintHeader"), d_iPrintHeader))
		d_iPrintHeader = util::range<byte>(d_iPrintHeader, 0, 3);

	if (ERROR_SUCCESS == reg.QueryDWORDValue(_T("iPrintFooter"), d_iPrintFooter))
		d_iPrintFooter = util::range<byte>(d_iPrintFooter, 0, 1);

	if (ERROR_SUCCESS == reg.QueryDWORDValue(_T("iPrintZoom"), d_iPrintZoom))
		d_iPrintZoom = util::range<sbyte>(d_iPrintZoom - 10, -10, 20);

	DWORD pglen = 0;
	if (CReg::SUCCESS == reg.QueryBinaryValue(_T("PrintMargin"), NULL, &pglen)
		&& pglen == sizeof(RECT) )
		reg.QueryBinaryValue(_T("PrintMargin"), &pagesetupMargin, &pglen);

	reg.QueryDWORDValue(_T("bSaveBeforeRunningTools"), d_bSaveBeforeRunningTools);

	if (ERROR_SUCCESS == reg.QueryDWORDValue(_T("iEscFunction"), d_EscFunction))
		d_EscFunction = util::range<byte>(d_EscFunction, 0, 2);

	reg.QueryDWORDValue(_T("bAlwaysOnTop"), d_bAlwaysOnTop);
	reg.QueryDWORDValue(_T("bMinimizeToTray"), d_bMinimizeToTray);
	if (GetProcAddress(GetModuleHandle(_T("User32")), "SetLayeredWindowAttributes") != NULL)
	{
		d_Transparent = LOWORD(reg.QueryDWORDValue(_T("iTransparent")));
		if (0 == d_Transparent)
			d_Transparent = 0xF0BF;
		else
			d_Transparent |= 0xF000;
	}
	else
		d_Transparent &= 0x0FFF;

	reg.QueryDWORDValue(_T("bShowToolbar"), d_bShowToolbar);
	reg.QueryDWORDValue(_T("bShowStatusbar"), d_bShowStatusbar);
	reg.Close();

	// 查询窗口位置
	reg.Open(APP_REG_ROOT, APP_REG_KEY _T("\\Window"));
	if (!m_flagPosParam)
	{ // ignore window position if /p was specified
		DWORD size = 0;
		if (CReg::SUCCESS == reg.QueryBinaryValue(_T("MainWnd"), NULL, &size)
			&& size == sizeof(WININFO) )
				reg.QueryBinaryValue(_T("MainWnd"), &pApp->d_WndInfo, &size);
		else {
			pApp->d_WndInfo.x = 30;
			pApp->d_WndInfo.y = 50;
			pApp->d_WndInfo.cx = 800;
			pApp->d_WndInfo.cy = 600;
			pApp->d_WndInfo.max = FALSE;
		}
	}
	reg.Close();

	TCHAR name[256];
	DWORD cname = countof(name);
	STRPAIR pair;

	for (DWORD idx = 0; CReg::EnumValue(APP_REG_ROOT, APP_REG_KEY _T("\\Editors"), 
		idx, name, &cname); idx++, cname = countof(name))
	{
		if (CReg::SUCCESS == reg.Open(APP_REG_ROOT, APP_REG_KEY _T("\\Editors")))
		{
			if (reg.QueryStringValue(name, pair.second) && name[0] && !pair.second.empty())
			{
				pair.first = name;
				pApp->Editor.push_back(pair);
			}
			reg.Close();
		}

		if (pApp->Editor.size() == 100)
			break;

	}

	if (!pApp->Editor.empty()) {
		std::sort(pApp->Editor.begin(), pApp->Editor.end());
		pApp->Editor.erase(std::unique(pApp->Editor.begin(), pApp->Editor.end()), pApp->Editor.end());
	}

	//load custom regxp
	LoadRegxp();

	// Scintilla Styles
	if (m_InternalVersion >= 1006)
	{
		LoadStyle();
	}
	else
	{
		for (uint i = 0; i < pApp->Lexer.GetSize(); ++i)
		{
			_tcscpy(pApp->Lexer[i]->szExtensions, pApp->Lexer[i]->pszDefExt);
		}
	}
	LoadFormatting();
}

void CAppModule::LoadFormatting()
{
	CReg reg;
	reg.Open(APP_REG_ROOT, APP_REG_KEY _T("\\Formatting"), KEY_READ);
	DWORD cMaxValueNameLen = 0, cMaxValueLen = 0;
	reg.QueryInfoKey(NULL, NULL, NULL, NULL, NULL, NULL, &cMaxValueNameLen, &cMaxValueLen);
	reg.Close();
	LONG res;
	if (cMaxValueNameLen > 0 && cMaxValueLen > 0)
	{
		pApp->Formatting.clear();
		int idx = 0;
		TCHAR name[255];
		DWORD cName = countof(name), cValue = 0;
		FMTPARAM fmt;
		while (CReg::EnumValue(APP_REG_ROOT, APP_REG_KEY _T("\\Formatting"), idx, name, &cName))
		{
			fmt.name = name;
			pApp->Formatting.push_back(fmt);
			cName = countof(name);
			idx++;
		}

		if (pApp->Formatting.size() > 0)
		{
			util::memory<BYTE> mem;
			reg.Open(APP_REG_ROOT, APP_REG_KEY _T("\\Formatting"));
			for (idx=0; idx<(int)pApp->Formatting.size(); idx++)
			{
				cValue = 0;
				FMTPARAM & fp = pApp->Formatting[idx];
				res = reg.QueryBinaryValue(fp.name.c_str(), NULL, &cValue);
				if (CReg::SUCCESS == res && cValue) {
					mem.resize(cValue+sizeof(WCHAR));
					res = reg.QueryBinaryValue(fp.name.c_str(), mem.ptr(), &cValue);
					if (CReg::SUCCESS == res && cValue) {
						memcpy(&fp.data, mem.ptr(), sizeof(FMTDATA));
						WCHAR * ptr = (WCHAR *)(mem.ptr() + fp.data.nStructSize);
						ptr[(cValue - fp.data.nStructSize)/sizeof(WCHAR)] = 0;
						fp.description = string_os_from_utf16(ptr);
					}
				}
			}
			reg.Close();
		}
	}
	if (0 == pApp->Formatting.size())
	{
		FMTPARAM fmt;

		fmt.name = _T("ansi");
		memset(&fmt.data, 0, sizeof(FMTDATA));
		fmt.data.Style = 'c';
		fmt.data.Indentation = 4;
		fmt.data.BracketFormatMode = BREAK_MODE;
		fmt.data.BracketIndent = false;
		fmt.description =	string_os_from_utf16(
							_W("ANSI style formatting/indenting.\n\n")
							_W("namespace foospace") _W("\n")
							_W("{") _W("\n")
							_W("    int Foo()") _W("\n")
							_W("    {") _W("\n")
							_W("        if (isBar)") _W("\n")
							_W("        {") _W("\n")
							_W("            bar();") _W("\n")
							_W("            return 1;") _W("\n")
							_W("        }") _W("\n")
							_W("        else") _W("\n")
							_W("            return 0;") _W("\n")
							_W("    }") _W("\n")
							_W("}") _W("\n")    );
		pApp->Formatting.push_back(fmt);

		fmt.name = _T("gnu");
		memset(&fmt.data, 0, sizeof(FMTDATA));
		fmt.data.Style = 'c';
		fmt.data.Indentation = 2;
		fmt.data.BracketFormatMode = BREAK_MODE;
		fmt.data.BlockIndent = true;
		fmt.description =	string_os_from_utf16(
							_W("GNU style formatting/indenting.\n\n")
							_W("namespace foospace") _W("\n")
							_W("  {") _W("\n")
							_W("    int Foo()") _W("\n")
							_W("      {") _W("\n")
							_W("        if (isBar)") _W("\n")
							_W("          {") _W("\n")
							_W("            bar();") _W("\n")
							_W("            return 1;") _W("\n")
							_W("          }") _W("\n")
							_W("        else") _W("\n")
							_W("          return 0;") _W("\n")
							_W("      }") _W("\n")
							_W("}") _W("\n")    );
		pApp->Formatting.push_back(fmt);

		fmt.name = _T("kr");
		memset(&fmt.data, 0, sizeof(FMTDATA));
		fmt.data.Style = 'c';
		fmt.data.Indentation = 4;
		fmt.data.BracketFormatMode = ATTACH_MODE;
		fmt.data.BracketIndent = false;
		fmt.description =	string_os_from_utf16(
							_W("Kernighan&Ritchie style formatting/indenting.\n\n")
							_W("namespace foospace {") _W("\n")
							_W("    int Foo() {") _W("\n")
							_W("        if (isBar) {") _W("\n")
							_W("            bar();") _W("\n")
							_W("            return 1;") _W("\n")
							_W("        } else") _W("\n")
							_W("            return 0;") _W("\n")
							_W("    }") _W("\n")
							_W("}") _W("\n")    );
		pApp->Formatting.push_back(fmt);

		fmt.name = _T("linux");
		memset(&fmt.data, 0, sizeof(FMTDATA));
		fmt.data.Style = 'c';
		fmt.data.Indentation = 8;
		fmt.data.BracketFormatMode = BDAC_MODE;
		fmt.data.BracketIndent = false;
		fmt.description =	string_os_from_utf16(
							_W("Linux style formatting/indenting (brackets are broken apart from class/function declarations, but connected to command lines, and indents are set to 8 spaces).\n\n")
							_W("namespace foospace") _W("\n")
							_W("{") _W("\n")
							_W("        int Foo()") _W("\n")
							_W("        {") _W("\n")
							_W("                if (isBar) {") _W("\n")
							_W("                        bar();") _W("\n")
							_W("                        return 1;") _W("\n")
							_W("                } else") _W("\n")
							_W("                        return 0;") _W("\n")
							_W("        }") _W("\n")
							_W("}") _W("\n")    );
		pApp->Formatting.push_back(fmt);

		fmt.name = _T("java");
		memset(&fmt.data, 0, sizeof(FMTDATA));
		fmt.data.Style = 'j';
		fmt.data.Indentation = 4;
		fmt.data.BracketFormatMode = ATTACH_MODE;
		fmt.data.BracketIndent = false;
		fmt.description =	string_os_from_utf16(
							_W("Java style formatting/indenting.\n\n")
							_W("class foospace {") _W("\n")
							_W("    int Foo() {") _W("\n")
							_W("        if (isBar) {") _W("\n")
							_W("            bar();") _W("\n")
							_W("            return 1;") _W("\n")
							_W("        } else") _W("\n")
							_W("            return 0;") _W("\n")
							_W("    }") _W("\n")
							_W("}") _W("\n")    );
		pApp->Formatting.push_back(fmt);

		fmt.name = _T("bluenet");
		memset(&fmt.data, 0, sizeof(FMTDATA));
		fmt.data.Style = 'c';
		fmt.data.Indentation = 0xFF04;
		fmt.data.BracketFormatMode = BREAK_MODE;
		fmt.data.OperatorPaddingMode = true;
		fmt.description = string_os_from_utf16(_W("Author's style formatting/indenting.\n\n"));
		pApp->Formatting.push_back(fmt);
		fmt.name = _T("");
		pApp->Formatting.push_back(fmt);

		SaveFormatting();
	}
}

void CAppModule::SaveFormatting()
{
	CReg::DeleteKey(APP_REG_ROOT, APP_REG_KEY _T("\\Formatting"));
	CReg reg;
	if (ERROR_SUCCESS != reg.Create(APP_REG_ROOT, APP_REG_KEY _T("\\Formatting"))
		|| pApp->Formatting.size() == 0)
		return;

	util::memory<BYTE> mem;
	for (unsigned int i = 0; i < pApp->Formatting.size(); i++)
	{
		FMTPARAM & fp = pApp->Formatting[i];
		wstring wstr;
		wstr.append(string_utf16_from_os(fp.description.c_str()));
		fp.data.nStructSize = sizeof(FMTDATA);
		unsigned int size = wstr.length() * sizeof(WCHAR) + sizeof(FMTDATA);
		mem.resize(size);
		mem.copy(&fp.data, sizeof(FMTDATA));
		mem.copy(wstr.c_str(), wstr.length() * sizeof(WCHAR), sizeof(FMTDATA));
		reg.SetBinaryValue(fp.name.c_str(), mem.ptr(), size);
	}
}

void CAppModule::SaveRegxp()
{
	CReg::DeleteKey(APP_REG_ROOT, APP_REG_KEY_NODE("CustomRegxp"));
	CReg reg;
	if (d_CustomRegxp.size() != 0 && ERROR_SUCCESS == reg.Create(APP_REG_ROOT, APP_REG_KEY_NODE("CustomRegxp")))
	{
		for (unsigned int i=0; i<d_CustomRegxp.size(); ++i)
		{
			if (0 == d_CustomRegxp[i]->third.length())
			{
				reg.SetStringValue(d_CustomRegxp[i]->first.c_str(), d_CustomRegxp[i]->second.c_str());
			}
			else
			{
				wstring str;
				str << d_CustomRegxp[i]->second;
				str.append(1, '\0');
				str << d_CustomRegxp[i]->third;
				reg.SetBinaryValue(d_CustomRegxp[i]->first.c_str(), str.c_str(), (str.length() + 1) * sizeof(wchar));
			}
		}
	}
}

void CAppModule::SaveSettings()
{
	CReg reg;
	reg.Create(APP_REG_ROOT, APP_REG_KEY);
	reg.SetDWORDValue(_T("InternalVersion"), APP_INTERNAL_VERSION);

	reg.Create(APP_REG_ROOT, APP_REG_KEY _T("\\Settings"));

	reg.SetDWORDValue(_T("FindFlag"), d_FindFlag);
	reg.SetStringValue(_T("OpenWithDir"), g_OpenWithDir.c_str());
	reg.SetStringValue(_T("Favorites"), pApp->FavDir.c_str());
	reg.SetDWORDValue(_T("bShortPathNames"), d_bShortPathNames);
	reg.SetDWORDValue(_T("bWordWrap"), d_WordWrap);
	reg.SetDWORDValue(_T("iWordWrapIndent"), d_WordWrapIndent);
	reg.SetDWORDValue(_T("iWordWrapSymbols"), d_WordWrapSymbols);
	reg.SetDWORDValue(_T("bShowWordWrapSymbols"), d_bShowWordWrapSymbols);
	reg.SetDWORDValue(_T("bMatchBraces"), d_bMatchBraces);
	reg.SetDWORDValue(_T("bAutoIndent"), d_bAutoIndent);
	reg.SetDWORDValue(_T("bAutoCloseTags"), d_bAutoCloseTags);
	reg.SetDWORDValue(_T("bShowIndentGuides"), d_bShowIndentGuides);
	reg.SetDWORDValue(_T("bTabsAsSpaces"), d_bTabsAsSpaces);
	reg.SetDWORDValue(_T("iTabWidth"), d_iTabWidth);
	reg.SetDWORDValue(_T("bMarkLongLines"), d_bMarkLongLines);
	reg.SetDWORDValue(_T("iLongLinesLimit"), d_iLongLinesLimit);
	reg.SetDWORDValue(_T("iLongLineMode"), d_LongLineMode);
	reg.SetDWORDValue(_T("bShowSelectionMargin"), d_bShowSelectionMargin);
	reg.SetDWORDValue(_T("bShowLineNumbers"), d_bShowLineNumbers);
	reg.SetDWORDValue(_T("bViewWhiteSpace"), d_bViewWhiteSpace);
	reg.SetDWORDValue(_T("bViewEOLs"), d_bViewEOLs);
	reg.SetDWORDValue(_T("iDefaultEOLMode"), d_iDefaultEOL);
	reg.SetDWORDValue(_T("iPrintHeader"), d_iPrintHeader);
	reg.SetDWORDValue(_T("iPrintFooter"), d_iPrintFooter);
	reg.SetDWORDValue(_T("iPrintZoom"), d_iPrintZoom + 10);
	reg.SetDWORDValue(_T("bSaveBeforeRunningTools"), d_bSaveBeforeRunningTools);
	reg.SetDWORDValue(_T("iEscFunction"), d_EscFunction);
	reg.SetDWORDValue(_T("bAlwaysOnTop"), d_bAlwaysOnTop);
	reg.SetDWORDValue(_T("bMinimizeToTray"), d_bMinimizeToTray);
	reg.SetDWORDValue(_T("iTransparent"), d_Transparent);
	reg.SetDWORDValue(_T("bShowToolbar"), d_bShowToolbar);
	reg.SetDWORDValue(_T("bShowStatusbar"), d_bShowStatusbar);

	reg.SetBinaryValue(_T("PrintMargin"), &pagesetupMargin, sizeof(RECT));
	reg.Close();
	reg.Create(APP_REG_ROOT, APP_REG_KEY_NODE("Window"));
	reg.SetBinaryValue(_T("MainWnd"), &pApp->d_WndInfo, sizeof(WININFO));
	reg.Close();

	CReg::DeleteKey(APP_REG_ROOT, APP_REG_KEY_NODE("Editors"));
	if (pApp->Editor.size() && ERROR_SUCCESS == reg.Create(APP_REG_ROOT, APP_REG_KEY_NODE("Editors")))
	{
		for (unsigned int i=0; i<pApp->Editor.size(); i++)
		{
			reg.SetStringValue(pApp->Editor[i].first.c_str(), pApp->Editor[i].second.c_str());
		}
		reg.Close();
	}

	// Scintilla Styles
	SaveStyle();
	SaveFormatting();
	SaveRegxp();
}

void CAppModule::LoadStyle()
{
	CReg reg;
	DWORD nChar = 0;
	DWORD nByte = 0;
	tstring name;

    for (int i = 0; i < (int)pApp->Lexer.GetSize(); i++)
    {
		_tcscpy(pApp->Lexer[i]->szExtensions, pApp->Lexer[i]->pszDefExt);
		name = pApp->Lexer[i]->pszDefExt;
		tstring::size_type pos = name.find(';');
		if (tstring::npos != pos) name.erase(pos);

		if ( ERROR_SUCCESS != reg.Open(APP_REG_ROOT,
			(APP_REG_KEY _T("\\Styles\\") + name).c_str() ))
			continue;

		if (ERROR_SUCCESS == reg.QueryStringValue(_T(""), NULL, &nChar))
		{
			if (nChar > 0) {
				nChar = util::min<DWORD>(nChar, countof(pApp->Lexer[i]->szExtensions));
				reg.QueryStringValue(_T(""), pApp->Lexer[i]->szExtensions, &nChar);
				tchar * p = pApp->Lexer[i]->szExtensions;
				util::tolower(p);
			}
		}
        int j = 0;
        while (pApp->Lexer[i]->Styles[j].iStyle != -1)
        {
			if (ERROR_SUCCESS == reg.QueryBinaryValue(util::itot(pApp->Lexer[i]->Styles[j].iStyle), NULL, &nByte))
			{
				if (nByte > 0) {
					nByte = util::min<DWORD>(nByte, sizeof(STYLE));
					reg.QueryBinaryValue(util::itot(pApp->Lexer[i]->Styles[j].iStyle), &pApp->Lexer[i]->Styles[j].Value, &nByte);
				}
			}
            j++;
			nByte = 0;
        }

		if (SCLEX_ASCII == pApp->Lexer[i]->iLexer)
		{
			pApp->Lexer[i]->Styles[0].Value.fCharSet = 0x02;
			_tcscpy(pApp->Lexer[i]->Styles[0].Value.FontName, _T("MS LineDraw"));
		}

		nChar = 0;
		reg.Close();
    }
}


void CAppModule::SaveStyle()
{
	CReg reg;
    int i;
	tstring name;

	CReg::DeleteKey(APP_REG_ROOT, APP_REG_KEY _T("\\Styles"));

    for (i = 0; i < (int)pApp->Lexer.GetSize(); i++)
    {
		name = pApp->Lexer[i]->pszDefExt;
		tstring::size_type pos = name.find(';');
		if (tstring::npos != pos) name.erase(pos);

		reg.Create(APP_REG_ROOT, (APP_REG_KEY _T("\\Styles\\") + name).c_str());
		reg.SetStringValue(_T(""), pApp->Lexer[i]->szExtensions);
        int j = 0;
        while (pApp->Lexer[i]->Styles[j].iStyle != -1)
        {
			STYLE * p = &pApp->Lexer[i]->Styles[j].Value;
			reg.SetBinaryValue(util::itot(pApp->Lexer[i]->Styles[j].iStyle), &pApp->Lexer[i]->Styles[j].Value, sizeof(STYLE));
            j++;
        }
		reg.Close();
    }
}

void CAppModule::RegisterFont()
{
	if (0 != CReg::QueryDWORDValue(APP_REG_ROOT, APP_REG_KEY, _T("FontRegistered")))
		return;

	do {
		std::vector<BYTE> bin;
		if (!api::LoadResourceT(bin, IDR_LINEDRAW, _T("BINARY")) || bin.size() == 0)
			break;

		tchar font[CONST_VALUE::MAX_PATH_BUFFER_SIZE];
		SHGetSpecialFolderPath(NULL, font, CSIDL_FONTS, true);
		_tcscat(font, _T("\\linedraw.ttf"));

		tstring tmp;
		api::GetTempPathT(tmp);
		tmp += _T("\\linedraw.ttf");
		CReader r;
		if (!r.open(tmp.c_str(), CReader::OPEN_WRITE))
			break;

		if (0 == r.write(&bin[0], bin.size()))
			break;

		CopyFile(tmp.c_str(), font, false);

		::AddFontResource(font);
		::SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);

		CReg::SetStringValue(HKLM, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts"),
			_T("MS LineDraw (TrueType)"), _T("linedraw.ttf"));
		CReg::SetDWORDValue(APP_REG_ROOT, APP_REG_KEY, _T("FontRegistered"), 1);

		DeleteFile(tmp.c_str());
	} while (false);
}

void CAppModule::LoadRegxp()
{
	TCHAR name[256];
	DWORD cname = countof(name);
	DWORD type;
	for (DWORD idx = 0; CReg::EnumValue(APP_REG_ROOT, APP_REG_KEY_NODE("CustomRegxp"), 
		idx, name, &cname, NULL, NULL, &type); idx++, cname = countof(name))
	{
		CReg reg;
		if (CReg::SUCCESS == reg.Open(APP_REG_ROOT, APP_REG_KEY_NODE("CustomRegxp")))
		{
			CustomRegxpItemType item;
			if (REG_SZ == type)
			{
				if (reg.QueryStringValue(name, item->second) && name[0] && !item->second.empty())
				{
					item->first = name;
					d_CustomRegxp.push_back(item);
				}
			}
			else if (REG_BINARY == type)
			{
				vector<byte> v;
				if (reg.QueryBinaryValue(name, v) && v.size() > 0)
				{
					v.push_back('\0');
					v.push_back('\0');
					vector<const wchar *> p;
					util::split((const wchar *)&v[0], v.size() / sizeof(wchar), p);
					if (p.size() != 0)
					{
						item->first = name;
						item->second << p[0];
						if (p.size() > 1)
						{
							item->third << p[1];
						}
						d_CustomRegxp.push_back(item);
					}
				}
			}
		}

		if (d_CustomRegxp.size() >= 100)
			break;

	}

	if (0 == m_InternalVersion && d_CustomRegxp.size() == 0)
	{
		CustomRegxpItemType item;
		item->first = _T("HTML-HTTP-JPEG");
		item->second = _T("<img\\s+[^>]*src=\\\"??(http://[^\\\">\\s]+\\.jpg)\\\"??[^>]*>");
		item->third = _T("\\1");
		d_CustomRegxp.push_back(item);

		CustomRegxpItemType item2;
		item2->first = _T("IP");
		item2->second = _T("^(?:(?:25[0-5]|2[0-4]\\d|[01]\\d\\d|\\d?\\d)(?(?=\\.?\\d)\\.)){4}$");
		d_CustomRegxp.push_back(item2);

		CustomRegxpItemType item3;
		item3->first = _T("DATE");
		item3->second = _T("((\\d{2}(([02468][048])|([13579][26]))[\\-\\/\\s]?((((0?[13578])|(1[02]))[\\-\\/\\s]?((0?[1-9])|([1-2][0-9])|(3[01])))|(((0?[469])|(11))[\\-\\/\\s]?((0?[1-9])|([1-2][0-9])|(30)))|(0?2[\\-\\/\\s]?((0?[1-9])|([1-2][0-9])))))|(\\d{2}(([02468][1235679])|([13579][01345789]))[\\-\\/\\s]?((((0?[13578])|(1[02]))[\\-\\/\\s]?((0?[1-9])|([1-2][0-9])|(3[01])))|(((0?[469])|(11))[\\-\\/\\s]?((0?[1-9])|([1-2][0-9])|(30)))|(0?2[\\-\\/\\s]?((0?[1-9])|(1[0-9])|(2[0-8]))))))\\s?(((0?[1-9])|(1[0-2]))\\:([0-5][0-9])((\\s)|(\\:([0-5][0-9]))))?\\s?(AM|PM|am|pm)?");
		d_CustomRegxp.push_back(item3);
	}
}


//CView
CView::CView()
: m_bEnumCodepageThreadFinished(FALSE)
, m_mru(10, APP_REG_ROOT, APP_REG_KEY _T("\\MRU\\Files"))
{
}

CView::~CView()
{
	UnregisterClass(APP_NAME, WpfGetInstance());
}

bool CView::Create(int nCmdShow)
{
	WNDCLASS wc;
	wc.style = CS_BYTEALIGNWINDOW | CS_DBLCLKS;
	wc.lpfnWndProc = CView::WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = WpfGetInstance();
	wc.hIcon = LoadIcon(WpfGetInstance(), MAKEINTRESOURCE(IDR_MAINWND));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAINWND);
	wc.lpszClassName = APP_NAME;

	if (!RegisterClass(&wc))
		return false;

	RECT rcWorkArea;

	// Transform window rect from workarea to screen
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);
	if (pApp->d_WndInfo.x != CW_USEDEFAULT)
		pApp->d_WndInfo.x += rcWorkArea.left;
	if (pApp->d_WndInfo.y != CW_USEDEFAULT)
		pApp->d_WndInfo.y += rcWorkArea.top;

	HWND hwnd = CreateWindowEx(
				   0,
				   APP_NAME,
				   APP_NAME,
				   WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
				   pApp->d_WndInfo.x,
				   pApp->d_WndInfo.y,
				   pApp->d_WndInfo.cx,
				   pApp->d_WndInfo.cy,
				   NULL,
				   NULL,
				   WpfGetInstance(),
				   this);

	if (NULL == hwnd)
	{
		tstring err;
		api::GetLastErrorT(err);
		::MessageBox(NULL, err.c_str(), ResStr(IDS_ERROR), MB_OK|MB_ICONEXCLAMATION);
		ASSERT(NULL != hwnd);
		return false;
	}

	m_hWnd = hwnd;
	if (pApp->m_flagStartAsTrayIcon)
	{
		::ShowWindow(m_hWnd, SW_HIDE);    // trick ShowWindow()
		ShowNotifyIcon(TRUE);
	}
	else if (pApp->d_WndInfo.max)
	{
		::ShowWindow(m_hWnd, SW_SHOWMAXIMIZED);
		::UpdateWindow(m_hWnd);
	}
	else
	{
		::ShowWindow(m_hWnd, nCmdShow);
		::UpdateWindow(m_hWnd);
	}

	this->ApplyView();

	// Pathname parameter
	if (pApp->CurFile.empty())
		LoadFile(true, true, ENC_NONE, _T(""));
	else
	{
		// Open from Directory
		if (PathIsDirectory(pApp->CurFile.c_str()))
		{
			const util::ptstring pfilter = CreateFullFilter(true);
			wpf::CFileDialog dlg(true, m_hWnd, pfilter->c_str(), NULL, NULL);
			if (dlg.DoModal())
				LoadFile(false, false, ENC_NONE, dlg.GetFileName());
		}
		else
		{
			if (LoadFile(false, false, ENC_NONE, pApp->CurFile.c_str()))
			{
				if (pApp->f_iInitialLine > 0 || pApp->f_iInitialColumn) // Jump to position
					m_doc.JumpTo(pApp->f_iInitialLine, pApp->f_iInitialColumn);
			}
		}
	}

	// Check for /c [if no file is specified] -- even if a file is specified
	/*else */if  (pApp->m_flagNewFromClipboard)
	{
		if (m_doc.SendMessage(SCI_CANPASTE, 0, 0))
		{
			bool bAutoIndent2 = pApp->d_bAutoIndent;
			pApp->d_bAutoIndent = false;
			m_doc.JumpTo(-1, 0);
			if (m_doc.SendMessage(SCI_GETLENGTH, 0, 0) > 0)
				m_doc.SendMessage(SCI_NEWLINE, 0, 0);
			m_doc.SendMessage(SCI_PASTE, 0, 0);
			m_doc.SendMessage(SCI_NEWLINE, 0, 0);
			pApp->d_bAutoIndent = bAutoIndent2;
		}
	}

	// Check for Paste Board option -- after loading files
	if (pApp->m_flagPasteBoard)
	{
		pApp->f_bLastCopyFromMe = true;
		hwndNextCBChain = SetClipboardViewer(m_hWnd);
		SetTitle(pApp->d_bShortPathNames, pApp->f_bModified, pApp->f_bReadOnly);
		pApp->f_bLastCopyFromMe = false;
	}

	return true;
}

//************************************
// Method:    ApplyView 应用主窗口外观方案
// FullName:  CView::ApplyView
// Access:    public 
// Returns:   void
// Qualifier: bluenet
//************************************

void CView::ApplyView()
{
	if (pApp->d_bAlwaysOnTop)
		SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	this->SetTransparent();

	if (pApp->m_flagStartAsTrayIcon)
	{
		::ShowWindow(m_hWnd, SW_HIDE);    // trick ShowWindow()
		ShowNotifyIcon(TRUE);
	}
	else if (pApp->d_WndInfo.max)
	{
		::ShowWindow(m_hWnd, SW_SHOWMAXIMIZED);
		::UpdateWindow(m_hWnd);
	}

	// If start as tray icon, set current filename as tooltip
	if (pApp->m_flagStartAsTrayIcon)
		SetNotifyIconTitle();

	UpdateToolbar();
	UpdateStatusbar();
}

//************************************
// Method:    SetTransparent 设置窗口透明度，仅支持 Win2000 及以上
// FullName:  CView::SetTransparent
// Access:    public 
// Returns:   void
// Qualifier: bluenet
//************************************

void CView::SetTransparent()
{
	if (pApp->d_Transparent & 0xF000)
		api::SetLayeredWindowAttributeT(m_hWnd, 
		pApp->d_Transparent & 0x0F00, LOBYTE(pApp->d_Transparent));
}

//************************************
// Method:    SetOnTop 设置窗口是否始终保持在最顶端
// FullName:  CView::SetOnTop
// Access:    public 
// Returns:   void
// Qualifier: bluenet
//************************************

void CView::SetOnTop()
{
	if (pApp->d_bAlwaysOnTop)
	{
		pApp->d_bAlwaysOnTop = false;
		SetWindowPos(HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	else
	{
		pApp->d_bAlwaysOnTop = true;
		SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	UpdateToolbar();
}

void CView::SetTitle(bool IsShort, bool IsModified, bool IsReadOnly)
{
    static const tchar * pszSep = _T(" - ");
    static const tchar * pszMod = _T("* ");

	tstring title;

    if (IsModified)
        title += pszMod;

    if (!pApp->CurFile.empty())
    {
        if (IsShort && !PathIsRoot(pApp->CurFile.c_str()))
        {
			title += util::rget_after(pApp->CurFile.str(), '\\');
        }
        else
            title += pApp->CurFile.c_str();
    }

    else
		title += ResStr(IDS_UNTITLED);

    if (IsReadOnly)
    {
        title += ' ';
		title += ResStr(IDS_READONLY);
    }

	title += pszSep;
	title += ResStr(IDS_APPTITLE);

	::SetWindowText(m_hWnd, title.c_str());
}

void CView::SetNotifyIconTitle()
{
	NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
	nid.hWnd = m_hWnd;
	nid.uID = 0;
	nid.uFlags = NIF_TIP;

	tstring title;
	if (!pApp->CurFile.empty())
	{
		title = pApp->CurFile.c_str();
	}
	else
		api::LoadStringT(title, IDS_UNTITLED);

	if (pApp->f_bModified)
		_tcscpy(nid.szTip, _T("* "));
	else
		_tcscpy(nid.szTip, _T(""));
	_tcscat(nid.szTip, title.c_str());

	Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void CView::ShowNotifyIcon(bool bAdd)
{
	static HICON hIcon;
	NOTIFYICONDATA nid;

	if (!hIcon)
		hIcon = (HICON)LoadImage(WpfGetInstance(), MAKEINTRESOURCE(IDR_MAINWND),
								 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

	ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = m_hWnd;
	nid.uID = 0;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_TRAYMESSAGE;
	nid.hIcon = hIcon;
	_tcscpy(nid.szTip, APP_NAME);

	if (bAdd)
		Shell_NotifyIcon(NIM_ADD, &nid);
	else
		Shell_NotifyIcon(NIM_DELETE, &nid);
}

void CView::UpdateToolbar()
{
	if (!pApp->d_bShowToolbar)
		return;

	m_toolbar.EnableTool(IDT_FILE_ADDTOFAV, !pApp->CurFile.empty());

	m_toolbar.EnableTool(IDT_EDIT_UNDO, m_doc.SendMessage(SCI_CANUNDO));
	m_toolbar.EnableTool(IDT_EDIT_REDO, m_doc.SendMessage(SCI_CANREDO));

	int i = m_doc.SendMessage(SCI_GETSELECTIONEND) - m_doc.SendMessage(SCI_GETSELECTIONSTART);
	m_toolbar.EnableTool(IDT_EDIT_CUT, i);
	m_toolbar.EnableTool(IDT_EDIT_COPY, i);

	m_toolbar.EnableTool(IDT_EDIT_COPYALL, m_doc.SendMessage(SCI_GETLENGTH, 0, 0));
	m_toolbar.EnableTool(IDT_EDIT_PASTE, m_doc.SendMessage(SCI_CANPASTE, 0, 0));
	m_toolbar.EnableTool(IDT_EDIT_CLEAR, i);

	i = m_doc.SendMessage(SCI_GETLENGTH, 0, 0);
	m_toolbar.EnableTool(IDT_EDIT_FIND, i);
	m_toolbar.EnableTool(IDT_EDIT_REPLACE, i);

	m_toolbar.CheckTool(IDT_VIEW_WORDWRAP, pApp->d_WordWrap);
	m_toolbar.CheckTool(IDT_VIEW_ALWAYSONTOP, pApp->d_bAlwaysOnTop);
}

void CView::UpdateStatusbar()
{
	if (!pApp->d_bShowStatusbar)
		return ;

	int iPos;
	int iLn;
	int iLines;
	int iCol;
	int iSel;
	TCHAR tchLn[32];
	TCHAR tchLines[32];
	TCHAR tchCol[32];
	TCHAR tchSel[32];

	int iBytes;

	TCHAR tchCodePage[32];
	TCHAR tchEOLMode[32];
	TCHAR tchOvrMode[32];

	iPos = m_doc.SendMessage(SCI_GETCURRENTPOS, 0, 0);

	iLn = m_doc.SendMessage(SCI_LINEFROMPOSITION, iPos, 0) + 1;
	wsprintf(tchLn, _T("%i"), iLn);
	FormatNumberStr(tchLn);

	iLines = m_doc.SendMessage(SCI_GETLINECOUNT, 0, 0);
	wsprintf(tchLines, _T("%i"), iLines);
	FormatNumberStr(tchLines);

	iCol = m_doc.SendMessage(SCI_GETCOLUMN, iPos, 0) + 1;
	wsprintf(tchCol, _T("%i"), iCol);
	FormatNumberStr(tchCol);

	if (SC_SEL_RECTANGLE != m_doc.SendMessage(SCI_GETSELECTIONMODE, 0, 0))
	{
		iSel = m_doc.SendMessage(SCI_GETSELECTIONEND) - m_doc.SendMessage(SCI_GETSELECTIONSTART);
		wsprintf(tchSel, _T("%i"), iSel);
		FormatNumberStr(tchSel);
	}
	else
		_tcscpy(tchSel, _T("--"));


	iBytes = m_doc.SendMessage(SCI_GETLENGTH, 0, 0);

	if (ENC_UNICODE_LE == pApp->CurFile.enc) {
		_tcscpy(tchCodePage, _T("Unicode LE"));
	} else if (ENC_UNICODE_BE == pApp->CurFile.enc) {
		_tcscpy(tchCodePage, _T("Unicode BE"));
	} else if (ENC_UTF8 == pApp->CurFile.enc) {
		_tcscpy(tchCodePage, _T("UTF-8"));
	} else{
		_tcscpy(tchCodePage, _T("ANSI"));
	}

	if (pApp->CurFile.eol == SC_EOL_CR)
		_tcscpy(tchEOLMode, _T("CR"));
	else if (pApp->CurFile.eol == SC_EOL_LF)
		_tcscpy(tchEOLMode, _T("LF"));
	else
		_tcscpy(tchEOLMode, _T("CR+LF"));

	if (m_doc.SendMessage(SCI_GETOVERTYPE, 0, 0))
		_tcscpy(tchOvrMode, _T("OVR"));
	else
		_tcscpy(tchOvrMode, _T("INS"));

	m_status.SetText(CStatusBar::ST_DOCPOS, util::format(ResStr(IDS_DOCPOS), tchLn, tchLines, tchCol, tchSel));
	m_status.SetText(CStatusBar::ST_DOCSIZE, FormatBytes(iBytes).c_str());
	m_status.SetText(CStatusBar::ST_CODEPAGE, tchCodePage);
	m_status.SetText(CStatusBar::ST_EOLMODE, tchEOLMode);
	m_status.SetText(CStatusBar::ST_OVRMODE, tchOvrMode);
	m_status.SetText(CStatusBar::ST_LEXER, GetCurrentLexerName().c_str());
}

void CView::UpdateLineNumberWidth()
{
	tchar tchLines[32];
	int iLineMarginWidthNow;
	int iLineMarginWidthFit;

	if (pApp->d_bShowLineNumbers)
	{
		_stprintf(tchLines, _T("_%i_"), m_doc.SendMessage(SCI_GETLINECOUNT, 0, 0));

		iLineMarginWidthNow = m_doc.SendMessage(SCI_GETMARGINWIDTHN, 0, 0);
		iLineMarginWidthFit = m_doc.SendMessage(SCI_TEXTWIDTH, STYLE_LINENUMBER, tchLines);

		if (iLineMarginWidthNow != iLineMarginWidthFit)
		{
			m_doc.SendMessage(SCI_SETMARGINWIDTHN, 0, iLineMarginWidthFit);
		}
	}
	else
		m_doc.SendMessage(SCI_SETMARGINWIDTHN, 0, 0);
}

LRESULT CALLBACK CView::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static CView * _this = NULL;
	if (_this)
	{
		return _this->OnMessage(uMsg, wParam, lParam);
	}
	else if (WM_CREATE == uMsg)
	{
		_this = (CView *)((CREATESTRUCT *)lParam)->lpCreateParams;
		ASSERT(_this);
		_this->m_hWnd = hwnd;
		return _this->OnMessage(uMsg, wParam, lParam);
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CView::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static BOOL bHideIcon = FALSE;
	static bool bContextMenu = false;

	switch (uMsg)
	{

		// Quickly handle painting and sizing messages, found in ScintillaWin.cxx
		// Cool idea, don't know if this has any effect... ;-)
	case WM_MOVE:
	case WM_MOUSEACTIVATE:
	case WM_NCHITTEST:
	case WM_NCCALCSIZE:
	case WM_NCPAINT:
	case WM_PAINT:
	case WM_ERASEBKGND:
	case WM_NCMOUSEMOVE:
	case WM_NCLBUTTONDOWN:
	case WM_WINDOWPOSCHANGING:
	case WM_WINDOWPOSCHANGED:
		return DefWindowProc(m_hWnd, uMsg, wParam, lParam);

	case WM_CREATE:
		return OnCreate(wParam, lParam);

	case WM_DESTROY:
		{
			WINDOWPLACEMENT wndpl;

			// GetWindowPlacement
			wndpl.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(m_hWnd, &wndpl);

			pApp->d_WndInfo.x = wndpl.rcNormalPosition.left;
			pApp->d_WndInfo.y = wndpl.rcNormalPosition.top;
			pApp->d_WndInfo.cx = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
			pApp->d_WndInfo.cy = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;
			pApp->d_WndInfo.max = (IsZoomed(m_hWnd) || (wndpl.flags & WPF_RESTORETOMAXIMIZED));

			DragAcceptFiles(m_hWnd, FALSE);

			// Restore clipboard chain...
			if (pApp->m_flagPasteBoard)
				ChangeClipboardChain(m_hWnd, hwndNextCBChain);

			// Remove tray icon if necessary
			ShowNotifyIcon(false);

			PostQuitMessage(0);
		}
		break;

	case WM_CLOSE:
		if (SaveFile(false, true, false, false))
			this->DestroyWindow();
		break;

	case WM_QUERYENDSESSION:
		if (SaveFile(false, true, false, false))
			return TRUE;
		else
			return FALSE;

		// Reinitialize theme-dependent values and resize windows
	case 0x031A /*WM_THEMECHANGED*/:
		OnThemeChanged(wParam, lParam);
		break;

		// update Scintilla colors
	case WM_SYSCOLORCHANGE:
		{
			//extern EDITLEXER * pLexCurrent;
			m_doc.SetLexer(pApp->Lexer.GetCurLexer());
			return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
		}

	case WM_SIZE:
		OnSize(wParam, lParam);
		break;


	case WM_SETFOCUS:
		::SetFocus(m_doc.GetHandle());
		break;


	case WM_DROPFILES:
		{
			HDROP hDrop = (HDROP)wParam;
			if (IsIconic(m_hWnd))
				ShowWindow(m_hWnd, SW_RESTORE);

			tstring file;
			api::DragQueryFileT(file, hDrop, 0);
			LoadFile(false, false, ENC_NONE, file.c_str());

			if (DragQueryFile(hDrop, (UINT)( -1), NULL, 0) > 1)
				this->MessageBox(ResStr(IDS_ERR_DROP), MB_OK|MB_ICONINFORMATION);

			DragFinish(hDrop);
		}
		break;


	case WM_COPYDATA:
		{
			PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT)lParam;

			if (pcds->dwData == DATA_NOTEPAD2_FILEARG)
			{
				tstring path((const TCHAR *)pcds->lpData, pcds->cbData);
				LoadFile(false, false, ENC_NONE, path.c_str());
			}
		}
		return TRUE;


	case WM_CONTEXTMENU:
		bContextMenu = true;
		return OnContextMenu(wParam, lParam);


	case WM_INITMENU:
		OnInitMenu(wParam, lParam, bContextMenu);
		bContextMenu = false;
		break;


	case WM_NOTIFY:
		return OnNotify(wParam, lParam);


	case WM_ACTIVATEAPP:
		if (LOWORD(wParam) && !pApp->CurFile.bad())
		{
			CFileInformation tmp;
			if (tmp.attach(pApp->CurFile.c_str(), true) && pApp->CurFile != tmp)
			{
				if (::IsIconic(m_hWnd))
					::ShowWindow(m_hWnd, SW_SHOWNORMAL);
				if (IDYES == MessageBox(util::format(ResStr(IDS_MODIFIED), 
					pApp->CurFile.c_str()), MB_YESNO | MB_ICONINFORMATION))
						LoadFile(false, false, ENC_NONE, pApp->CurFile.c_str());
				else
					pApp->CurFile.update();
			}
		}
		break;


	case WM_COMMAND:
		return OnCommand(wParam, lParam);


	case WM_SYSCOMMAND:
		switch (wParam)
		{
		case SC_MINIMIZE:
			ShowOwnedPopups(m_hWnd, FALSE);
			if (pApp->d_bMinimizeToTray)
			{
				MinimizeWndToTray(m_hWnd);
				ShowNotifyIcon(true);
				SetNotifyIconTitle();
				return 0;
			}
			else
				return DefWindowProc(m_hWnd, uMsg, wParam, lParam);

		case SC_RESTORE:
			{
				LRESULT lrv = DefWindowProc(m_hWnd, uMsg, wParam, lParam);
				ShowOwnedPopups(m_hWnd, TRUE);
				return (lrv);
			}
		}
		return DefWindowProc(m_hWnd, uMsg, wParam, lParam);


	case WM_DRAWCLIPBOARD:
		if (!pApp->f_bLastCopyFromMe)
		{
			if (m_doc.SendMessage(SCI_CANPASTE, 0, 0))
			{
				bool bAutoIndent2 = pApp->d_bAutoIndent;
				pApp->d_bAutoIndent = false;
				m_doc.JumpTo(-1, 0);
				if (m_doc.SendMessage(SCI_GETLENGTH, 0, 0) > 0)
					m_doc.SendMessage(SCI_NEWLINE, 0, 0);
				m_doc.SendMessage(SCI_PASTE, 0, 0);
				m_doc.SendMessage(SCI_NEWLINE, 0, 0);
				pApp->d_bAutoIndent = bAutoIndent2;
			}
		}
		else
			pApp->f_bLastCopyFromMe = false;
		if (hwndNextCBChain)
			::SendMessage(hwndNextCBChain, WM_DRAWCLIPBOARD, wParam, lParam);
		break;


	case WM_CHANGECBCHAIN:
		if ((HWND)wParam == hwndNextCBChain)
			hwndNextCBChain = (HWND)lParam;
		if (hwndNextCBChain)
			::SendMessage(hwndNextCBChain, WM_CHANGECBCHAIN, lParam, wParam);
		break;


	case WM_TRAYMESSAGE:
		switch (lParam)
		{
		case WM_RBUTTONUP:
			{

				HMENU hMenu = LoadMenu(WpfGetInstance(), MAKEINTRESOURCE(IDR_POPUPMENU));
				HMENU hMenuPopup = GetSubMenu(hMenu, 2);

				POINT pt;
				int iCmd;

				SetForegroundWindow(m_hWnd);

				GetCursorPos(&pt);
				SetMenuDefaultItem(hMenuPopup, IDM_TRAY_RESTORE, FALSE);
				iCmd = TrackPopupMenu(hMenuPopup,
									  TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
									  pt.x, pt.y, 0, m_hWnd, NULL);

				PostMessage(WM_NULL, 0, 0);

				DestroyMenu(hMenu);

				if (iCmd == IDM_TRAY_RESTORE)
				{
					RestoreWndFromTray(m_hWnd);
					ShowOwnedPopups(m_hWnd, TRUE);
					ShowNotifyIcon(false);
					bHideIcon = FALSE;
				}

				else if (iCmd == IDM_TRAY_EXIT)
				{
					SendMessage(WM_CLOSE, 0, 0);
				}
			}

			return TRUE;

		case WM_LBUTTONDBLCLK:
			RestoreWndFromTray(m_hWnd);
			ShowOwnedPopups(m_hWnd, TRUE);
			bHideIcon = TRUE;
			return TRUE;

		case WM_LBUTTONUP:
			if (bHideIcon)
			{
				ShowNotifyIcon(false);
				bHideIcon = FALSE;
			}
			return TRUE;
		}
		break;

	default:
		return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

LRESULT CView::OnCreate(WPARAM wParam, LPARAM lParam)
{
	if (!m_doc.Create(m_hWnd))
		return false;

	// Word wrap
	m_doc.SendMessage(SCI_SETWRAPMODE, pApp->d_WordWrap ? SC_WRAP_WORD : SC_WRAP_NONE, 0);
	m_doc.SendMessage(SCI_SETWRAPSTARTINDENT, pApp->d_WordWrapIndent, 0);
	if (pApp->d_bShowWordWrapSymbols)
	{
		m_doc.SendMessage(SCI_SETWRAPVISUALFLAGSLOCATION, WrapSymbols[pApp->d_WordWrapSymbols].location, 0);
		m_doc.SendMessage(SCI_SETWRAPVISUALFLAGS, WrapSymbols[pApp->d_WordWrapSymbols].flags, 0);
	}
	else
	{
		m_doc.SendMessage(SCI_SETWRAPVISUALFLAGS, 0, 0);
	}

	// Indent Guides
	m_doc.SendMessage(SCI_SETINDENTATIONGUIDES, pApp->d_bShowIndentGuides, 0);

	// Tabs
	m_doc.SendMessage(SCI_SETUSETABS, !pApp->d_bTabsAsSpaces, 0);
	m_doc.SendMessage(SCI_SETTABWIDTH, pApp->d_iTabWidth, 0);
	m_doc.SendMessage(SCI_SETINDENT, 0, 0);

	// Long Lines
	if (pApp->d_bMarkLongLines)
		m_doc.SendMessage(SCI_SETEDGEMODE, pApp->d_LongLineMode, 0);
	else
		m_doc.SendMessage(SCI_SETEDGEMODE, EDGE_NONE, 0);
	m_doc.SendMessage(SCI_SETEDGECOLUMN, pApp->d_iLongLinesLimit, 0);

	// Margins
	m_doc.SendMessage(SCI_SETMARGINWIDTHN, 2, 0);
	m_doc.SendMessage(SCI_SETMARGINWIDTHN, 1, pApp->d_bShowSelectionMargin ? 16 : 0);
	UpdateLineNumberWidth();

	// Nonprinting characters
	m_doc.SendMessage(SCI_SETVIEWWS, pApp->d_bViewWhiteSpace ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE, 0);
	m_doc.SendMessage(SCI_SETVIEWEOL, pApp->d_bViewEOLs, 0);

	if (!m_docframe.Create(m_hWnd))
		return false;

	if (api::IsAppThemedT())
	{
		::SetWindowLongPtr(m_doc.GetHandle(), GWL_EXSTYLE, 
			::GetWindowLongPtr(m_doc.GetHandle(), GWL_EXSTYLE) & ~WS_EX_CLIENTEDGE);
		m_doc.SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

		RECT rc;
		RECT rc2;

		::GetClientRect(m_docframe.GetHandle(), &rc);
		::GetWindowRect(m_docframe.GetHandle(), &rc2);

		pApp->f_cxEditFrame = ((rc2.right - rc2.left) - (rc.right - rc.left)) / 2;
		pApp->f_cyEditFrame = ((rc2.bottom - rc2.top) - (rc.bottom - rc.top)) / 2;
	}
	else
	{
		pApp->f_cxEditFrame = 0;
		pApp->f_cyEditFrame = 0;
	}

	// Create Toolbar and Statusbar
	m_toolbar.Create(m_hWnd);
	m_status.Create(m_hWnd);
	RECT rc;
	m_toolbar.SendMessage(TB_GETITEMRECT, 0, &rc);
	m_sizebox.Create(m_hWnd, m_toolbar.GetHandle(), rc);

	// Drag & Drop
	DragAcceptFiles(m_hWnd, TRUE);

	unsigned int uThreadID;
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadEnumCodepage, this, 0, &uThreadID);
	VERIFY(CloseHandle(hThread));

	m_mru.ReadList();

	return 0;
}

BOOL CView::EnumCodePagesProc(LPTSTR lpCodePageString)
{
	static CPINFOEX cpi;
	GetCPInfoEx((UINT)util::xtoi(lpCodePageString), 0, &cpi);
	if(NULL == _tcsstr(cpi.CodePageName, lpCodePageString))
		return TRUE;

	pApp->Codepage.push_back(tstring(cpi.CodePageName));
	if (pApp->Codepage.size() > 200)
		return FALSE;

	return TRUE;
}

unsigned int _stdcall CView::ThreadEnumCodepage(void * param)
{
	CView * _this = (CView *)param;
	pApp->Codepage.clear();
	wpf::CDynamicCallback<CODEPAGE_ENUMPROC> asdf(_this, &CView::EnumCodePagesProc);
	EnumSystemCodePages(asdf, CP_INSTALLED);
	InterlockedExchange((LONG *)&_this->m_bEnumCodepageThreadFinished, TRUE);
	return 0;
}

//Handle WM_THEMECHANGED
void CView::OnThemeChanged(WPARAM wParam, LPARAM lParam)
{
	RECT rc, rc2;

	// reinitialize edit frame
	if (api::IsAppThemedT())
	{

		::SetWindowLongPtr(m_doc.GetHandle(), GWL_EXSTYLE,
			::GetWindowLongPtr(m_doc.GetHandle(), GWL_EXSTYLE) & ~WS_EX_CLIENTEDGE);
		m_doc.SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);

		m_docframe.SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
		::GetClientRect(m_docframe.GetHandle(), &rc);
		::GetWindowRect(m_docframe.GetHandle(), &rc2);

		pApp->f_cxEditFrame = ((rc2.right - rc2.left) - (rc.right - rc.left)) / 2;
		pApp->f_cyEditFrame = ((rc2.bottom - rc2.top) - (rc.bottom - rc.top)) / 2;
	}
	else
	{

		::SetWindowLongPtr(m_doc.GetHandle(), GWL_EXSTYLE, WS_EX_CLIENTEDGE | 
			::GetWindowLongPtr(m_doc.GetHandle(), GWL_EXSTYLE));
		m_doc.SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

		pApp->f_cxEditFrame = 0;
		pApp->f_cyEditFrame = 0;
	}

	// recreate toolbar and statusbar
	m_toolbar.SaveToolBar();

	m_toolbar.DestroyWindow();
	m_sizebox.DestroyWindow();
	m_status.DestroyWindow();
	m_toolbar.Create(m_hWnd);
	m_status.Create(m_hWnd);
	RECT rc3;
	m_toolbar.SendMessage(TB_GETITEMRECT, 0, &rc3);
	m_sizebox.Create(m_hWnd, m_toolbar(), rc3);

	SendWMSize();
}

LRESULT CView::OnCodepage(WPARAM wParam, LPARAM lParam)
{
	HMENU hmenu = GetMenu(m_hWnd);
	tstring str;
	api::GetMenuStringT(str, hmenu, LOWORD(wParam), MF_BYCOMMAND);
	int nCP = util::xtoi(str.substr(0, str.find(' ')).c_str());
	if (nCP <= 0 || nCP == GetACP())
		return 0;

	// convert codepage
	int nTextLen = m_doc.SendMessage(sci::GETLENGTH);
	if (nTextLen <= 0)
		return 0;

	util::memory<BYTE> mem[2];
	char * ansi;
	TCHAR * final;
	TCHAR * orgi = (TCHAR *)mem[0].alloc((nTextLen + 1) * sizeof(TCHAR));
	m_doc.SendMessage(sci::GETTEXT, nTextLen + 1, orgi);

#ifdef UNICODE
#define NEXT_MEMOBJ 0

	ansi = (char *)mem[1].alloc(estimate_utf16_to_ansi(orgi, nTextLen));
	convert_utf16_to_ansi(orgi, ansi);
#else
#define NEXT_MEMOBJ 1

	ansi = orgi;
#endif

	int len = MultiByteToWideChar(nCP, 0, ansi, -1, NULL, 0);
	if (len <= 0) return 0;

	WCHAR * unicode = (WCHAR *)mem[NEXT_MEMOBJ].alloc((len + 1) * sizeof(WCHAR));
#undef NEXT_MEMOBJ

	len = MultiByteToWideChar(nCP, 0, ansi, -1, unicode, len);

#ifdef UNICODE

	final = unicode;
#else

	final = (char *)mem[0].alloc(estimate_utf16_to_ansi(unicode, len));
	len = convert_utf16_to_ansi(unicode, final);
#endif

	m_doc.SendMessage(SCI_BEGINUNDOACTION);
	m_doc.SendMessage(SCI_CLEARALL);
	m_doc.SendMessage(SCI_ADDTEXT, len - 1, final);
	m_doc.SendMessage(SCI_ENDUNDOACTION);
	UpdateToolbar();
	UpdateStatusbar();
	pApp->f_bModified = true;
	SetTitle(pApp->d_bShortPathNames, true, pApp->f_bReadOnly);
	return 0;
}

LRESULT CView::OnCommand(WPARAM wParam, LPARAM lParam)
{
	WORD id = LOWORD(wParam);

	//open recent file
	if (id >= RECENT_FILE_START && id < RECENT_FILE_START + m_mru.Size())
	{
		HMENU hmenu = GetMenu(m_hWnd);
		tstring file;
		api::GetMenuStringT(file, hmenu, id, MF_BYCOMMAND);
		LoadFile(false, false, ENC_NONE, file.c_str());
	}

	// format cpp
	else if (id >= FORMAT_SCHEME_START && id < FORMAT_SCHEME_START + pApp->Formatting.size())
	{
		ASFormatter formatter;
		tstring name;
		api::GetMenuStringT(name, GetMenu(m_hWnd), id, MF_BYCOMMAND);
		for (unsigned int i=0; i<pApp->Formatting.size(); i++)
		{
			if (name == pApp->Formatting[i].name) {
				SetFormatter(formatter, pApp->Formatting[i].data);
				break;
			}
		}
		m_doc.FormatCPP(formatter);
	}

	// codepage convert
	else if (id >= CODEPAGE_START && id < CODEPAGE_START + pApp->Codepage.size())
		return OnCodepage(wParam, lParam);

	else if (id >= OPENWITH_START && id < OPENWITH_END)
		return OnOpenWith(wParam, lParam);



	switch (id)
	{

	case IDM_EXPORT_UBB:
		m_doc.OnExportUBB();
		break;

	case IDM_COPYTO_HTML:
		m_doc.OnExportHTML();
		break;

	case IDM_CPP_BATCH_FORMAT:
		{
			wnd::BatchFormat dlg;
			dlg.DoModal(m_hWnd);
		}
		break;

	case IDM_NSIS_COMPILE:
		return OnCompileNSIS(wParam, lParam);

	case IDM_PY_COMMENT:
	case IDM_RB_COMMENT:
	case IDM_NSIS_COMMENT2:
		m_doc.CommentBlock(_T("#"), true);
		break;

	case IDM_PY_UNCOMMENT:
	case IDM_RB_UNCOMMENT:
		m_doc.CommentBlock(_T("#"), false);
		break;

	case IDM_NSIS_COMMENT:
		m_doc.CommentBlock(_T(";"), true);
		break;

	case IDM_NSIS_UNCOMMENT:
		m_doc.CommentBlock(_T(";"), false);
		m_doc.CommentBlock(_T("#"), false);
		break;

	case IDM_BAT_COMMENT:
		m_doc.CommentBlock(_T("REM "), true);
		break;

	case IDM_BAT_UNCOMMENT:
		m_doc.CommentBlock(_T("REM "), false);
		break;

	case IDM_CPP_COMMENT:
		m_doc.CommentBlock(_T("//"), true);
		break;

	case IDM_CPP_UNCOMMENT:
		m_doc.CommentBlock(_T("//"), false);
		break;

	case IDM_FILE_NEW:
		LoadFile(false, true, ENC_NONE, _T(""));
		break;


	case IDM_FILE_OPEN:
		LoadFile(false, false, ENC_NONE, _T(""));
		break;


	case IDM_FILE_REVERT:
		{
			int iCurPos = m_doc.SendMessage(SCI_GETCURRENTPOS, 0, 0);
			int iAnchorPos = m_doc.SendMessage(SCI_GETANCHOR, 0, 0);

			tstring CurFile2 = pApp->CurFile.c_str();

			if (LoadFile(false, false, ENC_NONE, CurFile2.c_str()))
				m_doc.SendMessage(SCI_SETSEL, iAnchorPos, iCurPos);
		}
		break;


	case IDM_FILE_SAVE:
		SaveFile(true, false, false, false);
		break;


	case IDM_FILE_SAVEAS:
		SaveFile(true, false, true, false);
		break;


	case IDM_FILE_SAVECOPY:
		SaveFile(true, false, true, true);
		break;


	case IDM_FILE_READONLY:
		if (!pApp->CurFile.empty())
		{
			BOOL bSuccess = FALSE;
			DWORD dwFileAttributes = GetFileAttributes(pApp->CurFile.c_str());
			if (dwFileAttributes != INVALID_FILE_ATTRIBUTES)
			{
				if (pApp->f_bReadOnly)
					dwFileAttributes = (dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
				else
					dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
				if (!SetFileAttributes(pApp->CurFile.c_str(), dwFileAttributes))
					this->MessageBox(util::format(ResStr(IDS_READONLY_MODIFY), 
					pApp->CurFile.c_str()), MB_OK|MB_ICONEXCLAMATION);
			}
			else
				this->MessageBox(util::format(ResStr(IDS_READONLY_MODIFY), 
				pApp->CurFile.c_str()), MB_OK|MB_ICONEXCLAMATION);

			dwFileAttributes = GetFileAttributes(pApp->CurFile.c_str());
			if (dwFileAttributes != INVALID_FILE_ATTRIBUTES)
				pApp->f_bReadOnly = (dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0;

			SetTitle(pApp->d_bShortPathNames, pApp->f_bModified, pApp->f_bReadOnly);
		}
		break;


	case IDM_FILE_NEWWINDOW:
	case IDM_FILE_NEWWINDOW2:
		{
			tchar szModuleName[MAX_PATH];
			tchar szFileName[MAX_PATH];
			tchar szParameters[MAX_PATH + 64];

			RECT rcWorkArea;
			WINDOWPLACEMENT wndpl;
			int x, y, cx, cy, imax;
			tchar tch[64];

			if (pApp->d_bSaveBeforeRunningTools && !SaveFile(false, true, false, false))
				break;

			GetModuleFileName(NULL, szModuleName, countof(szModuleName));

			_tcscpy(szParameters, _T("-n "));

			SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);
			wndpl.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(m_hWnd, &wndpl);

			// offset new window position +10/+10
			x = wndpl.rcNormalPosition.left + 10;
			y = wndpl.rcNormalPosition.top + 10;
			cx = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
			cy = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;

			// check if window fits work area
			if (x + cx > rcWorkArea.right - rcWorkArea.left ||
					y + cy > rcWorkArea.bottom - rcWorkArea.top)
				x = y = 0;

			imax = IsZoomed(m_hWnd);

			_stprintf(tch, _T("-p %i,%i,%i,%i,%i"), x, y, cx, cy, imax);
			_tcscat(szParameters, tch);

			if (LOWORD(wParam) != IDM_FILE_NEWWINDOW2 && !pApp->CurFile.empty())
			{
				_tcscpy(szFileName, pApp->CurFile.c_str());
				PathQuoteSpaces(szFileName);
				_tcscat(szParameters, _T(" "));
				_tcscat(szParameters, szFileName);
			}

			ShellExecute(m_hWnd, NULL, szModuleName, szParameters, NULL, SW_SHOWNORMAL);
		}
		break;


	case IDM_FILE_LAUNCH:
		{
			if (pApp->CurFile.empty())
				break;

			if (pApp->d_bSaveBeforeRunningTools && !SaveFile(false, true, false, false))
				break;

			SHELLEXECUTEINFO sei;
			ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));

			sei.cbSize = sizeof(SHELLEXECUTEINFO);
			sei.fMask = SEE_MASK_NOCLOSEPROCESS;
			sei.hwnd = m_hWnd;
			sei.lpVerb = NULL;
			sei.lpFile = pApp->CurFile.c_str();
			sei.lpParameters = NULL;
			sei.lpDirectory = NULL;
			sei.nShow = SW_SHOWNORMAL;

			ShellExecuteEx(&sei);
		}
		break;


	case IDM_FILE_RUN:
		{
			if (pApp->d_bSaveBeforeRunningTools && !SaveFile(false, true, false, false))
				break;

			wnd::Run dlg;
			dlg.DoModal(m_hWnd);
		}
		break;


	case IDM_OPENWITH_CONFIG:
		{
			wnd::OpenWith dlg;
			dlg.DoModal(m_hWnd);
		}
		break;


	case IDM_FILE_PAGESETUP:
		m_doc.PrintSetup();
		break;

	case IDM_FILE_PRINT:
		{
			tstring title, pagefmt;

			if (!pApp->CurFile.empty())
				title = util::rget_after(pApp->CurFile.str(), '\\');
			else
				api::LoadStringT(title, IDS_UNTITLED);

			api::LoadStringT(pagefmt, IDS_PRINT_PAGENUM);

			if (!m_doc.Print(title.c_str(), pagefmt.c_str(), &m_status))
				this->MessageBox(ResStr(IDS_PRINT_ERROR), MB_OK|MB_ICONEXCLAMATION);
		}
		break;


	case IDM_FILE_PROPERTIES:
		{
			if (pApp->CurFile.empty())
				break;

			SHELLEXECUTEINFO sei;
			ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));

			sei.cbSize = sizeof(SHELLEXECUTEINFO);
			sei.fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_NOCLOSEPROCESS;
			sei.hwnd = m_hWnd;
			sei.lpVerb = _T("properties");
			sei.lpFile = pApp->CurFile.c_str();
			sei.nShow = SW_SHOWNORMAL;

			ShellExecuteEx(&sei);
		}
		break;

	case IDM_FILE_CREATELINK:
		{
			if (pApp->CurFile.empty())
				break;

			tchar desktop[CONST_VALUE::MAX_PATH_BUFFER_SIZE];
			if (!SHGetSpecialFolderPath(m_hWnd, desktop, CSIDL_DESKTOPDIRECTORY, FALSE))
				break;

			tstring linkfile = desktop;
			linkfile += '\\';
			linkfile += util::rget_before(util::rget_after(pApp->CurFile.str(), '\\'), '.');
			linkfile += _T(".lnk");
			if (!com::CreateLink(linkfile.c_str(), pApp->CurFile.c_str()))
				this->MessageBox(ResStr(IDS_ERR_CREATELINK), MB_OK|MB_ICONEXCLAMATION);
		}
		break;


	case IDM_FILE_OPENFAV:
		{
			wnd::Favorites dlg;
			if (IDOK == dlg.DoModal(m_hWnd))
			{
				tstring file = dlg.GetSelFile();
				if (!file.empty() && FileExists(file.c_str())) {
					LoadFile(false, false, ENC_NONE, file.c_str());
				}
			}
		}
		break;


	case IDM_FILE_ADDTOFAV:
		if (!pApp->CurFile.empty())
		{
			wnd::AddToFav dlg;
			dlg.DoModal(m_hWnd);
		}
		break;


	case IDM_FILE_MANAGEFAV:
		{
			SHELLEXECUTEINFO sei;
			ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));

			sei.cbSize = sizeof(SHELLEXECUTEINFO);
			sei.fMask = SEE_MASK_NOCLOSEPROCESS;
			sei.hwnd = m_hWnd;
			sei.lpVerb = NULL;
			sei.lpFile = pApp->FavDir.c_str();
			sei.lpParameters = NULL;
			sei.lpDirectory = NULL;
			sei.nShow = SW_SHOWNORMAL;

			// Run favorites directory
			ShellExecuteEx(&sei);
		}
		break;


	case IDM_FILE_EXIT:
		SendMessage(WM_CLOSE, 0, 0);
		break;


	case IDM_RELOAD_AUTO:
	case IDM_RELOAD_ANSI:
	case IDM_RELOAD_UNICODE_LE:
	case IDM_RELOAD_UNICODE_BE:
	case IDM_RELOAD_UTF8:
		if (!pApp->CurFile.empty()) {
			pApp->CurFile.enc = ENC_NONE + (LOWORD(wParam) - IDM_RELOAD_AUTO);
			LoadFile(false, false, pApp->CurFile.enc, pApp->CurFile.c_str());
			pApp->f_bModified = false;
			UpdateToolbar();
			UpdateStatusbar();
			SetTitle(pApp->d_bShortPathNames, pApp->f_bModified, pApp->f_bReadOnly);
		}
		break;


	case IDM_ENCODING_ANSI:
	case IDM_ENCODING_UNICODE_LE:
	case IDM_ENCODING_UNICODE_BE:
	case IDM_ENCODING_UTF8:
		{
			pApp->CurFile.enc = ENC_ANSI + (LOWORD(wParam) - IDM_ENCODING_ANSI);
			pApp->f_bModified = true;
			UpdateToolbar();
			UpdateStatusbar();
			SetTitle(pApp->d_bShortPathNames, pApp->f_bModified, pApp->f_bReadOnly);
		}
		break;

	case IDM_LINEENDINGS_CRLF:
	case IDM_LINEENDINGS_LF:
	case IDM_LINEENDINGS_CR:
		{
			pApp->CurFile.eol = SC_EOL_CRLF + (LOWORD(wParam) - IDM_LINEENDINGS_CRLF);
			m_doc.SendMessage(SCI_SETEOLMODE, pApp->CurFile.eol, 0);
			m_doc.SendMessage(SCI_CONVERTEOLS, pApp->CurFile.eol, 0);
			UpdateToolbar();
			UpdateStatusbar();
			SetTitle(pApp->d_bShortPathNames, pApp->f_bModified, pApp->f_bReadOnly);
		}
		break;

	case IDM_EDIT_UNDO:
		m_doc.SendMessage(SCI_UNDO, 0, 0);
		break;


	case IDM_EDIT_REDO:
		m_doc.SendMessage(SCI_REDO, 0, 0);
		break;


	case IDM_EDIT_CUT:
		if (pApp->m_flagPasteBoard)
			pApp->f_bLastCopyFromMe = true;
		m_doc.SendMessage(SCI_CUT, 0, 0);
		break;


	case IDM_EDIT_COPY:
		if (pApp->m_flagPasteBoard)
			pApp->f_bLastCopyFromMe = true;
		m_doc.SendMessage(SCI_COPY, 0, 0);
		UpdateToolbar();
		break;


	case IDM_EDIT_COPYALL:
		if (pApp->m_flagPasteBoard)
			pApp->f_bLastCopyFromMe = true;
		m_doc.SendMessage(SCI_COPYRANGE, 0, m_doc.SendMessage(SCI_GETLENGTH, 0, 0));
		UpdateToolbar();
		break;


	case IDM_EDIT_PASTE:
		m_doc.SendMessage(SCI_PASTE, 0, 0);
		break;


	case IDM_EDIT_CLEAR:
		m_doc.SendMessage(SCI_CLEAR, 0, 0);
		break;


	case IDM_EDIT_CLEARALL:
		m_doc.SendMessage(SCI_CLEARALL, 0, 0);
		break;


	case IDM_EDIT_SELECTALL:
		m_doc.SendMessage(SCI_SELECTALL, 0, 0);
		break;


	case IDM_EDIT_SELECTWORD:
		{
			int iPos = m_doc.SendMessage(SCI_GETCURRENTPOS, 0, 0);

			int iWordStart = m_doc.SendMessage(SCI_WORDSTARTPOSITION, iPos, TRUE);
			int iWordEnd = m_doc.SendMessage(SCI_WORDENDPOSITION, iPos, TRUE);

			if (iWordStart == iWordEnd) // we are in whitespace salad...
			{
				iWordStart = m_doc.SendMessage(SCI_WORDENDPOSITION, iPos, FALSE);
				iWordEnd = m_doc.SendMessage(SCI_WORDENDPOSITION, iWordStart, TRUE);
				if (iWordStart != iWordEnd)
					m_doc.SendMessage(SCI_SETSEL, iWordStart, iWordEnd);
			}
			else
				m_doc.SendMessage(SCI_SETSEL, iWordStart, iWordEnd);
		}
		break;


	case IDM_EDIT_MOVELINEUP:
		{
			int iCurPos = m_doc.SendMessage(SCI_GETCURRENTPOS);
			int iCurLine = m_doc.SendMessage(SCI_LINEFROMPOSITION, iCurPos);
			int iLinePos = iCurPos - m_doc.SendMessage(SCI_POSITIONFROMLINE, iCurLine);
			if (iCurLine > 0)
			{
				m_doc.SendMessage(SCI_BEGINUNDOACTION, 0, 0);
				m_doc.SendMessage(SCI_LINETRANSPOSE, 0, 0);
				m_doc.SendMessage(SCI_GOTOPOS, m_doc.SendMessage(SCI_POSITIONFROMLINE, iCurLine - 1) + iLinePos);
				m_doc.SendMessage(SCI_CHOOSECARETX, 0, 0);
				m_doc.SendMessage(SCI_ENDUNDOACTION, 0, 0);
			}
		}
		break;


	case IDM_EDIT_MOVELINEDOWN:
		{
			int iCurPos = m_doc.SendMessage(SCI_GETCURRENTPOS);
			int iCurLine = m_doc.SendMessage(SCI_LINEFROMPOSITION, iCurPos);
			int iLinePos = iCurPos - m_doc.SendMessage(SCI_POSITIONFROMLINE, iCurLine);
			if (iCurLine < m_doc.SendMessage(SCI_GETLINECOUNT) - 1)
			{
				m_doc.SendMessage(SCI_BEGINUNDOACTION);
				m_doc.SendMessage(SCI_GOTOLINE, iCurLine + 1);
				m_doc.SendMessage(SCI_LINETRANSPOSE);
				m_doc.SendMessage(SCI_GOTOPOS, m_doc.SendMessage(SCI_POSITIONFROMLINE, iCurLine + 1) + iLinePos);
				m_doc.SendMessage(SCI_CHOOSECARETX);
				m_doc.SendMessage(SCI_ENDUNDOACTION);
			}
		}
		break;


	case IDM_EDIT_TOGGLELINE:
		{
			int iCurPos = m_doc.SendMessage(SCI_GETCURRENTPOS);
			int iCurLine = m_doc.SendMessage(SCI_LINEFROMPOSITION);
			int iCol = m_doc.SendMessage(SCI_GETCOLUMN, iCurPos) + 1;
			m_doc.SendMessage(SCI_BEGINUNDOACTION);
			m_doc.SendMessage(SCI_LINETRANSPOSE);
			m_doc.JumpTo(iCurLine + 1, iCol);
			m_doc.SendMessage(SCI_ENDUNDOACTION);
		}
		break;


	case IDM_EDIT_DUPLICATELINE:
		m_doc.SendMessage(SCI_LINEDUPLICATE, 0, 0);
		break;


	case IDM_EDIT_CUTLINE:
		if (pApp->m_flagPasteBoard)
			pApp->f_bLastCopyFromMe = true;
		m_doc.SendMessage(SCI_LINECUT, 0, 0);
		break;


	case IDM_EDIT_COPYLINE:
		if (pApp->m_flagPasteBoard)
			pApp->f_bLastCopyFromMe = true;
		m_doc.SendMessage(SCI_LINECOPY, 0, 0);
		UpdateToolbar();
		break;


	case IDM_EDIT_DELETELINE:
		m_doc.SendMessage(SCI_LINEDELETE, 0, 0);
		break;


	case IDM_EDIT_DELETELINELEFT:
		m_doc.SendMessage(SCI_DELLINELEFT, 0, 0);
		break;


	case IDM_EDIT_DELETELINERIGHT:
		m_doc.SendMessage(SCI_DELLINERIGHT, 0, 0);
		break;


	case IDM_EDIT_INDENT:
		m_doc.SendMessage(SCI_TAB, 0, 0);
		break;


	case IDM_EDIT_UNINDENT:
		m_doc.SendMessage(SCI_BACKTAB, 0, 0);
		break;


	case IDM_EDIT_STRIP1STCHAR:
		m_doc.SendMessage(SCI_SETCURSOR, SC_CURSORWAIT, 0);
		m_doc.StripFirstCharacter();
		m_doc.SendMessage(SCI_SETCURSOR, SC_CURSORNORMAL, 0);
		break;


	case IDM_EDIT_TRIMLINES:
		m_doc.SendMessage(SCI_SETCURSOR, SC_CURSORWAIT, 0);
		m_doc.StripTrailingBlanks();
		m_doc.SendMessage(SCI_SETCURSOR, SC_CURSORNORMAL, 0);
		break;


	case IDM_EDIT_REMOVEBLANKLINES:
		m_doc.SendMessage(SCI_SETCURSOR, SC_CURSORWAIT, 0);
		m_doc.RemoveBlankLines();
		m_doc.SendMessage(SCI_SETCURSOR, SC_CURSORNORMAL, 0);
		break;


	case IDM_EDIT_SPLITLINES:
		m_doc.SendMessage(SCI_SETCURSOR, SC_CURSORWAIT, 0);
		m_doc.SendMessage(SCI_TARGETFROMSELECTION, 0, 0);
		m_doc.SendMessage(SCI_LINESSPLIT, 0, 0);
		m_doc.SendMessage(SCI_SETCURSOR, SC_CURSORNORMAL, 0);
		break;


	case IDM_EDIT_JOINLINES:
		m_doc.SendMessage(SCI_SETCURSOR, SC_CURSORWAIT, 0);
		m_doc.SendMessage(SCI_TARGETFROMSELECTION, 0, 0);
		m_doc.SendMessage(SCI_LINESJOIN, 0, 0);
		m_doc.SendMessage(SCI_SETCURSOR, SC_CURSORNORMAL, 0);
		break;


	case IDM_EDIT_FINDMATCHINGBRACE:
		{
			int iBrace2 = -1;
			int iPos = m_doc.SendMessage(SCI_GETCURRENTPOS, 0, 0);
			char c = (tchar)m_doc.SendMessage(SCI_GETCHARAT, iPos, 0);
			if (_tcschr(_T("()[]{}"), c))
				iBrace2 = m_doc.SendMessage(SCI_BRACEMATCH, iPos, 0);
			// Try one before
			else
			{
				iPos = m_doc.SendMessage(SCI_POSITIONBEFORE, iPos, 0);
				c = (tchar)m_doc.SendMessage(SCI_GETCHARAT, iPos, 0);
				if (_tcschr(_T("()[]{}"), c))
					iBrace2 = m_doc.SendMessage(SCI_BRACEMATCH, iPos, 0);
			}
			if (iBrace2 != -1)
				m_doc.SendMessage(SCI_GOTOPOS, iBrace2);
		}
		break;


	case IDM_EDIT_SELTOMATCHINGBRACE:
		{
			int iBrace2 = -1;
			int iPos = m_doc.SendMessage(SCI_GETCURRENTPOS, 0, 0);
			char c = (tchar)m_doc.SendMessage(SCI_GETCHARAT, iPos, 0);
			if (_tcschr(_T("()[]{}"), c))
				iBrace2 = m_doc.SendMessage(SCI_BRACEMATCH, iPos, 0);
			// Try one before
			else
			{
				iPos = m_doc.SendMessage(SCI_POSITIONBEFORE, iPos, 0);
				c = (tchar)m_doc.SendMessage(SCI_GETCHARAT, iPos, 0);
				if (_tcschr(_T("()[]{}"), c))
					iBrace2 = m_doc.SendMessage(SCI_BRACEMATCH, iPos, 0);
			}
			if (iBrace2 != -1)
			{
				if (iBrace2 > iPos)
					m_doc.SendMessage(SCI_SETSEL, iPos, iBrace2 + 1);
				else
					m_doc.SendMessage(SCI_SETSEL, iPos + 1, iBrace2);
			}
		}
		break;


	case IDM_EDIT_GOTOLINE:
		{
			wnd::Goto dlg;
			dlg.DoModal(m_hWnd);
		}
		break;


	case IDM_EDIT_CONVERTUPPERCASE:
		m_doc.SendMessage(SCI_SETCURSOR, SC_CURSORWAIT, 0);
		m_doc.SendMessage(SCI_UPPERCASE, 0, 0);
		m_doc.SendMessage(SCI_SETCURSOR, SC_CURSORNORMAL, 0);
		break;


	case IDM_EDIT_CONVERTLOWERCASE:
		m_doc.SendMessage(SCI_SETCURSOR, SC_CURSORWAIT, 0);
		m_doc.SendMessage(SCI_LOWERCASE, 0, 0);
		m_doc.SendMessage(SCI_SETCURSOR, SC_CURSORNORMAL, 0);
		break;


	case IDM_EDIT_CONVERTTABS:
		m_doc.SwapTabsAndSpaces(pApp->d_iTabWidth, false);
		break;


	case IDM_EDIT_CONVERTSPACES:
		m_doc.SwapTabsAndSpaces(pApp->d_iTabWidth, true);
		break;


	case IDM_EDIT_CONVERTANSI:
	case IDM_EDIT_CONVERTOEM:
		{
			int iCurPos = m_doc.SendMessage(SCI_GETCURRENTPOS, 0, 0);
			int iAnchorPos = m_doc.SendMessage(SCI_GETANCHOR, 0, 0);
			if (iCurPos != iAnchorPos)
			{
				if (m_doc.DisableRectSel())
					break;

				int iSelCount = m_doc.SendMessage(SCI_GETSELECTIONEND, 0, 0) -
								m_doc.SendMessage(SCI_GETSELECTIONSTART, 0, 0);
				util::memory<tchar> buf1(iSelCount + 2);
				util::memory<tchar> buf2(iSelCount + 2);
				m_doc.SendMessage(SCI_SETCURSOR, SC_CURSORWAIT, 0);
				m_doc.SendMessage(SCI_GETSELTEXT, 0, buf1.ptr());
				if (IDM_EDIT_CONVERTANSI == id)
					OemToCharBuff(string_ansi_from_os(buf1.ptr()), buf2.ptr(), buf2.size());
				else
				{
					util::memory<char> bufa(iSelCount + 2);
					CharToOemBuff(buf1.ptr(), bufa.ptr(), bufa.size());
					convert_ansi_to_os(bufa.ptr(), buf2.ptr());
				}
				m_doc.SendMessage(SCI_BEGINUNDOACTION);
				m_doc.SendMessage(SCI_CLEAR);
				m_doc.SendMessage(SCI_ADDTEXT, iSelCount, buf2.ptr());
				m_doc.SendMessage(SCI_SETSEL, iAnchorPos, iCurPos);
				m_doc.SendMessage(SCI_ENDUNDOACTION);
				m_doc.SendMessage(SCI_SETCURSOR, SC_CURSORNORMAL);
			}
		}
		break;

	case IDM_CPP_QFORMAT:
		if (!m_doc.DisableRectSel())
		{

			ASFormatter formatter;
			FMTPARAM Default;
			vector<FMTPARAM>::iterator iter = find(pApp->Formatting.begin(), pApp->Formatting.end(), Default);
			if (iter != pApp->Formatting.end()) {
				SetFormatter(formatter, iter->data);
			}
			else {
				formatter.setBracketIndent(false);
				formatter.setSpaceIndentation(4);
				formatter.setBracketFormatMode(BREAK_MODE);
				formatter.setClassIndent(false);
				formatter.setSwitchIndent(false);
				formatter.setNamespaceIndent(false);
				formatter.setTabIndentation(4, true);
				formatter.setCStyle();
				formatter.setOperatorPaddingMode(true);
			}
			m_doc.FormatCPP(formatter);
		}
		break;

	case IDM_CPP_FORMAT:
		if (!m_doc.DisableRectSel())
		{
			ASFormatter formatter;
			wnd::Format dlg;
			if (IDOK == dlg.DoModal(m_hWnd)) {
				const FMTDATA & fd = dlg.GetFmtData();
				SetFormatter(formatter, fd);
				m_doc.FormatCPP(formatter);
			}
		}
		break;


	case IDM_EDIT_INSERT_SHORTDATE:
	case IDM_EDIT_INSERT_LONGDATE:
		{
			tchar tchDate[128];
			tchar tchTime[128];
			tchar tchDateTime[256];
			SYSTEMTIME st;

			GetLocalTime(&st);
			GetDateFormat(LOCALE_USER_DEFAULT, (
							  LOWORD(wParam) == IDM_EDIT_INSERT_SHORTDATE) ? DATE_SHORTDATE : DATE_LONGDATE,
						  &st, NULL, tchDate, countof(tchDate));
			GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, tchTime, countof(tchTime));

			_stprintf(tchDateTime, _T("%s %s"), tchTime, tchDate);

			m_doc.SendMessage(SCI_REPLACESEL, 0, tchDateTime);
			//}
		}
		break;


	case IDM_EDIT_INSERT_FILENAME:
	case IDM_EDIT_INSERT_PATHNAME:
		if (!pApp->CurFile.empty())
			if (LOWORD(wParam) == IDM_EDIT_INSERT_FILENAME)
				m_doc.SendMessage(SCI_REPLACESEL, 0, 
					util::rget_after(pApp->CurFile.str(), '\\').c_str());
			else
				m_doc.SendMessage(SCI_REPLACESEL, 0, pApp->CurFile.c_str());
		else
			m_doc.SendMessage(SCI_REPLACESEL, 0, ResStr(IDS_UNTITLED).GetPtr());
		break;


	case IDM_EDIT_BOOKMARK_NEXT:
		m_doc.JumpToBookmark(true);
		break;


	case IDM_EDIT_BOOKMARK_PREV:
		m_doc.JumpToBookmark(false);
		break;


	case IDM_EDIT_BOOKMARK_TOGGLE:
		m_doc.ToggleBookmark();
		break;


	case IDM_EDIT_BOOKMARK_CLEARALL:
		m_doc.ClearAllBookmarks();
		break;

	case IDM_EDIT_FIND:
		wnd::FindAndReplace(wnd::FindReplace::MT_FIND, 0);
		break;

	case IDM_EDIT_FINDNEXT:
	case IDM_EDIT_FINDPREV:
		wnd::FindAndReplace(wnd::FindReplace::MT_FIND, id);
		break;

	case IDM_EDIT_REPLACE:
		wnd::FindAndReplace(wnd::FindReplace::MT_REPLACE, 0);
		break;

	case IDM_EDIT_COLLECT:
		wnd::FindAndReplace(wnd::FindReplace::MT_COLLECT, 0);
		break;

	case IDM_VIEW_SCHEME:
		{
			wnd::SelectLexer dlg;
			if (IDOK == dlg.DoModal(m_hWnd))
			{
				UpdateStatusbar();
				UpdateLineNumberWidth();
			}
		}
		break;


	case IDM_VIEW_SCHEMECONFIG:
		{
			wnd::StyleConfig dlg;
			if (IDOK == dlg.DoModal(m_hWnd))
			{
				m_doc.ReloadLexer();
				UpdateStatusbar();
				UpdateLineNumberWidth();
			}
		}
		break;


	case IDM_VIEW_WORDWRAP:
		pApp->d_WordWrap = !pApp->d_WordWrap;
		m_doc.SendMessage(SCI_SETWRAPMODE, pApp->d_WordWrap ? SC_WRAP_WORD : SC_WRAP_NONE, 0);
		UpdateToolbar();
		break;


	case IDM_VIEW_WORDWRAPSETTINGS:
		{
			m_doc.SendMessage(SCI_SETWRAPSTARTINDENT, pApp->d_WordWrapIndent, 0);

			if (pApp->d_bShowWordWrapSymbols)
			{
				m_doc.SendMessage(SCI_SETWRAPVISUALFLAGSLOCATION, WrapSymbols[pApp->d_WordWrapSymbols].location, 0);
				m_doc.SendMessage(SCI_SETWRAPVISUALFLAGS, WrapSymbols[pApp->d_WordWrapSymbols].flags, 0);
			}
			else
			{
				m_doc.SendMessage(SCI_SETWRAPVISUALFLAGS, 0, 0);
			}
		}
		break;


	case IDM_VIEW_LONGLINEMARKER:
		pApp->d_bMarkLongLines = !pApp->d_bMarkLongLines;
		if (pApp->d_bMarkLongLines)
		{
			m_doc.SendMessage(SCI_SETEDGEMODE, pApp->d_LongLineMode, 0);
			m_doc.SetLongLineColors();
		}
		else
			m_doc.SendMessage(SCI_SETEDGEMODE, EDGE_NONE, 0);
		break;


	case IDM_VIEW_SHOWINDENTGUIDES:
		pApp->d_bShowIndentGuides = !pApp->d_bShowIndentGuides;
		m_doc.SendMessage(SCI_SETINDENTATIONGUIDES, pApp->d_bShowIndentGuides, 0);
		break;


	case IDM_VIEW_AUTOINDENTTEXT:
		pApp->d_bAutoIndent = !pApp->d_bAutoIndent;
		break;


	case IDM_VIEW_LINENUMBERS:
		pApp->d_bShowLineNumbers = !pApp->d_bShowLineNumbers;
		UpdateLineNumberWidth();
		break;


	case IDM_VIEW_MARGIN:
		pApp->d_bShowSelectionMargin = !pApp->d_bShowSelectionMargin;
		m_doc.SendMessage(SCI_SETMARGINWIDTHN, 1, pApp->d_bShowSelectionMargin ? 16 : 0);
		break;


	case IDM_VIEW_SHOWWHITESPACE:
		pApp->d_bViewWhiteSpace = !pApp->d_bViewWhiteSpace;
		m_doc.SendMessage(SCI_SETVIEWWS, pApp->d_bViewWhiteSpace ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE, 0);
		break;


	case IDM_VIEW_SHOWEOLS:
		pApp->d_bViewEOLs = !pApp->d_bViewEOLs;
		m_doc.SendMessage(SCI_SETVIEWEOL, pApp->d_bViewEOLs, 0);
		break;

	case IDM_VIEW_TRANSPARENT:
		pApp->d_Transparent &= ~(pApp->d_Transparent & 0x0F00);
		this->SetTransparent();
		break;


	case IDM_VIEW_WORDWRAPSYMBOLS:
		pApp->d_bShowWordWrapSymbols = !pApp->d_bShowWordWrapSymbols;
		if (pApp->d_bShowWordWrapSymbols)
		{
			m_doc.SendMessage(SCI_SETWRAPVISUALFLAGSLOCATION, WrapSymbols[pApp->d_WordWrapSymbols].location, 0);
			m_doc.SendMessage(SCI_SETWRAPVISUALFLAGS, WrapSymbols[pApp->d_WordWrapSymbols].flags, 0);
		}
		else
		{
			m_doc.SendMessage(SCI_SETWRAPVISUALFLAGS, 0, 0);
		}
		break;


	case IDM_VIEW_MATCHBRACES:
		pApp->d_bMatchBraces = !pApp->d_bMatchBraces;
		if (!pApp->d_bMatchBraces)
			m_doc.SendMessage(SCI_BRACEHIGHLIGHT, -1, -1);
		break;


	case IDM_VIEW_AUTOCLOSETAGS:
		pApp->d_bAutoCloseTags = !pApp->d_bAutoCloseTags;
		break;


	case IDM_VIEW_ZOOMIN:
		m_doc.SendMessage(SCI_ZOOMIN, 0, 0);
		break;


	case IDM_VIEW_ZOOMOUT:
		m_doc.SendMessage(SCI_ZOOMOUT, 0, 0);
		break;


	case IDM_VIEW_RESETZOOM:
		m_doc.SendMessage(SCI_SETZOOM, 0, 0);
		break;


	case IDM_VIEW_TOOLBAR:
		pApp->d_bShowToolbar = !pApp->d_bShowToolbar;
		if (!pApp->d_bShowToolbar)
		{
			::ShowWindow(m_sizebox(), SW_HIDE);
		}
		else
		{
			::ShowWindow(m_sizebox(), SW_SHOW);
			UpdateToolbar();
		}
		SendWMSize();
		break;


	case IDM_VIEW_CUSTOMIZETB:
		m_toolbar.SendMessage(TB_CUSTOMIZE, 0, 0);
		break;


	case IDM_VIEW_STATUSBAR:
		pApp->d_bShowStatusbar = !pApp->d_bShowStatusbar;
		if (!pApp->d_bShowStatusbar)
		{
			::ShowWindow(m_status(), SW_HIDE);
		}
		else
		{
			::ShowWindow(m_status(), SW_SHOW);
			UpdateStatusbar();
		}
		SendWMSize();
		break;

	case IDM_SETTING_FILEASSOCIATION:
		{
			wnd::FileAssoc dlg;
			dlg.DoModal(m_hWnd);
		}
		break;

	case IDM_TOOLS_SETTINGS:
		{
			wnd::PropGeneral page1(IDD_PROP_GENERAL);
			wnd::PropEditor page2(IDD_PROP_EDITOR);
			wpf::CPropertySheet dlg;
			dlg.AddPage(&page1);
			dlg.AddPage(&page2);
			dlg.DoModal(m_hWnd, 210, 150, ResStr(IDS_SETTING), ResStr(IDS_OK), ResStr(IDS_CANCEL));
		}
		break;

	case IDM_IMPORT:
		{
			wnd::ImportExport dlg(false);
			dlg.DoModal(m_hWnd);
		}
		break;

	case IDM_EXPORT:
		{
			wnd::ImportExport dlg(true);
			dlg.DoModal(m_hWnd);
		}
		break;

	case IDM_HELP_ABOUT:
		{
			wnd::About dlg;
			dlg.DoModal(m_hWnd);
		}
		break;

	case IDM_HELP_CMD:
		wnd::DisplayCmdLineHelp();
		break;

	case IDM_HELP_CHARCOUNT:
		OnCharCount();
		break;


	case CMD_ESCAPE:
		if (wnd::FindReplace::GetWndStatic() && wnd::FindReplace::GetWndStatic() == GetActiveWindow())
		{
			::DestroyWindow(wnd::FindReplace::GetWndStatic());
		}
		else
		{
			if (pApp->d_EscFunction == 1)
				SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
			else if (pApp->d_EscFunction == 2)
				SendMessage(WM_CLOSE, 0, 0);
		}
		break;


		// Newline with toggled auto indent setting
	case CMD_CTRLENTER:
		pApp->d_bAutoIndent = !pApp->d_bAutoIndent;
		m_doc.SendMessage(SCI_NEWLINE, 0, 0);
		pApp->d_bAutoIndent = !pApp->d_bAutoIndent;
		break;


	case CMD_LEXDEFAULT:
		m_doc.SetLexer(pApp->Lexer.GetDefault());
		UpdateStatusbar();
		UpdateLineNumberWidth();
		break;


	case CMD_LEXHTML:
		m_doc.SetHTMLLexer();
		UpdateStatusbar();
		UpdateLineNumberWidth();
		break;


	case CMD_LEXXML:
		m_doc.SetXMLLexer();
		UpdateStatusbar();
		UpdateLineNumberWidth();
		break;


	case IDT_FILE_NEW:
		this->DoToolBarCmd(IDM_FILE_NEW);
		break;


	case IDT_FILE_OPEN:
		this->DoToolBarCmd(IDM_FILE_OPEN);
		break;


	case IDT_VIEW_ALWAYSONTOP:
		this->SetOnTop();
		break;


	case IDT_FILE_SAVE:
		this->DoToolBarCmd(IDM_FILE_SAVE);
		break;


	case IDT_EDIT_UNDO:
		this->DoToolBarCmd(IDM_EDIT_UNDO);
		break;


	case IDT_EDIT_REDO:
		this->DoToolBarCmd(IDM_EDIT_REDO);
		break;


	case IDT_EDIT_CUT:
		this->DoToolBarCmd(IDM_EDIT_CUT);
		break;


	case IDT_EDIT_COPY:
		this->DoToolBarCmd(IDM_EDIT_COPY);
		break;


	case IDT_EDIT_PASTE:
		this->DoToolBarCmd(IDM_EDIT_PASTE);
		break;


	case IDT_EDIT_FIND:
		this->DoToolBarCmd(IDM_EDIT_FIND);
		break;


	case IDT_EDIT_REPLACE:
		this->DoToolBarCmd(IDM_EDIT_REPLACE);
		break;


	case IDT_VIEW_WORDWRAP:
		this->DoToolBarCmd(IDM_VIEW_WORDWRAP);
		break;


	case IDT_VIEW_ZOOMIN:
		this->DoToolBarCmd(IDM_VIEW_ZOOMIN);
		break;


	case IDT_VIEW_ZOOMOUT:
		this->DoToolBarCmd(IDM_VIEW_ZOOMOUT);
		break;


	case IDT_VIEW_SCHEME:
		this->DoToolBarCmd(IDM_VIEW_SCHEME);
		break;


	case IDT_VIEW_SCHEMECONFIG:
		this->DoToolBarCmd(IDM_VIEW_SCHEMECONFIG);
		break;


	case IDT_FILE_EXIT:
		SendMessage(WM_CLOSE, 0, 0);
		break;


	case IDT_FILE_SAVEAS:
		this->DoToolBarCmd(IDM_FILE_SAVEAS);
		break;


	case IDT_FILE_SAVECOPY:
		this->DoToolBarCmd(IDM_FILE_SAVECOPY);
		break;


	case IDT_EDIT_COPYALL:
		this->DoToolBarCmd(IDM_EDIT_COPYALL);
		break;


	case IDT_EDIT_CLEAR:
		this->DoToolBarCmd(IDM_EDIT_CLEAR);
		break;


	case IDT_EDIT_FINDNEXT:
		this->DoToolBarCmd(IDM_EDIT_FINDNEXT);
		break;

	case IDT_EDIT_FINDPREV:
		this->DoToolBarCmd(IDM_EDIT_FINDPREV);
		break;


	case IDT_FILE_PRINT:
		this->DoToolBarCmd(IDM_FILE_PRINT);
		break;


	case IDT_FILE_OPENFAV:
		this->DoToolBarCmd(IDM_FILE_OPENFAV);
		break;


	case IDT_FILE_ADDTOFAV:
		this->DoToolBarCmd(IDM_FILE_ADDTOFAV);
		break;

	}

	return 0;
}

LRESULT CView::OnContextMenu(WPARAM wParam, LPARAM lParam)
{
	int nID = GetDlgCtrlID((HWND)wParam);

	if ((nID != IDC_EDIT) && (nID != IDC_STATUSBAR) &&
			(nID != IDC_REBAR) && (nID != IDC_TOOLBAR))
		return DefWindowProc(m_hWnd, WM_CONTEXTMENU, wParam, lParam);

	HMENU hmenu = LoadMenu(WpfGetInstance(), MAKEINTRESOURCE(IDR_POPUPMENU));
	int imenu = 0;

	POINT pt;
	pt.x = (int)(short)LOWORD(lParam);
	pt.y = (int)(short)HIWORD(lParam);

	switch (nID)
	{
	case IDC_EDIT:
		{
			int iSelStart = m_doc.SendMessage(sci::GETSELECTIONSTART);
			int iSelEnd = m_doc.SendMessage(sci::GETSELECTIONEND);
			int nLexer = m_doc.SendMessage(sci::GETLEXER);

			if (iSelEnd > iSelStart) {
				HMENU sub = GetSubMenu(hmenu, 0);
				switch (nLexer)
				{
				case SCLEX_CPP:
					AppendMenu(sub, MF_SEPARATOR, 0, 0);
					AppendMenu(sub, MF_STRING, IDM_CPP_COMMENT, ResStr(IDS_COMMENT));
					AppendMenu(sub, MF_STRING, IDM_CPP_UNCOMMENT, ResStr(IDS_UNCOMMENT));
					break;
				case SCLEX_NSIS:
					AppendMenu(sub, MF_SEPARATOR, 0, 0);
					AppendMenu(sub, MF_STRING, IDM_NSIS_COMMENT, ResStr(IDS_COMMENT));
					AppendMenu(sub, MF_STRING, IDM_NSIS_UNCOMMENT, ResStr(IDS_UNCOMMENT));
					break;
				case SCLEX_BATCH:
					AppendMenu(sub, MF_SEPARATOR, 0, 0);
					AppendMenu(sub, MF_STRING, IDM_BAT_COMMENT, ResStr(IDS_COMMENT));
					AppendMenu(sub, MF_STRING, IDM_BAT_UNCOMMENT, ResStr(IDS_UNCOMMENT));
					break;
				case SCLEX_PYTHON:
					AppendMenu(sub, MF_SEPARATOR, 0, 0);
					AppendMenu(sub, MF_STRING, IDM_PY_COMMENT, ResStr(IDS_COMMENT));
					AppendMenu(sub, MF_STRING, IDM_PY_UNCOMMENT, ResStr(IDS_UNCOMMENT));
					break;
				case SCLEX_RUBY:
					AppendMenu(sub, MF_SEPARATOR, 0, 0);
					AppendMenu(sub, MF_STRING, IDM_RB_COMMENT, ResStr(IDS_COMMENT));
					AppendMenu(sub, MF_STRING, IDM_RB_UNCOMMENT, ResStr(IDS_UNCOMMENT));
					break;
				}
			}

			if (iSelStart == iSelEnd && pt.x != -1 && pt.y != -1)
			{
				int iNewPos;
				POINT ptc = { pt.x, pt.y };
				ScreenToClient(m_doc(), &ptc);
				iNewPos = m_doc.SendMessage(SCI_POSITIONFROMPOINT, ptc.x, ptc.y);
				m_doc.SendMessage(SCI_GOTOPOS, iNewPos);
			}

			if (pt.x == -1 && pt.y == -1)
			{
				int iCurrentPos = m_doc.SendMessage(SCI_GETCURRENTPOS);
				pt.x = m_doc.SendMessage(SCI_POINTXFROMPOSITION, 0, iCurrentPos);
				pt.y = m_doc.SendMessage(SCI_POINTYFROMPOSITION, 0, iCurrentPos);
				ClientToScreen(m_doc(), &pt);
			}
			imenu = 0;
		}
		break;

	case IDC_TOOLBAR:
	case IDC_STATUSBAR:
	case IDC_REBAR:
		if (pt.x == -1 && pt.y == -1)
			GetCursorPos(&pt);
		imenu = 1;
		break;
	}

	TrackPopupMenuEx(GetSubMenu(hmenu, imenu),
					 TPM_LEFTBUTTON | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd, NULL);

	DestroyMenu(hmenu);
	return 0;
}

LRESULT CView::OnCompileNSIS(WPARAM wParam, LPARAM lParam)
{
	SaveFile(false, true, false, false);
	if (pApp->CurFile.empty())
		return 0;

	LONG lRes;
	CReg reg;
	tstring makensis;
	lRes = reg.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\NSIS"));
	if (CReg::SUCCESS == lRes) {
		ULONG nChar;
		lRes = reg.QueryStringValue(_T(""), NULL, &nChar);
		if (nChar > 0) {
			makensis.assign(nChar + 1, 0);
			lRes = reg.QueryStringValue(_T(""), (tchar *)makensis.c_str(), &nChar);
			util::trim_right(makensis, '\0');
			makensis += _T("\\makensis.exe");
		}
	}

	if (CReg::SUCCESS != lRes || !FileExists(makensis.c_str())) {
		tstring filter;
		api::LoadStringT(filter, IDS_FILTER_EXE);
		filter += ResStr(IDS_FILTER_ALL);
		filter += '\n';
		FixMutliString(filter);

		if (!api::GetOpenFileNameT(makensis, m_hWnd, filter.c_str(), _T("makensis.exe")))
			return 0;
	}

	if (FileExists(makensis.c_str())) {
		wnd::CompileNSIS dlg(makensis.c_str());
		dlg.DoModal(m_hWnd);
	}

	return 0;
}


void CView::OnCharCount()
{
	unsigned int ascii = 0, wide = 0, cr = 0, lf = 0, other = 0;

	size_t len = m_doc.SendMessage(sci::GETLENGTH);
	if (len > 0)
	{
		util::memory<TCHAR> mem;
		TCHAR * text = mem.alloc(len + 1);
		m_doc.SendMessage(sci::GETTEXT, len + 1, text);
		text[len] = 0;

		TCHAR * prev = text, * cur = text;

		cur = CharNext(cur);
		while (*prev)
		{
			if ((BYTE *)cur - (BYTE *)prev == sizeof(char))
				if ('\r' == *prev)
					cr++;
				else if ('\n' == *prev)
					lf++;
				else
					ascii++;
			else if ((BYTE *)cur - (BYTE *)prev == sizeof(WCHAR)) {
				if (*reinterpret_cast<WCHAR *>(prev) < 0x80)
					if ('\r' == *prev)
						cr++;
					else if ('\n' == *prev)
						lf++;
					else
						ascii++;
				else
					wide++;
			}
			else
				other++;

			prev = cur;
			cur = CharNext(cur);
		}
	}

	tstring str;
	api::LoadStringT(str, IDS_CHARCOUNT);
	tchar buf[128];
	tstring::size_type pos = 0;

	_stprintf(buf, _T("%u"), ascii);
	util::replace_once(str, _T("%u"), FormatNumberStr(buf), pos);

	_stprintf(buf, _T("%u"), wide);
	util::replace_once(str, _T("%u"), FormatNumberStr(buf), pos);

	_stprintf(buf, _T("%u"), cr);
	util::replace_once(str, _T("%u"), FormatNumberStr(buf), pos);

	_stprintf(buf, _T("%u"), lf);
	util::replace_once(str, _T("%u"), FormatNumberStr(buf), pos);

	_stprintf(buf, _T("%u"), other);
	util::replace_once(str, _T("%u"), FormatNumberStr(buf), pos);

	_stprintf(buf, _T("%u"), ascii + wide + cr + lf + other);
	util::replace_once(str, _T("%u"), FormatNumberStr(buf), pos);

	this->MessageBox(str.c_str(), MB_OK|MB_ICONINFORMATION);
	//this->MessageBox(format(ResStr(IDS_CHARCOUNT), ascii, wide, cr, lf, other, 
	//	ascii + wide + cr + lf + other), MB_OK|MB_ICONINFORMATION);
}

void CView::OnInitMenu(WPARAM wParam, LPARAM lParam, bool bContextMenu)
{
	HMENU hmenu = (HMENU)wParam;
	int nSel = m_doc.SendMessage(sci::GETSELECTIONEND) - m_doc.SendMessage(sci::GETSELECTIONSTART);
	int nTextLen = m_doc.SendMessage(sci::GETLENGTH);
	int nLexer = m_doc.SendMessage(sci::GETLEXER);

	EnableCmd(hmenu, IDM_FILE_REVERT, !pApp->CurFile.empty());
	EnableCmd(hmenu, IDM_FILE_LAUNCH, !pApp->CurFile.empty());
	EnableCmd(hmenu, IDM_FILE_PROPERTIES, !pApp->CurFile.empty());
	EnableCmd(hmenu, IDM_FILE_CREATELINK, !pApp->CurFile.empty());
	EnableCmd(hmenu, IDM_FILE_ADDTOFAV, !pApp->CurFile.empty());

	EnableCmd(hmenu, IDM_FILE_READONLY, !pApp->CurFile.empty());
	CheckCmd(hmenu, IDM_FILE_READONLY, pApp->f_bReadOnly);

	EnableCmd(hmenu, IDM_RELOAD_UNICODE_BE, !pApp->CurFile.empty() && pApp->CurFile.enc != ENC_UNICODE_BE);
	EnableCmd(hmenu, IDM_RELOAD_UNICODE_LE, !pApp->CurFile.empty() && pApp->CurFile.enc != ENC_UNICODE_LE);
	EnableCmd(hmenu, IDM_RELOAD_UTF8, !pApp->CurFile.empty() && pApp->CurFile.enc != ENC_UTF8);
	EnableCmd(hmenu, IDM_RELOAD_ANSI, !pApp->CurFile.empty() && pApp->CurFile.enc != ENC_ANSI);

	EnableCmd(hmenu, IDM_ENCODING_UNICODE_BE, pApp->CurFile.enc != ENC_UNICODE_BE && SCLEX_ASCII != pApp->Lexer.GetCurLexer()->iLexer);
	EnableCmd(hmenu, IDM_ENCODING_UNICODE_LE, pApp->CurFile.enc != ENC_UNICODE_LE && SCLEX_ASCII != pApp->Lexer.GetCurLexer()->iLexer);
	EnableCmd(hmenu, IDM_ENCODING_UTF8, pApp->CurFile.enc != ENC_UTF8 && SCLEX_ASCII != pApp->Lexer.GetCurLexer()->iLexer);
	EnableCmd(hmenu, IDM_ENCODING_ANSI, pApp->CurFile.enc != ENC_ANSI && SCLEX_ASCII != pApp->Lexer.GetCurLexer()->iLexer);

	CheckMenuRadioItem(hmenu, IDM_ENCODING_ANSI, IDM_ENCODING_UTF8, IDM_ENCODING_ANSI + pApp->CurFile.enc - ENC_ANSI, MF_BYCOMMAND);
	CheckMenuRadioItem(hmenu, IDM_LINEENDINGS_CRLF, IDM_LINEENDINGS_LF, IDM_LINEENDINGS_CRLF + pApp->CurFile.eol - SC_EOL_CRLF, MF_BYCOMMAND);

	EnableCmd(hmenu, IDM_FILE_RECENT, m_mru.Size() > 0);

	EnableCmd(hmenu, IDM_EDIT_UNDO, m_doc.SendMessage(SCI_CANUNDO, 0, 0));
	EnableCmd(hmenu, IDM_EDIT_REDO, m_doc.SendMessage(SCI_CANREDO, 0, 0));

	EnableCmd(hmenu, IDM_EDIT_CUT, nSel);
	EnableCmd(hmenu, IDM_EDIT_COPY, nSel);

	EnableCmd(hmenu, IDM_EDIT_COPYALL, m_doc.SendMessage(SCI_GETLENGTH, 0, 0));
	EnableCmd(hmenu, IDM_EDIT_PASTE, m_doc.SendMessage(SCI_CANPASTE, 0, 0));
	EnableCmd(hmenu, IDM_EDIT_CLEAR, nSel);
	EnableCmd(hmenu, IDM_EDIT_CLEARALL, m_doc.SendMessage(SCI_GETLENGTH, 0, 0));

	EnableCmd(hmenu, IDM_EDIT_SPLITLINES, nSel);
	EnableCmd(hmenu, IDM_EDIT_JOINLINES, nSel);

	EnableCmd(hmenu, IDM_EDIT_CONVERTUPPERCASE, nSel);
	EnableCmd(hmenu, IDM_EDIT_CONVERTLOWERCASE, nSel);

	EnableCmd(hmenu, IDM_EDIT_CONVERTTABS, nSel);
	EnableCmd(hmenu, IDM_EDIT_CONVERTSPACES, nSel);

	EnableCmd(hmenu, IDM_EDIT_CONVERTOEM, nSel);
	EnableCmd(hmenu, IDM_EDIT_CONVERTANSI, nSel);

#ifndef _DEBUG
	EnableCmd(hmenu, IDM_EDIT_FIND, nTextLen);
	EnableCmd(hmenu, IDM_EDIT_FINDNEXT, nTextLen);
	EnableCmd(hmenu, IDM_EDIT_FINDPREV, nTextLen);
	EnableCmd(hmenu, IDM_EDIT_REPLACE, nTextLen);
	EnableCmd(hmenu, IDM_EDIT_COLLECT, nTextLen);
#endif

	CheckCmd(hmenu, IDM_VIEW_WORDWRAP, pApp->d_WordWrap);
	CheckCmd(hmenu, IDM_VIEW_LONGLINEMARKER, pApp->d_bMarkLongLines);
	CheckCmd(hmenu, IDM_VIEW_SHOWINDENTGUIDES, pApp->d_bShowIndentGuides);
	CheckCmd(hmenu, IDM_VIEW_AUTOINDENTTEXT, pApp->d_bAutoIndent);
	CheckCmd(hmenu, IDM_VIEW_LINENUMBERS, pApp->d_bShowLineNumbers);
	CheckCmd(hmenu, IDM_VIEW_MARGIN, pApp->d_bShowSelectionMargin);
	CheckCmd(hmenu, IDM_VIEW_SHOWWHITESPACE, pApp->d_bViewWhiteSpace);
	CheckCmd(hmenu, IDM_VIEW_SHOWEOLS, pApp->d_bViewEOLs);
	CheckCmd(hmenu, IDM_VIEW_WORDWRAPSYMBOLS, pApp->d_bShowWordWrapSymbols);
	CheckCmd(hmenu, IDM_VIEW_MATCHBRACES, pApp->d_bMatchBraces);
	CheckCmd(hmenu, IDM_VIEW_TOOLBAR, pApp->d_bShowToolbar);
	EnableCmd(hmenu, IDM_VIEW_CUSTOMIZETB, pApp->d_bShowToolbar);
	CheckCmd(hmenu, IDM_VIEW_STATUSBAR, pApp->d_bShowStatusbar);

	EnableCmd(hmenu, IDM_VIEW_AUTOCLOSETAGS, (nLexer == SCLEX_HTML || nLexer == SCLEX_XML));
	CheckCmd(hmenu, IDM_VIEW_AUTOCLOSETAGS, pApp->d_bAutoCloseTags && (nLexer == SCLEX_HTML || nLexer == SCLEX_XML));

	EnableCmd(hmenu, IDM_CPP_COMMENT, SCLEX_CPP == nLexer && nSel);
	EnableCmd(hmenu, IDM_CPP_UNCOMMENT, SCLEX_CPP == nLexer && nSel);
	EnableCmd(hmenu, IDM_CPP_QFORMAT, SCLEX_CPP == nLexer && nSel);
	EnableCmd(hmenu, IDM_CPP_FORMAT, SCLEX_CPP == nLexer && nSel);
	EnableCmd(hmenu, IDM_BAT_COMMENT, SCLEX_BATCH == nLexer && nSel);
	EnableCmd(hmenu, IDM_BAT_UNCOMMENT, SCLEX_BATCH == nLexer && nSel);
	EnableCmd(hmenu, IDM_NSIS_COMMENT, SCLEX_NSIS == nLexer && nSel);
	EnableCmd(hmenu, IDM_NSIS_COMMENT2, SCLEX_NSIS == nLexer && nSel);
	EnableCmd(hmenu, IDM_NSIS_UNCOMMENT, SCLEX_NSIS == nLexer && nSel);
	EnableCmd(hmenu, IDM_NSIS_COMPILE, SCLEX_NSIS == nLexer && nTextLen);
	EnableCmd(hmenu, IDM_PY_COMMENT, SCLEX_PYTHON == nLexer && nSel);
	EnableCmd(hmenu, IDM_PY_UNCOMMENT, SCLEX_PYTHON == nLexer && nSel);
	EnableCmd(hmenu, IDM_RB_COMMENT, SCLEX_RUBY == nLexer && nSel);
	EnableCmd(hmenu, IDM_RB_UNCOMMENT, SCLEX_RUBY == nLexer && nSel);

	if (bContextMenu)
		return;

	{ // setup MRU files
		if (!m_menu_mru.IsMenu()) {
			m_menu_mru.CreatePopupMenu();
			tstring str;
			api::GetMenuStringT(str, hmenu, IDM_RECENT_FILE, MF_BYCOMMAND);
			::ModifyMenu(hmenu, IDM_RECENT_FILE, MF_BYCOMMAND | MF_POPUP | MF_STRING,
				(UINT)m_menu_mru.GetHandler(), str.c_str());
		}
		else
			while (m_menu_mru.RemoveMenu(0, MF_BYPOSITION));

		for (int i = (int)m_mru.Size() - 1; i >= 0; i--)
		{
			m_menu_mru.AppendMenu(MF_STRING, RECENT_FILE_START + i, m_mru[i]);
		}
	}

	// setup codepage
	if (TRUE == InterlockedCompareExchange((LONG *)&m_bEnumCodepageThreadFinished, TRUE, TRUE))
	{
		if (!m_menu_codepage.IsMenu()) {
			m_menu_codepage.CreatePopupMenu();
			tstring str;
			api::GetMenuStringT(str, hmenu, IDM_CODEPAGE, MF_BYCOMMAND);
			::ModifyMenu(hmenu, IDM_CODEPAGE, MF_BYCOMMAND | MF_POPUP | MF_STRING,
				(UINT)m_menu_codepage.GetHandler(), str.c_str());
		}
		else
			while (m_menu_codepage.RemoveMenu(0, MF_BYPOSITION));

		UINT cp = GetACP();
		for (int i = (int)pApp->Codepage.size() - 1; i >= 0; i--)
		{
			UINT uFlags = MF_STRING;
			uFlags |= nTextLen ? MF_ENABLED : MF_GRAYED;
			if (cp == util::xtoi(util::get_before(pApp->Codepage[i], ' ').c_str())) {
				uFlags |= MF_GRAYED;
			}
			m_menu_codepage.AppendMenu(uFlags, CODEPAGE_START + i, pApp->Codepage[i].c_str());
		}
	}

	// setup format scheme
	if (pApp->Formatting.size() > 0) {
		if (!m_menu_scheme.IsMenu()) {
			m_menu_scheme.CreatePopupMenu();
			tstring str;
			api::GetMenuStringT(str, hmenu, IDM_FORMAT_SCHEME, MF_BYCOMMAND);
			::ModifyMenu(hmenu, IDM_FORMAT_SCHEME, MF_BYCOMMAND | MF_POPUP | MF_STRING,
				(UINT)m_menu_scheme.GetHandler(), str.c_str());
		}
		else
			while (m_menu_scheme.RemoveMenu(0, MF_BYPOSITION));

		for (int i=0; i<(int)pApp->Formatting.size(); i++)
		{
			if (pApp->Formatting[i].name.empty())
				continue;

			UINT uFlags = MF_STRING;
			//enable for cpp/c/h/hpp.../c#/java  and select a block
			uFlags |= (SCLEX_CPP == nLexer && nSel) ? MF_ENABLED : MF_GRAYED;
			m_menu_scheme.AppendMenu(uFlags, FORMAT_SCHEME_START + i, pApp->Formatting[i].name.c_str());
		}
	}

	if (pApp->Editor.size() > 0)
	{
		int pos = 0;
		HMENU h = FindMenu(GetSubMenu(hmenu, SUBM_FILE), IDM_OPENWITH_CONFIG, pos);
		if (h)
		{
			while (RemoveMenu(h, 1, MF_BYPOSITION));

			AppendMenu(h, MF_SEPARATOR, 0, 0);
			for (unsigned int i=0; i<pApp->Editor.size(); i++)
			{
				AppendMenu(h, MF_BYCOMMAND|MF_STRING,
					OPENWITH_START + i, pApp->Editor[i].first.c_str());
			}
		}
	}

}

LRESULT CView::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pnmh = (LPNMHDR)lParam;
	SCNotification * scn = (SCNotification *)lParam;

	switch (pnmh->idFrom)
	{

	case IDC_EDIT:

		switch (pnmh->code)
		{
		case SCN_UPDATEUI:
			UpdateToolbar();
			UpdateStatusbar();

			// Brace Match
			if (pApp->d_bMatchBraces)
			{
				int iPos = m_doc.SendMessage(SCI_GETCURRENTPOS, 0, 0);
				tchar c = (tchar)m_doc.SendMessage(SCI_GETCHARAT, iPos, 0);
				if (_tcschr(_T("()[]{}"), c))
				{
					int iBrace2 = m_doc.SendMessage(SCI_BRACEMATCH, iPos, 0);
					if (iBrace2 != -1)
						m_doc.SendMessage(SCI_BRACEHIGHLIGHT, iPos, iBrace2);
					else
						m_doc.SendMessage(SCI_BRACEBADLIGHT, iPos, 0);
				}
				// Try one before
				else
				{
					iPos = m_doc.SendMessage(SCI_POSITIONBEFORE, iPos, 0);
					c = (tchar)m_doc.SendMessage(SCI_GETCHARAT, iPos, 0);
					if (_tcschr(_T("()[]{}"), c))
					{
						int iBrace2 = m_doc.SendMessage(SCI_BRACEMATCH, iPos, 0);
						if (iBrace2 != -1)
							m_doc.SendMessage(SCI_BRACEHIGHLIGHT, iPos, iBrace2);
						else
							m_doc.SendMessage(SCI_BRACEBADLIGHT, iPos, 0);
					}
					else
						m_doc.SendMessage(SCI_BRACEHIGHLIGHT, -1, -1);
				}
			}
			break;

		case SCN_CHARADDED:
			// Auto indent
			if (pApp->d_bAutoIndent && (scn->ch == '\x0D' || scn->ch == '\x0A'))
			{
				// in CRLF mode handle LF only...
				if ((SC_EOL_CRLF == pApp->CurFile.eol && scn->ch != '\x0A') || SC_EOL_CRLF != pApp->CurFile.eol)
				{
					int iCurPos = m_doc.SendMessage(SCI_GETCURRENTPOS);
					int iCurLine = m_doc.SendMessage(SCI_LINEFROMPOSITION, iCurPos);
					int iLineLength = m_doc.SendMessage(SCI_LINELENGTH, iCurLine);

					if (iCurLine > 0 /* && iLineLength <= 2*/)
					{
						util::memory<tchar> buf;
						int iPrevLineLength = m_doc.SendMessage(SCI_LINELENGTH, iCurLine - 1, 0);
						buf.alloc(iPrevLineLength + 1);
						m_doc.SendMessage(SCI_GETLINE, iCurLine - 1, buf.ptr());
						buf[iPrevLineLength] = 0;
						for (tchar * pPos = buf.ptr(); *pPos; pPos++)
						{
							if (*pPos != ' ' && *pPos != '\t')
								*pPos = '\0';
						}
						if (buf[0])
						{
							m_doc.SendMessage(SCI_BEGINUNDOACTION, 0, 0);
							m_doc.SendMessage(SCI_ADDTEXT, _tcslen(buf.ptr()), buf.ptr());
							m_doc.SendMessage(SCI_ENDUNDOACTION, 0, 0);
						}
					}
				}
			}
			// Auto close tags
			else if (pApp->d_bAutoCloseTags && scn->ch == '>')
			{
				int iLexer = m_doc.SendMessage(sci::GETLEXER);
				if (iLexer == SCLEX_HTML || iLexer == SCLEX_XML)
				{
					tchar tchBuf[512];
					tchar tchIns[516] = _T("</");
					int cchIns = 2;
					int iCurPos = m_doc.SendMessage(SCI_GETCURRENTPOS, 0, 0);
					int iHelper = iCurPos - (countof(tchBuf) - 1);
					int iStartPos = util::max(0, iHelper);
					int iSize = iCurPos - iStartPos;

					if (iSize >= 3)
					{
						TextRange tr = { { iStartPos, iCurPos } , tchBuf };
						m_doc.SendMessage(SCI_GETTEXTRANGE, 0, &tr);

						if (tchBuf[iSize - 2] != '/')
						{
							const tchar * pBegin = &tchBuf[0];
							const tchar * pCur = &tchBuf[iSize - 2];

							while (pCur > pBegin && *pCur != '<' && *pCur != '>')
								--pCur;

							if (*pCur == '<')
							{
								pCur++;
								while (_tcschr(_T(":_-."), *pCur) || IsCharAlphaNumeric(*pCur))
								{
									tchIns[cchIns++] = *pCur;
									pCur++;
								}
							}

							tchIns[cchIns++] = '>';
							tchIns[cchIns] = 0;

							if (cchIns > 3)
							{
								m_doc.SendMessage(SCI_BEGINUNDOACTION);
								m_doc.SendMessage(SCI_REPLACESEL, 0, tchIns);
								m_doc.SendMessage(SCI_SETSEL, iCurPos, iCurPos);
								m_doc.SendMessage(SCI_ENDUNDOACTION);
							}
						}
					}
				}
			}
			break;

		case SCN_MODIFIED:
		case SCN_ZOOM:
			UpdateLineNumberWidth();
			break;

		case SCN_SAVEPOINTREACHED:
			pApp->f_bModified = false;
			SetTitle(pApp->d_bShortPathNames, pApp->f_bModified, pApp->f_bReadOnly);
			break;

		case SCN_SAVEPOINTLEFT:
			pApp->f_bModified = true;
			SetTitle(pApp->d_bShortPathNames, pApp->f_bModified, pApp->f_bReadOnly);
			break;
		}
		break;


	case IDC_TOOLBAR:

		switch (pnmh->code)
		{

		case TBN_ENDADJUST:
			UpdateToolbar();
			break;

		case TBN_QUERYDELETE:
		case TBN_QUERYINSERT:
			return TRUE;

		case TBN_GETBUTTONINFO:
			return m_toolbar.GetButtonInfo((NMTOOLBAR *)lParam);

		case TBN_RESET:
			m_toolbar.Reset();
			return 0;

		}
		break;


	case IDC_STATUSBAR:

		switch (pnmh->code)
		{

		case NM_DBLCLK:
			{
				switch (((LPNMMOUSE)lParam)->dwItemSpec)
				{
				case CStatusBar::ST_LEXER:
					SendMessage(WM_COMMAND, IDM_VIEW_SCHEME, 0);
					return TRUE;

				case CStatusBar::ST_OVRMODE:
					m_doc.SendMessage(SCI_EDITTOGGLEOVERTYPE, 0, 0);
					return TRUE;

				default:
					return FALSE;
				}
			}
			break;

		}
		break;


	default:

		switch (pnmh->code)
		{

		case TTN_NEEDTEXT:
			if (((LPTOOLTIPTEXT)lParam)->uFlags != TTF_IDISHWND)
			{
				tstring tmp;
				api::LoadStringT(tmp, pnmh->idFrom);
				_tcsncpy(((LPTOOLTIPTEXT)lParam)->szText, tmp.c_str(), 80);
			}
			break;

		}
		break;

	}

	return 0;
}

LRESULT CView::OnOpenWith(WPARAM wParam, LPARAM lParam)
{
	int idx = LOWORD(wParam) - OPENWITH_START;
	ASSERT(idx >= 0);
	ASSERT(idx < (int)pApp->Editor.size());

	if (pApp->d_bSaveBeforeRunningTools && !SaveFile(false, true, false, false))
		return 0;

	tstring cmd;
	cmd.reserve(pApp->Editor[idx].second.length() + pApp->CurFile.length() + 10);
	cmd = pApp->Editor[idx].second;
	cmd += _T(" \"");
	cmd += pApp->CurFile.c_str();
	cmd += '\"';
	api::CreateProcessT(cmd.c_str());

	return 0;
}

void CView::OnSize(WPARAM wParam, LPARAM lParam)
{
	RECT rc;

	if (wParam == SIZE_MINIMIZED)
		return ;

	int x = 0;
	int y = 0;

	int cx = LOWORD(lParam);
	int cy = HIWORD(lParam);

	if (pApp->d_bShowToolbar)
	{
		int cySizeBox = m_sizebox.GetHeight();
		/*  SendMessage(hwndToolbar,WM_SIZE,0,0);
		    GetWindowRect(hwndToolbar,&rc);
		    y = (rc.bottom - rc.top);
		    cy -= (rc.bottom - rc.top);*/

		//SendMessage(hwndToolbar,TB_GETITEMRECT,0,(LPARAM)&rc);
		m_sizebox.SetWindowPos(NULL, 0, 0, LOWORD(lParam), cySizeBox, SWP_NOZORDER);
		// the ReBar automatically sets the correct height
		// calling SetWindowPos() with the height of one toolbar button
		// causes the control not to temporarily use the whole client area
		// and prevents flickering

		//GetWindowRect(hwndReBar,&rc);
		y = cySizeBox; // + cyReBarFrame;    // define
		cy -= cySizeBox; // + cyReBarFrame;  // border
	}

	if (pApp->d_bShowStatusbar)
	{
		m_status.SendMessage(WM_SIZE, 0, 0);
		::GetWindowRect(m_status(), &rc);
		cy -= (rc.bottom - rc.top);
	}

	HDWP hdwp = BeginDeferWindowPos(2);

	DeferWindowPos(hdwp, m_docframe(), NULL, x, y, cx, cy,
				   SWP_NOZORDER | SWP_NOACTIVATE);

	DeferWindowPos(hdwp, m_doc(), NULL, x + pApp->f_cxEditFrame, y + pApp->f_cyEditFrame,
				   cx - 2 * pApp->f_cxEditFrame, cy - 2 * pApp->f_cyEditFrame, SWP_NOZORDER | SWP_NOACTIVATE);

	EndDeferWindowPos(hdwp);

	// Statusbar width
	int aWidth[6];
	aWidth[0] = util::max(120, util::min(cx / 3, 
		m_status.CalcPaneWidth(_T("Ln 9'999'999 : 9'999'999   Col 9'999'999   Sel 9'999'999"))));
	aWidth[1] = aWidth[0] + m_status.CalcPaneWidth(_T("9'999'999 Bytes"));
	aWidth[2] = aWidth[1] + m_status.CalcPaneWidth(_T("Unicode BE"));
	aWidth[3] = aWidth[2] + m_status.CalcPaneWidth(_T("CR+LF"));
	aWidth[4] = aWidth[3] + m_status.CalcPaneWidth(_T("OVR"));
	aWidth[5] = -1;

	m_status.SendMessage(SB_SETPARTS, countof(aWidth), aWidth);
}

bool CView::LoadFile(bool bDontSave, bool bNew, int Enc, const TCHAR * lpszFile)
{
	if (!bDontSave)
	{
		if (!SaveFile(false, true, false, false))
			return false;
	}

	if (bNew)
	{
		pApp->CurFile.clear();
		SetNewFileState();
		return true;
	}

	CFileInformation fi;
	if (!lpszFile || !lpszFile[0])
	{
		const util::ptstring pfilter = CreateFullFilter(true);
		wpf::CFileDialog dlg(true, m_hWnd, pfilter->c_str(), NULL, NULL);
		if (!dlg.DoModal())
			return false;
		fi.attach(dlg.GetFileName());
	}
	else
	{
		fi.attach(lpszFile);
	}

	fi.get_fullpath();
	fi.eol = pApp->d_iDefaultEOL;

	// Ask to create a new file...
	if (fi.bad())
	{
		if (MessageBox(util::format(ResStr(IDS_ASK_CREATE), fi.c_str()),
			MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
		{
			Reader reader;
			if (reader.open(fi.c_str(), Reader::OPEN_WRITE))
			{
				pApp->CurFile = fi;
				SetNewFileState();
				return true;
			}
			else
			{
				tstring err;
				api::GetLastErrorT(err);
				this->MessageBox(err.c_str(), MB_OK|MB_ICONEXCLAMATION);
				return false;
			}
		}
		else
			return false;
	}
	else
	{
		this->PreFileIO(util::format(ResStr(IDS_LOADFILE), util::rget_after(fi.str(), '\\').c_str()));
		if (m_doc.LoadFile(fi, Enc))
		{
			pApp->CurFile = fi;
			pApp->Lexer.SetCurLexer(pApp->Lexer.GetDefault());
			if (pApp->m_flagForceHTML)
			{
				m_doc.SetHTMLLexer();
				pApp->m_flagForceHTML = false;
			}
			else
			{
				m_doc.SetLexerFromFile(pApp->CurFile.c_str());
			}
			UpdateLineNumberWidth();
			pApp->f_bModified = false;
			m_doc.SendMessage(SCI_SETEOLMODE, pApp->CurFile.eol, 0);
			m_mru.Add(pApp->CurFile.c_str());
			SetTitle(pApp->d_bShortPathNames, pApp->f_bModified, pApp->f_bReadOnly);
			this->FinishFileIO(pApp->CurFile.c_str());
			return true;
		}
		this->FinishFileIO(pApp->CurFile.c_str());
	}

	return false;
}

bool CView::SaveFile(bool bSaveAlways, bool bAsk, bool bSaveAs, bool bSaveCopy)
{
	bool ret = false;

	if (!bSaveAlways && !pApp->f_bModified && !bSaveAs)
		return true;

	if (bAsk)
	{
		switch (MessageBox(util::format(ResStr(IDS_ASK_SAVE), 
			pApp->CurFile.empty() ? ResStr(IDS_UNTITLED) : pApp->CurFile.c_str()),
			MB_YESNOCANCEL | MB_ICONEXCLAMATION))
		{
		case IDCANCEL :
			return false;
		case IDNO:
			return true;
		}
	}

	// Read only...
	if (!bSaveAs && !bSaveCopy && !pApp->CurFile.empty())
	{
		DWORD dwFileAttributes = GetFileAttributes(pApp->CurFile.c_str());
		if (dwFileAttributes != INVALID_FILE_ATTRIBUTES)
			pApp->f_bReadOnly = (dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0;
		if (pApp->f_bReadOnly)
		{
			SetTitle(pApp->d_bShortPathNames, pApp->f_bModified, pApp->f_bReadOnly);
			if (IDYES == MessageBox(util::format(ResStr(IDS_READONLY_SAVE), pApp->CurFile.c_str()),
				MB_YESNO | MB_ICONWARNING))
				bSaveAs = TRUE;
			else
				return false;
		}
	}

	tstring file;

	// Save As...
	if (bSaveAs || bSaveCopy || pApp->CurFile.empty())
	{
		const util::ptstring pfilter = CreateFullFilter(false);
		wnd::SaveFile dlg(m_hWnd, pfilter->c_str(), pApp->CurFile.c_str(), _T("txt"));
		if (dlg.DoModal())
		{
			file = dlg.GetFileName();
			m_doc.SendMessage(SCI_SETEOLMODE, pApp->CurFile.enc);
		}
		else
			return false;
	}
	else {
		file = pApp->CurFile.c_str();
	}

	this->PreFileIO(file.c_str());
	if (ret = m_doc.SaveFile(file.c_str(), pApp->CurFile.enc, bSaveCopy))
	{
		if (!bSaveCopy)
		{
			pApp->f_bModified = false;
			pApp->CurFile.attach(file.c_str());
			m_mru.Add(pApp->CurFile.c_str());
		}

		if (bSaveAs)
		{
			m_doc.SetLexerFromFile(pApp->CurFile.c_str());
		}
	}
	this->FinishFileIO(file.c_str());

	this->UpdateStatusbar();
	SetTitle(pApp->d_bShortPathNames, pApp->f_bModified, pApp->f_bReadOnly);
	return ret;
}

inline void CView::PreFileIO(const tchar * status)
{
	BeginWaitCursor();

	m_status.SetText(CStatusBar::ST_HELP, status);
	m_status.SendMessage(SB_SIMPLE, TRUE, 0);

	InvalidateRect(m_status(), NULL, TRUE);
	UpdateWindow(m_status());
}

inline void CView::FinishFileIO(const tchar * file)
{
	DWORD dwFileAttributes = GetFileAttributes(file);
	pApp->f_bReadOnly = (dwFileAttributes != INVALID_FILE_ATTRIBUTES &&
		(dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0);

	m_status.SendMessage(SB_SIMPLE, FALSE, 0);
	EndWaitCursor();
}

inline void CView::SetNewFileState()
{
	pApp->CurFile.eol = pApp->d_iDefaultEOL;
	pApp->CurFile.enc = ENC_ANSI;
	m_doc.SetNewText(_T(""), 0);
	m_doc.SetLexer(NULL);
	UpdateLineNumberWidth();
	pApp->f_bModified = false;
	pApp->f_bReadOnly = false;
	m_doc.SendMessage(SCI_SETEOLMODE, pApp->CurFile.eol, 0);
	SetTitle(pApp->d_bShortPathNames, pApp->f_bModified, pApp->f_bReadOnly);
}








//const wchar nfo_table[] = {
/////*  0*/ 0x263a, 0x263b, 0x2665, 0x2666, 0x2663,
/////*  5*/ 0x2660, 0x25cf, 0x25d8, 0x2642, 0x2640,
/////* 10*/ 0x0000, 0x263c, 0x25ba, 0x0000, 0x2195,
/////* 15*/ 0x203c, 0x00b6, 0x00a7, 0x00f5, 0x21a8,
/////* 20*/ 0x2191, 0x2193, 0x2192, 0x2190, 0x221f,
/////* 25*/ 0x2194, 0x25b2, 0x25bc, 0x0000, 0x0000,
/////* 30*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////* 35*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////* 40*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////* 45*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////* 50*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////* 55*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////* 60*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////* 65*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////* 70*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////* 75*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////* 80*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////* 85*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////* 90*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////* 95*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*100*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*105*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*110*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*115*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*120*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*125*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*130*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*135*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*140*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*145*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*150*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*155*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*160*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*165*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*170*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*175*/ 0x0000, 0x2591, 0x2592, 0x2593, 0x2502,
/////*180*/ 0x2524, 0x2561, 0x2562, 0x2556, 0x2555,
/////*185*/ 0x2563, 0x2551, 0x2557, 0x255d, 0x255c,
/////*190*/ 0x255b, 0x2510, 0x2514, 0x2534, 0x252c,
/////*195*/ 0x251c, 0x2500, 0x253c, 0x255e, 0x255f,
/////*200*/ 0x255a, 0x2554, 0x2569, 0x2566, 0x2560,
/////*205*/ 0x2550, 0x256c, 0x2567, 0x2568, 0x2564,
/////*210*/ 0x2565, 0x2559, 0x2558, 0x2552, 0x2553,
/////*215*/ 0x256a, 0x256a, 0x2518, 0x250c, 0x2588,
/////*220*/ 0x2584, 0x258c, 0x2590, 0x2580, 0x03b1,
/////*225*/ 0x03b2, 0x0000, 0x0000, 0x0000, 0x0000,
/////*230*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*235*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*240*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/////*245*/ 0x0000, 0x0000, 0x0000, 0x25cb, 0x2014,
/////*250*/ 0x2013, 0x0000, 0x0000, 0x0000, 0x25a0,
/////*255*/ 0x0000,
//0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
//0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
//0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
//0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
//0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
//0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
//0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
//0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
//0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
//0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
//0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
//0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
//0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
//0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
//0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
//0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
//0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7,
//0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
//0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9,
//0x00ff, 0x00d6, 0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192,
//0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba,
//0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,
//0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
//0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510,
//0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f,
//0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567,
//0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b,
//0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580,
//0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4,
//0x03a6, 0x0398, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229,
//0x2261, 0x00b1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00f7, 0x2248,
//0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0,
//};

void GetVersion(VERSION & out)
{
	memset(&out, 0, sizeof(VERSION));
	DWORD d = 0;
	tstring path;
	api::GetModuleFileNameT(path, NULL);
	DWORD len = GetFileVersionInfoSize((TCHAR *)path.c_str(), &d);
	if (len) {
		util::memory<BYTE> mem(len);
		UINT uLen;
		VS_FIXEDFILEINFO * pvsf;
		if (GetFileVersionInfo((TCHAR *)path.c_str(), 0, len, mem.ptr()) &&
			VerQueryValue(mem.ptr(), _T("\\"), (void**)&pvsf, &uLen))
		{
			out.Ver1 = HIWORD(pvsf->dwFileVersionMS);
			out.Ver2 = LOWORD(pvsf->dwFileVersionMS);
			out.Ver3 = HIWORD(pvsf->dwFileVersionLS);
			out.Ver4 = LOWORD(pvsf->dwFileVersionLS);
		}
	}
}

tstring FormatBytes(DWORD dwBytes)
{
    TCHAR tch[256];
	TCHAR buf[64];
    double dBytes = dwBytes;
	tstring bs;
	api::LoadStringT(bs, IDS_BYTES);
	FixMutliString(bs);
	const TCHAR * p = bs.c_str();
	tstring res;

    if (dwBytes > 1023)
    {
        while (*p)
        {
			if (dBytes >= 1024.0) {
                dBytes /= 1024.0;
				p += _tcslen(p) + 1;
			}
            else break;
        }
        _stprintf(tch, _T("%.2f"), dBytes);
        GetNumberFormat(LOCALE_USER_DEFAULT, 0, tch, NULL, buf, countof(buf));
		res = buf;
		res += _T(" ");
		res += p;
    }

    else
    {
        _stprintf(buf, _T("%lu"), dwBytes);
        FormatNumberStr(buf);
		res = buf;
		res += _T(" ");
		res += p;
    }

	return res;
}


//unsigned int ConvertNFO(const char * in_ascii, char * out_utf8)
//{
//	//int len = MultiByteToWideChar(437, 0, in_ascii, -1, NULL, 0);
//	int len = strlen(in_ascii) + 1;
//	if (len <= 1) return 0;
//
//
//	WCHAR * unicode = new WCHAR[len+1];
//
//	//len = MultiByteToWideChar(437, 0, in_ascii, -1, unicode, len);
//	for (int i = 0; i < len - 1; ++i)
//		unicode[i] = nfo_table[(const byte)in_ascii[i]];
//	unicode[len - 1] = 0;
//
//	len = convert_utf16_to_utf8(unicode, out_utf8, len);
//	delete [] unicode;
//	return len;
//}
//
//unsigned int MakeNFO(const char * in_utf8, char * out_ascii)
//{
//	WCHAR * pwstr = new WCHAR[estimate_utf8_to_utf16(in_utf8)];
//	convert_utf8_to_utf16(in_utf8, pwstr);
//	int len = WideCharToMultiByte(437, 0, pwstr, -1, NULL, 0, NULL, NULL);
//	len = WideCharToMultiByte(437, 0, pwstr, -1, out_ascii, len, NULL, NULL);
//	delete [] pwstr;
//	out_ascii[len] = 0;
//	return len;
//}




void SetFormatter(ASFormatter & formatter, const FMTDATA & data)
{
	formatter.setCStyle();
	formatter.setSpaceIndentation(4);
	formatter.setBracketFormatMode(BREAK_MODE);

	if (data.Style == 'j')
		formatter.setJavaStyle();

	if (HIBYTE(data.Indentation))
		formatter.setTabIndentation(LOBYTE(data.Indentation), true);
	else
		formatter.setSpaceIndentation(LOBYTE(data.Indentation));

	if (NONE_MODE == data.BracketFormatMode)
		formatter.setBreakClosingHeaderBracketsMode(true);
	else
		formatter.setBracketFormatMode((BracketMode)data.BracketFormatMode);

	formatter.setClassIndent(data.ClassIndent);
	formatter.setSwitchIndent(data.SwitchIndent);
	formatter.setCaseIndent(data.CaseIndent);
	formatter.setBracketIndent(data.BracketIndent);
	formatter.setBlockIndent(data.BlockIndent);
	formatter.setNamespaceIndent(data.NamespaceIndent);
	formatter.setLabelIndent(data.LabelIndent);
	formatter.setPreprocessorIndent(data.PreprocessorIndent);
	formatter.setTabSpaceConversionMode(data.TabSpaceConversionMode);
	formatter.setEmptyLineFill(data.EmptyLineFill);
	formatter.setBreakBlocksMode(data.BreakBlocksMode);
	formatter.setBreakElseIfsMode(data.BreakElseIfsMode);
	formatter.setOperatorPaddingMode(data.OperatorPaddingMode);
	formatter.setParenthesisPaddingMode(data.ParenthesisPaddingMode);
	formatter.setSingleStatementsMode(data.SingleStatementsMode);
	formatter.setBreakOneLineBlocksMode(data.BreakOneLineBlocksMode);
	formatter.setMaxInStatementIndentLength(data.MaxInStatementIndentLength);
	formatter.setMinConditionalIndentLength(data.MinConditionalIndentLength);
}


int GetEOLMode(TCHAR * lpData, unsigned int cbData)
{
    TCHAR * p = (TCHAR *)lpData;

	unsigned int i;
	for (i=0; i<cbData && p[i] && p[i] != 0x0D && p[i] != 0x0A; i++);

	if (0x0D == p[i-1]) {
		if (i == cbData || 0x0A == p[i]) return SC_EOL_CRLF;
		else if (0x0A != p[i]) return SC_EOL_CR;
	} else if (0x0A == p[i-1]) {
		if (i == cbData || 0x0D == p[i]) return SC_EOL_LF;
	}

    return SC_EOL_CRLF;
}

const TCHAR * GetEOLModeString(int iMode)
{
	switch (iMode)
	{
	case SC_EOL_LF: return _T("\n");
	case SC_EOL_CR: return _T("\r");
	default: return _T("\r\n");
	}
}










#ifdef _SEH

int ExceptionFilter(LPEXCEPTION_POINTERS param)
{
	return 1;
}

void ExceptionHandler(LPEXCEPTION_POINTERS p, const char * path)
{
	char tmp[128];
	sprintf(tmp, "Notepad2 MOD v%u.%u.%u.%u", g_ver.Ver1, g_ver.Ver2, g_ver.Ver3, g_ver.Ver4);
	crash::DumpCrashInfo(p, path, tmp);
	PostQuitMessage(0);
}

#endif //_SEH

//tchar * CharPrevT(const tchar * start, const tchar * current)
//{
//	return CharPrev(start, current);
//}
//
//tchar * CharNextT(const tchar * text)
//{
//	return CharNext(text);
//}




