#include "notepad2.h"
#include "styles.h"
#include "dialogs.h"

#ifndef SHACF_FILESYSTEM
#define SHACF_FILESYSTEM                0x00000001  // This includes the File System as well as the rest of the shell (Desktop\My Computer\Control Panel\)
#endif

extern CAppModule * pApp;
extern CDocment * pDoc;
extern CView * pView;




namespace wnd
{




LRESULT SaveFile::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			RECT r1, r2, r3, r4;
			HWND hwCtrl;
			HFONT hfont = (HFONT)SendDlgItemMessage(ID_FILENAME, WM_GETFONT, 0, 0);

			GetChildRect(m_hWnd, ID_FILENAME, &r1);
			GetChildRect(m_hWnd, ID_TYPENAME, &r2);
			GetChildRect(m_hWnd, ID_CB_FILENAME, &r3);
			GetChildRect(m_hWnd, ID_CB_TYPENAME, &r4);
			const int offset = 3;

			//text for eol mode
			hwCtrl = CreateWindowEx(WSEX_STATIC,
				_T("STATIC"),
				ResStr(IDS_EOLMODE),
				WS_STATIC,
				r1.left,
				r4.top + r4.top - r3.top + offset,
				r2.right - r2.left,
				r2.bottom - r2.top,
				m_hWnd,
				(HMENU)ID_EOLMODE,
				WpfGetInstance(),
				NULL);
			::SendMessage(hwCtrl, WM_SETFONT, (WPARAM)hfont, TRUE);

			//text for encoding
			hwCtrl = CreateWindowEx(WSEX_STATIC,
				_T("STATIC"),
				ResStr(IDS_ENCODING),
				WS_STATIC,
				r1.left,
				r4.top + (r4.top - r3.top) * 2 + offset,
				r2.right - r2.left,
				r2.bottom - r2.top,
				m_hWnd,
				(HMENU)ID_ENCODING,
				WpfGetInstance(),
				NULL);
			::SendMessage(hwCtrl, WM_SETFONT, (WPARAM)hfont, TRUE);

			//combobox for eol mode
			hwCtrl = CreateWindowEx(WSEX_COMBOBOX,
				_T("COMBOBOX"),
				_T(""),
				WS_COMBOBOX,
				r3.left,
				r4.top + r4.top - r3.top,
				r4.right - r4.left,
				(r4.bottom - r4.top) * 10,
				m_hWnd,
				(HMENU)ID_CB_EOLMODE,
				WpfGetInstance(),
				NULL);
			::SendMessage(hwCtrl, WM_SETFONT, (WPARAM)hfont, TRUE);
			::SendMessage(hwCtrl, CB_ADDSTRING, 0, (LPARAM)_T("Windows (CR+LF)"));
			::SendMessage(hwCtrl, CB_ADDSTRING, 0, (LPARAM)_T("Unix (LF)"));
			::SendMessage(hwCtrl, CB_ADDSTRING, 0, (LPARAM)_T("Mac (CR)"));
			::SendMessage(hwCtrl, CB_SETCURSEL, (WPARAM)pApp->CurFile.eol, 0);

			//combobox for encoding
			hwCtrl = CreateWindowEx(WSEX_COMBOBOX,
				_T("COMBOBOX"),
				_T(""),
				WS_COMBOBOX,
				r3.left,
				r4.top + (r4.top - r3.top) * 2,
				r4.right - r4.left,
				(r4.bottom - r4.top) * 10,
				m_hWnd,
				(HMENU)ID_CB_ENCODING,
				WpfGetInstance(),
				NULL);
			::SendMessage(hwCtrl, WM_SETFONT, (WPARAM)hfont, TRUE);
			::SendMessage(hwCtrl, CB_ADDSTRING, 0, (LPARAM)_T("ANSI"));
			::SendMessage(hwCtrl, CB_ADDSTRING, 0, (LPARAM)_T("Unicode"));
			::SendMessage(hwCtrl, CB_ADDSTRING, 0, (LPARAM)_T("Unicode Big Endian"));
			::SendMessage(hwCtrl, CB_ADDSTRING, 0, (LPARAM)_T("UTF-8"));
			::SendMessage(hwCtrl, CB_SETCURSEL, (WPARAM)pApp->CurFile.enc, 0);

			//resize dialog
			::GetWindowRect(hwCtrl, &r2);
			::GetWindowRect(m_hWnd, &r1);
			::MoveWindow(m_hWnd, r1.left, r1.top, r1.right - r1.left, r2.bottom - r1.top + 20, TRUE);
		}
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			pApp->CurFile.eol = SendDlgItemMessage(ID_CB_EOLMODE, CB_GETCURSEL, 0, 0);
			pApp->CurFile.enc = SendDlgItemMessage(ID_CB_ENCODING, CB_GETCURSEL, 0, 0);
			break;
		}
		break; //WM_COMMAND

	case WM_SIZE:
		{
			RECT r1, r2;
			GetChildRect(m_hWnd, ID_CB_FILENAME, &r1);
			GetChildRect(m_hWnd, ID_CB_EOLMODE, &r2);
			::MoveWindow(GetDlgItem(ID_CB_EOLMODE), r2.left, r2.top,
				r1.right - r1.left, r2.bottom - r2.top, TRUE);
			GetChildRect(m_hWnd, ID_CB_ENCODING, &r2);
			::MoveWindow(GetDlgItem(ID_CB_ENCODING), r2.left, r2.top,
				r1.right - r1.left, r2.bottom - r2.top, TRUE);
		}
		break;
	}

	return CFileDialog::OnMessage(uMsg, wParam, lParam);
}

void FileAssoc::AddToContextMenu(const tstring & module, const TCHAR * ext)
{
	tstring dotext = ext;
	dotext.insert(dotext.begin(), '.');

	tstring base;
	CReg reg;
	if (CReg::SUCCESS == reg.Open(HKCR, dotext.c_str()) &&
		reg.QueryStringValue(_T(""), base) )
	{
		reg.Close();
		if (CReg::SUCCESS == reg.Open(HKCR, (base + _T("\\shell")).c_str()))
		{
			reg.Close();
			CReg::SetStringValue(HKCR, (base + _T("\\shell\\np2open")).c_str(),
				_T(""), util::format(ResStr(IDS_OPENBY), ResStr(IDS_APPTITLE)) );
			tstring path = _T("\"");
			path += module;
			path += _T("\" \"%1\"");
			CReg::SetStringValue(HKCR,
				(base + _T("\\shell\\np2open\\command")).c_str(), _T(""), path.c_str());
			// write backup
			if (!!_tcsnicmp(base.c_str(), APP_NAME, _tcslen(APP_NAME)))
			{
				CReg::SetStringValue(HKCR, APP_NAME _T(".file"),
					(_T("bckupex_") + base).c_str(), base.c_str());
			}
		}

	}
	else { //no association
		this->DirectAssoc(module, ext, util::format(ResStr(IDS_UNKNOWN_FILE), ext), II_MAIN);
		this->AddToContextMenu(module, ext);
	}
}

void FileAssoc::DirectAssoc(const tstring & module, const TCHAR * ext, const TCHAR * desc, int icon)
{
	tstring dotext = ext;
	dotext.insert(dotext.begin(), '.');

	tstring cur;
	CReg reg;
	if (CReg::SUCCESS == reg.Open(HKCR, dotext.c_str()) &&
		reg.QueryStringValue(_T(""), cur) )
	{
		reg.Close();
		if (!!_tcsnicmp(cur.c_str(), APP_NAME, countof(APP_NAME) - 1))
		{
			if (!cur.empty())
			{
				if (CReg::SUCCESS == reg.Create(HKCR, (APP_NAME + dotext).c_str()))
				{
					SHCopyKey(HKCR, cur.c_str(), reg, NULL);
					reg.Close();
				}
			}
		}
		else
			cur.erase();
	}
	// write backup
	CReg::SetStringValue(HKCR, APP_NAME _T(".file"), (_T("bckup_") + tstring(ext)).c_str(), cur.c_str());

	CReg::DeleteKey(HKCU, (_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\") + dotext).c_str());
	tstring base = APP_NAME + dotext;
	CReg::SetStringValue(HKCR, dotext.c_str(), _T(""), base.c_str());
	CReg::SetStringValue(HKCR, base.c_str(), _T(""), desc);
	CReg::SetStringValue(HKCR, (base + _T("\\shell")).c_str(), _T(""), _T("Open"));
	CReg::DeleteKey(HKCR, (base + _T("\\shell\\Open")).c_str());
	CReg::SetStringValue(HKCR, (base + _T("\\shell\\Open")).c_str(),
		_T(""), util::format(ResStr(IDS_OPENBY), ResStr(IDS_APPTITLE)));
	tstring str = module;
	str.insert(str.begin(), '\"');
	str += _T("\" \"%1\"");
	CReg::SetStringValue(HKCR,
		(base + _T("\\shell\\Open\\command")).c_str(), _T(""), str.c_str());
	str = module;
	str += ',';
	ASSERT(II_NONE != icon);
	if (II_CPP == icon && (!_tcsicmp(ext, _T("h")) || !_tcsicmp(ext, _T("hpp")) || !_tcsicmp(ext, _T("hxx"))
		|| !_tcsicmp(ext, _T("hm")) || !_tcsicmp(ext, _T("inl")) || !_tcsicmp(ext, _T("inc")) ))
	{
		str << (int)II_H;
	}
	else
	{
		str << icon;
	}
	CReg::SetStringValue(HKCR, (base + _T("\\DefaultIcon")).c_str(), _T(""), str.c_str());
	CReg::SetDWORDValue(HKCR, base.c_str(), _T("EditFlags"), 0x10000);
}

void FileAssoc::Restore()
{
	TCHAR name[255];
	DWORD cName = countof(name), idx = 0;
	while (CReg::EnumValue(HKCR, APP_NAME _T(".file"), idx, name, &cName))
	{
		name[cName] = 0;

		if (_tcslen(name) > 6 && !_tcsncmp(name, _T("bckup_"), 6))
		{
			tstring dotext = name + 6;
			dotext.insert(dotext.begin(), '.');
			CReg reg;

			tstring bak;
			reg.Open(HKCR, APP_NAME _T(".file"));
			reg.QueryStringValue(name, bak);
			reg.Close();

			tstring assoc;
			if ( CReg::SUCCESS == reg.Open(HKCR, dotext.c_str()) &&
				reg.QueryStringValue(_T(""), assoc) )
			{
				reg.Close();

				if (!_tcsnicmp(assoc.c_str(), APP_NAME, _tcslen(APP_NAME)))
				{
					if (bak.empty()) {
						CReg::DeleteKey(HKCR, dotext.c_str());
					}
					else {
						reg.Create(HKCR, dotext.c_str());
						reg.SetStringValue(_T(""), bak.c_str());
						reg.Close();
					}
				}
			}

			dotext.insert(0, APP_NAME);
			CReg::DeleteKey(HKCR, dotext.c_str());
		}
		else if (_tcslen(name) > 8 && !_tcsncmp(name, _T("bckupex_"), 8))
		{
			const TCHAR * ext = name + 8;
			CReg::DeleteKey(HKCR, (tstring(ext) + _T("\\shell\\np2open")).c_str());
		}

		cName = countof(name);
		idx++;
	}

	CReg::DeleteKey(HKCR, APP_NAME _T(".file"));
}

FileAssoc::SELFLAG FileAssoc::CheckGroupAssoc(tstring exts, bool bContextMenu)
{
	std::replace(exts.begin(), exts.end(), ';', '\0');
	for (const TCHAR * p = exts.c_str(); p < exts.c_str() + exts.length(); p += _tcslen(p) + 1)
	{
		if (SF_SEL == this->CheckAssoc(p, bContextMenu))
			return SF_SEL;
	}
	return SF_UNSEL;
}

FileAssoc::SELFLAG FileAssoc::CheckAssoc(const TCHAR * ext, bool bContextMenu)
{
	CReg reg;
	tstring dotext = _T(".");
	dotext += ext;
	if (CReg::SUCCESS == reg.Open(HKCR, dotext.c_str()))
	{
		tstring name;
		if (reg.QueryStringValue(_T(""), name))
		{
			if (bContextMenu)
			{
				reg.Close();
				if (CReg::SUCCESS == reg.Open(HKCR, (name + _T("\\shell\\np2open\\command")).c_str()))
					return SF_SEL;
			}
			else {
				if (!_tcsnicmp(name.c_str(), APP_NAME, countof(APP_NAME) - 1))
					return SF_SEL;
			}
		}
	}

	return SF_UNSEL;
}

void FileAssoc::AddFile()
{
	tstring partext;
	api::LoadStringT(partext, IDS_ASSOC_TYPE);
	FixMutliString(partext);
	TVINSERTSTRUCT tv = { 0 };
	tv.hInsertAfter = TVI_LAST;
	tv.item.stateMask = TVIS_EXPANDED;
	tv.item.state = TVIS_EXPANDED;

	tv.item.pszText = (TCHAR *)partext.c_str();
	tv.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_STATE | TVIF_CHILDREN;
	tv.item.cChildren = 1;
	tv.item.lParam = -1;
	this->hTreePar1.hTreeItem = TreeView_InsertItem(hWndTree, &tv);
	this->hTreePar1.SelFlag = SF_NONE;

	tv.item.pszText = (TCHAR *)partext.c_str() + _tcslen(partext.c_str()) + 1;
	tv.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_STATE | TVIF_CHILDREN;
	tv.item.cChildren = 1;
	tv.item.lParam = -1;
	this->hTreePar2.hTreeItem = TreeView_InsertItem(hWndTree, &tv);
	this->hTreePar2.SelFlag = SF_NONE;

	tv.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_STATE;
	tv.item.cChildren = 0;
	TREEPARAM tp;
	for (unsigned int i=0; i<pApp->Lexer.GetSize(); i++)
	{
		tv.hParent = II_NONE == pApp->Lexer[i]->IconID ? this->hTreePar2.hTreeItem : this->hTreePar1.hTreeItem;
		tstring text;
		api::LoadStringT(text, pApp->Lexer[i]->NameID);
		util::make_before(text, '\n');
		text += _T(" (");
		text += pApp->Lexer[i]->szExtensions;
		text += ')';
		tv.item.pszText = (TCHAR *)text.c_str();
		tv.item.lParam = i;
		tp.hTreeItem = TreeView_InsertItem(hWndTree, &tv);

		tp.SelFlag = this->CheckGroupAssoc(tstring(pApp->Lexer[i]->szExtensions), 
			II_NONE == pApp->Lexer[i]->IconID);

		hTreeItem.push_back(tp);
	}
}


void FileAssoc::RefreshComponents()
{
	TVITEM item = { 0 };
	item.stateMask = TVIS_STATEIMAGEMASK | TVIS_EXPANDED | TVIS_BOLD;
	item.mask = TVIF_STATE;

	this->hTreePar1.SelFlag = SF_NONE;
	this->hTreePar2.SelFlag = SF_NONE;

	for (unsigned int i=0; i<pApp->Lexer.GetSize(); i++)
	{
		item.hItem = this->hTreeItem[i].hTreeItem;
		ASSERT(this->hTreeItem[i].SelFlag != SF_PSSEL);
		item.state = INDEXTOSTATEIMAGEMASK(this->hTreeItem[i].SelFlag);
		TreeView_SetItem(this->hWndTree, &item);
		if (II_NONE == pApp->Lexer[i]->IconID)
			this->hTreePar2.SelFlag |= this->hTreeItem[i].SelFlag;
		else
			this->hTreePar1.SelFlag |= this->hTreeItem[i].SelFlag;
	}

	ASSERT(this->hTreePar1.SelFlag);
	ASSERT(this->hTreePar2.SelFlag);
	item.hItem = this->hTreePar1.hTreeItem;
	item.state = TVIS_EXPANDED;
	item.state |= INDEXTOSTATEIMAGEMASK(this->hTreePar1.SelFlag);
	TreeView_SetItem(this->hWndTree, &item);
	item.hItem = this->hTreePar2.hTreeItem;
	item.state = TVIS_EXPANDED;
	item.state |= INDEXTOSTATEIMAGEMASK(this->hTreePar2.SelFlag);
	TreeView_SetItem(this->hWndTree, &item);
}

HTREEITEM FileAssoc::TreeHitTest(HWND tree)
{
	TVHITTESTINFO ht;
	DWORD dwpos = GetMessagePos();

	ht.pt.x = (int)(short)LOWORD(dwpos);
	ht.pt.y = (int)(short)HIWORD(dwpos);
	ScreenToClient(tree, &ht.pt);

	TreeView_HitTest(tree, &ht);

	if (ht.flags & (TVHT_ONITEMSTATEICON|TVHT_ONITEMLABEL|TVHT_ONITEMRIGHT|TVHT_ONITEM))
		return ht.hItem;

	return 0;
}

FileAssoc::FileAssoc()
: pos(_T("FileAssoc"))
, hImageList(NULL)
, m_resizer(280, 280, 0, 0)
{
	m_resizer(IDOK, CDlgResizer::XY_MOVE)
			(IDCANCEL, CDlgResizer::XY_MOVE)
			(IDC_SIZEBOX, CDlgResizer::XY_MOVE)
			(IDC_TREEVIEW, CDlgResizer::X_SIZE|CDlgResizer::Y_SIZE)
			(IDC_SELALL, CDlgResizer::Y_MOVE)
			(IDC_SELNONE, CDlgResizer::Y_MOVE)
			(IDC_IE_VIEWER, CDlgResizer::Y_MOVE)
			(IDC_INFO, CDlgResizer::X_SIZE|CDlgResizer::Y_MOVE);
}

BOOL FileAssoc::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_resizer.IsProcess(m_hWnd, uMsg, wParam, lParam))
		return TRUE;

	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
			if (!pos.SetPos(m_hWnd, true))
				this->CenterWindow();

			hWndTree = GetDlgItem(IDC_TREEVIEW);
			HBITMAP hbmap = LoadBitmap(WpfGetInstance(), MAKEINTRESOURCE(IDB_CHECK));
			ASSERT(hbmap);
			hImageList = ImageList_Create(16,16, ILC_COLOR32|ILC_MASK, 6, 0);
			ASSERT(hImageList);
			ImageList_AddMasked(hImageList, hbmap, RGB(255,0,255));
			TreeView_SetImageList(hWndTree, hImageList, TVSIL_STATE);
			if (::SendMessage(hWndTree, TVM_GETITEMHEIGHT, 0, 0) < 16)
				::SendMessage(hWndTree, TVM_SETITEMHEIGHT, 16, 0);
			DeleteObject(hbmap);

			this->AddFile();
			this->RefreshComponents();
		}
		break;

	case WM_DESTROY:
		pos.GetPos(m_hWnd);
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDC_SELALL:
			{
				for (uint i=0; i<pApp->Lexer.GetSize(); ++i)
				{
					this->hTreeItem[i].SelFlag = SF_SEL;
					this->RefreshComponents();
				}
			}
			break;

		case IDC_SELNONE:
			{
				for (uint i=0; i<pApp->Lexer.GetSize(); ++i)
				{
					this->hTreeItem[i].SelFlag = SF_UNSEL;
					this->RefreshComponents();
				}
			}
			break;

		case IDOK:
			{
				BeginWaitCursor();
				tstring module;
				api::GetModuleFileNameT(module);

				this->Restore();
				for (uint i=0; i<pApp->Lexer.GetSize(); ++i)
				{
					if (SF_UNSEL == this->hTreeItem[i].SelFlag)
						continue;
					tstring exts = pApp->Lexer[i]->szExtensions;
					std::replace(exts.begin(), exts.end(), ';', '\0');
					tstring desc;
					api::LoadStringT(desc, pApp->Lexer[i]->NameID);
					util::make_before(desc, '\n');
					for (const TCHAR * p = exts.c_str(); p < exts.c_str() + exts.length(); p += _tcslen(p) + 1)
					{
						if (II_NONE == pApp->Lexer[i]->IconID)
							this->AddToContextMenu(module, p);
						else
							this->DirectAssoc(module, p, desc.c_str(), pApp->Lexer[i]->IconID);
					}
				}

				CReg reg;
				if (BST_CHECKED == SendDlgItemMessage(IDC_IE_VIEWER, BM_GETCHECK))
				{
					do {
						std::vector<BYTE> bin;
						if (!api::LoadResourceT(bin, IDR_NOTEPAD2HTML, _T("BINARY")) || bin.size() == 0)
							break;

						tstring dir;
						if (api::GetWindowsDirectoryT(dir) == 0)
							break;

						dir += _T("\\Notepad2HTML.exe");
						CReader r;
						if (!r.open(dir.c_str(), CReader::OPEN_WRITE))
							break;

						r.write(&bin[0], bin.size());

						if (CReg::SUCCESS != reg.Create(HKLM, _T("SOFTWARE\\Microsoft\\Internet Explorer\\View Source Editor\\Editor Name")))
							break;

						reg.SetStringValue(_T(""), dir.c_str());
					} while (false);
				}

				if (CReg::SUCCESS == reg.Create(HKLM, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Notepad2.exe")))
				{
					reg.SetStringValue(_T(""), module.c_str());
					reg.SetStringValue(_T("Path"), util::rget_before(module, '\\').c_str());
				}

				SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
				EndWaitCursor();
			}

		case IDCANCEL:
			this->EndDialog(wParam);
			return 1;
		}
		break; //break WM_COMMAND

	case WM_NOTIFY:
		{
			LPNMHDR lpnmh = (LPNMHDR)lParam;
			if (IDC_TREEVIEW == lpnmh->idFrom && NM_CLICK == lpnmh->code)
			{
				TVITEM tvItem;
				tvItem.hItem = TreeHitTest(this->hWndTree);
				if (tvItem.hItem)
				{
					tvItem.mask = TVIF_PARAM;
					if (TreeView_GetItem(this->hWndTree, &tvItem))
					{
						unsigned int x = tvItem.lParam;
						if (-1 == x) {
							unsigned int flag = SF_NONE;

							if (tvItem.hItem == this->hTreePar1.hTreeItem) {
								if (SF_SEL == this->hTreePar1.SelFlag)
									flag = SF_UNSEL;
								else
									flag = SF_SEL;

								for (unsigned int i=0; i<pApp->Lexer.GetSize(); i++)
								{
									if (II_NONE != pApp->Lexer[i]->IconID)
										this->hTreeItem[i].SelFlag = flag;
								}
							}
							else if (tvItem.hItem == this->hTreePar2.hTreeItem) {
								if (SF_SEL == this->hTreePar2.SelFlag)
									flag = SF_UNSEL;
								else
									flag = SF_SEL;

								for (unsigned int i=0; i<pApp->Lexer.GetSize(); i++)
								{
									if (II_NONE == pApp->Lexer[i]->IconID)
										this->hTreeItem[i].SelFlag = flag;
								}
							}
						}
						else {
							if (this->hTreeItem[x].SelFlag & SF_SEL)
								this->hTreeItem[x].SelFlag = SF_UNSEL;
							else
								this->hTreeItem[x].SelFlag = SF_SEL;
						}

						this->RefreshComponents();
					}
				}
			}
		}
		break;

	}

	return 0;
}

void SelectLexer::AddLexer(EDITLEXER * plex)
{
	LVITEM lvi = { 0 };
	tstring name;
	api::LoadStringT(name, plex->NameID);
	util::make_before(name, '\n');
	name += _T(" (");
	name += plex->szExtensions;
	name += _T(")");

	lvi.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_TEXT;
	lvi.iItem = ListView_GetItemCount(m_hwndLV);
	lvi.pszText = (LPTSTR)name.c_str();
	lvi.iImage = Style_GetLexerIconId(plex);
	lvi.lParam = (LPARAM)plex;
	if (plex->NameID == pApp->Lexer.GetCurLexer()->NameID)
	{
		m_cur_lexer = lvi.iItem;
	}

	ListView_InsertItem(m_hwndLV, &lvi);
}

SelectLexer::SelectLexer()
: m_pos(_T("SelectLexer"))
, m_cur_lexer(0)
, m_resizer(230, 250, 0, 0)
{
	m_resizer(IDOK, CDlgResizer::XY_MOVE)
			(IDCANCEL, CDlgResizer::XY_MOVE)
			(IDC_SIZEBOX, CDlgResizer::XY_MOVE)
			(IDC_STYLELIST, CDlgResizer::X_SIZE|CDlgResizer::Y_SIZE);
}


BOOL SelectLexer::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_resizer.IsProcess(m_hWnd, uMsg, wParam, lParam))
		return TRUE;

	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
			SHFILEINFO shfi;

			m_hwndLV = GetDlgItem(IDC_STYLELIST);

			ListView_SetImageList(m_hwndLV,
								  (HIMAGELIST)SHGetFileInfo(_T("C:\\"), 0, &shfi, sizeof(SHFILEINFO), SHGFI_SMALLICON | SHGFI_SYSICONINDEX),
								  LVSIL_SMALL);

			ListView_SetImageList(m_hwndLV,
								  (HIMAGELIST)SHGetFileInfo(_T("C:\\"), 0, &shfi, sizeof(SHFILEINFO), SHGFI_LARGEICON | SHGFI_SYSICONINDEX),
								  LVSIL_NORMAL);

			ListView_SetExtendedListViewStyle(m_hwndLV, LVS_EX_LABELTIP);

			// Add lexers
			for (int i = 0; i < (int)pApp->Lexer.GetSize(); i++)
			{
				AddLexer(pApp->Lexer[i]);
			}

			ListView_SetColumnWidth(m_hwndLV, 0, LVSCW_AUTOSIZE);
			ListView_Arrange(m_hwndLV, LVA_DEFAULT);

			// Select current lexer
			ListView_SetItemState(m_hwndLV, m_cur_lexer, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
			ListView_EnsureVisible(m_hwndLV, m_cur_lexer, FALSE);

			if (!m_pos.SetPos(m_hWnd, true))
				this->CenterWindow();
		}
		return TRUE;


	case WM_DESTROY:
		m_pos.GetPos(m_hWnd);
		break;


	case WM_NOTIFY:
		{
			if (((LPNMHDR)(lParam))->idFrom == IDC_STYLELIST)
			{

				switch (((LPNMHDR)(lParam))->code)
				{

				case NM_DBLCLK:
					SendMessage(WM_COMMAND, MAKELONG(IDOK, 1), 0);
					break;

				case LVN_ITEMCHANGED:
				case LVN_DELETEITEM:
					{
						int i = ListView_GetNextItem(m_hwndLV, -1, LVNI_ALL | LVNI_SELECTED);
						EnableWindow(GetDlgItem(IDOK), i != -1);
					}
					break;
				}
			}
		}

		return TRUE;


	case WM_COMMAND:

		switch (LOWORD(wParam))
		{

		case IDOK:
			{
				LVITEM lvi;

				lvi.mask = LVIF_PARAM;
				lvi.iItem = ListView_GetNextItem(m_hwndLV, -1, LVNI_ALL | LVNI_SELECTED);
				if (ListView_GetItem(m_hwndLV, &lvi))
				{
					pDoc->SetLexer((EDITLEXER *)lvi.lParam);
					EndDialog(IDOK);
				}
			}
			break;


		case IDCANCEL:
			EndDialog(IDCANCEL);
			break;

		}

		return TRUE;

	}

	return FALSE;
}



void StyleConfig::AddLexer()
{
	for (unsigned int i = 0; i < pApp->Lexer.GetSize(); i++)
	{
		HTREEITEM hTreeNode;
		tstring name;
		api::LoadStringT(name, pApp->Lexer[i]->NameID);
		FixMutliString(name);

		TVINSERTSTRUCT tvis = { 0 };
		tvis.hInsertAfter = TVI_LAST;
		tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
		tvis.item.pszText = (LPTSTR)name.c_str();
		tvis.item.iImage = Style_GetLexerIconId(pApp->Lexer[i]);
		tvis.item.iSelectedImage = tvis.item.iImage;
		tvis.item.lParam = (LPARAM)i;

		hTreeNode = (HTREEITEM)TreeView_InsertItem(hwndTV, &tvis);

		tvis.hParent = hTreeNode;

		tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
		//tvis.item.iImage = -1;
		//tvis.item.iSelectedImage = -1;

		int j = 0;
		TCHAR * p = (TCHAR *)name.c_str();
		p += _tcslen(p) + 1;
		while (pApp->Lexer[i]->Styles[j].iStyle != -1)
		{
			tvis.item.pszText = p;
			tvis.item.lParam = (LPARAM)j;
			TreeView_InsertItem(hwndTV, &tvis);
			p += _tcslen(p) + 1;
			if (p - name.c_str() > (int)name.length() + 1)
			{
				this->MessageBox(name.c_str(), ResStr(IDS_ERROR), MB_OK|MB_ICONSTOP);
				break;
			}
			j++;
		}
	}
}



void StyleConfig::RefreshStyleInfo()
{
	STYLE style = { sizeof(STYLE) };
	GetSelStyle(style, pCurrentStyle);
	tstring info;
	if (style.FontName[0]) {
		info << style.FontName;
		info << _T(" ");
		info << sci::ConvertFontSize(style.iFontSize);
		info << _T("pt");
	}
	SetDlgItemText(IDC_FONT_SIZE, info.c_str());
	SendDlgItemMessage(IDC_FONT_BOLD, BM_SETCHECK, style.fBold ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_FONT_ITALIC, BM_SETCHECK, style.fItalic ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_FONT_UNDERLINE, BM_SETCHECK, style.fUnderline ? BST_CHECKED : BST_UNCHECKED, 0);

	COLORREF color;

	if (style.rForeColor)
	{
		color = sci::ConvertColor(style.rForeColor);
		SetDlgItemText(IDC_FGCODE, sci::MakeHTMLColorString(color));
	}
	else
	{
		color = GetSysColor(COLOR_BTNFACE);
		SetDlgItemText(IDC_FGCODE, _T(""));
	}
	fg.CreateSolidBrush(color);
	RedrawWindow(GetDlgItem(IDC_FCOLOR), NULL, NULL, RDW_INVALIDATE);

	if (style.rBackColor)
	{
		color = sci::ConvertColor(style.rBackColor);
		SetDlgItemText(IDC_BGCODE, sci::MakeHTMLColorString(color));
	}
	else
	{
		color = GetSysColor(COLOR_BTNFACE);
		SetDlgItemText(IDC_BGCODE, _T(""));
	}
	bg.CreateSolidBrush(color);
	RedrawWindow(GetDlgItem(IDC_BCOLOR), NULL, NULL, RDW_INVALIDATE);
}

inline void StyleConfig::EnableControl(BOOL bEnable)
{
	EnableWindow(GetDlgItem(IDC_STYLEFONT), bEnable);
	EnableWindow(GetDlgItem(IDC_STYLEFORE), bEnable);
	EnableWindow(GetDlgItem(IDC_STYLEBACK), bEnable);
	EnableWindow(GetDlgItem(IDC_FONT_SIZE), bEnable);
	EnableWindow(GetDlgItem(IDC_FONT_BOLD), bEnable);
	EnableWindow(GetDlgItem(IDC_FONT_ITALIC), bEnable);
	EnableWindow(GetDlgItem(IDC_FONT_UNDERLINE), bEnable);
}

INT_PTR StyleConfig::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
			//int i;
			SHFILEINFO shfi;

			hwndTV = GetDlgItem(IDC_STYLELIST);
			fDragging = FALSE;

			TreeView_SetImageList(hwndTV,
					(HIMAGELIST)SHGetFileInfo(_T("C:\\"),0,&shfi,sizeof(SHFILEINFO),
						SHGFI_SMALLICON | SHGFI_SYSICONINDEX),TVSIL_NORMAL);

			// Add lexers
			this->AddLexer();

			pCurrentLexer = 0;
			pCurrentStyle = 0;
			//TreeView_Expand(hwndTV,TreeView_GetRoot(hwndTV),TVE_EXPAND);
			TreeView_Select(hwndTV, TreeView_GetRoot(hwndTV), TVGN_CARET);

			MakeBitmapButton(m_hWnd, IDC_PREVSTYLE, IDB_PREV);
			MakeBitmapButton(m_hWnd, IDC_NEXTSTYLE, IDB_NEXT);

			COLORREF color = GetSysColor(COLOR_BTNFACE);
			fg.CreateSolidBrush(color);
			bg.CreateSolidBrush(color);

			if (!pos.SetPos(m_hWnd))
				this->CenterWindow();
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if (GetDlgItem(IDC_FCOLOR) == (HWND)lParam)
		{
			return (INT_PTR)fg();
		}
		else if (GetDlgItem(IDC_BCOLOR) == (HWND)lParam)
		{
			return (INT_PTR)bg();
		}
		return FALSE;

	case WM_DESTROY:
		pos.GetPos(m_hWnd);
		return FALSE;

	case WM_NOTIFY:

		if (((LPNMHDR)(lParam))->idFrom == IDC_STYLELIST)
		{
			LPNMTREEVIEW lpnmtv = (LPNMTREEVIEW)lParam;

			switch (lpnmtv->hdr.code)
			{

			case TVN_SELCHANGED:
				{
					// a lexer has been selected
					if (!TreeView_GetParent(hwndTV, lpnmtv->itemNew.hItem))
					{
						//pCurrentStyle = 0;
						if (pCurrentLexer = pApp->Lexer[lpnmtv->itemNew.lParam])
						{
							SetDlgItemText(IDC_STYLEEDIT, pCurrentLexer->szExtensions);
						}
						else
						{
							SetDlgItemText(IDC_STYLEEDIT, _T(""));
						}
						this->EnableControl(FALSE);
						fg.ReleaseObject();
						bg.ReleaseObject();
					}

					// a style has been selected
					else if (pCurrentLexer)
					{
						//pCurrentLexer = 0;
						if (pCurrentStyle = &pCurrentLexer->Styles[lpnmtv->itemNew.lParam])
						{
							this->EnableControl(TRUE);
							this->RefreshStyleInfo();
						}
						else
						{
							this->EnableControl(FALSE);
						}
					}
				}
				break;

			case TVN_BEGINDRAG:
				{
					//HIMAGELIST himl;

					TreeView_Select(hwndTV, lpnmtv->itemNew.hItem, TVGN_CARET);

					//himl = TreeView_CreateDragImage(hwndTV,lpnmtv->itemNew.hItem);
					//ImageList_BeginDrag(himl,0,0,0);
					//ImageList_DragEnter(hwndTV,lpnmtv->ptDrag.x,lpnmtv->ptDrag.y);

					SetCapture(m_hWnd);
					fDragging = TRUE;
				}

			}
		}

		break;


	case WM_MOUSEMOVE:
		{
			HTREEITEM htiTarget;
			TVHITTESTINFO tvht;

			if (fDragging && pCurrentStyle)
			{
				LONG xCur = LOWORD(lParam);
				LONG yCur = HIWORD(lParam);

				//ImageList_DragMove(xCur,yCur);
				//ImageList_DragShowNolock(FALSE);

				tvht.pt.x = xCur;
				tvht.pt.y = yCur;

				//ClientToScreen(hwnd,&tvht.pt);
				//ScreenToClient(hwndTV,&tvht.pt);
				MapWindowPoints(m_hWnd, hwndTV, &tvht.pt, 1);

				if ((htiTarget = TreeView_HitTest(hwndTV, &tvht)) != NULL &&
						TreeView_GetParent(hwndTV, htiTarget) != NULL)
				{
					TreeView_SelectDropTarget(hwndTV, htiTarget);
					//TreeView_Expand(hwndTV,htiTarget,TVE_EXPAND);
					TreeView_EnsureVisible(hwndTV, htiTarget);
				}
				else
					TreeView_SelectDropTarget(hwndTV, NULL);

				//ImageList_DragShowNolock(TRUE);
			}
		}
		break;


	case WM_LBUTTONUP:
		{
			if (fDragging)
			{
				HTREEITEM htiTarget;

				//ImageList_EndDrag();

				if (htiTarget = TreeView_GetDropHilight(hwndTV))
				{
					TreeView_SelectDropTarget(hwndTV, NULL);
					TreeView_Select(hwndTV, htiTarget, TVGN_CARET);
				}
				ReleaseCapture();
				DestroyCursor(SetCursor(LoadCursor(NULL, IDC_ARROW)));
				fDragging = FALSE;
			}
		}
		break;


	case WM_CANCELMODE:
		{
			if (fDragging)
			{
				//ImageList_EndDrag();
				TreeView_SelectDropTarget(hwndTV, NULL);
				ReleaseCapture();
				DestroyCursor(SetCursor(LoadCursor(NULL, IDC_ARROW)));
				fDragging = FALSE;
			}
		}
		break;


	case WM_COMMAND:

		switch (wParam)
		{

		case MAKELONG(IDC_STYLEEDIT, EN_KILLFOCUS):
			if (pCurrentLexer)
			{
				tstring text;
				if (api::GetDlgItemTextT(text, m_hWnd, IDC_STYLEEDIT) <=0)
					break;

				if (HasBadFileChar(text.c_str()))
				{
					this->MessageBox(ResStr(IDS_BADFILECHAR), ResStr(IDS_APPTITLE), MB_OK|MB_ICONEXCLAMATION);
					SetFocus((HWND)lParam);
					break;
				}
				if (!text.empty()) {
					_tcscpy(pCurrentLexer->szExtensions, util::tolower(text).c_str());
				}
			}
			break;

		case IDC_FONT_BOLD:
			pCurrentStyle->Value.fBold = SendDlgItemMessage(IDC_FONT_BOLD, BM_GETCHECK, 0, 0) ? true : false;
			break;

		case IDC_FONT_ITALIC:
			pCurrentStyle->Value.fItalic = SendDlgItemMessage(IDC_FONT_ITALIC, BM_GETCHECK, 0, 0) ? true : false;
			break;

		case IDC_FONT_UNDERLINE:
			pCurrentStyle->Value.fUnderline = SendDlgItemMessage(IDC_FONT_UNDERLINE, BM_GETCHECK, 0, 0) ? true : false;
			break;

		case IDC_PREVSTYLE:
			{
				HTREEITEM hsel = TreeView_GetSelection(hwndTV);
				if (hsel)
				{
					HTREEITEM hnext = TreeView_GetPrevVisible(hwndTV, hsel);
					if (hnext)
					{
						TreeView_Select(hwndTV, hnext, TVGN_CARET);
					}
				}
			}
			break;

		case IDC_NEXTSTYLE:
			{
				HTREEITEM hsel = TreeView_GetSelection(hwndTV);
				if (hsel)
				{
					HTREEITEM hnext = TreeView_GetNextVisible(hwndTV, hsel);
					if (hnext)
					{
						TreeView_Select(hwndTV, hnext, TVGN_CARET);
					}
				}
			}
			break;

		case IDC_STYLEFONT:
			if (pCurrentStyle)
			{
				LOGFONT font = { 0 };
				STYLE style = { sizeof(STYLE) };
				GetSelStyle(style, pCurrentStyle);
				_tcscpy(font.lfFaceName, style.FontName);
				HDC dc = GetDC(NULL);
				font.lfHeight = -MulDiv(sci::ConvertFontSize(pCurrentStyle->Value.iFontSize), GetDeviceCaps(dc, LOGPIXELSY), 72);
				ReleaseDC(NULL, dc);
				font.lfWeight = style.fBold ? FW_BOLD : FW_NORMAL;
				font.lfItalic = style.fItalic ? TRUE : FALSE;
				font.lfUnderline = style.fUnderline ? TRUE : FALSE;
				font.lfCharSet = style.fCharSet;
				if (api::ChooseFontT(m_hWnd, font)) {
					if (SCLEX_ASCII == pCurrentLexer->iLexer)
					{
						_tcscpy(pCurrentStyle->Value.FontName, _T("MS LineDraw"));
						pCurrentStyle->Value.fCharSet = 0x02;
					}
					else
					{
						_tcscpy(pCurrentStyle->Value.FontName, font.lfFaceName);
						pCurrentStyle->Value.fCharSet = font.lfCharSet;
					}
					EDITSTYLE * p = &pApp->Lexer[0]->Styles[0];
					HDC dc = GetDC(NULL);
					pCurrentStyle->Value.iFontSize = LOWORD(-MulDiv(font.lfHeight, 72, GetDeviceCaps(dc, LOGPIXELSY)));
					ReleaseDC(NULL, dc);
					pCurrentStyle->Value.fBold = font.lfWeight > FW_NORMAL;
					pCurrentStyle->Value.fItalic = font.lfItalic > 0;
					pCurrentStyle->Value.fUnderline = font.lfUnderline > 0;
					this->RefreshStyleInfo();
				}
			}
			break;

		case IDC_STYLEFORE:
			if (pCurrentStyle)
			{
				STYLE style = { sizeof(STYLE) };
				GetSelStyle(style, pCurrentStyle);
				if (api::ChooseColorT(m_hWnd, style.rForeColor)) {
					pCurrentStyle->Value.rForeColor = style.rForeColor;
					this->RefreshStyleInfo();
				}
			}
			break;

		case IDC_STYLEBACK:
			if (pCurrentStyle)
			{
				STYLE style = { sizeof(STYLE) };
				GetSelStyle(style, pCurrentStyle);
				if (api::ChooseColorT(m_hWnd, style.rBackColor)) {
					pCurrentStyle->Value.rBackColor = style.rBackColor;
					this->RefreshStyleInfo();
				}
			}
			break;

		case IDOK:
			EndDialog(IDOK);
			break;


		case IDCANCEL:
			if (fDragging)
				SendMessage(WM_CANCELMODE, 0, 0);
			else
				EndDialog(IDCANCEL);
			break;

		}

		return TRUE;
	}

	return FALSE;
}

unsigned int _stdcall DirList::_ThreadAddIcon(void * param)
{
	DirList * _this = (DirList *)param;
	LV_ITEM lvi = { 0 };

	CoInitialize(NULL);
	LPSHELLICON lpshi = NULL;
	int count = ListView_GetItemCount(_this->_hWndLV);
	_this->_lpsf->QueryInterface(IID_IShellIcon, (void **)&lpshi);

	for (int i=0; i<count; i++)
	{
		lvi.iItem = i;
		lvi.mask = LVIF_PARAM;
		if (!ListView_GetItem(_this->_hWndLV, &lvi))
			continue;

		LPITEMIDLIST pidl = (LPITEMIDLIST)lvi.lParam;
		lvi.mask = LVIF_IMAGE;
		if (NOERROR != lpshi->GetIconOf(pidl, GIL_FORSHELL, &lvi.iImage))
			continue;

		// It proved necessary to reset the state bits...
		lvi.stateMask = 0;
		lvi.state = 0;

		// Link and Share Overlay
		DWORD dwAttributes = 0;
		_this->_lpsf->GetAttributesOf(1, (LPCITEMIDLIST *)&pidl, &dwAttributes);

		if (dwAttributes & SFGAO_LINK)
		{
			lvi.mask |= LVIF_STATE;
			lvi.stateMask |= LVIS_OVERLAYMASK;
			lvi.state |= INDEXTOOVERLAYMASK(2);
		}

		if (dwAttributes & SFGAO_SHARE)
		{
			lvi.mask |= LVIF_STATE;
			lvi.stateMask |= LVIS_OVERLAYMASK;
			lvi.state |= INDEXTOOVERLAYMASK(1);
		}

		lvi.iSubItem = 0;
		ListView_SetItem(_this->_hWndLV, &lvi);

		if (TRUE == InterlockedExchange(&_this->_bQuitFlag, FALSE))
			break;
	}

	if (lpshi)
		lpshi->Release();

	CoUninitialize();
	return 0;
}

void DirList::ListViewGetSelString(tstring & out)
{
	ASSERT(_hWndLV);
	out.erase();
	LV_ITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;

	if (ListView_GetSelectedCount(_hWndLV))
		lvi.iItem = ListView_GetNextItem(_hWndLV, -1, LVNI_ALL | LVNI_SELECTED);
	else
		return;

	if (!ListView_GetItem(_hWndLV, &lvi))
		return;

	LPITEMIDLIST pidl = (LPITEMIDLIST)lvi.lParam;
	com::GetNameFromPIDL(out, _lpsf, pidl, true);
}

void DirList::ListViewInit(HWND hWndLV)
{
	HIMAGELIST hil;
	SHFILEINFO shfi;

	// Add Imagelists
	hil = (HIMAGELIST)SHGetFileInfo(_T("C:\\"), 0, &shfi, sizeof(SHFILEINFO),
									SHGFI_SMALLICON | SHGFI_SYSICONINDEX);

	ListView_SetImageList(hWndLV, hil, LVSIL_SMALL);

	hil = (HIMAGELIST)SHGetFileInfo(_T("C:\\"), 0, &shfi, sizeof(SHFILEINFO),
									SHGFI_LARGEICON | SHGFI_SYSICONINDEX);

	ListView_SetImageList(hWndLV, hil, LVSIL_NORMAL);

	_hWndLV = hWndLV;
}

void DirList::ListViewAddPath(const TCHAR * path, bool bShowExt)
{
	ASSERT(path);
	ASSERT(_hWndLV);
	_WaitForThread();

	//reset listview
	SendMessage(_hWndLV, WM_SETREDRAW, FALSE, 0);
	ListView_DeleteAllItems(_hWndLV);

	LV_ITEM lvi = { 0 };
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.iItem = 0;
	lvi.iSubItem = 0;
	//lvi.pszText = LPSTR_TEXTCALLBACK;
	lvi.cchTextMax = MAX_PATH * 2;
	//lvi.iImage = I_IMAGECALLBACK;

	LPSHELLFOLDER lpsfDesktop = NULL;
	if (NOERROR != SHGetDesktopFolder(&lpsfDesktop))
	{
		SendMessage(_hWndLV, WM_SETREDRAW, TRUE, 0);
		return;
	}

	LPITEMIDLIST pidl = NULL;
	ULONG dwAttributes = 0;
	if (_lpsf)
		_lpsf->Release();
	LPENUMIDLIST lpe = NULL;
	WCHAR * wstr;
#ifdef UNICODE
	wstr = (WCHAR *)path;
#else
	util::memory<WCHAR> mem(estimate_ansi_to_utf16(path));
	wstr = mem.ptr();
	convert_ansi_to_utf16(path, wstr);
#endif
	if (NOERROR == lpsfDesktop->ParseDisplayName( // Convert path into a pidl
				_hWndLV,
				NULL,
				wstr,
				NULL,
				&pidl,
				&dwAttributes)
		&& NOERROR == lpsfDesktop->BindToObject(pidl, NULL, // Bind pidl to IShellFolder
				IID_IShellFolder, (void **)&_lpsf)
		// Create an Enumeration object for lpsf
		&& NOERROR == _lpsf->EnumObjects(_hWndLV, SHCONTF_FOLDERS|SHCONTF_NONFOLDERS, &lpe)  )

	{
		SHFILEINFO shfi;
		int iIconFolder, iIconFile;
		// Initialize default icons
		SHGetFileInfo(_T("Icon"), FILE_ATTRIBUTE_DIRECTORY, &shfi, sizeof(SHFILEINFO),
				  SHGFI_USEFILEATTRIBUTES | SHGFI_SMALLICON | SHGFI_SYSICONINDEX);
		iIconFolder = shfi.iIcon;
		SHGetFileInfo(_T("Icon"), FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(SHFILEINFO),
				  SHGFI_USEFILEATTRIBUTES | SHGFI_SMALLICON | SHGFI_SYSICONINDEX);
		iIconFile = shfi.iIcon;

		LPITEMIDLIST pidlEntry = NULL;
		std::list<LPITEMIDLIST> pidlist;
		// Enumerate the contents of lpsf
		while (NOERROR == lpe->Next(1, &pidlEntry, NULL))
		{
			// Check if it's part of the Filesystem
			dwAttributes = SFGAO_FILESYSTEM | SFGAO_FOLDER;
			_lpsf->GetAttributesOf(1, (LPCITEMIDLIST *)&pidlEntry, &dwAttributes);
			if (!(dwAttributes & SFGAO_FILESYSTEM))
				continue;

			if (!_filter.empty() && !(SFGAO_FOLDER & dwAttributes))
			{
				tstring name;
				if (!com::GetNameFromPIDL(name, _lpsf, pidlEntry, true))
					continue;

				bool match = false;
				for (const TCHAR * p = _filter.c_str();
					p < _filter.c_str() + _filter.length(); p += _tcslen(p) + 1)
				{
					if (0 == *p)
						continue;

					if (PathMatchSpec(name.c_str(), p))
					{
						match = true;
						break;
					}
				}

				if (!match)
					continue;
			}

			pidlist.push_back(pidlEntry);
		}

		// this function not work for vc6
		pidlist.sort(PathSort(_lpsf));
		lvi.iItem = 0;
		for (std::list<LPITEMIDLIST>::iterator iter = pidlist.begin();
			iter != pidlist.end(); ++iter, ++lvi.iItem)
		{
			dwAttributes = SFGAO_FILESYSTEM | SFGAO_FOLDER;
			_lpsf->GetAttributesOf(1, (LPCITEMIDLIST *)&*iter, &dwAttributes);
			lvi.lParam = (LPARAM)*iter;
			lvi.iImage = (dwAttributes & SFGAO_FOLDER) ? iIconFolder : iIconFile;

			tstring name;
			if (com::GetNameFromPIDL(name, _lpsf, *iter, bShowExt))
			{
				lvi.pszText = (TCHAR *)name.c_str();
				ListView_InsertItem(_hWndLV, &lvi);
			}
		}

		lpe->Release();
	}

	lpsfDesktop->Release();
	com::FreePIDL(pidl);
	SendMessage(_hWndLV, WM_SETREDRAW, TRUE, 0);
}

void Favorites::ResetListView(const TCHAR * dir)
{
	this->ListViewAddPath(dir);
	this->ListViewAddIcon();
	ListView_EnsureVisible(hWndList, 0, FALSE);
	ListView_SetItemState(hWndList, 0, LVIS_FOCUSED, LVIS_FOCUSED);
	ListView_SetColumnWidth(hWndList, 0, LVSCW_AUTOSIZE);
	ListView_Arrange(hWndList, LVA_DEFAULT);
	SetDlgItemText(IDC_FAVDIR, dir);
}

void Favorites::OnOK()
{
	if (!ListView_GetSelectedCount(hWndList))
		return;

	tstring name;
	this->ListViewGetSelString(name);

	if (!name.empty())
	{
		if (FileExists((name + _T("\\*")).c_str()))
		{
			CurDir = name;
			ResetListView(CurDir.c_str());
			return;
		}
		else if (FileExists(name.c_str()))
		{
			SelFile = name;
			this->EndDialog(IDOK);
		}
	}
}

void Favorites::OnFilterChange()
{
	int sel = this->SendDlgItemMessage(IDC_FILTER, CB_GETCURSEL, 0, 0);
	if (CB_ERR == sel)
		return;

	tstring text;
	if (api::GetDlgItemTextT(text, m_hWnd, IDC_FILTER) <= 0)
		return;

	const util::ptstring pfilter = sci::CreateSupptoredFilter();
	const tchar * filter = pfilter->c_str();
	if (!_tcscmp(text.c_str(), filter))
	{
		filter += _tcslen(filter) + 1;
		if (filter[0])
			this->ListViewSetFilter(filter);
	}
	else
	{
		const util::ptstring pfilter = sci::CreateAllFilter();
		filter = pfilter->c_str();
		if (!_tcscmp(text.c_str(), filter))
		{
			filter += _tcslen(filter) + 1;
			if (filter[0])
				this->ListViewSetFilter(filter);
		}
	}
}

Favorites::Favorites()
: pos(_T("Favorites2"))
, m_resizer(250, 230, 0, 0)
{
	m_resizer(IDOK, CDlgResizer::XY_MOVE)
			(IDCANCEL, CDlgResizer::XY_MOVE)
			(IDC_SIZEBOX, CDlgResizer::XY_MOVE)
			(IDC_GOTOFAVDIR, CDlgResizer::Y_MOVE)
			(IDC_UP, CDlgResizer::Y_MOVE)
			(IDC_FILTER, CDlgResizer::Y_MOVE)
			(IDC_INFO, CDlgResizer::X_SIZE|CDlgResizer::Y_SIZE)
			(IDC_FAVDIR, CDlgResizer::X_SIZE);
}

BOOL Favorites::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_resizer.IsProcess(m_hWnd, uMsg, wParam, lParam))
	{
		if (WM_SIZE == uMsg) {
			//resize the List Control column
			//RECT r;
			//LVCOLUMN lvc = {LVCF_WIDTH, 0, -1, 0, 0, -1};
			//HWND hWndList = GetDlgItem(hWnd, IDC_INFO);
			//GetClientRect(hWndList, &r);
			//lvc.cx = r.right - GetSystemMetrics(SM_CXHSCROLL);
			//ListView_SetColumn(hWndList, 0, &lvc);
			//ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_LABELTIP, LVS_EX_LABELTIP);
		}
		return 1;
	}

	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
			hWndList = GetDlgItem(IDC_INFO);

			ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_LABELTIP, LVS_EX_LABELTIP);
			this->ListViewInit(hWndList);
			CurDir = pApp->FavDir;
			const util::ptstring psupport = sci::CreateSupptoredFilter();
			tstring all;
			api::LoadStringT(all, IDS_FILTER_ALL);
			all += '\n';
			FixMutliString(all);

			this->SendDlgItemMessage(IDC_FILTER, CB_ADDSTRING, 0, psupport->c_str());
			this->SendDlgItemMessage(IDC_FILTER, CB_ADDSTRING, 0, all.c_str());
			this->SendDlgItemMessage(IDC_FILTER, CB_SETCURSEL);
			this->OnFilterChange();

			ResetListView(CurDir.c_str());
			MakeBitmapButton(m_hWnd, IDC_UP, IDB_PREV);

			if (!pos.SetPos(m_hWnd, true))
				this->CenterWindow();
		}
		return TRUE;

	case WM_DESTROY:
		pos.GetPos(m_hWnd);
		return FALSE;


	case WM_NOTIFY:
		{
			LPNMHDR pnmh = (LPNMHDR)lParam;

			if (pnmh->idFrom == IDC_INFO)
			{
				switch (pnmh->code)
				{
				case LVN_ITEMCHANGED:
					{
						NM_LISTVIEW *pnmlv = (NM_LISTVIEW*)lParam;
						EnableWindow(GetDlgItem(IDOK), (pnmlv->uNewState & LVIS_SELECTED));
					}
					break;

				case NM_DBLCLK:
					this->OnOK();
					break;
				}
			}
		}
		return TRUE;


	case WM_COMMAND:

		switch (wParam)
		{

		case MAKELONG(IDC_FILTER, CBN_SELCHANGE):
			this->OnFilterChange();
			this->ResetListView(CurDir.c_str());
			break;

		case IDC_UP:
			if (CurDir.length() > 3) {
				CurDir.erase(CurDir.rfind(_T("\\")));
				if (CurDir.length() < 3) {
					CurDir += _T("\\");
				}
				ResetListView(CurDir.c_str());
			}
			break;

		case IDC_GOTOFAVDIR:
			{
				if (api::SHBrowseForFolderT(pApp->FavDir, m_hWnd, ResStr(IDS_FAVORITES), pApp->FavDir.c_str()))
				{
					CurDir = pApp->FavDir;
					ResetListView(CurDir.c_str());
				}
			}
			break;


		case IDOK:
			this->OnOK();
			break;


		case IDCANCEL:
			this->EndDialog(IDCANCEL);
			break;

		}

		return TRUE;

	}

	return FALSE;
}

OpenWith::OpenWith()
: pos(_T("OpenWith2"))
, iColumnWidth(0)
, m_resizer(300, 250, 0, 0)
{
	m_resizer(IDOK, CDlgResizer::XY_MOVE)
			(IDCANCEL, CDlgResizer::XY_MOVE)
			(IDC_SIZEBOX, CDlgResizer::XY_MOVE)
			(IDC_INFO, CDlgResizer::X_SIZE|CDlgResizer::Y_SIZE)
			(IDC_STATIC_NAME, CDlgResizer::Y_MOVE)
			(IDC_STATIC_PATH, CDlgResizer::Y_MOVE)
			(IDC_NAME, CDlgResizer::X_SIZE|CDlgResizer::Y_MOVE)
			(IDC_PATH, CDlgResizer::X_SIZE|CDlgResizer::Y_MOVE)
			(IDC_BROWSE, CDlgResizer::XY_MOVE)
			(IDC_ADD, CDlgResizer::XY_MOVE)
			(IDC_REMOVE, CDlgResizer::XY_MOVE);
}


BOOL OpenWith::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_resizer.IsProcess(m_hWnd, uMsg, wParam, lParam))
	{
		this->ResetColumnWidth();
		return TRUE;
	}

	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
			lv.Attach(GetDlgItem(IDC_INFO));
			lv.InsertColumn(0, NULL, 50);
			lv.InsertColumn(1, NULL, 50);
			//lv.SetExtendedListViewStyle(LVS_EX_LABELTIP);
			for (unsigned int i=0; i<pApp->Editor.size(); i++)
			{
				lv.InsertItem(i, pApp->Editor[i].first.c_str());
				iColumnWidth = util::max(iColumnWidth, lv.GetStringWidth(pApp->Editor[i].first.c_str()));
				lv.SetItem(i, 1, pApp->Editor[i].second.c_str());
			}
			this->ResetColumnWidth();

			if (!pos.SetPos(m_hWnd, true))
				this->CenterWindow();
		}
		return TRUE;


	case WM_DESTROY:
		pos.GetPos(m_hWnd);
		return FALSE;


	case WM_COMMAND:

		switch (wParam)
		{

		case IDC_BROWSE:
			{
				tstring tmp;
				api::LoadStringT(tmp, IDS_FILTER_EXE);
				tmp += ResStr(IDS_FILTER_ALL);
				tmp += '\n';
				FixMutliString(tmp);

				if (!api::GetOpenFileNameT(tmp, m_hWnd, tmp.c_str()))
					break;

				SetDlgItemText(IDC_PATH, tmp.c_str());
				if (!IsWndTextEmpty(m_hWnd, IDC_NAME))
					break;

				util::rmake_after(tmp, '\\');
				SetDlgItemText(IDC_NAME, tmp.c_str());
			}
			break;

		case IDC_ADD:
			{
				STRPAIR pair;
				api::GetDlgItemTextT(pair.first, m_hWnd, IDC_NAME);
				api::GetDlgItemTextT(pair.second, m_hWnd, IDC_PATH);

				if (pair.first.empty() || pair.second.empty()) {
					this->MessageBox(ResStr(IDS_ERR_EMPTRY), ResStr(IDS_APPTITLE), MB_OK|MB_ICONEXCLAMATION);
					SetFocus(GetDlgItem(pair.first.empty() ? IDC_NAME : IDC_PATH));
				}

				else if (!FileExists(pair.second.c_str())) {
					this->MessageBox(ResStr(IDS_ERR_NOTFOUND), ResStr(IDS_APPTITLE), MB_OK|MB_ICONEXCLAMATION);
					SetFocus(GetDlgItem(IDC_PATH));
				}

				else if (pApp->Editor.end() != std::find(pApp->Editor.begin(), pApp->Editor.end(), pair))
				{
					this->MessageBox(ResStr(IDS_ERR_ALREADY_EXISTS), ResStr(IDS_APPTITLE), MB_OK|MB_ICONEXCLAMATION);
					SetFocus(GetDlgItem(IDC_NAME));
				}

				iColumnWidth = util::max(iColumnWidth, lv.GetStringWidth(pair.first.c_str()));
				int idx = lv.GetItemCount();
				lv.InsertItem(idx, pair.first.c_str());
				lv.SetItem(idx, 1, pair.second.c_str());
				this->ResetColumnWidth();
			}
			break;

		case IDC_REMOVE:
			if (lv.GetSelectedCount()) {
				lv.DeleteItem(lv.GetSelectedItem());
			}
			break;

		case IDOK:
			{
				pApp->Editor.clear();
				int count = lv.GetItemCount();
				if (count > 0) {
					STRPAIR pair;
					pApp->Editor.reserve(count + 1);
					for (int i=0; i<count; i++)
					{
						lv.GetItemText(i, 0, pair.first);
						lv.GetItemText(i, 1, pair.second);
						pApp->Editor.push_back(pair);
					}
				}
			}
		case IDCANCEL:
			this->EndDialog(wParam);
			break;

		}
		break; //WM_COMMAND

	}

	return FALSE;
}


unsigned int _stdcall BatchFormat::ThreadFormatFile(void * param)
{
	BatchFormat * _this = (BatchFormat *)param;

	HWND hWndList = ::GetDlgItem(_this->m_hWnd, IDC_INFO);
	HWND hWndProgress = ::GetDlgItem(_this->m_hWnd, IDC_PROGRESS);
	int count = ListView_GetItemCount(::GetDlgItem(_this->m_hWnd, IDC_INFO));
	if (count <= 0) return 0;

	int nSel = _this->SendDlgItemMessage(IDC_METHOD, CB_GETCURSEL, 0, 0);
	if (nSel < 0) return 0;

	vector<tstring> pathlist;
	for (int i=0; i<count; i++)
	{
		pathlist.push_back(api::GetListControlTextT(hWndList, i, 0));
	}
	::SendMessage(hWndList, LVM_DELETEALLITEMS, 0, 0);

	astyle::ASFormatter formatter;
	ASSERT((unsigned int)nSel < pApp->Formatting.size());
	SetFormatter(formatter, pApp->Formatting[nSel].data);
	::SendMessage(hWndProgress, PBM_SETRANGE, 0, MAKELPARAM(0,pathlist.size()));
	tstring success, fail, formatting;
	api::LoadStringT(success, IDS_FORMATTED);
	api::LoadStringT(fail, IDS_ERR_OPENFILE);
	api::LoadStringT(formatting, IDS_FORMATTING);

	for (unsigned int j=0; j<pathlist.size(); j++)
	{
		_this->ShowMessage(hWndList, (formatting + pathlist[j]).c_str(), false);
		if (_this->FormatFile(pathlist[j].c_str(), formatter)) {
			_this->ShowMessage(hWndList, (success + pathlist[j]).c_str(), true);
		}
		else {
			_this->ShowMessage(hWndList, (fail + pathlist[j]).c_str(), true);
		}
		::SendMessage(hWndProgress, PBM_SETPOS, j+1, 0);
	}

	::PostMessage(_this->m_hWnd, UM_FORMAT_FINISH, 0, 0);
	return 0;
}

void BatchFormat::ShowMessage(HWND hWnd, const TCHAR * Text, bool bUpdate)
{
	LVITEM item = { 0, 0, 0 };
	item.mask = LVIF_TEXT;
	item.iSubItem = 0;
	item.pszText = (LPTSTR)Text;
	item.iItem = ListView_GetItemCount(hWnd) - (bUpdate ? 1 : 0);
	::SendMessage(hWnd, bUpdate ? LVM_SETITEM : LVM_INSERTITEM, 0, (LPARAM)&item);
	if (!bUpdate)
		::SendMessage(hWnd, LVM_SCROLL, 0, 16);
}

bool BatchFormat::FormatFile(const TCHAR * File, astyle::ASFormatter & formatter)
{
	Reader fs;
	if (!fs.open(File, Reader::OPEN_READ_WRITE))
		return false;

	DWORD nSize = (DWORD)fs.get_size();
	util::memory<BYTE> mem(nSize+1);
	DWORD cbData = 0;
	cbData = fs.read(mem, nSize);
	if (0 == cbData)
		return false;

	char * lpData = (char *)mem.ptr();
	lpData[cbData] = 0;
	const char * eol = 0;
	{
		const char * p = lpData;
		unsigned int i;
		for (i=0; i<cbData && p[i] && p[i] != 0x0D && p[i] != 0x0A; i++);

		if (0x0D == p[i-1]) {
			if (i == cbData || 0x0A == p[i]) eol = "\r\n";
			else if (0x0A != p[i]) eol = "\r";
		} else if (0x0A == p[i-1]) {
			if (i == cbData || 0x0D == p[i]) eol = "\n";
		} else eol = "\r\n";
	}

	formatter.init(new astyle::ASBufferIterator(lpData));
	fs.seek(0, Reader::POS_BEGIN);
	while (formatter.hasMoreLines())
	{
		string text = formatter.nextLine();
		if (formatter.hasMoreLines())
			text += eol;
		fs.write(text.c_str(), text.length());
	}
	fs.set_eof();

	return true;
}

BatchFormat::BatchFormat()
: m_resizer(280, 230, 0, 0)
{
	m_resizer.Add(IDOK, CDlgResizer::XY_MOVE)
				(IDCANCEL, CDlgResizer::XY_MOVE)
				(IDC_SIZEBOX, CDlgResizer::XY_MOVE)
				(IDC_PATH, CDlgResizer::X_SIZE|CDlgResizer::Y_MOVE)
				(IDC_PROGRESS, CDlgResizer::X_SIZE|CDlgResizer::Y_MOVE)
				(IDC_STATIC1, CDlgResizer::Y_MOVE)
				(IDC_BROWSE, CDlgResizer::XY_MOVE)
				(IDC_INFO, CDlgResizer::X_SIZE|CDlgResizer::Y_SIZE);
}

BOOL BatchFormat::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_resizer.IsProcess(m_hWnd, uMsg, wParam, lParam))
	{
		if (WM_SIZE == uMsg) {
			//resize the List Control column
			RECT r;
			LVCOLUMN lvc = {LVCF_WIDTH, 0, -1, 0, 0, -1};
			HWND hWndList = GetDlgItem(IDC_INFO);
			::GetClientRect(hWndList, &r);
			lvc.cx = r.right - GetSystemMetrics(SM_CXHSCROLL);
			ListView_SetColumn(hWndList, 0, &lvc);
			ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_LABELTIP, LVS_EX_LABELTIP);
		}
		return 1;
	}

	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
			RECT r;
			LVCOLUMN lvc = {LVCF_WIDTH, 0, -1, 0, 0, -1};
			HWND hWndList = GetDlgItem(IDC_INFO);
			::GetClientRect(hWndList, &r);
			lvc.cx = r.right - GetSystemMetrics(SM_CXHSCROLL);
			ListView_InsertColumn(hWndList, 0, &lvc);
			ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_LABELTIP, LVS_EX_LABELTIP);

			HWND hWndCB = GetDlgItem(IDC_METHOD);
			for (size_t i=0; i<pApp->Formatting.size(); i++)
			{
				if (!pApp->Formatting[i].name.empty())
					::SendMessage(hWndCB, CB_ADDSTRING, 0, (LPARAM)pApp->Formatting[i].name.c_str());
			}
			::SendMessage(hWndCB, CB_SETCURSEL, 0, 0);

			this->CenterWindow();
		}
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			{
				int count = ListView_GetItemCount(GetDlgItem(IDC_INFO));
				if (count <= 0) return TRUE;

				int nSel = this->SendDlgItemMessage(IDC_METHOD, CB_GETCURSEL, 0, 0);
				if (nSel < 0) return TRUE;

				EnableWindow(GetDlgItem(IDOK), FALSE);
				EnableWindow(GetDlgItem(IDC_BROWSE), FALSE);
				EnableWindow(GetDlgItem(IDCANCEL), FALSE);

				unsigned int uThreadID;
				HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadFormatFile, this, 0, &uThreadID);
				CloseHandle(hThread);
			}
			return TRUE;
		case IDCANCEL:
			this->EndDialog(wParam);
			return TRUE;

		case IDC_BROWSE:
			{
				const util::ptstring pfilter = CreateCPPFilter();
				wpf::CFileDialog dlg(true, m_hWnd, pfilter->c_str());
				dlg.AddFlags(OFN_ALLOWMULTISELECT);
				tstring path;
				api::GetDlgItemTextT(path, m_hWnd, IDC_PATH);
				dlg.SetInitDir(path.c_str());
				if (!dlg.DoModal())
					break;

				const std::list<tstring> & l = dlg.GetMultiFileRawData();
				if (l.size() == 0)
					break;

				SetDlgItemText(IDC_PATH, util::rget_before(*l.begin(), '\\').c_str());
				HWND hWndList = GetDlgItem(IDC_INFO);
				for (std::list<tstring>::const_iterator iter = l.begin(); iter != l.end(); ++iter)
				{
					this->ShowMessage(hWndList, iter->c_str());
				}
			}
			break;
		}
		break; // break WM_COMMAND

	case WM_CONTEXTMENU:
		{
			HWND hWndList = GetDlgItem(IDC_INFO);
			if ((HWND)wParam == hWndList)
			{
				int count = ListView_GetItemCount(hWndList);
				if (count > 0)
				{
					HMENU menu = CreatePopupMenu();
					POINT pt;
					AppendMenu(menu, MF_STRING, 1, ResStr(IDT_EDIT_CLEAR));
					if (lParam == ((UINT) - 1))
					{
						RECT r;
						::GetWindowRect(hWndList, &r);
						pt.x = r.left;
						pt.y = r.top;
					}
					else
					{
						pt.x = LOWORD(lParam);
						pt.y = HIWORD(lParam);
					}
					if (1 == TrackPopupMenu(
								menu,
								TPM_NONOTIFY | TPM_RETURNCMD,
								pt.x,
								pt.y,
								0, hWndList, 0))
					{
						int nSel = ::SendMessage(hWndList, LVM_GETSELECTEDCOUNT, 0, 0);
						int nStart = ::SendMessage(hWndList, LVM_GETSELECTIONMARK, 0, 0);
						if (nSel <= 0 || nStart <= 0) return 0;

						for (int i=nStart; i<nStart+nSel; i++)
						{
							::SendMessage(hWndList, LVM_DELETEITEM, i, 0);
						}

					}
				}
			}
		}
		break; //break WM_CONTEXTMENU

	case UM_FORMAT_FINISH:
		EnableWindow(GetDlgItem(IDCANCEL), TRUE);
		SetDlgItemText(IDCANCEL, ResStr(IDS_CLOSE));
		break;

	}

	return 0;
}



unsigned int _stdcall CompileNSIS::ThreadMakeNSIS(void * p)
{
	HWND hwnd = (HWND)p;
	TCHAR cmd[1024];
	::SendMessage(hwnd, UM_GET_COMLIPE_CMD, 0, (LPARAM)cmd);

	STARTUPINFO si = {sizeof(si), };
	SECURITY_ATTRIBUTES sa = {sizeof(sa), };
	SECURITY_DESCRIPTOR sd = {0, };
	PROCESS_INFORMATION pi = {0, };
	HANDLE newstdout = 0, read_stdout = 0;
	OSVERSIONINFO osv = {sizeof(osv)};
	GetVersionEx(&osv);
	if (osv.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&sd, true, NULL, false);
		sa.lpSecurityDescriptor = &sd;
	}
	else
		sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = true;
	if (!CreatePipe(&read_stdout, &newstdout, &sa, 0))
	{
		::PostMessage(hwnd, UM_COMPILE_COMPLETE, MSG_ERR_CREATE_PIPE, 1);
		return 1;
	}
	GetStartupInfo(&si);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.hStdOutput = newstdout;
	si.hStdError = newstdout;
	if (!CreateProcess(NULL, cmd, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
	{
		CloseHandle(newstdout);
		CloseHandle(read_stdout);
		::PostMessage(hwnd, UM_COMPILE_COMPLETE, MSG_ERR_RUN_CMD, 1);
		return 1;
	}
	TCHAR szBuf[1024];
	DWORD dwRead = 1;
	DWORD dwExit = !STILL_ACTIVE;
	tstring unfinish;
	while (dwExit == STILL_ACTIVE || dwRead)
	{
		PeekNamedPipe(read_stdout, 0, 0, 0, &dwRead, NULL);
		if (dwRead)
		{
			ReadFile(read_stdout, szBuf, 1023, &dwRead, NULL);
			szBuf[dwRead] = 0;

			const TCHAR * p1 = szBuf;
			const TCHAR * p2 = p1;
			while (*p2)
			{
				p1 = p2;
				while(*p2 && '\r' != *p2 && '\n' != *p2) p2++;

				if (0 == *p2) {
					if (dwRead < 1023) {
						::PostMessage(hwnd, UM_INFO, 0, (LPARAM)(new tstring(p1, p2)));
					} else {
						unfinish.assign(p1, p2);
					}
				} else {
					tstring * ps = new tstring(p1, p2);
					if (!unfinish.empty()) ps->insert(0, unfinish);
					unfinish.erase();
					::PostMessage(hwnd, UM_INFO, 0, (LPARAM)ps);
				}

				while (*p2 && ('\r' == *p2 || '\n' == *p2)) p2++;
			}
		}
		else
			Sleep(100);
		GetExitCodeProcess(pi.hProcess, &dwExit);
		// Make sure we have no data before killing getting out of the loop
		if (dwExit != STILL_ACTIVE)
		{
			PeekNamedPipe(read_stdout, 0, 0, 0, &dwRead, NULL);
		}
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(newstdout);
	CloseHandle(read_stdout);
	::PostMessage(hwnd, UM_COMPILE_COMPLETE, MSG_EXIT, (LPARAM)dwExit);
	return 0;
}

CompileNSIS::CompileNSIS(const TCHAR * makensis)
: m_cmd(makensis)
, m_resizer(300, 250, 0, 0)
{
	m_resizer.Add(IDCANCEL, CDlgResizer::XY_MOVE)
				(IDC_SIZEBOX, CDlgResizer::XY_MOVE)
				(IDC_TEST, CDlgResizer::XY_MOVE)
				(IDC_INFO, CDlgResizer::X_SIZE|CDlgResizer::Y_SIZE);
}

BOOL CompileNSIS::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HANDLE hThread = NULL;

	if (m_resizer.IsProcess(m_hWnd, uMsg, wParam, lParam))
	{
		if (WM_SIZE == uMsg) {
			//resize the List Control column
			RECT r;
			LVCOLUMN lvc = {LVCF_WIDTH, 0, -1, 0, 0, -1};
			HWND hWndList = GetDlgItem(IDC_INFO);
			::GetClientRect(hWndList, &r);
			lvc.cx = r.right - GetSystemMetrics(SM_CXHSCROLL);
			ListView_SetColumn(hWndList, 0, &lvc);
			ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_LABELTIP, LVS_EX_LABELTIP);
		}
		return 1;
	}

	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
			RECT r;
			LVCOLUMN lvc = {LVCF_WIDTH, 0, -1, 0, 0, -1};
			HWND hWndList = GetDlgItem(IDC_INFO);
			::GetClientRect(hWndList, &r);
			lvc.cx = r.right - GetSystemMetrics(SM_CXHSCROLL);
			ListView_InsertColumn(hWndList, 0, &lvc);
			ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_LABELTIP, LVS_EX_LABELTIP);

			m_cmd << _T(" /NOTIFYHWND ");
			m_cmd << (int)m_hWnd;
			m_cmd << _T(" \"");
			m_cmd << pApp->CurFile.c_str();
			m_cmd << _T("\"");

			EnableWindow(GetDlgItem(IDCANCEL), FALSE);
			unsigned int uThreadID;
			hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadMakeNSIS, m_hWnd, 0, &uThreadID);

			this->CenterWindow();
		}
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		//case IDOK:
		case IDCANCEL:
			this->EndDialog(wParam);
			break;

		case IDC_TEST:
			if (!m_output.empty()) {
				ShellExecuteA(m_hWnd, "open", m_output.c_str(), NULL, NULL, SW_SHOWNORMAL);
			}
			break;
		}
		break; //break WM_COMMAND

	case WM_COPYDATA:
		{
			PCOPYDATASTRUCT cds = PCOPYDATASTRUCT(lParam);
			if (MAKENSIS_NOTIFY_OUTPUT == cds->dwData) {
				m_output = (char *)cds->lpData;
			}
		}
		break;

	case UM_GET_COMLIPE_CMD:
		_tcscpy((TCHAR *)lParam, m_cmd.c_str());
		return 1;

	case UM_INFO:
		{
			tstring * ps = (tstring *)lParam;
			HWND hWndList = GetDlgItem(IDC_INFO);
			LVITEM item;
			item.mask = LVIF_TEXT;
			item.pszText = (LPTSTR)ps->c_str();
			item.iItem = ListView_GetItemCount(hWndList);
			item.iSubItem = 0;
			::SendMessage(hWndList, LVM_INSERTITEM, 0, (LPARAM)&item);
			::SendMessage(hWndList, LVM_SCROLL, 0, 16);
			delete ps;
		}
		break;

	case UM_COMPILE_COMPLETE:
		switch (wParam)
		{
		case MSG_ERR_CREATE_PIPE:
		case MSG_ERR_RUN_CMD:
			MessageBox(ResStr(IDS_ERR_COMPILE_NSIS), ResStr(IDS_APPTITLE), MB_OK|MB_ICONEXCLAMATION);
			PostMessage(WM_COMMAND, IDCANCEL, 0);
			break;
		case MSG_EXIT:
			if (0 == lParam) {
				// success
				if (!m_output.empty()) {
					EnableWindow(GetDlgItem(IDC_TEST), TRUE);
				}
				MessageBeep(MB_ICONASTERISK);
			} else if (1 == lParam) {
				// error
				MessageBeep(MB_ICONHAND);
			}
			break;
		}

		if (hThread) {
			VERIFY(CloseHandle(hThread));
			hThread = NULL;
		}
		EnableWindow(GetDlgItem(IDCANCEL), TRUE);
		break;

	}

	return 0;
}



//inline BOOL About::PtInItem(int id, const POINT & pt)
//{
//	RECT rc;
//	::GetWindowRect(GetDlgItem(id), &rc);
//	::ScreenToClient(m_hWnd, (LPPOINT)&rc);
//	::ScreenToClient(m_hWnd, (LPPOINT)&rc + 1);
//	return PtInRect(&rc, pt);
//}
//
//int About::IdFromPt(const POINT & pt)
//{
//	int id = -1;
//	if (this->PtInItem(IDC_EMAIL, pt))
//		id = IDC_EMAIL;
//	else if (this->PtInItem(IDC_WEBPAGE, pt))
//		id = IDC_WEBPAGE;
//	else if (this->PtInItem(IDC_WEBPAGE2, pt))
//		id = IDC_WEBPAGE2;
//	return id;
//}

INT_PTR About::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CHECK_STATIC_LINK_MESSAGE(m_web1, m_hWnd, uMsg, wParam, lParam);
	CHECK_STATIC_LINK_MESSAGE(m_web2, m_hWnd, uMsg, wParam, lParam);
	CHECK_STATIC_LINK_MESSAGE(m_web3, m_hWnd, uMsg, wParam, lParam);
	CHECK_STATIC_LINK_MESSAGE(m_web4, m_hWnd, uMsg, wParam, lParam);
	CHECK_STATIC_LINK_MESSAGE(m_web5, m_hWnd, uMsg, wParam, lParam);
	CHECK_STATIC_LINK_MESSAGE(m_web6, m_hWnd, uMsg, wParam, lParam);

	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			SetWindowText(ResStr(IDS_ABOUT));
			this->CenterWindow();
			SetDlgItemText(IDC_VERSION, 
				util::format(_T("%s MOD v%s %s"), ResStr(IDS_APPTITLE).GetPtr(), APP_VERSION_STRING, ResStr(IDS_VERSION).GetPtr()));
			MessageBeep(MB_ICONINFORMATION);
		}
		return TRUE;

	case WM_COMMAND:

		switch (wParam)
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(wParam);
			break;
		}
		return TRUE;
	}

	return 0;
}

FormatSave::FormatSave()
: m_resizer(270, 250, 0, 0)
{
	m_resizer(IDOK, CDlgResizer::XY_MOVE)
			(IDCANCEL, CDlgResizer::XY_MOVE)
			(IDC_SIZEBOX, CDlgResizer::XY_MOVE)
			(IDC_NAME, CDlgResizer::X_SIZE)
			(IDC_INFO, CDlgResizer::X_SIZE|CDlgResizer::Y_SIZE);
}

INT_PTR FormatSave::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_resizer.IsProcess(m_hWnd, uMsg, wParam, lParam))
		return 1;

	switch (uMsg)
	{

	case WM_INITDIALOG:
		this->CenterWindow();
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			if (api::GetDlgItemTextT(m_name, m_hWnd, IDC_NAME) <= 0) {
				this->MessageBox(ResStr(IDS_ERR_EMPTRY), ResStr(IDS_ERROR), MB_OK|MB_ICONEXCLAMATION);
				break;
			}
			api::GetDlgItemTextT(m_description, m_hWnd, IDC_INFO);
		case IDCANCEL:
			this->EndDialog(wParam);
			break;
		}
		break;
	}

	return 0;
}


INT_PTR Format::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{

	case WM_INITDIALOG:
		this->CenterWindow();
		{
			HWND hWndCB = GetDlgItem(IDC_PREDEFINE);
			for (unsigned int i = 0; i < pApp->Formatting.size(); i++)
			{
				if (!pApp->Formatting[i].name.empty())
					::SendMessage(hWndCB, CB_ADDSTRING, 0, (LPARAM)pApp->Formatting[i].name.c_str());
			}
			if (pApp->Formatting.size())
			{
				::SendMessage(hWndCB, CB_SETCURSEL, 0, 0);
				this->SetControl(pApp->Formatting[0].data);
				SendDlgItemMessage(IDC_DESCRIPTION, WM_SETTEXT, 0, pApp->Formatting[0].description.c_str());
			}
		}
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			{
				memset(&m_data, 0, sizeof(FMTDATA));
				m_data.nStructSize = sizeof(FMTDATA);
				this->MakeData(m_data);
			}
		case IDCANCEL:
			this->EndDialog(wParam);
			break;

		case MAKELONG(IDC_PREDEFINE, CBN_SELCHANGE):
			{
				int lResult = this->SendDlgItemMessage(IDC_PREDEFINE, CB_GETCURSEL, 0, 0);
				if (CB_ERR == lResult)
					break;

				this->SetControl(pApp->Formatting[lResult].data);
				SendDlgItemMessage(IDC_DESCRIPTION, WM_SETTEXT, 0, pApp->Formatting[lResult].description.c_str());
			}
			break;

		case IDC_SETDEFAULT:
			{
				int lResult = this->SendDlgItemMessage(IDC_PREDEFINE, CB_GETCURSEL, 0, 0);
				if (CB_ERR == lResult)
					break;

				FMTPARAM Default;
				vector<FMTPARAM>::iterator iter = find(pApp->Formatting.begin(), pApp->Formatting.end(), Default);
				if (iter != pApp->Formatting.end()) {
					iter->data = pApp->Formatting[lResult].data;
				}
			}
			break;

		case IDC_SAVEAS:
			{
				wnd::FormatSave dlg;
				if (IDOK == dlg.DoModal(m_hWnd))
				{
					FMTPARAM fmt;
					fmt.name = dlg.GetName();
					fmt.description = dlg.GetDescription();
					this->MakeData(fmt.data);

					vector<FMTPARAM>::iterator iter = find(pApp->Formatting.begin(), pApp->Formatting.end(), fmt);
					if (iter != pApp->Formatting.end()) {
						pApp->Formatting.erase(iter);
					}
					pApp->Formatting.push_back(fmt);
				}
			}
			break;

		}
		break; // break case WM_COMMAND

	}

	return 0;
}

void Format::MakeData(FMTDATA & data)
{
	if (BST_CHECKED == SendDlgItemMessage(IDC_C, BM_GETCHECK, 0, 0))
	{
		data.Style = 'c';
	}
	else
	{
		data.Style = 'j';
	}

	TCHAR lpNum[64];
	int nNum;

	GetDlgItemText(IDC_SPACE_NUM, lpNum, sizeof(lpNum));
	if (BST_CHECKED == SendDlgItemMessage(IDC_SPACE, BM_GETCHECK, 0, 0))
	{
		data.Indentation = util::max(1, util::xtoi(lpNum));
	}
	else
	{
		data.Indentation = util::max(1, util::xtoi(lpNum)) | 0xFF00;
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_INDENT_CLASS, BM_GETCHECK, 0, 0))
	{
		data.ClassIndent = true;
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_INDENT_SWITCH, BM_GETCHECK, 0, 0))
	{
		data.SwitchIndent = true;
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_INDENT_CASE, BM_GETCHECK, 0, 0))
	{
		data.CaseIndent = true;
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_INDENT_BRACKET, BM_GETCHECK, 0, 0))
	{
		data.BracketIndent = true;
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_INDENT_BLOCK, BM_GETCHECK, 0, 0))
	{
		data.BlockIndent = true;
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_INDENT_NAMESPACE, BM_GETCHECK, 0, 0))
	{
		data.NamespaceIndent = true;
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_INDENT_LABEL, BM_GETCHECK, 0, 0))
	{
		data.LabelIndent = true;
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_INDENT_PREPROCESSOR, BM_GETCHECK, 0, 0))
	{
		data.PreprocessorIndent = true;
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_CONVERT_TAB, BM_GETCHECK, 0, 0))
	{
		data.TabSpaceConversionMode = true;
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_FILL_EMPTY_LINE, BM_GETCHECK, 0, 0))
	{
		data.EmptyLineFill = true;
	}

	GetDlgItemText(IDC_MAX_INDENT, lpNum, sizeof(lpNum));
	nNum = util::xtoi(lpNum);
	if (nNum <= 0)
		nNum = 40;
	data.MaxInStatementIndentLength = nNum;

	GetDlgItemText(IDC_MAX_INDENT, lpNum, sizeof(lpNum));
	nNum = util::xtoi(lpNum);
	if (nNum < 0)
		nNum = 0;
	data.MinConditionalIndentLength = nNum;

	if (BST_CHECKED == SendDlgItemMessage(IDC_BLOCK_STD, BM_GETCHECK, 0, 0))
	{
		data.BracketFormatMode = BREAK_MODE;
	}
	else if (BST_CHECKED == SendDlgItemMessage(IDC_BLOCK_ATTACH, BM_GETCHECK, 0, 0))
	{
		data.BracketFormatMode = ATTACH_MODE;
	}
	else if (BST_CHECKED == SendDlgItemMessage(IDC_BLOCK_LINUX, BM_GETCHECK, 0, 0))
	{
		data.BracketFormatMode = BDAC_MODE;
	}
	else
	{
		data.BracketFormatMode = NONE_MODE;
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_PAD_BLOCK_HEADER, BM_GETCHECK, 0, 0))
	{
		data.BreakBlocksMode = true;
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_SEPER_ELSEIF, BM_GETCHECK, 0, 0))
	{
		data.BreakElseIfsMode = true;
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_PAD_OPERATION, BM_GETCHECK, 0, 0))
	{
		data.OperatorPaddingMode = true;
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_PAD_PARENT, BM_GETCHECK, 0, 0))
	{
		data.ParenthesisPaddingMode = true;
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_MIX_STATEMENT, BM_GETCHECK, 0, 0))
	{
		data.SingleStatementsMode = true;
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_MIX_BLOCK, BM_GETCHECK, 0, 0))
	{
		data.BreakOneLineBlocksMode = true;
	}
}

void Format::ResetControl()
{
	SendDlgItemMessage(IDC_C, BM_CLICK, 0, 0);
	SendDlgItemMessage(IDC_SPACE, BM_CLICK, 0, 0);
	SetDlgItemText(IDC_SPACE_NUM, _T("4"));
	SendDlgItemMessage(IDC_BLOCK_STD, BM_CLICK, 0, 0);
	SendDlgItemMessage(IDC_INDENT_CLASS, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_INDENT_SWITCH, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_INDENT_CASE, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_INDENT_BRACKET, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_INDENT_BLOCK, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_INDENT_NAMESPACE, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_INDENT_LABEL, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_INDENT_PREPROCESSOR, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_CONVERT_TAB, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_FILL_EMPTY_LINE, BM_SETCHECK, BST_UNCHECKED, 0);
	SetDlgItemText(IDC_MAX_INDENT, _T("0"));
	SetDlgItemText(IDC_MIN_CI, _T("40"));
	SendDlgItemMessage(IDC_PAD_BLOCK_HEADER, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_SEPER_ELSEIF, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_PAD_OPERATION, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_PAD_PARENT, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_MIX_STATEMENT, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_MIX_BLOCK, BM_SETCHECK, BST_UNCHECKED, 0);
}

void Format::SetControl(const FMTDATA & data)
{
	ResetControl();

	if (data.Style == 'j')
		SendDlgItemMessage(IDC_JAVA, BM_CLICK, 0, 0);

	if (HIBYTE(data.Indentation))
		SendDlgItemMessage(IDC_TAB, BM_CLICK, 0, 0);
	SetDlgItemText(IDC_SPACE_NUM, util::itot(LOBYTE(data.Indentation)));

	if (BREAK_MODE == data.BracketFormatMode)
		SendDlgItemMessage(IDC_BLOCK_STD, BM_CLICK, 0, 0);
	else if (ATTACH_MODE == data.BracketFormatMode)
		SendDlgItemMessage(IDC_BLOCK_ATTACH, BM_CLICK, 0, 0);
	else if (BDAC_MODE == data.BracketFormatMode)
		SendDlgItemMessage(IDC_BLOCK_LINUX, BM_CLICK, 0, 0);
	else
		SendDlgItemMessage(IDC_BLOCK_BCH, BM_CLICK, 0, 0);

	if (data.ClassIndent)
		SendDlgItemMessage(IDC_CLASS, BM_SETCHECK, BST_CHECKED, 0);

	if (data.SwitchIndent)
		SendDlgItemMessage(IDC_CLASS, IDC_INDENT_SWITCH, BST_CHECKED, 0);

	if (data.CaseIndent)
		SendDlgItemMessage(IDC_CASE, BM_SETCHECK, BST_CHECKED, 0);

	if (data.BracketIndent)
		SendDlgItemMessage(IDC_INDENT_BRACKET, BM_SETCHECK, BST_CHECKED, 0);

	if (data.BlockIndent)
		SendDlgItemMessage(IDC_INDENT_BLOCK, BM_SETCHECK, BST_CHECKED, 0);

	if (data.NamespaceIndent)
		SendDlgItemMessage(IDC_INDENT_NAMESPACE, BM_SETCHECK, BST_CHECKED, 0);

	if (data.LabelIndent)
		SendDlgItemMessage(IDC_INDENT_LABEL, BM_SETCHECK, BST_CHECKED, 0);

	if (data.PreprocessorIndent)
		SendDlgItemMessage(IDC_INDENT_PREPROCESSOR, BM_SETCHECK, BST_CHECKED, 0);

	if (data.TabSpaceConversionMode)
		SendDlgItemMessage(IDC_CONVERT_TAB, BM_SETCHECK, BST_CHECKED, 0);

	if (data.EmptyLineFill)
		SendDlgItemMessage(IDC_FILL_EMPTY_LINE, BM_SETCHECK, BST_CHECKED, 0);

	if (data.BreakBlocksMode)
		SendDlgItemMessage(IDC_PAD_BLOCK_HEADER, BM_SETCHECK, BST_CHECKED, 0);

	if (data.BreakElseIfsMode)
		SendDlgItemMessage(IDC_SEPER_ELSEIF, BM_SETCHECK, BST_CHECKED, 0);

	if (data.OperatorPaddingMode)
		SendDlgItemMessage(IDC_PAD_OPERATION, BM_SETCHECK, BST_CHECKED, 0);

	if (data.ParenthesisPaddingMode)
		SendDlgItemMessage(IDC_PAD_PARENT, BM_SETCHECK, BST_CHECKED, 0);

	if (data.SingleStatementsMode)
		SendDlgItemMessage(IDC_MIX_STATEMENT, BM_SETCHECK, BST_CHECKED, 0);

	if (data.BreakOneLineBlocksMode)
		SendDlgItemMessage(IDC_MIX_BLOCK, BM_SETCHECK, BST_CHECKED, 0);

	SetDlgItemText(IDC_MAX_INDENT, util::itot(data.MaxInStatementIndentLength));
	SetDlgItemText(IDC_MIN_CI, util::itot(data.MinConditionalIndentLength));
}

HWND FindReplace::hWndStatic = NULL;

FindReplace::FindReplace(METHOD Method, util::memory<TCHAR> & InputText)
: xm_method(Method)
, m_text(InputText)
{
	ASSERT(m_text.ptr());
}

void FindReplace::CheckControl(bool bState)
{
	EnableDlgItem(IDOK, bState);
	if (MT_REPLACE == xm_method)
	{
		EnableDlgItem(IDC_REPLACE, bState);
		EnableDlgItem(IDC_REPLACEALL, bState);
		EnableDlgItem(IDC_REPLACEINSEL, bState);
	}
	else if (MT_COLLECT == xm_method)
	{
		EnableDlgItem(IDC_COLLECT, bState);
	}
}

void FindReplace::OnSwitch()
{
	RECT r1, r2;
	long cy = 0;

	switch (xm_method)
	{
	case MT_FIND:
		{
			EnableDlgItem(IDC_DO_FIND, false);
			EnableDlgItem(IDC_DO_REPLACE, true);
			EnableDlgItem(IDC_DO_COLLECT, true);

			ShowDlgItem(m_hWnd, IDC_REPLACE, SW_HIDE);
			ShowDlgItem(m_hWnd, IDC_REPLACEALL, SW_HIDE);
			ShowDlgItem(m_hWnd, IDC_REPLACEINSEL, SW_HIDE);
			ShowDlgItem(m_hWnd, IDC_COLLECT, SW_HIDE);

			GetChildRect(m_hWnd, IDC_REPLACE, &r1);
			this->MoveTo(IDCANCEL, r1.top);

			ShowDlgItem(m_hWnd, IDC_REPLACETEXT, SW_HIDE);
			ShowDlgItem(m_hWnd, IDC_STATIC_REPLACETEXT, SW_HIDE);
			ShowDlgItem(m_hWnd, IDC_MENU_REPLACE, SW_HIDE);

			GetChildRect(m_hWnd, IDC_STATIC_REPLACETEXT, &r1);
			GetChildRect(m_hWnd, IDC_FINDCASE, &r2);
			this->MoveTo(IDC_FINDCASE, r1.top);
			GetChildRect(m_hWnd, IDC_FINDCASE, &r1);
			cy = r1.top - r2.top;
			this->MoveDlgItem(IDC_FINDWORD, cy);
			this->MoveDlgItem(IDC_FINDSTART, cy);
			this->MoveDlgItem(IDC_FINDREGEXP, cy);

			this->MoveDlgItem(IDC_FINDUP, cy);
			this->MoveDlgItem(IDC_FINDCLOSE, cy);
		}
		break;

	case MT_REPLACE:
		{
			EnableDlgItem(IDC_DO_FIND, true);
			EnableDlgItem(IDC_DO_REPLACE, false);
			EnableDlgItem(IDC_DO_COLLECT, true);

			ShowDlgItem(m_hWnd, IDC_REPLACE, SW_SHOW);
			ShowDlgItem(m_hWnd, IDC_REPLACEALL, SW_SHOW);
			ShowDlgItem(m_hWnd, IDC_REPLACEINSEL, SW_SHOW);
			ShowDlgItem(m_hWnd, IDC_COLLECT, SW_HIDE);

			ShowDlgItem(m_hWnd, IDC_REPLACETEXT, SW_SHOW);
			ShowDlgItem(m_hWnd, IDC_STATIC_REPLACETEXT, SW_SHOW);
			ShowDlgItem(m_hWnd, IDC_MENU_REPLACE, SW_SHOW);

			EnableDlgItem(IDC_REPLACETEXT, true);
			EnableDlgItem(IDC_STATIC_REPLACETEXT, true);

			GetChildRect(m_hWnd, IDC_YPOS, &r1);
			this->MoveTo(IDCANCEL, r1.top);

			GetChildRect(m_hWnd, IDC_FINDCASE, &r2);
			cy = r1.top - r2.top;
			if (0 == cy)
				break;

			GetChildRect(m_hWnd, IDC_STATIC_REPLACETEXT, &r2);
			cy = r1.top - r2.top;

			this->MoveDlgItem(IDC_FINDREGEXP, cy);
			this->MoveDlgItem(IDC_FINDSTART, cy);
			this->MoveDlgItem(IDC_FINDWORD, cy);
			this->MoveDlgItem(IDC_FINDCASE, cy);

			this->MoveDlgItem(IDC_FINDCLOSE, cy);
			this->MoveDlgItem(IDC_FINDUP, cy);
		}
		break;

	case MT_COLLECT:
		{
			EnableDlgItem(IDC_DO_FIND, true);
			EnableDlgItem(IDC_DO_REPLACE, true);
			EnableDlgItem(IDC_DO_COLLECT, false);

			ShowDlgItem(m_hWnd, IDC_REPLACE, SW_HIDE);
			ShowDlgItem(m_hWnd, IDC_REPLACEALL, SW_HIDE);
			ShowDlgItem(m_hWnd, IDC_REPLACEINSEL, SW_HIDE);
			ShowDlgItem(m_hWnd, IDC_COLLECT, SW_SHOW);

			GetChildRect(m_hWnd, IDC_REPLACEALL, &r1);
			this->MoveTo(IDCANCEL, r1.top);

			GetChildRect(m_hWnd, IDC_YPOS, &r1);

			GetChildRect(m_hWnd, IDC_FINDCASE, &r2);
			cy = r1.top - r2.top;
			if (0 == cy)
				break;

			GetChildRect(m_hWnd, IDC_STATIC_REPLACETEXT, &r2);
			cy = r1.top - r2.top;

			this->MoveDlgItem(IDC_FINDREGEXP, cy);
			this->MoveDlgItem(IDC_FINDSTART, cy);
			this->MoveDlgItem(IDC_FINDWORD, cy);
			this->MoveDlgItem(IDC_FINDCASE, cy);

			this->MoveDlgItem(IDC_FINDCLOSE, cy);
			this->MoveDlgItem(IDC_FINDUP, cy);

			ShowDlgItem(m_hWnd, IDC_REPLACETEXT, SW_SHOW);
			ShowDlgItem(m_hWnd, IDC_STATIC_REPLACETEXT, SW_SHOW);
			ShowDlgItem(m_hWnd, IDC_MENU_REPLACE, SW_SHOW);
		}
		break;
	}

	if (cy)
	{
		::GetWindowRect(m_hWnd, &r1);
		::MoveWindow(m_hWnd, r1.left, r1.top, r1.right - r1.left, r1.bottom - r1.top + cy, TRUE);
	}
}

void FindReplace::DoMRU(const TCHAR * FindText)
{
	static MostRecentlyUsed mruf(20, APP_REG_ROOT, APP_REG_KEY _T("\\MRU\\Find"), true);
	static MostRecentlyUsed mrur(20, APP_REG_ROOT, APP_REG_KEY _T("\\MRU\\Replace"), true);
	HWND h = GetDlgItem(IDC_FINDTEXT);

	if (NULL == FindText)
	{
		for (int i = (int)mruf.Size(); i > 0; i--)
		{
			::SendMessage(h, CB_ADDSTRING, 0, (LPARAM)mruf[i - 1]);
		}
		if (MT_REPLACE == xm_method) {
			h = GetDlgItem(IDC_REPLACETEXT);
			for (int i = (int)mrur.Size(); i > 0; i--)
			{
				::SendMessage(h, CB_ADDSTRING, 0, (LPARAM)mrur[i - 1]);
			}
		}
	}
	else
	{
		if (FindText[0])
			mruf.Add(FindText);

		tstring rep;
		if (api::GetDlgItemTextT(rep, m_hWnd, IDC_REPLACETEXT) > 0)
			mrur.Add(rep.c_str());
	}
}

INT_PTR FindReplace::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static WindowPos pos(_T("FindReplace"));

	switch (uMsg)
	{

	case WM_INITDIALOG:
		OnInitDialog();
		if (!pos.SetPos(m_hWnd))
		{
			this->CenterWindow();
		}

		pApp->AddDialog(m_hWnd);
		hWndStatic = m_hWnd;
		return TRUE;

	case WM_CLOSE:
		DestroyWindow();
		break;

	case WM_DESTROY:
		xm_theme.Unload();
		x_cbFind.RestoreProc();
		x_cbReplace.RestoreProc();
		pos.GetPos(m_hWnd);
		pApp->RemoveDialog(m_hWnd);
		hWndStatic = NULL;
		break;

	case WM_COMMAND:

		switch (wParam)
		{

		case IDC_MENU_REPLACE:
			OnReplaceMenu();
			break;

		case IDC_MENU_FIND:
			OnFindMenu();
			break;

		case IDC_DO_FIND:
			xm_method = MT_FIND;
			OnSwitch();
			this->CheckControl(!IsWndTextEmpty(m_hWnd, IDC_FINDTEXT));
			if (!::SetFocus(GetDlgItem(IDOK)))
				::SetFocus(GetDlgItem(IDC_FINDTEXT));
			break;

		case IDC_DO_REPLACE:
			xm_method = MT_REPLACE;
			OnSwitch();
			this->CheckControl(!IsWndTextEmpty(m_hWnd, IDC_FINDTEXT));
			if (!::SetFocus(GetDlgItem(IDOK)))
				::SetFocus(GetDlgItem(IDC_FINDTEXT));
			break;

		case IDC_DO_COLLECT:
			xm_method = MT_COLLECT;
			OnSwitch();
			this->CheckControl(!IsWndTextEmpty(m_hWnd, IDC_FINDTEXT));
			if (!::SetFocus(GetDlgItem(IDOK)))
				::SetFocus(GetDlgItem(IDC_FINDTEXT));
			break;

		case MAKELONG(IDC_FINDTEXT, CBN_CLOSEUP):
			PostMessage(WM_COMMAND, MAKELONG(IDC_FINDTEXT, CBN_EDITCHANGE), 0);
			::SetFocus(GetDlgItem(IDOK));
			break;

		case MAKELONG(IDC_FINDTEXT, CBN_EDITCHANGE):
			this->CheckControl(!IsWndTextEmpty(m_hWnd, IDC_FINDTEXT));
			break;

		case IDC_FINDCASE:
			if (IsDlgButtonChecked(m_hWnd, wParam) == BST_CHECKED)
				pApp->d_FindFlag |= SCFIND_MATCHCASE;
			else
				pApp->d_FindFlag &= ~SCFIND_MATCHCASE;
			break;

		case IDC_FINDWORD:
			if (IsDlgButtonChecked(m_hWnd, wParam) == BST_CHECKED)
				pApp->d_FindFlag |= SCFIND_WHOLEWORD;
			else
				pApp->d_FindFlag &= ~SCFIND_WHOLEWORD;
			break;

		case IDC_FINDSTART:
			if (IsDlgButtonChecked(m_hWnd, wParam) == BST_CHECKED)
				pApp->d_FindFlag |= SCFIND_WORDSTART;
			else
				pApp->d_FindFlag &= ~SCFIND_WORDSTART;
			break;

		case IDC_FINDREGEXP:
			if (IsDlgButtonChecked(m_hWnd, wParam) == BST_CHECKED)
			{
				pApp->d_FindFlag |= SCFIND_REGEXP;
			}
			else
			{
				pApp->d_FindFlag &= ~SCFIND_REGEXP;
			}
			EnableDlgItem(IDC_FINDUP, 0 == (pApp->d_FindFlag & SCFIND_REGEXP));
			EnableDlgItem(IDC_FINDSTART, 0 == (pApp->d_FindFlag & SCFIND_REGEXP));
			EnableDlgItem(IDC_FINDWORD, 0 == (pApp->d_FindFlag & SCFIND_REGEXP));
			EnableDlgItem(IDC_MENU_FIND, 0 != (pApp->d_FindFlag & SCFIND_REGEXP));
			EnableDlgItem(IDC_MENU_REPLACE, 0 != (pApp->d_FindFlag & SCFIND_REGEXP));
			if (MT_COLLECT == this->xm_method)
			{
				EnableDlgItem(IDC_REPLACETEXT, 0 != (pApp->d_FindFlag & SCFIND_REGEXP));
				EnableDlgItem(IDC_STATIC_REPLACETEXT, 0 != (pApp->d_FindFlag & SCFIND_REGEXP));
			}
			break;

		case IDC_FINDUP:
			if (IsDlgButtonChecked(m_hWnd, wParam) == BST_CHECKED)
				pApp->d_FindFlag |= SCFIND_FINDUP;
			else
				pApp->d_FindFlag &= ~SCFIND_FINDUP;
			break;

		case IDC_FINDCLOSE:
			if (IsDlgButtonChecked(m_hWnd, wParam) == BST_CHECKED)
				pApp->d_FindFlag |= SCFIND_AUTOCLOSE;
			else
				pApp->d_FindFlag &= ~SCFIND_AUTOCLOSE;
			break;

		case IDOK: //find
		case IDC_REPLACE:
		case IDC_REPLACEALL:
		case IDC_REPLACEINSEL:
		case IDC_COLLECT:
			{
				tstring find;
				if (api::GetDlgItemTextT(find, m_hWnd, IDC_FINDTEXT) <= 0) {
					this->CheckControl(FALSE);
					return TRUE;
				}

				::ShowWindow(m_hWnd, SW_HIDE);
				this->DoMRU(find.c_str()); //save
				this->DoMRU(NULL); //load

				if (IDOK == wParam)  // find
					if (!(pApp->d_FindFlag & SCFIND_REGEXP) && (pApp->d_FindFlag & SCFIND_FINDUP))
						pDoc->FindPrev(find.c_str(), pApp->d_FindFlag);
					else
						pDoc->FindNext(find.c_str(), pApp->d_FindFlag);
				else if (MT_REPLACE == xm_method)
				{
					tstring rep;
					api::GetDlgItemTextT(rep, m_hWnd, IDC_REPLACETEXT);
					if (IDC_REPLACE == wParam)
						pDoc->Replace(find.c_str(), rep.c_str(), pApp->d_FindFlag);
					else if (IDC_REPLACEALL == wParam)
						pDoc->ReplaceAll(find.c_str(), rep.c_str(), true, pApp->d_FindFlag);
					else if (IDC_REPLACEINSEL == wParam)
						pDoc->ReplaceSelection(find.c_str(), rep.c_str(), true, pApp->d_FindFlag);
				}
				else if (MT_COLLECT == xm_method)
				{
					tstring rep;
					api::GetDlgItemTextT(rep, m_hWnd, IDC_REPLACETEXT);
					pDoc->Collect(find.c_str(), rep.c_str(), 0 != (pApp->d_FindFlag & SCFIND_MATCHCASE), 0 != (pApp->d_FindFlag & SCFIND_REGEXP));
				}

				m_text.resize(find.length() + 1);
				m_text.assign(find.c_str(), find.length() + 1);
				::ShowWindow(m_hWnd, SW_SHOW);

				if (BST_CHECKED != SendDlgItemMessage(IDC_FINDCLOSE, BM_GETCHECK, 0, 0))
					break;
			}

		case IDCANCEL:
			SendMessage(WM_CLOSE);
			break;

		}
		break; //WM_COMMAND

	}

	return 0;
}

// 界面初始化
void FindReplace::OnInitDialog()
{
	xm_theme.Load();
	xm_theme.EnableThemeDialogTexture(m_hWnd);
	x_cbFind.Attach(m_hWnd, IDC_FINDTEXT);
	x_cbReplace.Attach(m_hWnd, IDC_REPLACETEXT);
	MakeBitmapButton(m_hWnd, IDC_MENU_FIND, IDB_RIGHT);
	MakeBitmapButton(m_hWnd, IDC_MENU_REPLACE, IDB_RIGHT);

	wpf::ShowDlgItem(m_hWnd, IDC_YPOS, SW_HIDE);
	this->OnSwitch();

	// Load MRUs
	this->DoMRU(NULL);

	if (m_text[0])
	{
		x_cbFind.SetEditText(m_text.ptr());
	}

	if (pApp->d_FindFlag & SCFIND_MATCHCASE)
		CheckDlgButton(m_hWnd, IDC_FINDCASE, BST_CHECKED);

	if (pApp->d_FindFlag & SCFIND_WHOLEWORD)
		CheckDlgButton(m_hWnd, IDC_FINDWORD, BST_CHECKED);

	if (pApp->d_FindFlag & SCFIND_WORDSTART)
		CheckDlgButton(m_hWnd, IDC_FINDSTART, BST_CHECKED);

	if (pApp->d_FindFlag & SCFIND_REGEXP)
	{
		CheckDlgButton(m_hWnd, IDC_FINDREGEXP, BST_CHECKED);
		EnableDlgItem(IDC_FINDUP, false);
		EnableDlgItem(IDC_FINDSTART, false);
		EnableDlgItem(IDC_FINDWORD, false);
	}
	else
	{
		EnableDlgItem(IDC_MENU_FIND, false);
		EnableDlgItem(IDC_MENU_REPLACE, false);
	}

	if (pApp->d_FindFlag & SCFIND_FINDUP)
		CheckDlgButton(m_hWnd, IDC_FINDUP, BST_CHECKED);

	if (pApp->d_FindFlag & SCFIND_AUTOCLOSE)
		CheckDlgButton(m_hWnd, IDC_FINDCLOSE, BST_CHECKED);

	this->CheckControl(0 != m_text[0]);
}

void FindReplace::OnReplaceMenu()
{
	HMENU hMenu = LoadMenu(WpfGetInstance(), MAKEINTRESOURCE(IDR_POPUPMENU));
	HMENU hMenuPopup = GetSubMenu(hMenu, 4);

	POINT pt;

	SetForegroundWindow(m_hWnd);

	GetCursorPos(&pt);
	int cmd = TrackPopupMenu(hMenuPopup,
						  TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
						  pt.x, pt.y, 0, m_hWnd, NULL);

	PostMessage(WM_NULL, 0, 0);
	DestroyMenu(hMenu);

	if (cmd >= IDM_MATCH_SUBEXP0 && cmd <= IDM_MATCH_SUBEXP9)
	{
		tstring str = _T("\\");
		str << cmd - IDM_MATCH_SUBEXP0;
		x_cbReplace.ReplaceSelectedText(str.c_str());
	}
	else
	{
		const tchar * str = 0;
		switch (cmd)
		{
		case IDM_NEWLINE:
			str = _T("\\n");
			break;

		case IDM_CR:
			str = _T("\\r");
			break;

		case IDM_TAB:
			str = _T("\\t");
			break;
		}

		if (0 != str)
		{
			x_cbReplace.ReplaceSelectedText(str);
		}
	}

	// 通知查找对话框
	SendMessage(WM_COMMAND, MAKELONG(IDC_FINDTEXT, CBN_EDITCHANGE), x_cbFind.GetHandle());
}

void FindReplace::OnFindMenu()
{
	HMENU hMenu = LoadMenu(WpfGetInstance(), MAKEINTRESOURCE(IDR_POPUPMENU));
	HMENU hMenuPopup = GetSubMenu(hMenu, 3);

	if (pApp->d_CustomRegxp.size())
	{
		::AppendMenu(hMenuPopup, MF_SEPARATOR, 0, 0);
		for (uint i = 0; i < pApp->d_CustomRegxp.size(); ++i)
		{
			::AppendMenu(hMenuPopup, MF_STRING, i + 1, pApp->d_CustomRegxp[i]->first.c_str());
		}
	}

	POINT pt;

	SetForegroundWindow(m_hWnd);

	GetCursorPos(&pt);
	int cmd = TrackPopupMenu(hMenuPopup,
						  TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
						  pt.x, pt.y, 0, m_hWnd, NULL);

	PostMessage(WM_NULL, 0, 0);
	DestroyMenu(hMenu);

	if (cmd > 0 && cmd <= (int)pApp->d_CustomRegxp.size())
	{
		SetDlgItemText(IDC_FINDTEXT, pApp->d_CustomRegxp[cmd - 1]->second.c_str());
		if (0 != pApp->d_CustomRegxp[cmd - 1]->third.length())
		{
			SetDlgItemText(IDC_REPLACETEXT, pApp->d_CustomRegxp[cmd - 1]->third.c_str());
		}
		SendMessage(WM_COMMAND, MAKELONG(IDC_FINDTEXT, CBN_EDITCHANGE), GetDlgItem(IDC_FINDTEXT));
		return;
	}

	const tchar * str = 0;
	switch (cmd)
	{
	case IDM_LINEFRONT:
		str = _T("^");
		break;

	case IDM_LINEEND:
		str = _T("$");
		break;

	case IDM_SINGLE_CHAR:
		str = _T(".");
		break;

	case IDM_ZERO_OR_MORE:
		str = _T("*");
		break;

	case IDM_ONE_OR_MORE:
		str = _T("+");
		break;

	case IDM_ZERO_OR_ONE:
		str = _T("?");
		break;

	case IDM_NM:
		str = _T("{n,m}");
		break;

	case IDM_NG_ZERO_OR_MORE:
		str = _T("*?");
		break;

	case IDM_NG_ONE_OR_MORE:
		str = _T("+?");
		break;

	case IDM_NG_ZERO_OR_ONE:
		str = _T("??");
		break;

	case IDM_NG_NM:
		str = _T("{n,m}?");
		break;

	case IDM_GROUP:
		str = _T("()");
		break;

	case IDM_OR:
		str = _T("|");
		break;

	case IDM_CHAR_SET:
		str = _T("[]");
		break;

	case IDM_NCHAR_SET:
		str = _T("[^]");
		break;

	case IDM_NEWLINE:
		str = _T("\\n");
		break;

	case IDM_CR:
		str = _T("\\r");
		break;

	case IDM_TAB:
		str = _T("\\t");
		break;

	case IDM_WORD:
		str = _T("\\w");
		break;

	case IDM_SPACE:
		str = _T("\\s");
		break;

	case IDM_DIGIT:
		str = _T("\\d");
		break;

	case IDM_LOWER:
		str = _T("\\l");
		break;

	case IDM_UPPER:
		str = _T("\\u");
		break;

	case IDM_ESC:
		str = _T("\\");
		break;

	case IDM_CUSTOM:
		{
			wnd::CustomRegxp dlg;
			dlg.DoModal(m_hWnd);
		}
		break;
	}

	if (0 != str)
	{
		x_cbFind.ReplaceSelectedText(str);
	}

	// 通知查找对话框
	SendMessage(WM_COMMAND, MAKELONG(IDC_FINDTEXT, CBN_EDITCHANGE), x_cbFind.GetHandle());
}

void FindAndReplace(wnd::FindReplace::METHOD method, int caller)
{
	static util::memory<TCHAR> mem;
	if (!mem.ptr())
	{
		mem.alloc(10);
		mem[0] = 0;
	}

	if (caller > 0 && wnd::FindReplace::MT_FIND == method)
	{
		if (IDM_EDIT_FINDNEXT == caller)
			pDoc->FindNext(mem.ptr(), pApp->d_FindFlag);
		else if (IDM_EDIT_FINDPREV == caller)
			pDoc->FindPrev(mem.ptr(), pApp->d_FindFlag);

		return;
	}

	if (wnd::FindReplace::GetWndStatic() != NULL)
	{
		::ShowWindow(wnd::FindReplace::GetWndStatic(), SW_SHOW);
		return;
	}

	int nSel = pDoc->SendMessage(sci::GETSELECTIONEND) - pDoc->SendMessage(sci::GETSELECTIONSTART);
	mem.resize(nSel + 1);
	pDoc->SendMessage(sci::GETSELTEXT, 0, mem.ptr());
	mem[nSel] = 0;
	TCHAR * p = mem.ptr();

	// Check lpszSelection and truncate bad chars
	while (*p)
	{
		if ('\r' == *p || '\n' == *p) {
			*p = 0;
			break;
		}
		p++;
	}

	wnd::FindReplace * pdlg = new wnd::FindReplace(method, mem);
	pdlg->Create(pView->GetHandle());
}


INT_PTR Run::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
			MakeBitmapButton(m_hWnd, IDC_SEARCHEXE, IDB_OPEN);

			SendDlgItemMessage(IDC_COMMANDLINE, EM_LIMITTEXT, MAX_PATH - 1, 0);
			SHAutoComplete(GetDlgItem(IDC_COMMANDLINE), SHACF_FILESYSTEM);
			PostMessage(WM_COMMAND, MAKELONG(IDC_COMMANDLINE, EN_CHANGE), 0);

			this->CenterWindow();
		}
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDC_SEARCHEXE:
			{
				tstring filter;
				api::LoadStringT(filter, IDS_FILTER_EXE);
				filter += ResStr(IDS_FILTER_ALL);
				filter += '\n';
				FixMutliString(filter);

				if (api::GetOpenFileNameT(filter, m_hWnd, filter.c_str()))
				{
					SetDlgItemText(IDC_COMMANDLINE, filter.c_str());
				}
			}
			break;

		case MAKELONG(IDC_COMMANDLINE, EN_CHANGE):
			{
				EnableWindow(GetDlgItem(IDOK), 
					!IsWndTextEmpty(m_hWnd, IDC_COMMANDLINE));
			}
			break;


		case IDOK:
			{
				tstring cmd;
				if (api::GetDlgItemTextT(cmd, m_hWnd, IDC_COMMANDLINE) > 0)
				{
					api::ExpandEnvironmentStringsT(cmd, cmd.c_str());
					api::CreateProcessT(cmd.c_str());
				}
			}
			break;

		case IDCANCEL:
			EndDialog(IDCANCEL);
			break;
		}

		break; //WM_COMMAND

	}

	return 0;
}

INT_PTR AddToFav::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{

	case WM_INITDIALOG:
		SendDlgItemMessage(100, EM_LIMITTEXT, MAX_PATH - 1, 0);
		SetDlgItemText(100, util::rget_before(
			util::rget_after(pApp->CurFile.str(), '\\'), '.').c_str());

		this->CenterWindow();
		PostMessage(WM_COMMAND, MAKELONG(100, EN_CHANGE), 0);
		break;

	case WM_COMMAND:
		switch (wParam)
		{

		case MAKELONG(100, EN_CHANGE):
			EnableWindow(GetDlgItem(IDOK),
				!IsWndTextEmpty(m_hWnd, 100));
			break;

		case IDOK:
			{
				tstring name;
				if (api::GetDlgItemTextT(name, m_hWnd, 100) <= 0)
					break;

				if (!com::CreateLink((pApp->FavDir + (tchar)'\\' + name + _T(".lnk")).c_str(), pApp->CurFile.c_str()))
					this->MessageBox(ResStr(IDS_FAV_FAILURE), ResStr(IDS_APPTITLE), MB_OK|MB_ICONEXCLAMATION);
				else
					this->MessageBox(ResStr(IDS_FAV_SUCCESS), ResStr(IDS_APPTITLE), MB_OK|MB_ICONINFORMATION);
			}

		case IDCANCEL:
			EndDialog(wParam);
			break;

		}
		break; //WM_COMMAND

	}
	return 0;
}

INT_PTR Goto::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
			int iCurLine = pDoc->SendMessage(SCI_LINEFROMPOSITION,
					pDoc->SendMessage(SCI_GETCURRENTPOS, 0, 0), 0) + 1;

			SetDlgItemInt(m_hWnd, IDC_LINENUM, iCurLine, FALSE);
			SendDlgItemMessage(IDC_LINENUM, EM_LIMITTEXT, 15, 0);

			SendDlgItemMessage(IDC_COLNUM, EM_LIMITTEXT, 15, 0);

			CenterWindow();
		}
		break;

	case WM_COMMAND:

		switch (wParam)
		{

		case IDOK:
			{

				BOOL fTranslated;
				BOOL fTranslated2;

				int iNewCol;

				int iNewLine = GetDlgItemInt(m_hWnd, IDC_LINENUM, &fTranslated, FALSE);
				int iMaxLine = pDoc->SendMessage(SCI_GETLINECOUNT, 0, 0);

				if (SendDlgItemMessage(IDC_COLNUM, WM_GETTEXTLENGTH, 0, 0) > 0)
					iNewCol = GetDlgItemInt(m_hWnd, IDC_COLNUM, &fTranslated2, FALSE);
				else
				{
					iNewCol = 1;
					fTranslated2 = TRUE;
				}

				if (!fTranslated || !fTranslated2)
				{
					PostMessage(WM_NEXTDLGCTL, (GetDlgItem((!fTranslated) ? IDC_LINENUM : IDC_COLNUM)), 1);
					return TRUE;
				}

				if (iNewLine > 0 && iNewLine <= iMaxLine && iNewCol > 0)
				{
					pDoc->JumpTo(iNewLine, iNewCol);

					EndDialog(IDOK);
				}

				else
					PostMessage(WM_NEXTDLGCTL, (GetDlgItem((!(iNewLine > 0 && iNewLine <= iMaxLine)) ? IDC_LINENUM : IDC_COLNUM)), 1);

			}
			break;


		case IDCANCEL:
			EndDialog(IDCANCEL);
			break;

		}
		return TRUE;
	}

	return FALSE;
}

INT_PTR PropGeneral::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
			theme.Load();
			theme.EnableThemeDialogTexture(m_hWnd);
			SendDlgItemMessage(IDC_ALWAYSONTOP, BM_SETCHECK, pApp->d_bAlwaysOnTop);
			SendDlgItemMessage(IDC_REUSEWINDOW, BM_SETCHECK, 
				0 != CReg::QueryDWORDValue(APP_REG_ROOT, 
				APP_REG_KEY _T("\\Settings"), _T("bReuseWindow")));
			SendDlgItemMessage(IDC_MINTOTRAY, BM_SETCHECK, pApp->d_bMinimizeToTray);

			SendDlgItemMessage(IDC_SLIDER_ALPHA, TBM_SETRANGE, 0, MAKELONG(0,255));
			SendDlgItemMessage(IDC_SLIDER_ALPHA, TBM_SETPOS, 1, pApp->d_Transparent & 0x00FF);
			if ((pApp->d_Transparent & 0xF000) == 0) {
				EnableDlgItem(IDC_TRANSPARENT, false);
			}
			else if ((pApp->d_Transparent & 0x0F00) != 0) {
				SendDlgItemMessage(IDC_TRANSPARENT, BM_SETCHECK, BST_CHECKED);
			}

			if ((pApp->d_Transparent & 0xFF00) != 0xFF00)
			{
				EnableDlgItem(IDC_SLIDER_ALPHA, false);
				ShowDlgItem(m_hWnd, IDC_ALPHA, SW_HIDE);
			}
			else
				SetDlgItemText(IDC_ALPHA, util::itot(pApp->d_Transparent & 0x00FF));

			SendDlgItemMessage(IDC_SHOWFULLPATH, BM_SETCHECK, !pApp->d_bShortPathNames);
			switch (pApp->d_EscFunction)
			{
			case 0:
				SendDlgItemMessage(IDC_NOESNFUNC, BM_SETCHECK, BST_CHECKED);
				break;
			case 1:
				SendDlgItemMessage(IDC_ESCMINIMIZE, BM_SETCHECK, BST_CHECKED);
				break;
			case 2:
				SendDlgItemMessage(IDC_ESCEXIT, BM_SETCHECK, BST_CHECKED);
				break;
			}
		}
		break;

	case WM_DESTROY:
		theme.Unload();
		break;

	case WM_HSCROLL:
		switch (::GetDlgCtrlID((HWND)lParam))
		{
		case IDC_SLIDER_ALPHA:
			pApp->d_Transparent = (pApp->d_Transparent & 0xFF00) | 
				LOBYTE(::SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
			SetDlgItemText(IDC_ALPHA, util::itot(pApp->d_Transparent & 0x00FF));
			pView->SetTransparent();
			break;
		}
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDC_REUSEWINDOW:
			{
				CReg reg;
				reg.Create(APP_REG_ROOT, APP_REG_KEY _T("\\Settings"));
				reg.SetDWORDValue(_T("bReuseWindow"), 
					!reg.QueryDWORDValue(_T("bReuseWindow")));
			}
			break;

		case IDC_ALWAYSONTOP:
			{
				HWND hf = ::GetFocus();
				pView->SendMessage(WM_COMMAND, IDT_VIEW_ALWAYSONTOP);
				::SetFocus(hf);
			}
			break;

		case IDC_MINTOTRAY:
			pApp->d_bMinimizeToTray = BST_CHECKED == SendDlgItemMessage(wParam, BM_GETCHECK);
			break;

		case IDC_TRANSPARENT:
			if (BST_UNCHECKED == SendDlgItemMessage(wParam, BM_GETCHECK))
			{
				EnableDlgItem(IDC_SLIDER_ALPHA, false);
				ShowDlgItem(m_hWnd, IDC_ALPHA, SW_HIDE);
				pApp->d_Transparent &= 0xF0FF;
			}
			else
			{
				EnableDlgItem(IDC_SLIDER_ALPHA, true);
				ShowDlgItem(m_hWnd, IDC_ALPHA, SW_SHOW);
				pApp->d_Transparent |= 0x0F00;
				SetDlgItemText(IDC_ALPHA, util::itot(pApp->d_Transparent & 0x00FF));
			}
			pView->SetTransparent();
			break;

		case IDC_SHOWFULLPATH:
			pApp->d_bShortPathNames = BST_UNCHECKED == SendDlgItemMessage(wParam, BM_GETCHECK);
			pView->SetTitle(pApp->d_bShortPathNames, pApp->f_bModified, pApp->f_bReadOnly);
			break;

		case IDC_SAVEVEFORERUNNINGTOOLS:
			pApp->d_bSaveBeforeRunningTools = BST_CHECKED == SendDlgItemMessage(wParam, BM_GETCHECK);
			break;

		case IDC_ESCMINIMIZE:
			pApp->d_EscFunction = 1;
			break;

		case IDC_ESCEXIT:
			pApp->d_EscFunction = 2;
			break;

		case IDC_NOESNFUNC:
			pApp->d_EscFunction = 0;
			break;
		}
		break;
	}

	return 0;
}

INT_PTR PropEditor::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
			theme.Load();
			theme.EnableThemeDialogTexture(m_hWnd);
			SendDlgItemMessage(IDC_INDENT, EM_LIMITTEXT, 15, 0);
			SetDlgItemText(IDC_INDENT, util::itot(pApp->d_WordWrapIndent));

			tstring tmp;
			api::LoadStringT(tmp, IDS_WORDWRAPVISUALS);
			tmp += '\n';
			FixMutliString(tmp);
			for (const tchar * p = tmp.c_str(); (uint)(p - tmp.c_str()) < tmp.length(); p += _tcslen(p) + 1)
				SendDlgItemMessage(IDC_SYMBOL, CB_ADDSTRING, 0, p);

			SendDlgItemMessage(IDC_SYMBOL, CB_SETCURSEL, 
				pApp->d_bShowWordWrapSymbols ? pApp->d_WordWrapSymbols + 1 : 0);
			SendDlgItemMessage(IDC_TABASSPACE, BM_SETCHECK, pApp->d_bTabsAsSpaces);
			SetDlgItemText(IDC_TABWIDTH, util::itot(pApp->d_iTabWidth));
			SetDlgItemText(IDC_LONGLINES, util::itot(pApp->d_iLongLinesLimit));
			SendDlgItemMessage(EDGE_LINE == pApp->d_LongLineMode ? IDC_EDGE : IDC_BACKGROUND,
				BM_SETCHECK, BST_CHECKED);
		}
		break;

	case WM_DESTROY:
		theme.Unload();
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDC_EDGE:
			pApp->d_LongLineMode = EDGE_LINE;
			break;

		case IDC_BACKGROUND:
			pApp->d_LongLineMode = EDGE_BACKGROUND;
			break;

		case MAKELONG(IDC_INDENT, EN_CHANGE):
			{
				tstring tmp;
				api::GetDlgItemTextT(tmp, m_hWnd, IDC_INDENT);
				int i = util::xtoi(tmp.c_str());
				if (i > 0 && i <= 1024) {
					pApp->d_WordWrapIndent = (WORD)i;
					pView->SendMessage(WM_COMMAND, IDM_VIEW_WORDWRAPSETTINGS);
				}
			}
			break;

		case MAKELONG(IDC_SYMBOL, CBN_CLOSEUP):
			{
				int sel = SendDlgItemMessage(IDC_SYMBOL, CB_GETCURSEL);
				if (sel > 0)
				{
					pApp->d_bShowWordWrapSymbols = true;
					pApp->d_WordWrapSymbols = sel - 1;
				}
				else if (0 == sel)
				{
					pApp->d_bShowWordWrapSymbols = false;
				}
			}
			break;

		case IDC_TABASSPACE:
			pApp->d_bTabsAsSpaces = BST_CHECKED == SendDlgItemMessage(wParam, BM_GETCHECK);
			pDoc->SendMessage(SCI_SETUSETABS, !pApp->d_bTabsAsSpaces, 0);
			break;

		case MAKELONG(IDC_TABWIDTH, EN_CHANGE):
			{
				tstring tmp;
				if (api::GetDlgItemTextT(tmp, m_hWnd, IDC_TABWIDTH) > 0)
				{
					int i = util::xtoi(tmp.c_str());
					if (i >= 1 && i <= 24)
						pApp->d_iTabWidth = (WORD)i;
						pDoc->SendMessage(SCI_SETTABWIDTH, pApp->d_iTabWidth, 0);
						pDoc->SendMessage(SCI_SETINDENT, 0, 0);
				}
			}
			break;

		case MAKELONG(IDC_LONGLINES, EN_CHANGE):
			{
				tstring tmp;
				if (api::GetDlgItemTextT(tmp, m_hWnd, IDC_LONGLINES) > 0)
				{
					int i = util::xtoi(tmp.c_str());
					if (i >= 1 && i <= 1024)
						pApp->d_iLongLinesLimit = (WORD)i;
						pDoc->SendMessage(SCI_SETTABWIDTH, pApp->d_iTabWidth, 0);
						pDoc->SendMessage(SCI_SETINDENT, 0, 0);
				}
			}
			break;
		}
		break;
	}

	return 0;
}

CustomRegxp::CustomRegxp()
: m_resizer(300, 250, 0, 0)
{
	m_resizer.Add(IDCANCEL, CDlgResizer::XY_MOVE)
				(IDC_SIZEBOX, CDlgResizer::XY_MOVE)
				(IDC_LIST, CDlgResizer::XY_SIZE)
				(IDC_STATIC_NAME, CDlgResizer::Y_MOVE)
				(IDC_NAME, CDlgResizer::X_SIZE_Y_MOVE)
				(IDC_STATIC_VALUE, CDlgResizer::Y_MOVE)
				(IDC_VALUE, CDlgResizer::X_SIZE_Y_MOVE)
				(IDC_ADD, CDlgResizer::Y_MOVE)
				(IDC_MODIFY, CDlgResizer::Y_MOVE)
				(IDC_REMOVE, CDlgResizer::Y_MOVE);
}

INT_PTR CustomRegxp::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_resizer.IsProcess(m_hWnd, uMsg, wParam, lParam))
	{
		return 1;
	}

	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			xm_lv.Attach(GetDlgItem(IDC_LIST));
			xm_lv.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
			tstring name;
			api::LoadStringT(name, IDS_NAME);
			xm_lv.InsertColumn(0, name.c_str(), 200);

			RECT r;
			::GetClientRect(xm_lv.GetHandle(), &r);
			xm_lv.InsertColumn(1, ResStr(IDS_EXPRESSION), r.right - GetSystemMetrics(SM_CXHSCROLL) - 200);

			for (uint i = 0; i < pApp->d_CustomRegxp.size(); ++i)
			{
				xm_lv.InsertItem(i, pApp->d_CustomRegxp[i]->first.c_str());
				xm_lv.SetItem(i, 1, pApp->d_CustomRegxp[i]->second.c_str());
			}
		}
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDC_REMOVE:
			{
				int sel = xm_lv.GetSelectedItem();
				CAppModule::CustomRegxpType::iterator iter = pApp->d_CustomRegxp.begin();
				std::advance(iter, sel);
				pApp->d_CustomRegxp.erase(iter);
				xm_lv.DeleteItem(sel);
			}
			break;

		case IDC_MODIFY:
			{
				CAppModule::CustomRegxpItemType item;
				if (GetDlgItemText(IDC_NAME, item->first) <= 0)
				{
					MessageBox(ResStr(IDS_ERR_EMPTRY), ResStr(IDS_ERROR), MB_OK | MB_ICONEXCLAMATION);
					::SetFocus(GetDlgItem(IDC_NAME));
					break;
				}

				if (GetDlgItemText(IDC_VALUE, item->second) <= 0)
				{
					MessageBox(ResStr(IDS_ERR_EMPTRY), ResStr(IDS_ERROR), MB_OK | MB_ICONEXCLAMATION);
					::SetFocus(GetDlgItem(IDC_VALUE));
					break;
				}

				GetDlgItemText(IDC_VALUE2, item->third);

				uint i = 0;
				for (; i < pApp->d_CustomRegxp.size(); ++i)
				{
					if (!_tcsicmp(item->first.c_str(), pApp->d_CustomRegxp[i]->first.c_str()))
					{
						break;
					}
				}
				if (i == pApp->d_CustomRegxp.size())
				{
					MessageBox(util::format(ResStr(IDS_ERR_NOTFOUND), item->first.c_str()), ResStr(IDS_ERROR), MB_OK | MB_ICONEXCLAMATION);
					return 0;
				}

				pApp->d_CustomRegxp[i]->second = item->second;
				xm_lv.SetItem(i, 1, item->second.c_str());
			}
			break;

		case IDC_ADD:
			{
				CAppModule::CustomRegxpItemType item;
				if (GetDlgItemText(IDC_NAME, item->first) <= 0)
				{
					MessageBox(ResStr(IDS_ERR_EMPTRY), ResStr(IDS_ERROR), MB_OK | MB_ICONEXCLAMATION);
					::SetFocus(GetDlgItem(IDC_NAME));
					break;
				}

				if (GetDlgItemText(IDC_VALUE, item->second) <= 0)
				{
					MessageBox(ResStr(IDS_ERR_EMPTRY), ResStr(IDS_ERROR), MB_OK | MB_ICONEXCLAMATION);
					::SetFocus(GetDlgItem(IDC_VALUE));
					break;
				}

				GetDlgItemText(IDC_VALUE2, item->third);

				for (uint i = 0; i < pApp->d_CustomRegxp.size(); ++i)
				{
					if (!_tcsicmp(item->first.c_str(), pApp->d_CustomRegxp[i]->first.c_str()))
					{
						MessageBox(util::format(ResStr(IDS_ERR_ALREADY_EXISTS), item->first.c_str()), ResStr(IDS_ERROR), MB_OK | MB_ICONEXCLAMATION);
						return 0;
					}
				}

				pApp->d_CustomRegxp.push_back(item);
				uint last = pApp->d_CustomRegxp.size() - 1;
				xm_lv.InsertItem(last, item->first.c_str());
				xm_lv.SetItem(last, 1, item->second.c_str());
			}
			break;

		case IDCANCEL:
			EndDialog(wParam);
			break;
		}
		break; //WM_COMMAND

	case WM_NOTIFY:
		{
			if (IDC_LIST == ((NMHDR *)lParam)->idFrom)
			{
				NMLISTVIEW * pnm = (NMLISTVIEW *)lParam;
				switch (pnm->hdr.code)
				{
				case NM_CLICK:
				case NM_DBLCLK:
					if (pnm->iItem >= 0)
					{
						SetDlgItemText(IDC_NAME, pApp->d_CustomRegxp[pnm->iItem]->first.c_str());
						SetDlgItemText(IDC_VALUE, pApp->d_CustomRegxp[pnm->iItem]->second.c_str());
						SetDlgItemText(IDC_VALUE2, pApp->d_CustomRegxp[pnm->iItem]->third.c_str());
					}
					break;
				}
			}
		}
		break; //WM_NOTIFY
	}

	return 0;
}

INT_PTR ImportExport::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			this->CenterWindow();
			if (xm_is_export)
			{
				this->SendDlgItemMessage(IDC_CHK_SETTING, BM_SETCHECK, BST_CHECKED);
				this->SendDlgItemMessage(IDC_CHK_STYLE, BM_SETCHECK, BST_CHECKED);
				this->SendDlgItemMessage(IDC_CHK_FORMATTING, BM_SETCHECK, BST_CHECKED);
				this->SendDlgItemMessage(IDC_CHK_REGXP, BM_SETCHECK, BST_CHECKED);
				this->SendDlgItemMessage(IDC_CHK_TOOLBAR, BM_SETCHECK, BST_CHECKED);
			}
			else
			{
				wpf::EnableDlgItem(m_hWnd, IDC_CHK_SETTING, false);
				wpf::EnableDlgItem(m_hWnd, IDC_CHK_STYLE, false);
				wpf::EnableDlgItem(m_hWnd, IDC_CHK_FORMATTING, false);
				wpf::EnableDlgItem(m_hWnd, IDC_CHK_REGXP, false);
				wpf::EnableDlgItem(m_hWnd, IDC_CHK_TOOLBAR, false);
				wpf::EnableDlgItem(m_hWnd, IDC_CHK_MRU, false);
				wpf::EnableDlgItem(m_hWnd, IDC_CHK_WINDOWS_POSITION, false);
			}
		}
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDC_BROWSE:
			{
				tstring file, filter;
				GetDlgItemText(IDC_EDT_FILE, file);
				api::LoadStringT(filter, IDS_PROFILE);
				FixMutliString(filter);
				if (xm_is_export)
				{
					if (api::GetSaveFileNameT(file, m_hWnd, filter.c_str(), file.c_str(), _T("xml")))
					{
						SetDlgItemText(IDC_EDT_FILE, file.c_str());
					}
				}
				else
				{
					if (api::GetOpenFileNameT(file, m_hWnd, filter.c_str(), file.c_str(), _T("xml")))
					{
						SetDlgItemText(IDC_EDT_FILE, file.c_str());
					}
				}
			}
			break;

		case IDOK:
			{
				tstring file;
				int c = api::GetDlgItemTextT(file, m_hWnd, IDC_EDT_FILE);
				if (c <= 0)
				{
					this->MessageBox(ResStr(IDS_ERR_EMPTY_FILE), ResStr(IDS_ERROR), MB_OK | MB_ICONEXCLAMATION);
					SetFocus(GetDlgItem(IDC_EDT_FILE));
					break;
				}

				if (xm_is_export)
				{
					Export(file);
				}
				else
				{
					Import(file);
				}
			}
		case IDCANCEL:
			this->EndDialog(wParam);
			break;
		}
		break; //WM_COMMAND
	}

	return 0;
}

void ImportExport::Import(const tstring & file)
{
	CReader r;
	if (!r.open(file.c_str(), CReader::OPEN_READ))
	{
		this->MessageBox(ResStr(IDS_ERR_OPENFILE), ResStr(IDS_ERROR), MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	ulong size = (ulong)r.get_size();
	if (0 == size)
	{
		this->MessageBox(ResStr(IDS_ERR_OPENFILE), ResStr(IDS_ERROR), MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	util::memory<byte> mem(size + 2);
	r.read(mem.ptr(), size);
	mem[size] = mem[size + 1] = 0;

	if (size < 4 || !!memcmp(mem.ptr(), util::utf8::XML_UTF8_HEADER, util::utf8::UTF8_SIGN_SIZE))
	{
		this->MessageBox(ResStr(IDS_ERR_XML_PARSE), ResStr(IDS_ERROR), MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	CMarkupSTL xml;
	if (!xml.SetDoc((char *)mem.ptr()))
	{
		this->MessageBox(ResStr(IDS_ERR_XML_PARSE), ResStr(IDS_ERROR), MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	tstring key = APP_REG_KEY;
	util::rmake_before(key, '\\');
	WriteRegistry(xml, key);

	pApp->LoadSettings();
	pView->ApplyView();
}

void ImportExport::Export(const tstring & file)
{
	pApp->SaveSettings();

	CMarkupSTL xml(util::utf8::XML_UTF8_HEADER);
	xml.AddElem(APP_NAME_ANSI);
	ReadRegistry(xml, APP_REG_KEY, false);
	xml.ResetPos();
	if (xml.FindChildElem("Key"))
	{
		xml.IntoElem();
		string tmp = xml.GetAttrib("Name");
		if (tmp != APP_NAME_ANSI)
		{
			return;
		}
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_CHK_SETTING, BM_GETCHECK))
	{
		tstring key = APP_REG_KEY;
		key += _T("\\Settings");
		ReadRegistry(xml, key);
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_CHK_STYLE, BM_GETCHECK))
	{
		tstring key = APP_REG_KEY;
		key += _T("\\Styles");
		ReadRegistry(xml, key);
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_CHK_FORMATTING, BM_GETCHECK))
	{
		tstring key = APP_REG_KEY;
		key += _T("\\Formatting");
		ReadRegistry(xml, key);
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_CHK_REGXP, BM_GETCHECK))
	{
		tstring key = APP_REG_KEY;
		key += _T("\\CustomRegxp");
		ReadRegistry(xml, key);
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_CHK_TOOLBAR, BM_GETCHECK))
	{
		tstring key = APP_REG_KEY;
		key += _T("\\Toolbar");
		ReadRegistry(xml, key);
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_CHK_MRU, BM_GETCHECK))
	{
		tstring key = APP_REG_KEY;
		key += _T("\\MRU");
		ReadRegistry(xml, key);
	}

	if (BST_CHECKED == SendDlgItemMessage(IDC_CHK_WINDOWS_POSITION, BM_GETCHECK))
	{
		tstring key = APP_REG_KEY;
		key += _T("\\Window");
		ReadRegistry(xml, key);
	}

	CReader r;
	if (!r.open(file.c_str(), CReader::OPEN_WRITE))
	{
		this->MessageBox(ResStr(IDS_ERR_OPENFILE), ResStr(IDS_ERROR), MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	const string & doc = xml.GetDoc();
	r.write(doc.c_str(), doc.length());
}

//************************************
// Method:    ReadRegistry 把注册表数据导入到 XML
// FullName:  wnd::ImportExport::ReadRegistry
// Access:    private const 
// Returns:   void
// Qualifier: bluenet
// Parameter: CMarkupSTL & xml XML 对象
// Parameter: const tstring & key 注册表健
// Parameter: bool recursive 是否递归处理
//************************************

void ImportExport::ReadRegistry(CMarkupSTL & xml, const tstring & key, bool recursive)
{
	tstring elem = util::rget_after(key, '\\');
	xml.AddChildElem("Key");
	xml.IntoElem();
	xml.AddAttrib("Name", t2u(elem.c_str()));
	tchar name[255];
	DWORD idx = 0, cname = countof(name), type;
	while (CReg::EnumValue(APP_REG_ROOT, key.c_str(), idx, name, &cname, NULL, NULL, &type))
	{
		switch (type)
		{
		case REG_DWORD:
			{
				DWORD data = CReg::QueryDWORDValue(APP_REG_ROOT, key.c_str(), name);
				xml.AddChildElem("Value", util::itoa(data));
				xml.IntoElem();
				xml.AddAttrib("Name", t2u(name));
				xml.AddAttrib("Type", util::itoa(type));
				xml.OutOfElem();
			}
			break;

		case REG_SZ:
			{
				tstring data;
				CReg reg;
				reg.Open(APP_REG_ROOT, key.c_str());
				reg.QueryStringValue(name, data);
				xml.AddChildElem("Value", t2u(data.c_str()));
				xml.IntoElem();
				xml.AddAttrib("Name", t2u(name));
				xml.AddAttrib("Type", util::itoa(type));
				xml.OutOfElem();
			}
			break;

		case REG_BINARY:
			{
				vector<byte> data;
				CReg reg;
				reg.Open(APP_REG_ROOT, key.c_str());
				reg.QueryBinaryValue(name, data);
				tstring str;
				util::bin2string(&data[0], data.size(), str);
				xml.AddChildElem("Value", t2u(str.c_str()));
				xml.IntoElem();
				xml.AddAttrib("Name", t2u(name));
				xml.AddAttrib("Type", util::itoa(type));
				xml.OutOfElem();
			}
			break;
		}

		cname = countof(name);
		++idx;
	}

	if (recursive)
	{
		idx = 0, cname = countof(name);
		while (CReg::EnumKey(APP_REG_ROOT, key.c_str(), idx, name, &cname))
		{
			tstring tmp = key;
			tmp += '\\';
			tmp += name;
			ReadRegistry(xml, tmp, recursive);

			cname = countof(name);
			++idx;
		}
	}

	xml.OutOfElem();
}





} // end of dialog namespace

//************************************
// Method:    WriteRegistry 把 XML 数据导入注册表
// FullName:  wnd::ImportExport::WriteRegistry
// Access:    private const 
// Returns:   void
// Qualifier: bluenet
// Parameter: CMarkupSTL & xml XML 对象
// Parameter: const tstring & key 注册表健
// Parameter: bool recursive 是否递归处理
//************************************

void wnd::ImportExport::WriteRegistry(CMarkupSTL & xml, const tstring & key, bool recursive) 
{
	while (xml.FindChildElem("Key"))
	{
		xml.IntoElem();
		tstring subkey = key;
		subkey += '\\';
		subkey += u2t(xml.GetAttrib("Name").c_str()).get_ptr();

		while (xml.FindChildElem("Value"))
		{
			xml.IntoElem();

			string name = xml.GetAttrib("Name");
			string type = xml.GetAttrib("Type");
			string data = xml.GetData();
			switch (util::xtoi(type.c_str()))
			{
			case REG_SZ:
				{
					CReg reg;
					if (CReg::SUCCESS == reg.Create(APP_REG_ROOT, subkey.c_str()))
					{
						reg.SetStringValue(u2t(name.c_str()), u2t(data.c_str()));
					}
				}
				break;

			case REG_DWORD:
				{
					CReg reg;
					if (CReg::SUCCESS == reg.Create(APP_REG_ROOT, subkey.c_str()))
					{
						reg.SetDWORDValue(u2t(name.c_str()), util::xtoi(data.c_str()));
					}
				}
			    break;

			case REG_BINARY:
				{
					vector<byte> bin;
					util::string2bin(u2t(data.c_str()), bin);
					CReg reg;
					if (CReg::SUCCESS == reg.Create(APP_REG_ROOT, subkey.c_str()))
					{
						reg.SetBinaryValue(u2t(name.c_str()), &bin[0], bin.size());
					}
				}
				break;

			default:
			    break;
			}

			xml.OutOfElem();
		}

		if (recursive)
		{
			WriteRegistry(xml, subkey, recursive);
		}

		xml.OutOfElem();
	}
}







