// ***************************************************************
//  gdi   version:  1.0    date: 11/22/2006
//  -------------------------------------------------------------
//     .��� �ġ� �Ħ�.
//  ��(���^��^��^��^����)�� 
//  Author: bluenet
//  Summary: 
//  -------------------------------------------------------------
//  Copyright (C) 2006 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef _DEFINED_78D63371_F805_4DEA_9CD8_C0F29B340A3C 
#define _DEFINED_78D63371_F805_4DEA_9CD8_C0F29B340A3C 
#if _MSC_VER > 1000 
    #pragma once  
#endif // _MSC_VER > 1000



#include "utils.h"
#include "utf8.h"

namespace wpf {

	uint GetTextHeight(HDC dc);
	uint GetFontHeight(HFONT font);

	inline size_t GetTextWidth(HDC dc, const TCHAR * src, size_t len)
	{
		SIZE s;
		GetTextExtentPoint32(dc, src, (int)len, &s);
		return (size_t)s.cx;
	}

	// ���� GDI ����Ļ���
	class CGdiObject
	{
	public:
		CGdiObject() : m_hObject(NULL) {}
		virtual ~CGdiObject() { ReleaseObject(); }

		inline void ReleaseObject()
		{
			if (m_hObject)
			{
				::DeleteObject(m_hObject);
				m_hObject = NULL;
			}
		}

		// ��ʽת������
		inline operator HGDIOBJ() const { return GetHandle(); }
		// ��ʽת������
		inline HGDIOBJ GetHandle() const { return m_hObject; }
		// ��ʽת�����Ͳ����� ()
		inline HGDIOBJ operator()() const { return GetHandle(); }

		inline void operator=(HGDIOBJ hObject) { Attach(hObject); }

		inline bool Attach(HGDIOBJ hObject)
		{
			ASSERT(NULL == m_hObject);
			m_hObject = hObject;
			return true;
		}

		inline HGDIOBJ Detach()
		{
			HGDIOBJ hObject = m_hObject;
			m_hObject = NULL;
			return hObject;
		}

		inline bool IsEmpty()
		{
			return NULL != m_hObject;
		}

	protected:
		HGDIOBJ m_hObject;
	};

	class CBrush : public CGdiObject
	{
	public:
		CBrush() {}
		CBrush(COLORREF crColor) { CreateSolidBrush(crColor); }

		inline HBRUSH CreateSolidBrush(COLORREF crColor)
		{
			ReleaseObject();
			Attach(::CreateSolidBrush(crColor));
			return GetBrush();
		}

		inline HBRUSH CreateSysColorBrush(int nIndex)
		{
			ReleaseObject();
			Attach(DuplicateHandle(::GetSysColorBrush(nIndex)));
			return GetBrush();
		}

		inline int GetLogBrush(LOGBRUSH * pLogBrush)
		{
			ASSERT(m_hObject != NULL);
			return ::GetObject(m_hObject, sizeof(LOGBRUSH), pLogBrush);
		}

		static HBRUSH DuplicateHandle(HBRUSH hBrush)
		{
			LOGBRUSH lb;
			::GetObject(hBrush, sizeof(LOGBRUSH), &lb);
			return ::CreateSolidBrush(lb.lbColor);
		}

		// operator
		inline void operator=(HBRUSH hBrush) { Attach(hBrush); }

		// Attributes
		inline operator HBRUSH() const { return GetBrush(); }
		inline HBRUSH GetBrush() const { return (HBRUSH)GetHandle(); }
		inline HBRUSH operator()() const { return GetBrush(); }
	};

	class CFont : public CGdiObject
	{
	public:
		inline HFONT CreateFontIndirect(const LOGFONT * lpLogFont)
		{
			ReleaseObject();
			Attach(::CreateFontIndirect(lpLogFont));
			return GetFont();
		}

		inline HFONT CreateFont(int nHeight, int nWidth, int nEscapement,
			int nOrientation, int nWeight, BYTE bItalic, BYTE bUnderline,
			BYTE cStrikeOut, BYTE nCharSet, BYTE nOutPrecision,
			BYTE nClipPrecision, BYTE nQuality, BYTE nPitchAndFamily,
			LPCTSTR lpszFacename)
		{
			ReleaseObject();
			Attach(::CreateFont(nHeight, nWidth, nEscapement,
				nOrientation, nWeight, bItalic, bUnderline, cStrikeOut,
				nCharSet, nOutPrecision, nClipPrecision, nQuality,
				nPitchAndFamily, lpszFacename));
			return GetFont();
		}

		inline int GetLogFont(LOGFONT * pLogFont)
		{
			ASSERT(m_hObject != NULL);
			return ::GetObject(m_hObject, sizeof(LOGFONT), pLogFont);
		}

		// �Ӵ��ڻ�ȡ�����������ʧ�����ȡ�˵���Ի����Ĭ������
		HFONT GetFromWindow(HWND hWnd)
		{
			HFONT f = (HFONT)::SendMessage(hWnd, WM_GETFONT, 0, 0);
			if (NULL == f)
			{
				f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
			}
			Attach(DuplicateHandle(f));
			return GetFont();
		}

		// �ӿؼ���ȡ������
		inline HFONT GetFromDlgItem(HWND hWnd, UINT nCtlID)
		{
			return GetFromWindow(::GetDlgItem(hWnd, nCtlID));
		}

		static HFONT DuplicateHandle(HFONT hFont)
		{
			LOGFONT lf;
			::GetObject(hFont, sizeof(LOGFONT), &lf);
			return ::CreateFontIndirect(&lf);
		}

		// operator
		inline void operator=(HFONT hFont) { Attach(hFont); }

		// Attributes
		inline operator HFONT() const { return GetFont(); }
		inline HFONT GetFont() const { return (HFONT)GetHandle(); }
		inline HFONT operator()() const { return GetFont(); }
	};


}


#endif // _DEFINED_uniqueheadername