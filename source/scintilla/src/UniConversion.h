// Scintilla source code edit control
/** @file UniConversion.h
 ** Functions to handle UFT-8 and UCS-2 strings.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

namespace sci {

unsigned int UTF8Length(const wchar_t * uptr, unsigned int tlen);
unsigned int UTF8FromUCS2(const wchar_t * src, unsigned int len, char * dst, unsigned int dstlen);
unsigned int UCS2Length(const char *s, unsigned int len);
unsigned int UCS2FromUTF8(const char * src, unsigned int len, wchar_t * dst, unsigned int dstlen);

}; //namespace sci
