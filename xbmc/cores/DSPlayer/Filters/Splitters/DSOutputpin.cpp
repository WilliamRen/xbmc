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



#include "DSOutputpin.h"

#include "moreuuids.h"
#include "streams.h"
#include "dvdmedia.h"
#include "DShowUtil/DShowUtil.h"
#include "DVDPlayer/DVDClock.h"
#include "DSDemuxerFilter.h"
#pragma warning(disable: 4355)

//
// CDSOutputPin
//

CDSOutputPin::CDSOutputPin(CSource* pFilter, HRESULT* phr, LPCWSTR pName, CDSStreamInfo& hints)
 : CSourceStream(NAME("CDSOutputPin"), phr, pFilter, pName)
 , m_hints(hints)
 , m_messageQueue(DShowUtil::WToA(pName).c_str())
 , m_rtStart(0)
 , m_bDiscontinuity(false)
 , m_bIsProcessing(false) 
{
  /*for (std::vector<CMediaType>::iterator it = mts.begin(); it != mts.end(); it++)
  {
    m_mts.push_back(*it);
  }*/
  m_messageQueue.SetMaxDataSize(40 * 1024 * 1024);
  m_messageQueue.SetMaxTimeSize(8.0);

  m_mt = hints.mtype;
  if (hints.type == STREAM_VIDEO)
  {
    m_mt.SetSampleSize(1);
  }
  else if (hints.type == STREAM_AUDIO)
  {
    
  }
  
	m_mts.push_back(m_mt);
}

CDSOutputPin::~CDSOutputPin()
{
  m_mts.clear();
}

HRESULT CDSOutputPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pAllocProps)
{
  ASSERT(pAlloc);
  ASSERT(pAllocProps);
  ALLOCATOR_PROPERTIES	Actual;
  HRESULT hr = NOERROR;

  pAllocProps->cBuffers = 8;
  
// compute buffer size
	switch(m_hints.type)
	{
	case STREAM_VIDEO:
		if (pAllocProps->cbBuffer < m_hints.height * 4 * m_hints.width * 4)
		{
			pAllocProps->cbBuffer = m_hints.height * 4 * m_hints.width * 4;
		}
		break;
	case STREAM_AUDIO:
	case STREAM_SUBTITLE:
		///< \todo check buffer size
		pAllocProps->cbBuffer = 0xFFFF;
		break;
	}
	
	pAllocProps->cbAlign = 4;

  if(m_mt.subtype == MEDIASUBTYPE_Vorbis && m_mt.formattype == FORMAT_VorbisFormat)
  {
    // oh great, the oggds vorbis decoder assumes there will be two at least, stupid thing...
    pAllocProps->cBuffers = std::max(pAllocProps->cBuffers, (long)2);
  }

  if(FAILED(hr = pAlloc->SetProperties(pAllocProps, &Actual))) 
    return hr;

  if(Actual.cbBuffer < pAllocProps->cbBuffer) return E_FAIL;
  ASSERT(Actual.cBuffers == pAllocProps->cBuffers);

    return NOERROR;
}

HRESULT CDSOutputPin::CheckMediaType(const CMediaType* pmt)
{
  for(unsigned int i = 0; i < m_mts.size(); i++)
  {
    if(*pmt == m_mts[i])
      return S_OK;
  }  
  

  return E_INVALIDARG;
}

HRESULT CDSOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
  CAutoLock cAutoLock(m_pLock);

  if(iPosition < 0) 
    return E_INVALIDARG;

  if(iPosition >= m_mts.size()) 
    return VFW_S_NO_MORE_ITEMS;

  *pmt = m_mts[iPosition];

  return S_OK;
}

HRESULT CDSOutputPin::OnThreadStartPlay(void)
{
  m_bDiscontinuity = true;	
	return NOERROR;
}

HRESULT CDSOutputPin::OnThreadDestroy(void)
{
  m_bIsProcessing = false;
	return NOERROR;
}

HRESULT CDSOutputPin::DeliverBeginFlush()
{
  m_bIsProcessing = false;
  Flush();
  HRESULT result = CBaseOutputPin::DeliverBeginFlush();
  CLog::DebugLog("%s",__FUNCTION__);
  return result;
}

HRESULT CDSOutputPin::DeliverEndFlush()
{
  m_messageQueue.End();
  m_messageQueue.Init();
  HRESULT result = CBaseOutputPin::DeliverEndFlush();
  CLog::DebugLog("%s",__FUNCTION__);
	return result;
}

void CDSOutputPin::EndStream()
{
	CLog::DebugLog("%s",__FUNCTION__);
	m_messageQueue.End();
}

void CDSOutputPin::Reset()
{
	CLog::DebugLog("%s",__FUNCTION__);
	m_messageQueue.Init();
}

void CDSOutputPin::DisableWriteBlock()
{
	CLog::DebugLog("%s",__FUNCTION__);
	//m_messageQueue.
}

void CDSOutputPin::EnableWriteBlock()
{
	CLog::DebugLog("%s",__FUNCTION__);
	//m_messageQueue.
}

void CDSOutputPin::Flush()
{
  /* flush using message as this get's called from dvdplayer thread */
  /* and any demux packet that has been taken out of queue need to */
  /* be disposed of before we flush */
  m_messageQueue.Flush();
  m_messageQueue.Put(new CDVDMsg(CDVDMsg::GENERAL_FLUSH), 1);
}

HRESULT CDSOutputPin::FillBuffer(IMediaSample *pSample)
{
  return S_OK;
}

HRESULT CDSOutputPin::DoBufferProcessingLoop(void)
{
  return S_OK;
}

STDMETHODIMP CDSOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
  CheckPointer(ppv, E_POINTER);
  if(riid == IID_IMediaSeeking)
    return m_pFilter->NonDelegatingQueryInterface(riid,ppv);

  __super::NonDelegatingQueryInterface(riid, ppv);
}  
