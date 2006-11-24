// Scintilla source code edit control
/** @file UniConversion.cxx
 ** Functions to handle UFT-8 and UCS-2 strings.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>

#include "UniConversion.h"

//namespace util {
//namespace utf8 {
//
//size_t utf8_decode_char(const char * src, unsigned int * out, size_t src_bytes = -1); //returns length in bytes
//size_t utf8_encode_char(unsigned int c, char * out); //returns used length in bytes, max 6
//size_t utf16_decode_char(const wchar_t * src, unsigned int * out);
//size_t utf16_encode_char(unsigned intc, wchar_t * out);
//
//};
//};
#include "..\..\Common\utf8.h"
using namespace util::utf8;

namespace sci {

unsigned int UTF8Length(const wchar_t * uptr, unsigned int tlen) {
	unsigned int len = 0;
	for (unsigned int i = 0; i < tlen && uptr[i]; i++) {
		unsigned int uch = uptr[i];
		if (uch < 0x80)
			len++;
		else if (uch < 0x800)
			len += 2;
		else
			len +=3;
	}
	return len;
}

unsigned int UTF8FromUCS2(const wchar_t * src, unsigned int len, char * dst, unsigned int dstlen) {
	//int k = 0;
	//for (unsigned int i = 0; i < tlen && uptr[i]; i++) {
	//	unsigned int uch = uptr[i];
	//	if (uch < 0x80) {
	//		putf[k++] = static_cast<char>(uch);
	//	} else if (uch < 0x800) {
	//		putf[k++] = static_cast<char>(0xC0 | (uch >> 6));
	//		putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
	//	} else {
	//		putf[k++] = static_cast<char>(0xE0 | (uch >> 12));
	//		putf[k++] = static_cast<char>(0x80 | ((uch >> 6) & 0x3f));
	//		putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
	//	}
	//}
	//putf[len] = '\0';
	size_t rv = 0;
	while(*src && len)
	{
		unsigned int c;
		size_t d = utf16_decode_char(src,&c);
		if (d==0 || d>len) break;
		src += d;
		len -= d;
		d = utf8_encode_char(c,dst);
		if (d==0) break;
		dst += d;
		rv += d;
	}
	*dst = 0;
	return rv;
}

unsigned int UCS2Length(const char *s, unsigned int len) {
	unsigned int ulen = 0;
	for (unsigned int i=0;i<len;i++) {
		unsigned char ch = static_cast<unsigned char>(s[i]);
		if ((ch < 0x80) || (ch > (0x80 + 0x40)))
			ulen++;
	}
	return ulen;
}

unsigned int UCS2FromUTF8(const char * src, unsigned int len, wchar_t * dst, unsigned int dstlen) {
	//unsigned int ui=0;
	//const unsigned char *us = reinterpret_cast<const unsigned char *>(s);
	//unsigned int i=0;
	//while ((i<len) && (ui<tlen)) {
	//	unsigned char ch = us[i++];
	//	if (ch < 0x80) {
	//		tbuf[ui] = ch;
	//	} else if (ch < 0x80 + 0x40 + 0x20) {
	//		tbuf[ui] = static_cast<wchar_t>((ch & 0x1F) << 6);
	//		ch = us[i++];
	//		tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + (ch & 0x7F));
	//	} else {
	//		tbuf[ui] = static_cast<wchar_t>((ch & 0xF) << 12);
	//		ch = us[i++];
	//		tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + ((ch & 0x7F) << 6));
	//		ch = us[i++];
	//		tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + (ch & 0x7F));
	//	}
	//	ui++;
	//}
	//return ui;
	size_t rv = 0;
	while(*src && len)
	{
		unsigned int c;
		size_t d = utf8_decode_char(src,&c,len);
		if (d==0 || d>len) break;
		src += d;
		len -= d;
		d = utf16_encode_char(c,dst);
		if (d==0) break;
		dst += d;
		rv += d;
	}
	*dst = 0;
	return rv;
}

}; //namespace sci