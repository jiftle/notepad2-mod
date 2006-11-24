#ifndef STYLE__H__
#define STYLE__H__





// Number of Lexers in pLexArray
//#define NUMLEXERS 22


namespace sci {





void GetSelStyle(STYLE & out, EDITSTYLE * pStyle);
void GetCurStyle(STYLE & out, int StyleID);
void CopyStyle(STYLE & dest, const STYLE & src);

const util::ptstring CreateFullFilter(bool is_open = true);
const util::ptstring CreateCPPFilter();
const util::ptstring CreateSupptoredFilter();
const util::ptstring CreateAllFilter();
const TCHAR * MakeHTMLColorString(COLORREF color);
tstring MakeHTMLFontString(const STYLE & style);
tstring GetCurrentLexerName();



inline COLORREF ConvertColor(const COLORREF & color)
{
	return color & 0xFFFFFF;
}
DWORD ConvertFontSize(DWORD size);


typedef vector<EDITLEXER *> LEXERDATA;
class MyLexerManager
{
public:
	MyLexerManager();
	//inline LEXERDATA & GetRefData() { return data; }
	inline unsigned int GetSize() const { return data.size(); }
	inline EDITLEXER * GetDefault() { return data[0]; }
	inline EDITLEXER * operator [] (unsigned int idx) { ASSERT(idx < data.size()); return data[idx]; }
	inline EDITLEXER * GetCurLexer() const { return curlexer; }
	inline void SetCurLexer(EDITLEXER * pNewLexer) { curlexer = pNewLexer; }

private:
	//struct Less
	//{
	//	bool operator()(EDITLEXER * & _X, EDITLEXER * & _Y) const
	//	{ return !_X->bContextMenu && _Y->bContextMenu; }
	//};


private:
	LEXERDATA data;
	EDITLEXER * curlexer;
};


}; //end of namespace sci


using namespace sci;



void   Style_SetLongLineColors(HWND);
int    Style_GetLexerIconId(EDITLEXER *);







#endif
