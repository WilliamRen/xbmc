#ifndef WMA_CODEC_H_
#define WMA_CODEC_H_

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef HAS_WMA_CODEC

#include "ICodec.h"
#include "FileSystem/File.h"

#include "DllWMA.h"

struct WMAInfo
{
  XFILE::CFile fileReader;
  char buffer[65536];
  DWORD iStartOfBuffer;
};


class WMACodec : public ICodec
{
public:
  WMACodec();
  virtual ~WMACodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

private:
  __int64 m_iDataPos;

  DllWMA m_dll;
  void*  m_hnd;
};

#endif
#endif
