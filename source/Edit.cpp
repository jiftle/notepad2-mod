#include "notepad2.h"
#include "dialogs.h"
#include "styles.h"
#include "edit.h"

#include <stack>

#if !defined(_MSC_VER) || (_MSC_VER > 1300)  // 1300 == VC++ 7.0
#define BOOST_REGEX_NO_LIB
#include <boost/regex.hpp>

typedef boost::basic_regex<TCHAR, boost::regex_traits<TCHAR> > tregex;
typedef boost::match_results<const TCHAR *> tcmatch;
typedef boost::match_results<tstring::const_iterator> tsmatch;

enum REGEX_FLAG {
	RF_NORMAL = boost::regbase::normal,
	RF_ICASE = boost::regbase::icase,
};
#endif


extern RECT pagesetupMargin;

extern CView * pView;
extern CAppModule * pApp;
CDocment * pDoc = NULL;



// automatically select lexer
bool bAutoSelect = true;

CDocment::CDocment()
: m_hDevMode(NULL)
, m_hDevNames(NULL)
{
	pDoc = this;
}

CDocment::~CDocment()
{
	sci::Scintilla_ReleaseResources();
}

inline int CDocment::MessageBox(const tchar * lpText, UINT uType, const tchar * lpCaption) const
{
	return ::MessageBox(m_hWnd, lpText, lpCaption, uType);
}

bool CDocment::Create(HWND hWndParent)
{
	if (!sci::Scintilla_RegisterClasses(WpfGetInstance()))
		return false;

	this->m_hWnd = CreateWindowEx(
				   WS_EX_CLIENTEDGE,
				   _T("Scintilla"),
				   NULL,
				   WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
				   0,0,0,0,
				   hWndParent,
				   (HMENU)IDC_EDIT,
				   WpfGetInstance(),
				   NULL);

	if (NULL == this->m_hWnd)
	{
		tstring err;
		api::GetLastErrorT(err);
		::MessageBox(hWndParent, err.c_str(), ResStr(IDS_ERROR), MB_OK|MB_ICONEXCLAMATION);
		ASSERT(NULL != this->m_hWnd);
		return false;
	}

	SendMessage(SCI_SETCODEPAGE, GetACP(), 0);
    SendMessage(SCI_SETEOLMODE, SC_EOL_CRLF, 0);

    SendMessage(SCI_SETMODEVENTMASK, /*SC_MODEVENTMASKALL*/SC_MOD_INSERTTEXT|SC_MOD_DELETETEXT, 0);

    SendMessage(SCI_USEPOPUP, FALSE, 0);

    SendMessage(SCI_ASSIGNCMDKEY, (SCK_NEXT + (SCMOD_CTRL << 16)), SCI_PARADOWN);
    SendMessage(SCI_ASSIGNCMDKEY, (SCK_PRIOR + (SCMOD_CTRL << 16)), SCI_PARAUP);
    SendMessage(SCI_ASSIGNCMDKEY, (SCK_NEXT + ((SCMOD_CTRL | SCMOD_SHIFT) << 16)), SCI_PARADOWNEXTEND);
    SendMessage(SCI_ASSIGNCMDKEY, (SCK_PRIOR + ((SCMOD_CTRL | SCMOD_SHIFT) << 16)), SCI_PARAUPEXTEND);

    SendMessage(SCI_ASSIGNCMDKEY, (SCK_HOME + (0 << 16)), SCI_VCHOMEWRAP);
    SendMessage(SCI_ASSIGNCMDKEY, (SCK_END + (0 << 16)), SCI_LINEENDWRAP);
    SendMessage(SCI_ASSIGNCMDKEY, (SCK_HOME + (SCMOD_SHIFT << 16)), SCI_VCHOMEWRAPEXTEND);
    SendMessage(SCI_ASSIGNCMDKEY, (SCK_END + (SCMOD_SHIFT << 16)), SCI_LINEENDWRAPEXTEND);

	this->PrintInit();
	return true;
}

void CDocment::PrintInit()
{
	if (pagesetupMargin.left == 0 || pagesetupMargin.top == 0 ||
			pagesetupMargin.right == 0 || pagesetupMargin.bottom == 0)
	{
		tchar localeInfo[3];
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, localeInfo, 3);

		if (localeInfo[0] == '0')
		{  // Metric system. '1' is US System
			pagesetupMargin.left = 2000;
			pagesetupMargin.top = 2000;
			pagesetupMargin.right = 2000;
			pagesetupMargin.bottom = 2000;
		}

		else
		{
			pagesetupMargin.left = 1000;
			pagesetupMargin.top = 1000;
			pagesetupMargin.right = 1000;
			pagesetupMargin.bottom = 1000;
		}
	}
}

void CDocment::SetStyles(int iStyle, const STYLE & Style)
{
	// Font
	if (Style.FontName[0])
	{
		SendMessage(SCI_STYLESETFONT, iStyle, Style.FontName);
		SendMessage(SCI_STYLESETCHARACTERSET, iStyle, Style.fCharSet);
	}

	// Size
	if (Style.iFontSize)
		SendMessage(SCI_STYLESETSIZE, iStyle, ConvertFontSize(Style.iFontSize));

	// Fore
	if (Style.rForeColor)
		SendMessage(SCI_STYLESETFORE, iStyle, ConvertColor(Style.rForeColor));

	// Back
	if (Style.rBackColor)
		SendMessage(SCI_STYLESETBACK, iStyle, ConvertColor(Style.rBackColor));

	// Bold
	if (Style.fBold)
		SendMessage(SCI_STYLESETBOLD, iStyle, TRUE);
	else
		SendMessage(SCI_STYLESETBOLD, iStyle, FALSE);

	// Italic
	if (Style.fItalic)
		SendMessage(SCI_STYLESETITALIC, iStyle, TRUE);
	else
		SendMessage(SCI_STYLESETITALIC, iStyle, FALSE);

	// Underline
	if (Style.fUnderline)
		SendMessage(SCI_STYLESETUNDERLINE, iStyle, TRUE);
	else
		SendMessage(SCI_STYLESETUNDERLINE, iStyle, FALSE);

	// EOL Filled
	if (Style.sEOLFilled)
		SendMessage(SCI_STYLESETEOLFILLED, iStyle, TRUE);
	else
		SendMessage(SCI_STYLESETEOLFILLED, iStyle, FALSE);

	// Case
	//if (Style.sCaseSensitive)
	//	SendMessage(SCI_STYLESETCASE, iStyle, iValue);
}

void CDocment::ReloadLexer()
{
	SetLexer(pApp->Lexer.GetCurLexer());
}

void CDocment::SetLexer(EDITLEXER * pLexNew)
{
	// Select default if NULL is specified
	if (!pLexNew)
		pLexNew = pApp->Lexer.GetDefault();

	// code page
	UINT cp = GetACP();
//	if (SCLEX_ASCII == pLexNew->iLexer)
//	{
//#ifdef UNICODE
//		cp = GetACP();
//#else
//		cp = SC_CP_UTF8;
//#endif
//		if (SCLEX_ASCII != g_lexer.GetCurLexer()->iLexer)
//		{
//			int len = SendMessage(sci::GETLENGTH);
//			if (len > 0)
//			{
//				util::memory<TCHAR> src(len + 1);
//				SendMessage(sci::GETTEXT, len + 1, src.ptr());
//				src[len] = 0;
//#ifdef UNICODE
//				util::memory<wchar> dest(len * 3 / 2 + 1);
//				unsigned int dest_len = convert_ansi_to_utf16(string_ansi_from_utf16(src.ptr()), dest.ptr(), -1, 437);
//#else
//				unsigned int dest_len = estimate_ansi_to_utf8(src) * 4 / 3;
//				util::memory<char> dest(dest_len);
//				dest_len = ConvertNFO(src.ptr(), dest);
//#endif
//				SetNewText(dest.ptr(), dest_len);
//			}
//		}
//	}
//	else
//	{
//		if (SCLEX_ASCII == g_lexer.GetCurLexer()->iLexer)
//		{
//			int len = SendMessage(sci::GETLENGTH);
//			if (len > 0)
//			{
//				util::memory<TCHAR> src(len + 1);
//				SendMessage(sci::GETTEXT, len + 1, src.ptr());
//				src[len] = 0;
//#ifdef UNICODE
//				util::memory<wchar> dest(len * 3 / 2 + 1);
//				unsigned int dest_len = convert_ansi_to_utf16(string_ansi_from_utf16(src.ptr(), -1, 437), dest.ptr());
//#else
//				int dest_len = estimate_ansi_to_utf8(src) * 4 / 3;
//				util::memory<TCHAR> dest(dest_len);
//				dest_len = MakeNFO((char *)src.ptr(), (char *)dest.ptr());
//				while (dest[dest_len-1] == 0)
//					dest_len--;
//#endif
//				SetNewText(dest.ptr(), dest_len);
//			}
//		}
//	}

	// Clear
	SendMessage(SCI_CLEARDOCUMENTSTYLE, 0, 0);

	// Lexer
	SendMessage(SCI_SETLEXER, pLexNew->iLexer, 0);

	if (pLexNew->iLexer == SCLEX_HTML || pLexNew->iLexer == SCLEX_XML)
		SendMessage(SCI_SETSTYLEBITS, 7, 0);
	else
		SendMessage(SCI_SETSTYLEBITS, 5, 0);

	// Add KeyWord Lists
	for (int i = 0; i < countof(pLexNew->pKeyWords->pszKeyWords); i++)
	{
		SendMessage(SCI_SETKEYWORDS, i, pLexNew->pKeyWords->pszKeyWords[i]);
	}

	// Default Values are always set
	SendMessage(SCI_STYLERESETDEFAULT, 0, 0);
	SendMessage(SCI_STYLESETCHARACTERSET, STYLE_DEFAULT, CharsetFromCodepage(cp));

	pApp->BaseFontSize = DEFAULT_FONTSIZE;
	if (LOWORD(pApp->Lexer.GetDefault()->Styles[0].Value.iFontSize) &&
		0 == HIWORD(pApp->Lexer.GetDefault()->Styles[0].Value.iFontSize))
	{
		pApp->BaseFontSize = pApp->Lexer.GetDefault()->Styles[0].Value.iFontSize;
	}
	if (LOWORD(pLexNew->Styles[0].Value.iFontSize) && 0 == HIWORD(pLexNew->Styles[0].Value.iFontSize))
	{
		pApp->BaseFontSize = pLexNew->Styles[0].Value.iFontSize;
	}

	SetStyles(pApp->Lexer.GetDefault()->Styles[0].iStyle, pApp->Lexer.GetDefault()->Styles[0].Value);  // default

	SendMessage(sci::SETCODEPAGE, cp);

	if (0 == pApp->Lexer.GetDefault()->Styles[0].Value.rForeColor)
		SendMessage(SCI_STYLESETFORE, STYLE_DEFAULT, GetSysColor(COLOR_WINDOWTEXT));   // default text color
	if (0 == pApp->Lexer.GetDefault()->Styles[0].Value.rBackColor)
		SendMessage(SCI_STYLESETBACK, STYLE_DEFAULT, GetSysColor(COLOR_WINDOW));       // default window color

	if (pLexNew->iLexer != SCLEX_NULL)
	{
		SetStyles(pLexNew->Styles[0].iStyle, pLexNew->Styles[0].Value);
	} // lexer default
	SendMessage(SCI_STYLECLEARALL, 0, 0);

	SetStyles(pApp->Lexer.GetDefault()->Styles[1].iStyle, pApp->Lexer.GetDefault()->Styles[1].Value); // linenumber
	SetStyles(pApp->Lexer.GetDefault()->Styles[2].iStyle, pApp->Lexer.GetDefault()->Styles[2].Value); // brace light
	SetStyles(pApp->Lexer.GetDefault()->Styles[3].iStyle, pApp->Lexer.GetDefault()->Styles[3].Value); // brace bad
	SetStyles(pApp->Lexer.GetDefault()->Styles[4].iStyle, pApp->Lexer.GetDefault()->Styles[4].Value); // control char
	SetStyles(pApp->Lexer.GetDefault()->Styles[5].iStyle, pApp->Lexer.GetDefault()->Styles[5].Value); // indent guide

	// More default values...
	if (pApp->Lexer.GetDefault()->Styles[6].Value.rForeColor) // selection fore
		SendMessage(SCI_SETSELFORE, TRUE, ConvertColor(pApp->Lexer.GetDefault()->Styles[6].Value.rForeColor));
	else
		SendMessage(SCI_SETSELFORE, 0, 0);

	if (pApp->Lexer.GetDefault()->Styles[6].Value.rBackColor) // selection back
		SendMessage(SCI_SETSELBACK, TRUE, ConvertColor(pApp->Lexer.GetDefault()->Styles[6].Value.rBackColor));
	else
		SendMessage(SCI_SETSELBACK, TRUE, RGB(0xC0, 0xC0, 0xC0)); // use a default value...

	if (pApp->Lexer.GetDefault()->Styles[7].Value.rForeColor) // whitespace fore
		SendMessage(SCI_SETWHITESPACEFORE, TRUE, ConvertColor(pApp->Lexer.GetDefault()->Styles[7].Value.rForeColor));
	else
		SendMessage(SCI_SETWHITESPACEFORE, 0, 0);

	if (pApp->Lexer.GetDefault()->Styles[7].Value.rBackColor) // whitespace back
		SendMessage(SCI_SETWHITESPACEBACK, TRUE, ConvertColor(pApp->Lexer.GetDefault()->Styles[7].Value.rBackColor));
	else
		SendMessage(SCI_SETWHITESPACEBACK, 0, 0);    // use a default value...

	if (pApp->Lexer.GetDefault()->Styles[8].Value.rBackColor) // caret line back
	{
		SendMessage(SCI_SETCARETLINEVISIBLE, TRUE, 0);
		SendMessage(SCI_SETCARETLINEBACK, ConvertColor(pApp->Lexer.GetDefault()->Styles[8].Value.rBackColor), 0);
	}
	else
		SendMessage(SCI_SETCARETLINEVISIBLE, FALSE, 0);

	if (pApp->Lexer.GetDefault()->Styles[9].Value.rForeColor) // caret fore
		SendMessage(SCI_SETCARETFORE, ConvertColor(pApp->Lexer.GetDefault()->Styles[9].Value.rForeColor), 0);
	else
		SendMessage(SCI_SETCARETFORE, GetSysColor(COLOR_WINDOWTEXT), 0); // default value

	// caret width
	DWORD caret_width = util::range((int)LOWORD(pApp->Lexer.GetDefault()->Styles[10].Value.iFontSize), 1, 3);
	SendMessage(SCI_SETCARETWIDTH, caret_width, 0);

	if (SendMessage(SCI_GETEDGEMODE, 0, 0) == EDGE_LINE)
	{
		if (pApp->Lexer.GetDefault()->Styles[11].Value.rForeColor) // edge fore
			SendMessage(SCI_SETEDGECOLOUR, ConvertColor(pApp->Lexer.GetDefault()->Styles[11].Value.rForeColor), 0);
		else
			SendMessage(SCI_SETEDGECOLOUR, GetSysColor(COLOR_3DLIGHT), 0);
	}
	else
	{
		if (pApp->Lexer.GetDefault()->Styles[11].Value.rBackColor) // edge back
			SendMessage(SCI_SETEDGECOLOUR, ConvertColor(pApp->Lexer.GetDefault()->Styles[11].Value.rBackColor), 0);
		else
			SendMessage(SCI_SETEDGECOLOUR, GetSysColor(COLOR_3DLIGHT), 0);
	}

	DWORD bookmark_icon = LOWORD(pApp->Lexer.GetDefault()->Styles[12].Value.iFontSize);
	if (bookmark_icon > 0) {
		bookmark_icon = util::range((int)bookmark_icon + 99, IDX_BM0, IDX_BM8);
		std::vector<BYTE> v;
		if (api::LoadResourceT(v, bookmark_icon, _T("xpm")))
		{
			SendMessage(SCI_MARKERDEFINE, 0, SC_MARK_PIXMAP);
			SendMessage(SCI_MARKERDEFINEPIXMAP, 0, &v[0]);
		}
	}

	SendMessage(SCI_MARKERSETFORE, 0, RGB(0, 0, 0));             // bookmark line fore (default)

	if (pApp->Lexer.GetDefault()->Styles[13].Value.rBackColor) // bookmark line back
		SendMessage(SCI_MARKERSETBACK, 0, ConvertColor(pApp->Lexer.GetDefault()->Styles[13].Value.rBackColor));
	else
		SendMessage(SCI_MARKERSETBACK, 0, RGB(0xAB, 0xCD, 0xEF));

	if (pLexNew->iLexer != SCLEX_NULL)
	{
		int i = 1;
		while (pLexNew->Styles[i].iStyle != -1)
		{
			SetStyles(pLexNew->Styles[i].iStyle, pLexNew->Styles[i].Value);
			i++;
		}
	}

	SendMessage(SCI_COLOURISE, 0, -1);

	if (SCLEX_ASCII == pLexNew->iLexer)
	{
		SendMessage(SCI_STYLECLEARALL);
	}

	// Save current lexer
	pApp->Lexer.SetCurLexer(pLexNew);
}

void CDocment::SetLexerFromFile(const tstring & file)
{
	tstring ext = util::tolower(util::rget_after(file, '.'));
	if (ext.empty() || !_tcsicmp(file.c_str(), ext.c_str()))
		return;

	EDITLEXER * pLexNew = pApp->Lexer[0];

	if (bAutoSelect) // bAutoSelect == FALSE skips lexer search
	{
		ext.insert(ext.begin(), 1, ';');
		ext += ';';
		tstring exts;

		// Check Lexers
		for (unsigned int i = 0; i < pApp->Lexer.GetSize(); i++)
		{
			exts = pApp->Lexer[i]->szExtensions;
			ASSERT(!exts.empty());
			exts.insert(exts.begin(), ';');
			exts += ';';
			if (tstring::npos != exts.find(ext))
			{
				pLexNew = pApp->Lexer[i];
				break;
			}
		}
	}

	// Apply the new lexer
	SetLexer(pLexNew);
}

void CDocment::SetHTMLLexer()
{
	SetLexer(pApp->Lexer[3]);
}

void CDocment::SetXMLLexer()
{
	SetLexer(pApp->Lexer[4]);
}

void CDocment::SetLongLineColors()
{
	if (SendMessage(SCI_GETEDGEMODE, 0, 0) == EDGE_LINE)
	{
		if (pApp->Lexer.GetDefault()->Styles[11].Value.rForeColor) // edge fore
			SendMessage(SCI_SETEDGECOLOUR, pApp->Lexer.GetDefault()->Styles[11].Value.rForeColor, 0);
		else
			SendMessage(SCI_SETEDGECOLOUR, GetSysColor(COLOR_3DLIGHT), 0);
	}
	else
	{
		if (pApp->Lexer.GetDefault()->Styles[11].Value.rBackColor) // edge back
			SendMessage(SCI_SETEDGECOLOUR, pApp->Lexer.GetDefault()->Styles[11].Value.rBackColor, 0);
		else
			SendMessage(SCI_SETEDGECOLOUR, GetSysColor(COLOR_3DLIGHT), 0);
	}
}


void CDocment::SetNewText(LPCTSTR lpstrText, DWORD cbText)
{
    if (SendMessage(SCI_GETREADONLY))
        SendMessage(SCI_SETREADONLY, FALSE);

    SendMessage(SCI_CANCEL);
    SendMessage(SCI_SETUNDOCOLLECTION);
    SendMessage(SCI_CLEARALL);
    SendMessage(SCI_MARKERDELETEALL, -1);

    if (cbText > 0)
        SendMessage(SCI_ADDTEXT, cbText, lpstrText);

    SendMessage(SCI_SETUNDOCOLLECTION, 1);
    SendMessage(EM_EMPTYUNDOBUFFER);
    SendMessage(SCI_SETSAVEPOINT);
    SendMessage(SCI_GOTOPOS);
    SendMessage(SCI_CHOOSECARETX);
}

void CDocment::JumpTo(int iNewLine, int iNewCol)
{
    int iMaxLine = SendMessage(SCI_GETLINECOUNT);

    // Jumpt to end with line set to -1
    if (iNewLine == -1)
    {
        SendMessage(SCI_DOCUMENTEND);
        return;
    }

    // Line maximum is iMaxLine
	iNewLine = util::min(iNewLine, iMaxLine);

    // Column minimum is 1
    iNewCol = util::max(iNewCol,1);

    if (iNewLine > 0 && iNewLine <= iMaxLine && iNewCol > 0)
    {
        int iNewPos  = SendMessage(SCI_POSITIONFROMLINE, iNewLine-1, 0);
        int iLineEndPos = SendMessage(SCI_GETLINEENDPOSITION, iNewLine-1, 0);

        while (iNewCol-1 > SendMessage(SCI_GETCOLUMN, iNewPos, 0))
        {
            if (iNewPos >= iLineEndPos)
                break;

            iNewPos = SendMessage(SCI_POSITIONAFTER, iNewPos, 0);
        }

        iNewPos = util::min(iNewPos, iLineEndPos);
        SendMessage(SCI_GOTOPOS, iNewPos, 0);
        SendMessage(SCI_CHOOSECARETX, 0, 0);
    }
}

void CDocment::CommentBlock(const TCHAR * CommentSymbol, bool bDoComment)
{
	size_t nStart = SendMessage(sci::GETSELECTIONSTART);
	size_t nEnd = SendMessage(sci::GETSELECTIONEND);
	if (nStart == nEnd)
		return;

	if (this->DisableRectSel())
		return;

	ASSERT(nEnd > nStart);
	size_t nSel = nEnd - nStart;
	util::memory<TCHAR> mem(nSel + 2);
	TCHAR * pszText = mem.ptr();
	SendMessage(SCI_SETCURSOR, SC_CURSORWAIT, 0);
	SendMessage(SCI_GETSELTEXT, 0, pszText);
	pszText[nSel] = 0;
	SendMessage(SCI_BEGINUNDOACTION, 0, 0);
	SendMessage(SCI_CLEAR, 0, 0);

	tstring text;
	const TCHAR * p1 = pszText;
	const TCHAR * p2 = pszText;

	while (*p2)
	{
		p1 = p2;
		while(*p2 && '\r' != *p2 && '\n' != *p2) p2++;

		const TCHAR * p3 = p1;
		while (p3 < p2 && (' ' == *p3 || '\t' == *p3)) p3++;

		int mblen = _tcslen(CommentSymbol);

		if (bDoComment)
		{
			if ( p3 == p2 || (p2-p3 >= mblen && !_tcsnicmp(p3, CommentSymbol, mblen)) ) {
				text.append(p1, p2);
			} else {
				text.append(p1, p3);
				text.append(CommentSymbol);
				text.append(p3, p2);
			}
		}
		else
		{
			const TCHAR * p4 = p3;
			while (!_tcsnicmp(p4, CommentSymbol, mblen)) p4 += mblen;
			text.append(p1, p3);
			text.append(p4, p2);
		}

		p3 = p2;
		while (*p3 && ('\r' == *p3 || '\n' == *p3)) p3++;

		text.append(p2, p3);
		p2 = p3;
	}

	SendMessage(SCI_ADDTEXT, text.length(), text.c_str());

	SendMessage(SCI_SETSEL, nStart, nStart + text.length());
	SendMessage(SCI_ENDUNDOACTION, 0, 0);
	SendMessage(SCI_SETCURSOR, SC_CURSORNORMAL, 0);
}

void CDocment::OnExportUBB()
{
	size_t nStart = SendMessage(sci::GETSELECTIONSTART);
	size_t nEnd = SendMessage(sci::GETSELECTIONEND);
	if (nStart == nEnd)
		return;

	int sbit = SendMessage(sci::GETSTYLEBITS);
	ASSERT(sbit >= 0 && sbit < sizeof(TBYTE) * 8);
	TBYTE mark = -1;
	mark >>= (sizeof(TCHAR) * 8 - 1 - sbit);

	util::memory<STYLEPAIR> mem(nEnd - nStart + 10);
	TextRange tr;
	tr.chrg.cpMin = nStart;
	tr.chrg.cpMax = nEnd;
	tr.lpstrText = (TCHAR *)mem.ptr();
	SendMessage(sci::GETSTYLEDTEXT, 0, &tr);

	std::stack<const tchar *> st;
	STYLEPAIR * p1, * p2;
	p1 = p2 = mem.ptr();
	STYLE style = { sizeof(STYLE) };
	GetCurStyle(style, (int)(p2->StyleID & mark));
	//init
	//bool bOpenFont = false;
	//bool bOpenColor = false;
	//bool bOpenBold = false;
	//bool bOpenItalic = false;
	//bool bOpenUnderline = false;
	tstring text;
	text.reserve(nEnd - nStart + 10);
	text = _T("[quote]\r\n");
	if (style.FontName[0]) {
		text += _T("[font=");
		text += style.FontName;
		text += _T("]");
		//bOpenFont = true;
	}
	text += _T("[color=");
	text += MakeHTMLColorString(style.rForeColor);
	text += _T("]");
	st.push(_T("[/color]"));
	//bOpenColor = true;
	if (style.fBold) {
		text += _T("[b]");
		st.push(_T("[/b]"));
		//bOpenBold = true;
	}
	if (style.fItalic) {
		text += _T("[i]");
		st.push(_T("[/i]"));
		//bOpenItalic = true;
	}
	if (style.fUnderline) {
		text += _T("[u]");
		st.push(_T("[/u]"));
		//bOpenUnderline = true;
	}

	while (p2->Char)
	{
		if ('\t' == p2->Char) {
			for (int i=0; i<pApp->d_iTabWidth; i++)
				text += _T(" ");
		}
		else if ('\r' == p2->Char || '\n' == p2->Char || ' ' == p2->Char) {
			text += p2->Char;
		}
		else
		{
			if ((p1->StyleID & mark) != (p2->StyleID & mark))
			{
				STYLE tmp_style = { sizeof(STYLE) };
				GetCurStyle(tmp_style, (int)(p2->StyleID & mark));
				if (!!memcmp(&tmp_style, &style, sizeof(STYLE)))
				{
					for (uint i = 0; i < st.size(); ++i) //close font style tag
					{
						text += st.top();
						st.pop();
					}

					if (!!_tcsicmp(style.FontName, tmp_style.FontName)) //close font name tag
					{
						text += _T("[/font]");
						text += _T("[font=");
						text += tmp_style.FontName;
						text += _T("]");
					}
					memcpy(&style, &tmp_style, sizeof(STYLE));

					//if (bOpenColor) {
					//	text += _T("[/color]");
					//	bOpenColor = false;
					//}
					//if (bOpenUnderline) {
					//	text += _T("[/u]");
					//	bOpenUnderline = false;
					//}
					//if (bOpenItalic) {
					//	text += _T("[/i]");
					//	bOpenItalic = false;
					//}
					//if (bOpenBold) {
					//	text += _T("[/b]");
					//	bOpenBold = false;
					//}

					//using new style
					text += _T("[color=");
					text += MakeHTMLColorString(style.rForeColor);
					text += _T("]");
					st.push(_T("[/color]"));
					//bOpenColor = true;
					if (style.fBold) {
						text += _T("[b]");
						st.push(_T("[/b]"));
						//bOpenBold = true;
					}
					if (style.fItalic) {
						text += _T("[i]");
						st.push(_T("[/i]"));
						//bOpenItalic = true;
					}
					if (style.fUnderline) {
						text += _T("[u]");
						st.push(_T("[/u]"));
						//bOpenUnderline = true;
					}
				}

				p1 = p2;
			}

			text += p2->Char;
		}
		p2++;
	}

	for (uint i = 0; i < st.size(); ++i) //close font style tag
	{
		text += st.top();
		st.pop();
	}

	//if (bOpenColor) text += _T("[/color]");
	//if (bOpenUnderline) text += _T("[/u]");
	//if (bOpenItalic) text += _T("[/i]");
	//if (bOpenBold) text += _T("[/b]");
	text += _T("[/font]"); //close font name tag
	text += _T("[/quote]");
	CopyToClipboard(text.c_str(), text.length());
}

void CDocment::OnExportHTML()
{
	size_t nStart = SendMessage(sci::GETSELECTIONSTART);
	size_t nEnd = SendMessage(sci::GETSELECTIONEND);
	if (nStart == nEnd)
		return;

	int sbit = SendMessage(sci::GETSTYLEBITS);
	ASSERT(sbit >= 0 && sbit < sizeof(TBYTE) * 8);
	TBYTE mark = -1;
	mark >>= (sizeof(TCHAR) * 8 - 1 - sbit);

	util::memory<STYLEPAIR> mem(nEnd - nStart + 10);
	TextRange tr;
	tr.chrg.cpMin = nStart;
	tr.chrg.cpMax = nEnd;
	tr.lpstrText = (TCHAR *)mem.ptr();
	SendMessage(sci::GETSTYLEDTEXT, 0, &tr);

	STYLEPAIR * p1, * p2;
	p1 = p2 = mem.ptr();
	STYLE style = { sizeof(STYLE) };
	GetCurStyle(style, (int)(p2->StyleID & mark));
	//start
	tstring text;
	text.reserve(nEnd - nStart + 10);
	text = _T("<code>");
	text += sci::MakeHTMLFontString(style);
	bool bOpenTag = true;

	while (p2->Char)
	{
		if (' ' == p2->Char)
		{
			text += _T("&nbsp;");
		}
		else if ('\t' == p2->Char)
		{
			for (int i=0; i<pApp->d_iTabWidth; i++)
				text += _T("&nbsp;");
		}
		else if ('\r' == p2->Char || '\n' == p2->Char)
		{
			//if (bOpenTag) {
			//	text += _T("</font>");
			//	bOpenTag = false;
			//}
			text += _T("<br />\r\n");
			if ( ('\r' == (p2+1)->Char || '\n' == (p2+1)->Char) &&
				(p2+1)->Char != p2->Char ) {
					p2++;
			}
		}
		else
		{
			if ((p1->StyleID & mark) != (p2->StyleID & mark))
			{	//style changed
				STYLE tmp_style = { sizeof(STYLE) };
				GetCurStyle(tmp_style, (int)(p2->StyleID & mark));
				if (!!memcmp(&tmp_style, &style, sizeof(STYLE)))
				{
					memcpy(&style, &tmp_style, sizeof(STYLE));
					if (bOpenTag) {
						text += _T("</span>");
						bOpenTag = false;
					}
					text += sci::MakeHTMLFontString(style); //new style
					bOpenTag = true;
				}
				p1 = p2;
			}
			if ('<' == p2->Char) text += _T("&lt;");
			else if ('>' == p2->Char) text += _T("&gt;");
			else if ('\"' == p2->Char) text += _T("&quot;");
			else if ('&' == p2->Char) text += _T("&amp;");
			else text += p2->Char;
		}
		p2++;
	}
	if (bOpenTag)
		text += _T("</span>");
	text += _T("</code>\r\n");
	CopyToClipboard(text.c_str(), text.length());
}

void CDocment::PrintSetup()
{
	PAGESETUPDLG pdlg = { 0 };
	pdlg.lStructSize = sizeof(PAGESETUPDLG);

	pdlg.Flags = PSD_ENABLEPAGESETUPHOOK | PSD_ENABLEPAGESETUPTEMPLATE;
	pdlg.lpfnPageSetupHook = PrintSetupHook;
	pdlg.lpPageSetupTemplateName = MAKEINTRESOURCE(IDD_PAGESETUP);
	pdlg.lCustData = (LPARAM)this;

	pdlg.hwndOwner = m_hWnd;
	pdlg.hInstance = WpfGetInstance();

	if (pagesetupMargin.left != 0 || pagesetupMargin.right != 0 ||
			pagesetupMargin.top != 0 || pagesetupMargin.bottom != 0)
	{
		pdlg.Flags |= PSD_MARGINS;

		pdlg.rtMargin.left = pagesetupMargin.left;
		pdlg.rtMargin.top = pagesetupMargin.top;
		pdlg.rtMargin.right = pagesetupMargin.right;
		pdlg.rtMargin.bottom = pagesetupMargin.bottom;
	}

	pdlg.hDevMode = m_hDevMode;
	pdlg.hDevNames = m_hDevNames;

	if (!PageSetupDlg(&pdlg))
		return ;

	pagesetupMargin.left = pdlg.rtMargin.left;
	pagesetupMargin.top = pdlg.rtMargin.top;
	pagesetupMargin.right = pdlg.rtMargin.right;
	pagesetupMargin.bottom = pdlg.rtMargin.bottom;

	m_hDevMode = pdlg.hDevMode;
	m_hDevNames = pdlg.hDevNames;
}

//=============================================================================
//
//  PrintSetup() - Code from SciTE
//
//  Custom controls: 30 Zoom
//                   31 Spin
//                   32 Header
//                   33 Footer
//
UINT_PTR CALLBACK CDocment::PrintSetupHook(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static CDocment * _this = NULL;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			_this = (CDocment *)((PAGESETUPDLG *)lParam)->lCustData;
			tchar *p1, *p2;

			::SendDlgItemMessage(hwnd, 30, EM_LIMITTEXT, 32, 0);

			::SendDlgItemMessage(hwnd, 31, UDM_SETRANGE, 0, MAKELONG((short)20, (short) - 10));
			::SendDlgItemMessage(hwnd, 31, UDM_SETPOS, 0, MAKELONG((short)pApp->d_iPrintZoom, 0));

			// Set header options
			tstring tmp;
			api::LoadStringT(tmp, IDS_PRINT_HEADER);
			tmp += '|';
			p1 = (tchar *)tmp.c_str();
			while (p2 = _tcschr(p1, '|'))
			{
				*p2++ = '\0';
				if (*p1)
					::SendDlgItemMessage(hwnd, 32, CB_ADDSTRING, 0, (LPARAM)p1);
				p1 = p2;
			}
			::SendDlgItemMessage(hwnd, 32, CB_SETCURSEL, (WPARAM)pApp->d_iPrintHeader, 0);

			// Set footer options
			api::LoadStringT(tmp, IDS_PRINT_FOOTER);
			tmp += '|';
			p1 = (tchar *)tmp.c_str();
			while (p2 = _tcschr(p1, '|'))
			{
				*p2++ = '\0';
				if (*p1)
					::SendDlgItemMessage(hwnd, 33, CB_ADDSTRING, 0, (LPARAM)p1);
				p1 = p2;
			}
			::SendDlgItemMessage(hwnd, 33, CB_SETCURSEL, (WPARAM)pApp->d_iPrintFooter, 0);

			// Make combos handier
			::SendDlgItemMessage(hwnd, 32, CB_SETEXTENDEDUI, TRUE, 0);
			::SendDlgItemMessage(hwnd, 33, CB_SETEXTENDEDUI, TRUE, 0);
			::SendDlgItemMessage(hwnd, 1137, CB_SETEXTENDEDUI, TRUE, 0);
			::SendDlgItemMessage(hwnd, 1138, CB_SETEXTENDEDUI, TRUE, 0);
		}
		break;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			LONG lPos = ::SendDlgItemMessage(hwnd, 31, UDM_GETPOS, 0, 0);
			if (HIWORD(lPos) == 0)
				pApp->d_iPrintZoom = (sbyte)(short)LOWORD(lPos);
			else
				pApp->d_iPrintZoom = 0;

			pApp->d_iPrintHeader = ::SendDlgItemMessage(hwnd, 32, CB_GETCURSEL, 0, 0);
			pApp->d_iPrintFooter = ::SendDlgItemMessage(hwnd, 33, CB_GETCURSEL, 0, 0);
		}
		break;

	default:
		break;
	}
	return 0;
}

bool CDocment::Print(const tchar * pszDocTitle, const tchar * pszPageFormat, CStatusBar * pStatus)
{
	// Don't print empty documents
	if (SendMessage(SCI_GETLENGTH, 0, 0) == 0)
	{
		MessageBox(ResStr(IDS_PRINT_EMPTY), MB_OK|MB_ICONINFORMATION);
		return true;
	}

	int startPos;
	int endPos;

	HDC hdc;

	RECT rectMargins;
	RECT rectPhysMargins;
	RECT rectSetup;
	POINT ptPage;
	POINT ptDpi;

	TEXTMETRIC tm;

	int headerLineHeight;
	HFONT fontHeader;

	int footerLineHeight;
	HFONT fontFooter;

	tchar dateString[256];

	DOCINFO di = { sizeof(DOCINFO), 0, 0, 0, 0 };

	LONG lengthDoc;
	LONG lengthDocMax;
	LONG lengthPrinted;

	RangeToFormat frPrint;

	int pageNum;
	BOOL printPage;

	tchar pageString[32];

	HPEN pen;
	HPEN penOld;

	PRINTDLG pdlg = { sizeof(PRINTDLG) };
	pdlg.hwndOwner = m_hWnd;
	pdlg.hInstance = WpfGetInstance();
	pdlg.Flags = PD_USEDEVMODECOPIES | PD_ALLPAGES | PD_RETURNDC;
	pdlg.nFromPage = 1;
	pdlg.nToPage = 1;
	pdlg.nMinPage = 1;
	pdlg.nMaxPage = 0xffffU;
	pdlg.nCopies = 1;
	pdlg.hDC = 0;
	pdlg.hDevMode = m_hDevMode;
	pdlg.hDevNames = m_hDevNames;

	startPos = SendMessage(SCI_GETSELECTIONSTART, 0, 0);
	endPos = SendMessage(SCI_GETSELECTIONEND, 0, 0);

	if (startPos == endPos)
	{
		pdlg.Flags |= PD_NOSELECTION;
	}
	else
	{
		pdlg.Flags |= PD_SELECTION;
	}
	if (0)
	{
		// Don't display dialog box, just use the default printer and options
		pdlg.Flags |= PD_RETURNDEFAULT;
	}
	if (!PrintDlg(&pdlg))
	{
		return true; // False means error...
	}

	m_hDevMode = pdlg.hDevMode;
	m_hDevNames = pdlg.hDevNames;

	hdc = pdlg.hDC;

	// Get printer resolution
	ptDpi.x = GetDeviceCaps(hdc, LOGPIXELSX);    // dpi in X direction
	ptDpi.y = GetDeviceCaps(hdc, LOGPIXELSY);    // dpi in Y direction

	// Start by getting the physical page size (in device units).
	ptPage.x = GetDeviceCaps(hdc, PHYSICALWIDTH);   // device units
	ptPage.y = GetDeviceCaps(hdc, PHYSICALHEIGHT);  // device units

	// Get the dimensions of the unprintable
	// part of the page (in device units).
	rectPhysMargins.left = GetDeviceCaps(hdc, PHYSICALOFFSETX);
	rectPhysMargins.top = GetDeviceCaps(hdc, PHYSICALOFFSETY);

	// To get the right and lower unprintable area,
	// we take the entire width and height of the paper and
	// subtract everything else.
	rectPhysMargins.right = ptPage.x            // total paper width
			- GetDeviceCaps(hdc, HORZRES) // printable width
			- rectPhysMargins.left;        // left unprintable margin

	rectPhysMargins.bottom = ptPage.y            // total paper height
			- GetDeviceCaps(hdc, VERTRES)  // printable height
			- rectPhysMargins.top;        // right unprintable margin

	// At this point, rectPhysMargins contains the widths of the
	// unprintable regions on all four sides of the page in device units.

	// Take in account the page setup given by the user (if one value is not null)
	if (pagesetupMargin.left != 0 || pagesetupMargin.right != 0 ||
			pagesetupMargin.top != 0 || pagesetupMargin.bottom != 0)
	{

		// Convert the hundredths of millimeters (HiMetric) or
		// thousandths of inches (HiEnglish) margin values
		// from the Page Setup dialog to device units.
		// (There are 2540 hundredths of a mm in an inch.)

		tchar localeInfo[3];
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, localeInfo, 3);

		if (localeInfo[0] == '0')
		{  // Metric system. '1' is US System
			rectSetup.left = MulDiv (pagesetupMargin.left, ptDpi.x, 2540);
			rectSetup.top = MulDiv (pagesetupMargin.top, ptDpi.y, 2540);
			rectSetup.right = MulDiv(pagesetupMargin.right, ptDpi.x, 2540);
			rectSetup.bottom = MulDiv(pagesetupMargin.bottom, ptDpi.y, 2540);
		}
		else
		{
			rectSetup.left = MulDiv(pagesetupMargin.left, ptDpi.x, 1000);
			rectSetup.top = MulDiv(pagesetupMargin.top, ptDpi.y, 1000);
			rectSetup.right = MulDiv(pagesetupMargin.right, ptDpi.x, 1000);
			rectSetup.bottom = MulDiv(pagesetupMargin.bottom, ptDpi.y, 1000);
		}

		// Dont reduce margins below the minimum printable area
		rectMargins.left = util::max(rectPhysMargins.left, rectSetup.left);
		rectMargins.top = util::max(rectPhysMargins.top, rectSetup.top);
		rectMargins.right = util::max(rectPhysMargins.right, rectSetup.right);
		rectMargins.bottom = util::max(rectPhysMargins.bottom, rectSetup.bottom);
	}
	else
	{
		rectMargins.left = rectPhysMargins.left;
		rectMargins.top = rectPhysMargins.top;
		rectMargins.right = rectPhysMargins.right;
		rectMargins.bottom = rectPhysMargins.bottom;
	}

	// rectMargins now contains the values used to shrink the printable
	// area of the page.

	// Convert device coordinates into logical coordinates
	DPtoLP(hdc, (LPPOINT)&rectMargins, 2);
	DPtoLP(hdc, (LPPOINT)&rectPhysMargins, 2);

	// Convert page size to logical units and we're done!
	DPtoLP(hdc, (LPPOINT) &ptPage, 1);

	headerLineHeight = MulDiv(10, ptDpi.y, 72);
	fontHeader = CreateFont(headerLineHeight,
			0, 0, 0,
			FW_BOLD,
			0,
			0,
			0, 0, 0,
			0, 0, 0,
			_T("Arial"));
	SelectObject(hdc, fontHeader);
	GetTextMetrics(hdc, &tm);
	headerLineHeight = tm.tmHeight + tm.tmExternalLeading;

	footerLineHeight = MulDiv(9, ptDpi.y, 72);
	fontFooter = CreateFont(footerLineHeight,
			0, 0, 0,
			FW_NORMAL,
			0,
			0,
			0, 0, 0,
			0, 0, 0,
			_T("Arial"));
	SelectObject(hdc, fontFooter);
	GetTextMetrics(hdc, &tm);
	footerLineHeight = tm.tmHeight + tm.tmExternalLeading;

	di.lpszDocName = pszDocTitle;
	di.lpszOutput = 0;
	di.lpszDatatype = 0;
	di.fwType = 0;
	if (StartDoc(hdc, &di) < 0)
	{
		return false;
	}

	// Get current date...
	SYSTEMTIME st;
	GetLocalTime(&st);
	GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, dateString, 256);

	// Get current time...
	if (pApp->d_iPrintHeader == 0)
	{
		tchar timeString[128];
		GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, timeString, 128);
		_tcscat(dateString, _T(" "));
		_tcscat(dateString, timeString);
	}

	// Set print zoom...
	SendMessage(SCI_SETPRINTMAGNIFICATION, pApp->d_iPrintZoom, 0);

	lengthDoc = SendMessage(SCI_GETLENGTH, 0, 0);
	lengthDocMax = lengthDoc;
	lengthPrinted = 0;

	// Requested to print selection
	if (pdlg.Flags & PD_SELECTION)
	{
		if (startPos > endPos)
		{
			lengthPrinted = endPos;
			lengthDoc = startPos;
		}
		else
		{
			lengthPrinted = startPos;
			lengthDoc = endPos;
		}

		if (lengthPrinted < 0)
			lengthPrinted = 0;
		if (lengthDoc > lengthDocMax)
			lengthDoc = lengthDocMax;
	}

	// We must substract the physical margins from the printable area
	frPrint.hdc = hdc;
	frPrint.hdcTarget = hdc;
	frPrint.rc.left = rectMargins.left - rectPhysMargins.left;
	frPrint.rc.top = rectMargins.top - rectPhysMargins.top;
	frPrint.rc.right = ptPage.x - rectMargins.right - rectPhysMargins.left;
	frPrint.rc.bottom = ptPage.y - rectMargins.bottom - rectPhysMargins.top;
	frPrint.rcPage.left = 0;
	frPrint.rcPage.top = 0;
	frPrint.rcPage.right = ptPage.x - rectPhysMargins.left - rectPhysMargins.right - 1;
	frPrint.rcPage.bottom = ptPage.y - rectPhysMargins.top - rectPhysMargins.bottom - 1;
	frPrint.rc.top += headerLineHeight + headerLineHeight / 2;
	frPrint.rc.bottom -= footerLineHeight + footerLineHeight / 2;
	// Print each page
	pageNum = 1;

	//PropSet propsPrint;
	//propsPrint.superPS = &props;
	//SetFileProperties(propsPrint);

	while (lengthPrinted < lengthDoc)
	{
		printPage = (!(pdlg.Flags & PD_PAGENUMS) ||
				(pageNum >= pdlg.nFromPage) && (pageNum <= pdlg.nToPage));

		_stprintf(pageString, pszPageFormat, pageNum);
		//propsPrint.Set("CurrentPage", pageString);

		if (printPage)
		{

			// Show wait cursor...
			SendMessage(SCI_SETCURSOR, SC_CURSORWAIT, 0);

			// Display current page number in Statusbar
			pStatus->SetText(255, util::format(ResStr(IDS_PRINTFILE), pageNum));
			pStatus->SendMessage(SB_SIMPLE, TRUE, 0);

			InvalidateRect(pStatus->GetHandle(), NULL, TRUE);
			UpdateWindow(pStatus->GetHandle());

			StartPage(hdc);

			SetTextColor(hdc, RGB(0, 0, 0));
			SetBkColor(hdc, RGB(255, 255, 255));
			SelectObject(hdc, fontHeader);
			UINT ta = SetTextAlign(hdc, TA_BOTTOM);
			RECT rcw = {frPrint.rc.left, frPrint.rc.top - headerLineHeight - headerLineHeight / 2,
					frPrint.rc.right, frPrint.rc.top - headerLineHeight / 2};
			rcw.bottom = rcw.top + headerLineHeight;

			if (pApp->d_iPrintHeader < 3)
			{
				ExtTextOut(hdc, frPrint.rc.left + 5, frPrint.rc.top - headerLineHeight / 2,
						/*ETO_OPAQUE*/0,  &rcw,  pszDocTitle,
						_tcslen(pszDocTitle), NULL);
			}

			// Print date in header
			if (pApp->d_iPrintHeader == 0 || pApp->d_iPrintHeader == 1)
			{
				SIZE sizeInfo;
				SelectObject(hdc, fontFooter);
				GetTextExtentPoint32(hdc, dateString, _tcslen(dateString), &sizeInfo);
				ExtTextOut(hdc, frPrint.rc.right - 5 - sizeInfo.cx, frPrint.rc.top - headerLineHeight / 2,
						/*ETO_OPAQUE*/0,  &rcw,  dateString, _tcslen(dateString), NULL);
			}

			SetTextAlign(hdc, ta);
			pen = CreatePen(0, 1, RGB(0, 0, 0));
			penOld = (HPEN)SelectObject(hdc, pen);
			MoveToEx(hdc, frPrint.rc.left, frPrint.rc.top - headerLineHeight / 4, NULL);
			LineTo(hdc, frPrint.rc.right, frPrint.rc.top - headerLineHeight / 4);
			SelectObject(hdc, penOld);
			DeleteObject(pen);
		}

		frPrint.chrg.cpMin = lengthPrinted;
		frPrint.chrg.cpMax = lengthDoc;

		lengthPrinted = SendMessage(SCI_FORMATRANGE, printPage, &frPrint);

		if (printPage)
		{
			SetTextColor(hdc, RGB(0, 0, 0));
			SetBkColor(hdc, RGB(255, 255, 255));
			SelectObject(hdc, fontFooter);
			UINT ta = SetTextAlign(hdc, TA_TOP);
			RECT rcw = {frPrint.rc.left, frPrint.rc.bottom + footerLineHeight / 2,
					frPrint.rc.right, frPrint.rc.bottom + footerLineHeight + footerLineHeight / 2};

			if (pApp->d_iPrintFooter == 0)
			{
				SIZE sizeFooter;
				GetTextExtentPoint32(hdc, pageString, _tcslen(pageString), &sizeFooter);
				ExtTextOut(hdc, frPrint.rc.right - 5 - sizeFooter.cx, frPrint.rc.bottom + footerLineHeight / 2,
						/*ETO_OPAQUE*/0,  &rcw,  pageString,
						_tcslen(pageString), NULL);
			}

			SetTextAlign(hdc, ta);
			pen = ::CreatePen(0, 1, RGB(0, 0, 0));
			penOld = (HPEN)SelectObject(hdc, pen);
			SetBkColor(hdc, RGB(0, 0, 0));
			MoveToEx(hdc, frPrint.rc.left, frPrint.rc.bottom + footerLineHeight / 4, NULL);
			LineTo(hdc, frPrint.rc.right, frPrint.rc.bottom + footerLineHeight / 4);
			SelectObject(hdc, penOld);
			DeleteObject(pen);

			EndPage(hdc);
		}
		pageNum++;

		if ((pdlg.Flags & PD_PAGENUMS) && (pageNum > pdlg.nToPage))
			break;
	}

	SendMessage(SCI_FORMATRANGE, FALSE, 0);

	EndDoc(hdc);
	DeleteDC(hdc);
	if (fontHeader)
	{
		DeleteObject(fontHeader);
	}
	if (fontFooter)
	{
		DeleteObject(fontFooter);
	}

	// Reset Statusbar to default mode
	pStatus->SendMessage(SB_SIMPLE, FALSE, 0);

	// Remove wait cursor...
	SendMessage(SCI_SETCURSOR, SC_CURSORNORMAL, 0);

	return true;
}





TBSAVEPARAMS CToolbar::m_tbsp = { APP_REG_ROOT, APP_REG_KEY _T("\\Toolbar"), _T("Settings") };
TBBUTTON m_tbbtn[] = { {0, IDT_FILE_NEW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {1, IDT_FILE_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {2, IDT_FILE_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {0, 0, 0, TBSTYLE_SEP, 0, 0},
						  {3, IDT_EDIT_UNDO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {4, IDT_EDIT_REDO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {0, 0, 0, TBSTYLE_SEP, 0, 0},
						  {5, IDT_EDIT_CUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {6, IDT_EDIT_COPY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {7, IDT_EDIT_PASTE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {0, 0, 0, TBSTYLE_SEP, 0, 0},
						  {8, IDT_EDIT_FIND, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {9, IDT_EDIT_REPLACE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {0, 0, 0, TBSTYLE_SEP, 0, 0},
						  {10, IDT_VIEW_ALWAYSONTOP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {11, IDT_VIEW_WORDWRAP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {0, 0, 0, TBSTYLE_SEP, 0, 0},
						  {12, IDT_VIEW_ZOOMIN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {13, IDT_VIEW_ZOOMOUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {0, 0, 0, TBSTYLE_SEP, 0, 0},
						  {14, IDT_VIEW_SCHEME, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {15, IDT_VIEW_SCHEMECONFIG, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {0, 0, 0, TBSTYLE_SEP, 0, 0},
						  {16, IDT_FILE_EXIT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {17, IDT_FILE_SAVEAS, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {18, IDT_FILE_SAVECOPY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {19, IDT_EDIT_COPYALL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {20, IDT_EDIT_CLEAR, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {21, IDT_EDIT_FINDNEXT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {22, IDT_EDIT_FINDPREV, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {23, IDT_FILE_PRINT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {24, IDT_FILE_OPENFAV, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
						  {25, IDT_FILE_ADDTOFAV, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0} };

const unsigned int CToolbar::m_bmp_num = 26; //bitmap number
const unsigned int CToolbar::m_btn_num = 24; //default number


CToolbar::CToolbar()
: m_himl1(NULL)
, m_himl2(NULL)
{
}

CToolbar::~CToolbar()
{
	if (m_himl1)
		ImageList_Destroy(m_himl1);
	if (m_himl2)
		ImageList_Destroy(m_himl2);

	SaveToolBar();
}

bool CToolbar::GetButtonInfo(NMTOOLBAR * ptb)
{
	if (ptb->iItem < countof(m_tbbtn))
	{
		tstring tmp;
		api::LoadStringT(tmp, m_tbbtn[ptb->iItem].idCommand);
		_tcsncpy(ptb->pszText, tmp.c_str(), ptb->cchText);
		memcpy(&ptb->tbButton, &m_tbbtn[ptb->iItem], sizeof(TBBUTTON));
		return true;
	}
	return false;
}

void CToolbar::Reset()
{
	int c = SendMessage(TB_BUTTONCOUNT, 0, 0);
	for (int i = 0; i < c; i++)
		SendMessage(TB_DELETEBUTTON, 0, 0);
	SendMessage(TB_ADDBUTTONS, m_btn_num, m_tbbtn);
}

bool CToolbar::Create(HWND hWndParent)
{
	DWORD dwToolbarStyle = WS_TOOLBAR;
	if (pApp->d_bShowToolbar)
		dwToolbarStyle |= WS_VISIBLE;

	this->m_hWnd = CreateWindowEx(0,
		TOOLBARCLASSNAME,
		NULL, dwToolbarStyle,
		0, 0, 0, 0,
		hWndParent,
		(HMENU)IDC_TOOLBAR,
		WpfGetInstance(),
		NULL);

	if (NULL == this->m_hWnd)
	{
		tstring err;
		api::GetLastErrorT(err);
		::MessageBox(hWndParent, err.c_str(), ResStr(IDS_ERROR), MB_OK|MB_ICONEXCLAMATION);
		ASSERT(NULL != this->m_hWnd);
		return false;
	}

	SendMessage(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

	BITMAP bmp;
	HBITMAP hbmp;

	hbmp = LoadBitmap(WpfGetInstance(), MAKEINTRESOURCE(IDR_MAINWND));
	GetObject(hbmp, sizeof(BITMAP), &bmp);
	if (NULL == m_himl1)
	{
		m_himl1 = ImageList_Create(bmp.bmWidth / m_bmp_num, bmp.bmHeight, ILC_COLORDDB | ILC_MASK, 0, 0);
	}
	ImageList_AddMasked(m_himl1, hbmp, RGB(255, 0, 255));
	DeleteObject(hbmp);
	SendMessage(TB_SETIMAGELIST, 0, m_himl1);

	hbmp = LoadBitmap(WpfGetInstance(), MAKEINTRESOURCE(IDB_DISABLEDTOOLS));
	GetObject(hbmp, sizeof(BITMAP), &bmp);
	if (NULL == m_himl2)
	{
		m_himl2 = ImageList_Create(bmp.bmWidth / m_bmp_num, bmp.bmHeight, ILC_COLORDDB | ILC_MASK, 0, 0);
	}
	ImageList_AddMasked(m_himl2, hbmp, RGB(255, 0, 255));
	DeleteObject(hbmp);
	SendMessage(TB_SETDISABLEDIMAGELIST, 0, m_himl2);

	SendMessage(TB_SETEXTENDEDSTYLE, 0,
				SendMessage(TB_GETEXTENDEDSTYLE, 0, 0) | TBSTYLE_EX_MIXEDBUTTONS);

	SendMessage(TB_ADDBUTTONS, m_btn_num, m_tbbtn);
	this->SaveToolBar();
	return true;
}

void CDocment::StripFirstCharacter()
{
	int iSelStart = SendMessage(sci::GETSELECTIONSTART);
	int iSelEnd = SendMessage(sci::GETSELECTIONEND);

	if (iSelStart == iSelEnd)
	{
		iSelStart = 0;
		iSelEnd = SendMessage(SCI_GETLENGTH, 0, 0);
	}

	if (this->DisableRectSel())
		return;

	int iLine;

	int iLineStart = SendMessage(SCI_LINEFROMPOSITION, iSelStart, 0);
	int iLineEnd = SendMessage(SCI_LINEFROMPOSITION, iSelEnd, 0);

	if (iSelStart > SendMessage(SCI_POSITIONFROMLINE, iLineStart, 0))
		iLineStart++;

	if (iSelEnd <= SendMessage(SCI_POSITIONFROMLINE, iLineEnd, 0))
		iLineEnd--;

	SendMessage(SCI_BEGINUNDOACTION, 0, 0);

	for (iLine = iLineStart; iLine <= iLineEnd; iLine++)
	{
		int iPos = SendMessage(SCI_POSITIONFROMLINE, iLine);
		if (SendMessage(SCI_GETLINEENDPOSITION, iLine) - iPos > 0)
		{
			SendMessage(SCI_SETTARGETSTART, iPos);
			SendMessage(SCI_SETTARGETEND, SendMessage(SCI_POSITIONAFTER, iPos));
			SendMessage(SCI_REPLACETARGET, 0, _T(""));
		}
	}
	SendMessage(SCI_ENDUNDOACTION, 0, 0);
}

void CDocment::StripTrailingBlanks()
{
	// Check if there is any selection... simply use a regular expression replace!
	if (SendMessage(sci::GETSELECTIONEND) - SendMessage(sci::GETSELECTIONSTART))
	{
		if (!this->DisableRectSel())
		{
			ReplaceSelection(_T("[ \t]+$"), _T(""), false, SCFIND_REGEXP);
		}
	}
	// Code from SciTE...
	else
	{
		int line;
		int maxLines;
		int lineStart;
		int lineEnd;
		int i;
		tchar ch;

		SendMessage(SCI_BEGINUNDOACTION, 0, 0);
		maxLines = SendMessage(SCI_GETLINECOUNT, 0, 0);
		for (line = 0; line < maxLines; line++)
		{
			lineStart = SendMessage(SCI_POSITIONFROMLINE, line, 0);
			lineEnd = SendMessage(SCI_GETLINEENDPOSITION, line, 0);
			i = lineEnd - 1;
			ch = (tchar)SendMessage(SCI_GETCHARAT, i, 0);
			while ((i >= lineStart) && ((ch == ' ') || (ch == '\t')))
			{
				i--;
				ch = (tchar)SendMessage(SCI_GETCHARAT, i, 0);
			}
			if (i < (lineEnd - 1))
			{
				SendMessage(SCI_SETTARGETSTART, i + 1, 0);
				SendMessage(SCI_SETTARGETEND, lineEnd, 0);
				SendMessage(SCI_REPLACETARGET, 0, _T(""));
			}
		}
		SendMessage(SCI_ENDUNDOACTION, 0, 0);
	}
}

void CDocment::RemoveBlankLines()
{
	int iSelStart = SendMessage(sci::GETSELECTIONSTART);
	int iSelEnd = SendMessage(sci::GETSELECTIONEND);

	if (iSelStart == iSelEnd)
	{
		iSelStart = 0;
		iSelEnd = SendMessage(SCI_GETLENGTH, 0, 0);
	}

	if (this->DisableRectSel())
		return;

	int iLine;

	int iLineStart = SendMessage(SCI_LINEFROMPOSITION, iSelStart);
	int iLineEnd = SendMessage(SCI_LINEFROMPOSITION, iSelEnd);

	if (iSelStart > SendMessage(SCI_POSITIONFROMLINE, iLineStart))
		iLineStart++;

	if (iSelEnd <= SendMessage(SCI_POSITIONFROMLINE, iLineEnd))
		iLineEnd--;

	SendMessage(SCI_BEGINUNDOACTION, 0, 0);

	for (iLine = iLineStart; iLine <= iLineEnd; )
	{
		int iPos = SendMessage(SCI_POSITIONFROMLINE, iLine);
		if (SendMessage(SCI_GETLINEENDPOSITION, iLine) == iPos)
		{
			int iPos2 = SendMessage(SCI_POSITIONFROMLINE, iLine + 1);
			SendMessage(SCI_SETTARGETSTART, iPos);
			SendMessage(SCI_SETTARGETEND, iPos2);
			SendMessage(SCI_REPLACETARGET, 0, _T(""));
			iLineEnd--;
		}
		else
			iLine++;
	}
	SendMessage(SCI_ENDUNDOACTION, 0, 0);
}

void CDocment::SwapTabsAndSpaces(int nTabWidth, bool bTabsToSpaces)
{
	SendMessage(sci::SETCURSOR, SC_CURSORWAIT);
	tstring find, rep;
	if (bTabsToSpaces) {
		find = '\t';
		for (int i = 0; i < nTabWidth; i++)
			rep += ' ';
	}
	else {
		rep = '\t';
        for (int i = 0; i < nTabWidth; i++)
            find += ' ';
	}

	// Check if there is any selection... simply use a regular expression replace!
	if (SendMessage(sci::GETSELECTIONEND) - SendMessage(sci::GETSELECTIONSTART))
	{
		ReplaceSelection(find.c_str(), rep.c_str(), false, 0);
	}

	SendMessage(sci::SETCURSOR, SC_CURSORNORMAL);
}

void CDocment::FormatCPP(astyle::ASFormatter & formatter)
{
	size_t nStart = SendMessage(sci::GETSELECTIONSTART);
	size_t nEnd = SendMessage(sci::GETSELECTIONEND);
	if (nStart == nEnd)
		return;

	if (this->DisableRectSel())
		return;

	ASSERT(nEnd > nStart);
	size_t nSel = nEnd - nStart;
	util::memory<TCHAR> mem(nSel + 2);
	TCHAR * pszText = mem.ptr();
	SendMessage(SCI_SETCURSOR, SC_CURSORWAIT, 0);
	SendMessage(SCI_GETSELTEXT, 0, pszText);
	pszText[nSel] = 0;
	SendMessage(SCI_BEGINUNDOACTION, 0, 0);
	SendMessage(SCI_CLEAR, 0, 0);

	formatter.init(new astyle::ASBufferIterator(pszText));
	string text;
	const TCHAR * eolstr = GetEOLModeString(pApp->CurFile.eol);
	while (formatter.hasMoreLines())
	{
		text += formatter.nextLine();
		if (formatter.hasMoreLines())
			text += string_ansi_from_os(eolstr);
	}
	SendMessage(SCI_ADDTEXT, text.length(), text.c_str());

	SendMessage(SCI_SETSEL, nStart, nStart + text.length());
	SendMessage(SCI_ENDUNDOACTION, 0, 0);
	SendMessage(SCI_SETCURSOR, SC_CURSORNORMAL, 0);
}

void CDocment::JumpToBookmark(bool bNext)
{
	int iCurrentPos = SendMessage(SCI_GETCURRENTPOS);
	int iCurrentLine = SendMessage(SCI_LINEFROMPOSITION, iCurrentPos);
	int iMaxLine = SendMessage(SCI_GETLINECOUNT) - 1;

	if (bNext)
	{
		int iMarkerLine = -1;

		if (iCurrentLine < iMaxLine)
			iMarkerLine = SendMessage(SCI_MARKERNEXT, iCurrentLine + 1, 1);

		if (iMarkerLine == -1)
			iMarkerLine = SendMessage(SCI_MARKERNEXT, 0, 1);

		if (iMarkerLine != -1)
		{
			SendMessage(SCI_GOTOLINE, iMarkerLine);
			SendMessage(SCI_CHOOSECARETX);
		}
	}
	else
	{
		int iMarkerLine = -1;

		if (iCurrentLine > 0)
			iMarkerLine = SendMessage(SCI_MARKERPREVIOUS, iCurrentLine - 1, 1);

		if (iMarkerLine == -1)
			iMarkerLine = SendMessage(SCI_MARKERPREVIOUS, iMaxLine, 1);

		if (iMarkerLine != -1)
		{
			SendMessage(SCI_GOTOLINE, iMarkerLine);
			SendMessage(SCI_CHOOSECARETX);
		}
	}
}

void CDocment::ToggleBookmark()
{
	int iCurrentPos = SendMessage(SCI_GETCURRENTPOS);
	int iCurrentLine = SendMessage(SCI_LINEFROMPOSITION, iCurrentPos);
	int iMarkerBits = SendMessage(SCI_MARKERGET, iCurrentLine);

	if (iMarkerBits & 1) // Bit 0 represents the Notepad2 bookmark
		SendMessage(SCI_MARKERDELETE, iCurrentLine);
	else
		SendMessage(SCI_MARKERADD, iCurrentLine);
}

bool CDocment::FindPrev(const TCHAR * text, DWORD flag)
{
	ASSERT(text);
	if (!text[0])
		return false;

	TextToFind ttf = { 0 };

	ttf.chrg.cpMin = util::max(0, static_cast<int>(SendMessage(sci::GETSELECTIONSTART)-1));
    ttf.chrg.cpMax = 0;
    ttf.lpstrText = (TCHAR *)text;

    int iPos = SendMessage(sci::FINDTEXT, flag, &ttf);

	int iLength = SendMessage(sci::GETLENGTH);
    if (iPos == -1 && ttf.chrg.cpMin < iLength)
    {
        // wrapped
		this->MessageBox(ResStr(IDS_FIND_WRAPRE), MB_OK|MB_ICONINFORMATION);

        ttf.chrg.cpMin = iLength;
        iPos = SendMessage(sci::FINDTEXT, flag, &ttf);
    }

    if (iPos == -1)
    {
        // notfound
		this->MessageBox(ResStr(IDS_NOTFOUND), MB_OK|MB_ICONINFORMATION);
        return false;
    }

    SendMessage(sci::SETSEL, ttf.chrgText.cpMin, ttf.chrgText.cpMax);
    return true;

}

bool CDocment::FindNext(const TCHAR * text, DWORD flag)
{
	ASSERT(text);
	if (!text[0])
		return false;

	TextToFind ttf = { 0 };
    ttf.chrg.cpMin = SendMessage(sci::GETSELECTIONEND);
    ttf.chrg.cpMax = SendMessage(sci::GETLENGTH);

#if !defined(_MSC_VER) || (_MSC_VER > 1300)  // 1300 == VC++ 7.0
	if (flag & SCFIND_REGEXP)
	{
		int pos = ttf.chrg.cpMin;
		int size = ttf.chrg.cpMax - pos;
		ASSERT(size >= 0);
		if (size == 0)
			return true;

		util::memory<tchar> mem(size + 1);
		ttf.lpstrText = mem.ptr();
		SendMessage(sci::GETTEXTRANGE, 0, &ttf);
		mem[size] = 0;

		try {
			tregex reg(text, (SCFIND_MATCHCASE & flag) ? 
				tregex::normal : tregex::normal|tregex::icase|tregex::nosubs);
			tcmatch res;

			if (boost::regex_search(mem.ptr(), res, reg)) {
				ttf.chrgText.cpMin = pos + res.position();
				ttf.chrgText.cpMax = pos + res.position() + res.length();
			}
			else {
				this->MessageBox(ResStr(IDS_NOTFOUND), MB_OK|MB_ICONINFORMATION);
				return false;
			}
		}
		catch (const boost::bad_expression & e)
		{
			MessageBox(util::format(ResStr(IDS_ERR_REGEX), e.what()), MB_OK|MB_ICONEXCLAMATION);
			return false;
		}
	}
	else
#endif
	{
		ttf.lpstrText = (TCHAR *)text;
		int iPos = SendMessage(sci::FINDTEXT, flag, &ttf);

		if (iPos == -1 && ttf.chrg.cpMin > 0)
		{
			// wrapped
			this->MessageBox(ResStr(IDS_FIND_WRAPFW), MB_OK|MB_ICONINFORMATION);

			ttf.chrg.cpMin = 0;
			iPos = SendMessage(sci::FINDTEXT, flag, &ttf);
		}

		if (iPos == -1)
		{
			// notfound
			this->MessageBox(ResStr(IDS_NOTFOUND), MB_OK|MB_ICONINFORMATION);
			return false;
		}
	}

    SendMessage(sci::SETSEL, ttf.chrgText.cpMin, ttf.chrgText.cpMax);
    return true;
}

bool CDocment::Replace(const TCHAR * find, const TCHAR * rep, DWORD flag)
{
	ASSERT(find && rep);
	if (!find[0])
		return false;

	TextToFind ttf = { 0 };
    ttf.chrg.cpMin = SendMessage(sci::GETSELECTIONSTART); // Start!
    ttf.chrg.cpMax = SendMessage(sci::GETLENGTH);

#if !defined(_MSC_VER) || (_MSC_VER > 1300)  // 1300 == VC++ 7.0
	if (flag & SCFIND_REGEXP)
	{
		int pos = ttf.chrg.cpMin;
		int size = ttf.chrg.cpMax - pos;
		ASSERT(size >= 0);
		if (size == 0)
			return true;

		util::memory<tchar> mem(size + 1);
		ttf.lpstrText = mem.ptr();
		SendMessage(sci::GETTEXTRANGE, 0, &ttf);
		mem[size] = 0;

		try {
			tregex reg(find, (SCFIND_MATCHCASE & flag) ? 
				RF_NORMAL : RF_NORMAL|RF_ICASE);
			tcmatch res;
			tstring tmp;

			if (boost::regex_search(mem.ptr(), res, reg)) {
				tmp = res.format(rep, boost::match_default);

				SendMessage(sci::SETTARGETSTART, pos + res.position());
				SendMessage(sci::SETTARGETEND, pos + res.position() + res.length());
				SendMessage(SCI_REPLACETARGET, -1, tmp.c_str());
			}
			else {
				this->MessageBox(ResStr(IDS_NOTFOUND), MB_OK|MB_ICONINFORMATION);
				return false;
			}
		}
		catch (const boost::bad_expression & e)
		{
			MessageBox(util::format(ResStr(IDS_ERR_REGEX), e.what()), MB_OK|MB_ICONEXCLAMATION);
			return false;
		}
	}
	else
#endif
	{
		ttf.lpstrText = (TCHAR *)find;
		int iPos = SendMessage(sci::FINDTEXT, flag, &ttf);

		if (iPos == -1 && ttf.chrg.cpMin > 0)
		{
			// wrapped
			this->MessageBox(ResStr(IDS_FIND_WRAPFW), MB_OK|MB_ICONINFORMATION);

			ttf.chrg.cpMin = 0;
			iPos = SendMessage(sci::FINDTEXT, flag, &ttf);
		}

		if (iPos == -1)
		{
			// notfound
			this->MessageBox(ResStr(IDS_NOTFOUND), MB_OK|MB_ICONINFORMATION);
			return false;
		}

		SendMessage(sci::SETTARGETSTART, ttf.chrgText.cpMin);
		SendMessage(sci::SETTARGETEND, ttf.chrgText.cpMax);
		SendMessage(SCI_REPLACETARGET, -1, rep);
	}

    SendMessage(sci::SETSEL, SendMessage(sci::GETTARGETSTART), SendMessage(sci::GETTARGETEND));
    return true;
}


bool CDocment::ReplaceAll(const TCHAR * find, const TCHAR * rep, bool bShowInfo, DWORD flag)
{
	ASSERT(find && rep);
	if (!find[0])
		return false;

	TextToFind ttf = { 0 };
    ttf.chrg.cpMin = 0;
	ttf.chrg.cpMax = SendMessage(sci::GETLENGTH);

    int count = 0;

    // Show wait cursor...
    SendMessage(sci::SETCURSOR, SC_CURSORWAIT);

#if !defined(_MSC_VER) || (_MSC_VER > 1300)  // 1300 == VC++ 7.0
	if (flag & SCFIND_REGEXP)
	{
		int size = ttf.chrg.cpMax;
		ASSERT(size >= 0);
		if (size == 0)
			return true;

		tstring buf(size + 1, '\0');
		SendMessage(sci::GETTEXT, size + 1, buf.c_str());
		buf.erase(size);
		unsigned int pos = 0;

		try {
			tregex reg(find, (SCFIND_MATCHCASE & flag) ? 
				RF_NORMAL : RF_NORMAL|RF_ICASE);
			tcmatch res;
			tstring tmp;

			while (boost::regex_search(buf.c_str() + pos, res, reg))
			{
				++count;

				size_t res_pos = res.position();
				size_t res_len = res.length();
				tmp = res.format(rep, boost::match_default);
				buf.erase(pos + res_pos, res_len);
				buf.insert(pos + res_pos, tmp.c_str());
				pos += res_pos + tmp.length();
			}
		}
		catch (const boost::bad_expression & e)
		{
			MessageBox(util::format(ResStr(IDS_ERR_REGEX), e.what()), MB_OK|MB_ICONEXCLAMATION);
			return false;
		}

		if (count > 0) {
			SendMessage(sci::BEGINUNDOACTION);
			SendMessage(sci::CLEARALL);
			SendMessage(sci::ADDTEXT, buf.length(), buf.c_str());
			SendMessage(sci::ENDUNDOACTION);
		}
	}
	else
#endif
	{
		ttf.lpstrText = (TCHAR *)find;
		int pos;

		while ((pos = SendMessage(sci::FINDTEXT, flag, &ttf)) != -1)
		{
			count++;

			if (count == 1)
				SendMessage(sci::BEGINUNDOACTION);

			SendMessage(sci::SETTARGETSTART, ttf.chrgText.cpMin);
			SendMessage(sci::SETTARGETEND, ttf.chrgText.cpMax);
			SendMessage(SCI_REPLACETARGET, -1, rep);

			ttf.chrg.cpMin = SendMessage(sci::GETTARGETEND);
			ttf.chrg.cpMax = SendMessage(sci::GETLENGTH);

			if (ttf.chrg.cpMin == ttf.chrg.cpMax) // check for empty target
				break;
		}

		if (count)
			SendMessage(sci::ENDUNDOACTION);
	}

    // Remove wait cursor
    SendMessage(sci::SETCURSOR, SC_CURSORNORMAL);

    if (bShowInfo)
		this->MessageBox(util::format(ResStr(IDS_REPLCOUNT), count), MB_OK|MB_ICONINFORMATION);

    return true;
}

bool CDocment::Collect(const tchar * find, const tchar * rep, bool bmatch_case, bool bregxp)
{
	size_t len = SendMessage(sci::GETLENGTH);
	if (0 == len)
		return true;

	const tchar * eol_str = GetEOLModeString(pApp->CurFile.eol);
	util::memory<tchar> buf(len + 1);
	len = SendMessage(sci::GETTEXT, len + 1, buf.ptr());
	ASSERT(len);

    int count = 0;
	tstring res;

    // Show wait cursor...
    SendMessage(sci::SETCURSOR, SC_CURSORWAIT);

#if !defined(_MSC_VER) || (_MSC_VER > 1300)  // 1300 == VC++ 7.0
	if (bregxp)
	{
		unsigned int pos = 0;

		try {
			tregex reg(find, bmatch_case ? RF_NORMAL : RF_NORMAL|RF_ICASE);
			boost::regex_iterator<const tchar *> iter(buf.ptr(), buf.ptr() + len - 1, reg);
			boost::regex_iterator<const tchar *> end;
			if (0 != rep[0])
			{
				for (; iter != end; ++iter, ++count)
				{
					//res += iter->str();
					res += iter->format(rep, boost::match_default);
					res += eol_str;
				}
			}
			else
			{
				for (; iter != end; ++iter, ++count)
				{
					res += iter->str();
					res += eol_str;
				}
			}
		}
		catch (const boost::bad_expression & e)
		{
			MessageBox(util::format(ResStr(IDS_ERR_REGEX), e.what()), MB_OK|MB_ICONEXCLAMATION);
			return false;
		}
	}
	else
#endif
	{
		size_t str_len = _tcslen(find);
		for (const tchar * p = buf.ptr(); p < buf.ptr() + buf.size() - 1; ++p)
		{
			if (!_tcsncmp(find, p, str_len))
			{
				res += find;
				res += eol_str;
				++count;
				p += str_len - 1;
			}
		}
	}

	SendMessage(sci::BEGINUNDOACTION);
	SendMessage(sci::CLEARALL);
	SendMessage(sci::ADDTEXT, res.length(), res.c_str());
	SendMessage(sci::ENDUNDOACTION);

    // Remove wait cursor
    SendMessage(sci::SETCURSOR, SC_CURSORNORMAL);

	this->MessageBox(util::format(ResStr(IDS_COLLECT), count), MB_OK|MB_ICONINFORMATION);

    return true;
}


bool CDocment::ReplaceSelection(const TCHAR * find, const TCHAR * rep, bool bShowInfo, DWORD flag)
{
	ASSERT(find && rep);
	if (!find[0])
		return false;

    if (this->DisableRectSel())
        return false;

    int count = 0;

    // Show wait cursor...
   SendMessage(sci::SETCURSOR,SC_CURSORWAIT);

#if !defined(_MSC_VER) || (_MSC_VER > 1300)  // 1300 == VC++ 7.0
	if (flag & SCFIND_REGEXP)
	{
		int start =SendMessage(sci::GETSELECTIONSTART);
		int size =SendMessage(sci::GETSELECTIONEND) - start;
		ASSERT(size >= 0);
		if (size == 0)
			return true;

		tstring buf(size + 1, '\0');
		SendMessage(sci::GETSELTEXT, 0, buf.c_str());
		buf.erase(size);
		int len = _tcslen(rep);
		unsigned int pos = 0;

		try {
			tregex reg(find, (SCFIND_MATCHCASE & flag) ? 
				RF_NORMAL : RF_NORMAL|RF_ICASE);
			tcmatch res;
			tstring tmp;

			while (boost::regex_search(buf.c_str() + pos, res, reg))
			{
				++count;
				size_t res_pos = res.position();
				size_t res_len = res.length();
				tmp = res.format(rep, boost::match_default);
				buf.erase(pos + res_pos, res_len);
				buf.insert(pos + res_pos, tmp.c_str());
				pos += res_pos + tmp.length();
			}
		}
		catch (const boost::bad_expression & e)
		{
			this->MessageBox(util::format(ResStr(IDS_ERR_REGEX), e.what()), MB_OK|MB_ICONEXCLAMATION);
			return false;
		}

		if (count > 0) {
			SendMessage(sci::BEGINUNDOACTION);
			SendMessage(sci::REPLACESEL, 0, buf.c_str());
			SendMessage(sci::SETSEL, start, start + buf.length());
			SendMessage(sci::ENDUNDOACTION);
		}
	}
	else
#endif
	{
		TextToFind ttf = { 0 };
		ttf.chrg.cpMin =SendMessage(sci::GETSELECTIONSTART);
		ttf.chrg.cpMax =SendMessage(sci::GETLENGTH);
		ttf.lpstrText = (TCHAR *)find;

		int pos;
		bool fCancel = false;

		while (( pos =SendMessage(sci::FINDTEXT, flag, &ttf)) != -1 && !fCancel)
		{
			if (ttf.chrgText.cpMin >=SendMessage(sci::GETSELECTIONSTART) &&
					ttf.chrgText.cpMax <=SendMessage(sci::GETSELECTIONEND))
			{
				count++;

				if (count == 1)
					SendMessage(sci::BEGINUNDOACTION);

				SendMessage(sci::SETTARGETSTART, ttf.chrgText.cpMin);
				SendMessage(sci::SETTARGETEND, ttf.chrgText.cpMax);
				SendMessage(SCI_REPLACETARGET, -1, rep);

				ttf.chrg.cpMin =SendMessage(sci::GETTARGETEND);
				ttf.chrg.cpMax =SendMessage(sci::GETLENGTH);
			}

			else
			{
				fCancel = true;
			}

			if (ttf.chrg.cpMin == ttf.chrg.cpMax) // check for empty target
				break;
		}

		if (count)
			SendMessage(sci::ENDUNDOACTION);
	}

    // Remove wait cursor
   SendMessage(sci::SETCURSOR, SC_CURSORNORMAL);

    if (bShowInfo)
		MessageBox(util::format(ResStr(IDS_REPLCOUNT), count), MB_OK|MB_ICONINFORMATION);

    return true;
}

bool CDocment::LoadFile(CFileInformation & fi, int enc)
{
	ASSERT(fi.empty());
	if (fi.empty())
		return false;

	Reader reader;
	if (!reader.open(fi.c_str(), Reader::OPEN_READ))
	{
		tstring err;
		api::GetLastErrorT(err);
		this->MessageBox(err.c_str(), MB_OK|MB_ICONEXCLAMATION);
		return false;
	}


    // calculate buffer limit
	DWORD dwFileSize = reader.get_size32();

    // Check if a warning message should be displayed for large files
	CReg reg;
	reg.Open(APP_REG_ROOT, APP_REG_KEY _T("\\Settings"));
    DWORD dwFileSizeLimit = reg.QueryDWORDValue(_T("FileLoadWarningMB"));
    if (dwFileSizeLimit != 0 && dwFileSizeLimit * 1024 * 1024 < dwFileSize)
    {
        if (MessageBox(ResStr(IDS_WARNLOADBIGFILE), MB_YESNO|MB_ICONEXCLAMATION) != IDYES)
        {
            return false;
        }
    }

	util::memory<BYTE> mem;
	BYTE * lpData = mem.alloc(dwFileSize+2);
	DWORD cbData = reader.read(lpData, dwFileSize);
	if (0 == cbData) {
		fi.eol = SC_EOL_CRLF;
		fi.enc = enc >= 0 ? enc : ENC_ANSI;
		return true;
	}

	ASSERT(cbData <= dwFileSize);
	lpData[dwFileSize] = 0;
	lpData[dwFileSize+1] = 0;

	if ( ENC_UNICODE_LE == enc || 
		ENC_UNICODE_BE == enc || 
		(ENC_NONE == enc && is_utf16_sign(lpData, cbData)) )
    {
		bool be = is_utf16be_sign(lpData, cbData);
		if (ENC_UNICODE_LE != enc && ENC_UNICODE_BE != enc) {
			lpData += 2;
			cbData -= 2;
		}

		if (ENC_UNICODE_BE == enc || be)
		{
			swap_utf16_order((WCHAR *)lpData);
			enc = ENC_UNICODE_BE;
		} else enc = ENC_UNICODE_LE;

#ifndef UNICODE
		util::memory<char> ansi(estimate_utf16_to_ansi((WCHAR *)lpData, cbData / 2));
		cbData = convert_utf16_to_ansi((WCHAR *)lpData, ansi);
		lpData = (BYTE *)ansi.ptr();
#endif
        SetNewText((TCHAR *)lpData, cbData);
		fi.eol = GetEOLMode((TCHAR *)lpData, cbData);
    }

    else if ( ENC_UTF8 == enc ||
		( ENC_NONE == enc &&
		(is_utf8_sign(lpData, cbData) || 
		(!is_lower_ascii((const char *)lpData) && is_valid_utf8((const char *)lpData)) ) ) )
    {
		if (ENC_UTF8 != enc) {
			enc = ENC_UTF8;

			if (is_utf8_sign(lpData, cbData)) {
				lpData += 3;
				cbData -= 3;
			}
		}

		util::memory<TCHAR> os(estimate_utf8_to_os((char *)lpData, cbData));
		cbData = convert_utf8_to_os((char *)lpData, os);
		SetNewText(os, cbData);
		fi.eol = GetEOLMode(os, cbData);
    }

    else //ANSI
    {
#ifdef UNICODE
		util::memory<WCHAR> wstr(estimate_ansi_to_utf16((char *)lpData, cbData));
		cbData = convert_ansi_to_utf16((char *)lpData, wstr);
		lpData = (BYTE *)wstr.ptr();
#endif
		//}
		fi.eol = GetEOLMode((TCHAR *)lpData, cbData);
		enc = ENC_ANSI;
        SetNewText((TCHAR *)lpData, cbData);
    }
	fi.enc = enc;

    return true;
}


bool CDocment::SaveFile(LPCTSTR pszFile, int iEncoding, BOOL bSaveCopy)
{
    unsigned long lRes = 0;
	Reader reader;
	if (!reader.open(pszFile, Reader::OPEN_WRITE))
	{
		tstring err;
		api::GetLastErrorT(err);
		this->MessageBox(err.c_str(), MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

    // get text
	DWORD cbData = pDoc->SendMessage(sci::GETLENGTH);
	util::memory<TCHAR> mem(cbData + 2);
	TCHAR * lpData = mem.ptr();
	pDoc->SendMessage(sci::GETTEXT, cbData+1, lpData);
	lpData[cbData] = 0;

	if (cbData == 0) {
		reader.set_eof();
		lRes = 1;
	}
    else
    {

		//if (SCLEX_ASCII == g_lexer.GetCurLexer()->iLexer)
		//{
		//	unsigned int dest_len = estimate_utf8_to_ansi((char *)lpData) * 4 / 3;
		//	util::memory<char> mem(dest_len);
		//	dest_len = MakeNFO((char *)lpData, mem);
		//	while (mem[dest_len-1] == 0) dest_len--;

		//	lRes = reader.write(mem, dest_len);
		//}
        
		if (ENC_UNICODE_LE == iEncoding || ENC_UNICODE_BE == iEncoding)
        {
            LPWSTR lpDataWide;
            int    cbDataWide;
#ifdef UNICODE
			lpDataWide = lpData;
			cbDataWide = cbData;
#else
			util::memory<WCHAR> wstr(cbData + 1);
			lpDataWide = wstr.ptr();
			cbDataWide = convert_ansi_to_utf16(lpData, lpDataWide);
#endif

			if (ENC_UNICODE_BE == iEncoding)
                reader.write((LPCVOID)"\xFE\xFF", 2);
            else
                reader.write((LPCVOID)"\xFF\xFE", 2);

            if (ENC_UNICODE_BE == iEncoding)
				swap_utf16_order(lpDataWide);

			lRes = reader.write(lpDataWide, cbDataWide * sizeof(WCHAR));
        }

        else if (ENC_UTF8 == iEncoding)
        {
			util::memory<char> utf8;
			char * lpWrite = utf8.alloc(estimate_os_to_utf8(lpData));
			int cbWrite = convert_os_to_utf8(lpData, lpWrite);
            reader.write((LPCVOID)"\xEF\xBB\xBF", 3);

			lRes = reader.write(lpWrite, cbWrite);
        }

        else // ANSI
        {
			char * lpWrite;
			int cbWrite;
#ifdef UNICODE
			util::memory<char> ansi;
			lpWrite = ansi.alloc(estimate_utf16_to_ansi(lpData));
			cbWrite = convert_utf16_to_ansi(lpData, lpWrite);
#else
			lpWrite = lpData;
			cbWrite = cbData;
#endif
			lRes = reader.write(lpWrite, cbWrite);
        }
    }

    if (lRes > 0)
    {
        if (!bSaveCopy)
            SendMessage(SCI_SETSAVEPOINT);

        return true;
    }

    else
        return false;

}


bool CDocmentFrame::Create(HWND hWndParent)
{
	this->m_hWnd = CreateWindowEx(
						WS_EX_CLIENTEDGE,
						WC_LISTVIEW,
						NULL,
						WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
						0, 0, 100, 100,
						hWndParent,
						(HMENU)IDC_EDITFRAME,
						WpfGetInstance(),
						NULL);

	if (NULL == this->m_hWnd)
	{
		tstring err;
		api::GetLastErrorT(err);
		::MessageBox(hWndParent, err.c_str(), ResStr(IDS_ERROR), MB_OK|MB_ICONEXCLAMATION);
		ASSERT(NULL != this->m_hWnd);
		return false;
	}
	return true;
}


bool CStatusBar::Create(HWND hWndParent)
{
	DWORD dwStatusbarStyle = WS_CHILD | WS_CLIPSIBLINGS;
	if (pApp->d_bShowStatusbar)
		dwStatusbarStyle |= WS_VISIBLE;

	this->m_hWnd = CreateStatusWindow(dwStatusbarStyle, NULL, hWndParent, IDC_STATUSBAR);
	if (NULL == this->m_hWnd)
	{
		tstring err;
		api::GetLastErrorT(err);
		::MessageBox(hWndParent, err.c_str(), ResStr(IDS_ERROR), MB_OK|MB_ICONEXCLAMATION);
		ASSERT(NULL != this->m_hWnd);
		return false;
	}
	return true;
}

int CStatusBar::CalcPaneWidth(LPCTSTR lpsz)
{
    SIZE  size;
    HDC   hdc   = GetDC(m_hWnd);
    HFONT hfont = (HFONT)SendMessage(WM_GETFONT, 0, 0);
    HFONT hfold = (HFONT)SelectObject(hdc, hfont);
    int   mmode = SetMapMode(hdc, MM_TEXT);

    GetTextExtentPoint32(hdc, lpsz, _tcslen(lpsz), &size);

    SetMapMode(hdc, mmode);
    SelectObject(hdc, hfold);
    ReleaseDC(m_hWnd, hdc);

    return size.cx + 9;
}



bool CSizeBox::Create(HWND hWndParent, HWND hWndChild, const RECT & rc)
{
	DWORD dwReBarStyle = WS_REBAR;
	if (pApp->d_bShowToolbar)
		dwReBarStyle |= WS_VISIBLE;
	this->m_hWnd = CreateWindowEx(WS_EX_TOOLWINDOW,
								REBARCLASSNAME,
								NULL,
								dwReBarStyle,
							   0, 0, 0, 0,
							   hWndParent,
							   (HMENU)IDC_REBAR,
							   WpfGetInstance(),
							   NULL);
	if (NULL == this->m_hWnd)
	{
		tstring err;
		api::GetLastErrorT(err);
		::MessageBox(hWndParent, err.c_str(), ResStr(IDS_ERROR), MB_OK|MB_ICONEXCLAMATION);
		ASSERT(NULL != this->m_hWnd);
		return false;
	}

	bool bIsAppThemed = api::IsAppThemedT();
	REBARINFO rbi;
	rbi.cbSize = sizeof(REBARINFO);
	rbi.fMask = 0;
	rbi.himl = (HIMAGELIST)NULL;
	SendMessage(RB_SETBARINFO, 0, &rbi);

	REBARBANDINFO rbBand;
	rbBand.cbSize = sizeof(REBARBANDINFO);
	rbBand.fMask =  /*RBBIM_COLORS | RBBIM_TEXT | RBBIM_BACKGROUND | */
		RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE /*| RBBIM_SIZE*/;
	rbBand.fStyle =  /*RBBS_CHILDEDGE |*/ /* RBBS_BREAK |*/ RBBS_FIXEDSIZE /*| RBBS_GRIPPERALWAYS*/;
	if (bIsAppThemed)
		rbBand.fStyle |= RBBS_CHILDEDGE;
	rbBand.hbmBack = NULL;
	rbBand.lpText = _T("Toolbar");
	rbBand.hwndChild = hWndChild;
	rbBand.cxMinChild = (rc.right - rc.left); // * countof(m_tbbtn);
	rbBand.cyMinChild = (rc.bottom - rc.top) + 2 * rc.top;
	rbBand.cx = 0;
	SendMessage(RB_INSERTBAND, -1, &rbBand);

	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOZORDER);
	RECT rc2;
	::GetWindowRect(m_hWnd, &rc2);
	m_cy = rc2.bottom - rc2.top;

	//cyReBarFrame = bIsAppThemed ? 0 : 2;
	return true;
}




