#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 
#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "PixelShaderCompiler.h"
#include "StdString.h"

class TiXmlElement;

class CExternalPixelShader
{
public:
  HRESULT Compile(CPixelShaderCompiler *pCompiler);
  bool Load();
  CExternalPixelShader(TiXmlElement* xml);
  bool IsValid() const { return m_valid; }
  CStdString GetName() const { return m_name; }
  int GetId() const { return m_id; }

  uint32_t GetIndex() const { return m_index; }
  void SetIndex(const uint32_t index) { m_index = index; }

  bool IsEnabled() const { return m_enabled; }
  void SetEnabled(const bool enabled) { m_enabled = enabled; }

  TiXmlElement ToXML();

  Com::SmartPtr<IDirect3DPixelShader9> m_pPixelShader;
private:
  CStdString m_SourceFile;
  CStdString m_SourceTarget;
  CStdString m_SourceData;
  CStdString m_name;
  int m_id;
  bool m_valid;
  bool m_enabled;
  uint32_t m_index;
};

bool SortPixelShader(CExternalPixelShader* p1, CExternalPixelShader* p2);