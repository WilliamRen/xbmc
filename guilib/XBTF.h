/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#ifndef XBTF_H_
#define XBTF_H_

#include <string>
#include <vector>
#include <stdint.h>

#define XBTF_MAGIC "XBTF"
#define XBTF_VERSION "1"

#define XB_FMT_DXT_MASK 14
#define XB_FMT_UNKNOWN  1
#define XB_FMT_DXT1     2
#define XB_FMT_DXT3     4
#define XB_FMT_DXT5     8
#define XB_FMT_B8G8R8A8 16
#define XB_FMT_R8G8B8A8 32
#define XB_FMT_LZO      64
#define XB_FMT_A8       128

class CXBTFFrame
{
public:  
  CXBTFFrame();
  uint32_t GetWidth() const;
  void SetWidth(uint32_t width);
  uint32_t GetX() const;
  void SetX(uint32_t x);
  uint32_t GetY() const;
  void SetY(uint32_t y);
  uint32_t GetHeight() const;
  void SetHeight(uint32_t height);
  uint64_t GetUnpackedSize() const;
  void SetUnpackedSize(uint64_t size);
  uint64_t GetPackedSize() const;
  void SetPackedSize(uint64_t size);
  uint64_t GetOffset() const;
  void SetOffset(uint64_t offset);
  uint64_t GetHeaderSize() const;
  uint32_t GetDuration() const;
  void SetDuration(uint32_t duration);

private:
  uint32_t m_width;
  uint32_t m_height;
  uint32_t m_x;
  uint32_t m_y;
  uint64_t m_packedSize;
  uint64_t m_unpackedSize;
  uint64_t m_offset;
  uint32_t m_duration;
};

class CXBTFFile
{
public:
  CXBTFFile();
  CXBTFFile(const CXBTFFile& ref);
  char* GetPath();
  void SetPath(const std::string& path);
  uint32_t GetLoop() const;
  void SetLoop(uint32_t loop);
  uint32_t GetFormat() const;
  void SetFormat(uint32_t format);  
  
  std::vector<CXBTFFrame>& GetFrames();  
  uint64_t GetHeaderSize() const;
  
private:
  char         m_path[256];
  uint32_t     m_loop;
  uint32_t     m_format;    
  std::vector<CXBTFFrame> m_frames;
};

class CXBTF
{
public:
  CXBTF();
  uint64_t GetHeaderSize() const;
  std::vector<CXBTFFile>& GetFiles();  
  
private:
  std::vector<CXBTFFile> m_files;
};

#endif
