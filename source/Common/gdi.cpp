#include "gdi.h"


namespace wpf {

	uint GetFontHeight(HFONT font)
	{
		uint ret;
		HDC dc = CreateCompatibleDC(0);
		SelectObject(dc, font);
		ret = GetTextHeight(dc);
		DeleteDC(dc);
		return ret;
	}

	uint GetTextHeight(HDC dc)
	{
		TEXTMETRIC tm;
		POINT poo;
		GetTextMetrics(dc, &tm);
		poo.x = 0;
		poo.y = tm.tmHeight;
		LPtoDP(dc, &poo, 1);
		return poo.y > 1 ? poo.y : 1;
	}

} // namespace wpf