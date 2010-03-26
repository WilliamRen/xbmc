/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "atlwfile.h"

class CTextFile : protected ATL::CFile
{
public:
	typedef enum {ASCII, UTF8, LE16, BE16, ANSI} enc;

private:
	enc m_encoding, m_defaultencoding;
	int m_offset;
  CStdString m_strFileName;

public:
	CTextFile(enc e = ASCII);

	virtual bool Open(LPCTSTR lpszFileName);
	virtual bool Save(LPCTSTR lpszFileName, enc e /*= ASCII*/);

	void SetEncoding(enc e);
	enc GetEncoding();
	bool IsUnicode();

	// CFile

	CStdString GetFilePath() const;

	// CStdioFile

  ULONGLONG GetPosition();
	ULONGLONG GetLength();
	ULONGLONG Seek(LONGLONG lOff, UINT nFrom);

	void WriteString(LPCSTR lpsz/*CStdStringA str*/);
	void WriteString(LPCWSTR lpsz/*CStdStringW str*/);
	BOOL ReadString(CStdStringA& str);
	BOOL ReadString(CStdStringW& str);

};

class CWebTextFile : public CTextFile
{
	LONGLONG m_llMaxSize;
	CStdString m_tempfn;

public:
	CWebTextFile(LONGLONG llMaxSize = 1024*1024);

	bool Open(LPCTSTR lpszFileName);
	bool Save(LPCTSTR lpszFileName, enc e /*= ASCII*/);
	void Close();
};

extern CStdStringW AToW(CStdStringA str);
extern CStdStringA WToA(CStdStringW str);
extern CStdString AToT(CStdStringA str);
extern CStdString WToT(CStdStringW str);
extern CStdStringA TToA(CStdString str);
extern CStdStringW TToW(CStdString str);
