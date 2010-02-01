/*
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2007 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "DShowUtil/dshowutil.h"
#include "DShowUtil/DSGeometry.h"
#include "cores/VideoRenderers/RenderManager.h"

#include "WindowingFactory.h" //d3d device and d3d interface
#include "VMR9AllocatorPresenter.h"
#include "application.h"
#include "ApplicationRenderer.h"
#include "utils/log.h"
#include "MacrovisionKicker.h"
#include "IPinHook.h"
#include "GuiSettings.h"
#include "dxerr.h"

class COuterVMR9
  : public CUnknown
  , public IVideoWindow
  , public IBasicVideo2
  , public IVMRWindowlessControl
{
  IUnknown* m_pVMR;
  CVMR9AllocatorPresenter *m_pAllocatorPresenter;

public:

  COuterVMR9(const TCHAR* pName, LPUNKNOWN pUnk, CVMR9AllocatorPresenter *_pAllocatorPresenter) : CUnknown(pName, pUnk)
  {
    CoCreateInstance(CLSID_VideoMixingRenderer9, NULL, CLSCTX_ALL, __uuidof(m_pVMR), (void**) &m_pVMR);
    m_pAllocatorPresenter = _pAllocatorPresenter;
  }

  ~COuterVMR9()
  {
    SAFE_RELEASE(m_pVMR);
  }

  DECLARE_IUNKNOWN;
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
  {
    HRESULT hr;
    if(riid == __uuidof(IVMRMixerBitmap9))
      return GetInterface((IVMRMixerBitmap9*)this, ppv);

    hr = m_pVMR ? m_pVMR->QueryInterface(riid, ppv) : E_NOINTERFACE;
    if(m_pVMR && FAILED(hr))
    {
      if(riid == __uuidof(IVideoWindow))
        return GetInterface((IVideoWindow*)this, ppv);
      if(riid == __uuidof(IBasicVideo))
        return GetInterface((IBasicVideo*)this, ppv);
      if(riid == __uuidof(IBasicVideo2))
        return GetInterface((IBasicVideo2*)this, ppv);
    }

    return SUCCEEDED(hr) ? hr : __super::NonDelegatingQueryInterface(riid, ppv);
  }

  // IVMRWindowlessControl

  STDMETHODIMP GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
  {
    IVMRWindowlessControl9* pWC9 = NULL;
    HRESULT hr = E_NOTIMPL;
    m_pVMR->QueryInterface(__uuidof(IVMRWindowlessControl9), (void**)&pWC9);
    
    if(pWC9)
      hr = pWC9->GetNativeVideoSize(lpWidth, lpHeight, lpARWidth, lpARHeight);
    
    SAFE_RELEASE(pWC9);
    return hr;
  }

  STDMETHODIMP GetMinIdealVideoSize(LONG* lpWidth, LONG* lpHeight) {return E_NOTIMPL;}

  STDMETHODIMP GetMaxIdealVideoSize(LONG* lpWidth, LONG* lpHeight) {return E_NOTIMPL;}

  STDMETHODIMP SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect) {return E_NOTIMPL;}

  STDMETHODIMP GetVideoPosition(LPRECT lpSRCRect, LPRECT lpDSTRect)
  {
    HRESULT hr = E_NOTIMPL;
    IVMRWindowlessControl9* pWC9 = NULL;
    if (SUCCEEDED(m_pVMR->QueryInterface(__uuidof(IVMRWindowlessControl9), (void**)&pWC9)))
    {
      hr = pWC9->GetVideoPosition(lpSRCRect, lpDSTRect);
    }
    SAFE_RELEASE(pWC9);
    return E_NOTIMPL;
  }

  STDMETHODIMP GetAspectRatioMode(DWORD* lpAspectRatioMode)
  {
    IVMRWindowlessControl9* pWC9 = NULL;
    if (SUCCEEDED(m_pVMR->QueryInterface(__uuidof(IVMRWindowlessControl9), (void**)&pWC9)))
    {
      *lpAspectRatioMode = VMR_ARMODE_NONE;
      SAFE_RELEASE(pWC9);
      return S_OK;
    }
    SAFE_RELEASE(pWC9);
    return E_NOTIMPL;
  }

  STDMETHODIMP SetAspectRatioMode(DWORD AspectRatioMode) {return E_NOTIMPL;}
  STDMETHODIMP SetVideoClippingWindow(HWND hwnd) {return E_NOTIMPL;}
  STDMETHODIMP RepaintVideo(HWND hwnd, HDC hdc) {return E_NOTIMPL;}
  STDMETHODIMP DisplayModeChanged() {return E_NOTIMPL;}
  STDMETHODIMP GetCurrentImage(BYTE** lpDib) {return E_NOTIMPL;}
  STDMETHODIMP SetBorderColor(COLORREF Clr) {return E_NOTIMPL;}
  STDMETHODIMP GetBorderColor(COLORREF* lpClr) {return E_NOTIMPL;}
  STDMETHODIMP SetColorKey(COLORREF Clr) {return E_NOTIMPL;}
  STDMETHODIMP GetColorKey(COLORREF* lpClr) {return E_NOTIMPL;}

  // IVideoWindow
  STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) {return E_NOTIMPL;}
  STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) {return E_NOTIMPL;}
  STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId) {return E_NOTIMPL;}
  STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {return E_NOTIMPL;}
  STDMETHODIMP put_Caption(BSTR strCaption) {return E_NOTIMPL;}
  STDMETHODIMP get_Caption(BSTR* strCaption) {return E_NOTIMPL;}
  STDMETHODIMP put_WindowStyle(long WindowStyle) {return E_NOTIMPL;}
  STDMETHODIMP get_WindowStyle(long* WindowStyle) {return E_NOTIMPL;}
  STDMETHODIMP put_WindowStyleEx(long WindowStyleEx) {return E_NOTIMPL;}
  STDMETHODIMP get_WindowStyleEx(long* WindowStyleEx) {return E_NOTIMPL;}
  STDMETHODIMP put_AutoShow(long AutoShow) {return E_NOTIMPL;}
  STDMETHODIMP get_AutoShow(long* AutoShow) {return E_NOTIMPL;}
  STDMETHODIMP put_WindowState(long WindowState) {return E_NOTIMPL;}
  STDMETHODIMP get_WindowState(long* WindowState) {return E_NOTIMPL;}
  STDMETHODIMP put_BackgroundPalette(long BackgroundPalette) {return E_NOTIMPL;}
  STDMETHODIMP get_BackgroundPalette(long* pBackgroundPalette) {return E_NOTIMPL;}
  STDMETHODIMP put_Visible(long Visible) {return E_NOTIMPL;}
  STDMETHODIMP get_Visible(long* pVisible) {return E_NOTIMPL;}
  STDMETHODIMP put_Left(long Left) {return E_NOTIMPL;}
  STDMETHODIMP get_Left(long* pLeft) {return E_NOTIMPL;}
  STDMETHODIMP put_Width(long Width) {return E_NOTIMPL;}
  STDMETHODIMP get_Width(long* pWidth)
  {
    IVMRWindowlessControl9* pWC9 = NULL;
    if (SUCCEEDED(m_pVMR->QueryInterface(__uuidof(IVMRWindowlessControl9), (void**)&pWC9)))
    {
      tagRECT s, d;
      HRESULT hr = pWC9->GetVideoPosition(&s, &d);
      *pWidth = d.right-d.left;
      SAFE_RELEASE(pWC9);
      return hr;
    }
    SAFE_RELEASE(pWC9);
    return E_NOTIMPL;
  }
  STDMETHODIMP put_Top(long Top) {return E_NOTIMPL;}
  STDMETHODIMP get_Top(long* pTop) {return E_NOTIMPL;}
  STDMETHODIMP put_Height(long Height) {return E_NOTIMPL;}
  STDMETHODIMP get_Height(long* pHeight)
  {
    IVMRWindowlessControl9* pWC9 = NULL;
    if (SUCCEEDED(m_pVMR->QueryInterface(__uuidof(IVMRWindowlessControl9), (void**)&pWC9)))
    {
      tagRECT s, d;
      HRESULT hr = pWC9->GetVideoPosition(&s, &d);
      *pHeight = g_geometryHelper.GetHeight(d);
      SAFE_RELEASE(pWC9);
      return hr;
    }
    SAFE_RELEASE(pWC9);
    return E_NOTIMPL;
  }
  STDMETHODIMP put_Owner(OAHWND Owner) {return E_NOTIMPL;}
  STDMETHODIMP get_Owner(OAHWND* Owner) {return E_NOTIMPL;}
  STDMETHODIMP put_MessageDrain(OAHWND Drain) {return E_NOTIMPL;}
  STDMETHODIMP get_MessageDrain(OAHWND* Drain) {return E_NOTIMPL;}
  STDMETHODIMP get_BorderColor(long* Color) {return E_NOTIMPL;}
  STDMETHODIMP put_BorderColor(long Color) {return E_NOTIMPL;}
  STDMETHODIMP get_FullScreenMode(long* FullScreenMode) {return E_NOTIMPL;}
  STDMETHODIMP put_FullScreenMode(long FullScreenMode) {return E_NOTIMPL;}
  STDMETHODIMP SetWindowForeground(long Focus) {return E_NOTIMPL;}
  STDMETHODIMP NotifyOwnerMessage(OAHWND hwnd, long uMsg, LONG_PTR wParam, LONG_PTR lParam) {return E_NOTIMPL;}
  STDMETHODIMP SetWindowPosition(long Left, long Top, long Width, long Height) {return E_NOTIMPL;}
  STDMETHODIMP GetWindowPosition(long* pLeft, long* pTop, long* pWidth, long* pHeight) {return E_NOTIMPL;}
  STDMETHODIMP GetMinIdealImageSize(long* pWidth, long* pHeight) {return E_NOTIMPL;}
  STDMETHODIMP GetMaxIdealImageSize(long* pWidth, long* pHeight) {return E_NOTIMPL;}
  STDMETHODIMP GetRestorePosition(long* pLeft, long* pTop, long* pWidth, long* pHeight) {return E_NOTIMPL;}
  STDMETHODIMP HideCursor(long HideCursor) {return E_NOTIMPL;}
  STDMETHODIMP IsCursorHidden(long* CursorHidden) {return E_NOTIMPL;}

  // IBasicVideo2
  STDMETHODIMP get_AvgTimePerFrame(REFTIME* pAvgTimePerFrame) {return E_NOTIMPL;}
  STDMETHODIMP get_BitRate(long* pBitRate) {return E_NOTIMPL;}
  STDMETHODIMP get_BitErrorRate(long* pBitErrorRate) {return E_NOTIMPL;}
  STDMETHODIMP get_VideoWidth(long* pVideoWidth) {return E_NOTIMPL;}
  STDMETHODIMP get_VideoHeight(long* pVideoHeight) {return E_NOTIMPL;}
  STDMETHODIMP put_SourceLeft(long SourceLeft) {return E_NOTIMPL;}
  STDMETHODIMP get_SourceLeft(long* pSourceLeft) {return E_NOTIMPL;}
  STDMETHODIMP put_SourceWidth(long SourceWidth) {return E_NOTIMPL;}
  STDMETHODIMP get_SourceWidth(long* pSourceWidth) {return E_NOTIMPL;}
  STDMETHODIMP put_SourceTop(long SourceTop) {return E_NOTIMPL;}
  STDMETHODIMP get_SourceTop(long* pSourceTop) {return E_NOTIMPL;}
  STDMETHODIMP put_SourceHeight(long SourceHeight) {return E_NOTIMPL;}
  STDMETHODIMP get_SourceHeight(long* pSourceHeight) {return E_NOTIMPL;}
  STDMETHODIMP put_DestinationLeft(long DestinationLeft) {return E_NOTIMPL;}
  STDMETHODIMP get_DestinationLeft(long* pDestinationLeft) {return E_NOTIMPL;}
  STDMETHODIMP put_DestinationWidth(long DestinationWidth) {return E_NOTIMPL;}
  STDMETHODIMP get_DestinationWidth(long* pDestinationWidth) {return E_NOTIMPL;}
  STDMETHODIMP put_DestinationTop(long DestinationTop) {return E_NOTIMPL;}
  STDMETHODIMP get_DestinationTop(long* pDestinationTop) {return E_NOTIMPL;}
  STDMETHODIMP put_DestinationHeight(long DestinationHeight) {return E_NOTIMPL;}
  STDMETHODIMP get_DestinationHeight(long* pDestinationHeight) {return E_NOTIMPL;}
  STDMETHODIMP SetSourcePosition(long Left, long Top, long Width, long Height) {return E_NOTIMPL;}
  STDMETHODIMP GetSourcePosition(long* pLeft, long* pTop, long* pWidth, long* pHeight)
  {
    // DVD Nav. bug workaround fix
    *pLeft = *pTop = 0;
    return GetVideoSize(pWidth, pHeight);
  }
/*
    if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
    {
      tagRECT s, d;
      HRESULT hr = pWC9->GetVideoPosition(&s, &d);
      *pLeft = s.left;
      *pTop = s.top;
      *pWidth = s.Width();
      *pHeight = s.Height();
      return hr;
    }
*/
  STDMETHODIMP SetDefaultSourcePosition() {return E_NOTIMPL;}
  STDMETHODIMP SetDestinationPosition(long Left, long Top, long Width, long Height) {return E_NOTIMPL;}
  STDMETHODIMP GetDestinationPosition(long* pLeft, long* pTop, long* pWidth, long* pHeight)
  {
    IVMRWindowlessControl9* pWC9 = NULL;
    if (SUCCEEDED(m_pVMR->QueryInterface(__uuidof(IVMRWindowlessControl9), (void**)&pWC9)))
    {
      tagRECT s, d;
      HRESULT hr = pWC9->GetVideoPosition(&s, &d);
      *pLeft = d.left;
      *pTop = d.top;
      *pWidth = g_geometryHelper.GetWidth(d);
      *pHeight = g_geometryHelper.GetHeight(d);
      SAFE_RELEASE(pWC9);
      return hr;
    }
    SAFE_RELEASE(pWC9);
    return E_NOTIMPL;
  }
  STDMETHODIMP SetDefaultDestinationPosition() {return E_NOTIMPL;}
  STDMETHODIMP GetVideoSize(long* pWidth, long* pHeight)
  {
    IVMRWindowlessControl9* pWC9 = NULL;
    if (SUCCEEDED(m_pVMR->QueryInterface(__uuidof(IVMRWindowlessControl9), (void**)&pWC9)))
    {
      LONG aw, ah;
      HRESULT hr = pWC9->GetNativeVideoSize(pWidth, pHeight, &aw, &ah);
      *pWidth = *pHeight * aw / ah;
      SAFE_RELEASE(pWC9);
      return hr;
    }
    SAFE_RELEASE(pWC9);
    return E_NOTIMPL;
  }

  STDMETHODIMP GetVideoPaletteEntries(long StartIndex, long Entries, long* pRetrieved, long* pPalette) {return E_NOTIMPL;}
  STDMETHODIMP GetCurrentImage(long* pBufferSize, long* pDIBImage) {return E_NOTIMPL;}
  STDMETHODIMP IsUsingDefaultSource() {return E_NOTIMPL;}
  STDMETHODIMP IsUsingDefaultDestination() {return E_NOTIMPL;}

  STDMETHODIMP GetPreferredAspectRatio(long* plAspectX, long* plAspectY)
  {
    HRESULT hr = E_NOTIMPL;
    IVMRWindowlessControl9* pWC9 = NULL;
    if (SUCCEEDED(m_pVMR->QueryInterface(__uuidof(IVMRWindowlessControl9), (void**)&pWC9)))
    {
      LONG w, h;
      hr = pWC9->GetNativeVideoSize(&w, &h, plAspectX, plAspectY);
    }
    SAFE_RELEASE(pWC9);
    return hr;
  }
};

CVMR9AllocatorPresenter::CVMR9AllocatorPresenter(HRESULT& hr, CStdString &_Error)
: CDsRenderer(),
  m_refCount(1),
  m_pNbrSurface(0),
  m_pCurSurface(0)
{
  hr = S_OK;
  m_bNeedNewDevice = false;
}

CVMR9AllocatorPresenter::~CVMR9AllocatorPresenter()
{
  DeleteVmrSurfaces();
  DeleteSurfaces();

  SAFE_DELETE(m_pMacrovisionKicker);
  SAFE_DELETE(m_pOuterVMR9);
}

void CVMR9AllocatorPresenter::DeleteVmrSurfaces()
{
  CAutoLock cAutoLock(this);
	CAutoLock cRenderLock(&m_RenderLock);
  for( size_t i = 0; i < m_pSurfaces.size(); ++i ) 
  {
    SAFE_RELEASE(m_pSurfaces[i]);
  }
}

//IVMRSurfaceAllocator9
STDMETHODIMP CVMR9AllocatorPresenter::InitializeDevice(DWORD_PTR dwUserID ,VMR9AllocationInfo *lpAllocInfo, DWORD *lpNumBuffers)
{
  CAutoLock cAutoLock(this);
  CAutoLock cRenderLock(&m_RenderLock);
  CLog::Log(LOGDEBUG,"%s %dx%d AR %d:%d flags:%d buffers:%d  fmt:(%x) %c%c%c%c", __FUNCTION__,
    lpAllocInfo->dwWidth ,lpAllocInfo->dwHeight ,lpAllocInfo->szAspectRatio.cx,lpAllocInfo->szAspectRatio.cy,
    lpAllocInfo->dwFlags ,*lpNumBuffers, lpAllocInfo->Format, ((char)lpAllocInfo->Format&0xff),
	  ((char)(lpAllocInfo->Format>>8)&0xff) ,((char)(lpAllocInfo->Format>>16)&0xff) ,((char)(lpAllocInfo->Format>>24)&0xff));

  if( !lpAllocInfo || !lpNumBuffers )
    return E_POINTER;
  if( !m_pIVMRSurfAllocNotify)
    return E_FAIL;
  
  HRESULT hr = S_OK;

  if(lpAllocInfo->Format == '21VY' || lpAllocInfo->Format == '024I')
    return E_FAIL;
  
  int nOriginal = *lpNumBuffers;

  //To do implement the texture surface on the present image
  //if(lpAllocInfo->dwFlags & VMR9AllocFlag_3DRenderTarget)
  //lpAllocInfo->dwFlags |= VMR9AllocFlag_TextureSurface;
  if (lpAllocInfo->dwFlags & VMR9AllocFlag_3DRenderTarget)
    CLog::Log(LOGDEBUG,"VMR9AllocFlag_3DRenderTarget");
  if (lpAllocInfo->dwFlags & VMR9AllocFlag_DXVATarget)
    CLog::Log(LOGDEBUG,"VMR9AllocFlag_DXVATarget");
  if (lpAllocInfo->dwFlags & VMR9AllocFlag_OffscreenSurface) 
    CLog::Log(LOGDEBUG,"VMR9AllocFlag_OffscreenSurface");
  if (lpAllocInfo->dwFlags & VMR9AllocFlag_RGBDynamicSwitch) 
    CLog::Log(LOGDEBUG,"VMR9AllocFlag_RGBDynamicSwitch");
  if (lpAllocInfo->dwFlags & VMR9AllocFlag_TextureSurface )
    CLog::Log(LOGDEBUG,"VMR9AllocFlag_TextureSurface");
  
  if (*lpNumBuffers == 1)
	{
		*lpNumBuffers = 4;
		m_pNbrSurface = 4;
	}
	else
		m_pNbrSurface = 0;

  m_pSurfaces.resize(*lpNumBuffers);
  hr = m_pIVMRSurfAllocNotify->AllocateSurfaceHelper(lpAllocInfo, lpNumBuffers, & m_pSurfaces.at(0) );
  if(FAILED(hr))
  {
    CLog::Log(LOGERROR,"%s AllocateSurfaceHelper returned:0x%x",__FUNCTION__,hr);
    return hr;
  }
  m_iVideoWidth = lpAllocInfo->dwWidth;
  m_iVideoHeight = lpAllocInfo->dwHeight;

  //INITIALIZE VIDEO SURFACE THERE
  hr = CreateSurfaces();

  if ( FAILED( hr ) )
    return hr;

  for( int i = 0; i < DS_NBR_3D_SURFACE-1; ++i ) 
  {
    hr = g_Windowing.Get3DDevice()->ColorFill(m_pVideoSurface[i], NULL, 0);
    if (SUCCEEDED(hr))
      break;
  }

  if ( FAILED( hr ) )
  {
    CLog::Log(LOGERROR,"%s ColorFill returned:0x%x",__FUNCTION__,hr);
    DeleteSurfaces();
    return hr;
  }
  if (m_pNbrSurface && m_pNbrSurface != *lpNumBuffers)
		m_pNbrSurface = *lpNumBuffers;
  
	*lpNumBuffers = (nOriginal < *lpNumBuffers) ? nOriginal : *lpNumBuffers;
	m_pCurSurface = 0;
  return hr;
}

void CVMR9AllocatorPresenter::GetCurrentVideoSize()
{
  IBaseFilter* pVMR9;
  IPin* pPin;
  CMediaType mt;
  if (SUCCEEDED (m_pIVMRSurfAllocNotify->QueryInterface (__uuidof(IBaseFilter), (void**)&pVMR9)) &&
      SUCCEEDED (pVMR9->FindPin(L"VMR Input0", &pPin)) &&
      SUCCEEDED (pPin->ConnectionMediaType(&mt)) )
  {
    DShowUtil::ExtractAvgTimePerFrame(&mt,m_rtTimePerFrame);
    if (mt.formattype == FORMAT_VideoInfo || mt.formattype == FORMAT_MPEGVideo) {

      VIDEOINFOHEADER *vh = (VIDEOINFOHEADER*)mt.pbFormat;
      m_iVideoWidth = vh->bmiHeader.biWidth;
      m_iVideoHeight = abs(vh->bmiHeader.biHeight);
      if (vh->rcTarget.right - vh->rcTarget.left > 0)
        m_iVideoWidth = vh->rcTarget.right - vh->rcTarget.left;
      else if (vh->rcSource.right - vh->rcSource.left > 0)
        m_iVideoWidth = vh->rcSource.right - vh->rcSource.left;
      if (vh->rcTarget.bottom - vh->rcTarget.top > 0)
        m_iVideoHeight = vh->rcTarget.bottom - vh->rcTarget.top;
      else if (vh->rcSource.bottom - vh->rcSource.top > 0)
        m_iVideoHeight = vh->rcSource.bottom - vh->rcSource.top;

    } else if (mt.formattype==FORMAT_VideoInfo2 || mt.formattype==FORMAT_MPEG2Video) {

      VIDEOINFOHEADER2 *vh = (VIDEOINFOHEADER2*)mt.pbFormat;
      m_iVideoWidth = vh->bmiHeader.biWidth;
      m_iVideoHeight = abs(vh->bmiHeader.biHeight);
      if (vh->rcTarget.right - vh->rcTarget.left > 0)
        m_iVideoWidth = vh->rcTarget.right - vh->rcTarget.left;
      else if (vh->rcSource.right - vh->rcSource.left > 0)
        m_iVideoWidth = vh->rcSource.right - vh->rcSource.left;

      if (vh->rcTarget.bottom - vh->rcTarget.top > 0)
        m_iVideoHeight = vh->rcTarget.bottom - vh->rcTarget.top;
      else if (vh->rcSource.bottom - vh->rcSource.top > 0)
        m_iVideoHeight = vh->rcSource.bottom - vh->rcSource.top;

    }

    // If 0 defaulting framerate to 23.97...
		if (m_rtTimePerFrame == 0) 
      m_rtTimePerFrame = 417166;

		m_fps = (float) ( 10000000.0 / m_rtTimePerFrame );
    
    g_renderManager.Configure(m_iVideoWidth, m_iVideoHeight, m_iVideoWidth, m_iVideoHeight, m_fps, CONF_FLAGS_FULLSCREEN);

  }
  SAFE_RELEASE(pVMR9);
}

STDMETHODIMP CVMR9AllocatorPresenter::TerminateDevice(DWORD_PTR dwID)
{
    DeleteVmrSurfaces();
    return S_OK;
}
    
STDMETHODIMP CVMR9AllocatorPresenter::GetSurface(DWORD_PTR dwUserID ,DWORD SurfaceIndex ,DWORD SurfaceFlags ,IDirect3DSurface9 **lplpSurface)
{
  if( !lplpSurface )
    return E_POINTER;

  //return if the surface index is higher than the size of the surfaces we have
  if (SurfaceIndex >= m_pSurfaces.size() ) 
    return E_FAIL;
  
  CAutoLock cRenderLock(&m_RenderLock);
  if (m_pNbrSurface)
  {
    ++m_pCurSurface;
    m_pCurSurface = m_pCurSurface % m_pNbrSurface;
    if (m_pSurfaces[m_pCurSurface + SurfaceIndex])
      (*lplpSurface = m_pSurfaces[m_pCurSurface + SurfaceIndex])->AddRef();
  }
  else
  {
    m_pNbrSurface = SurfaceIndex;
    (*lplpSurface = m_pSurfaces[SurfaceIndex])->AddRef();
  }

  return S_OK;
}
    
STDMETHODIMP CVMR9AllocatorPresenter::AdviseNotify(IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify)
{
  CAutoLock cAutoLock(this);
  CAutoLock cRenderLock(&m_RenderLock);
  HRESULT hr;
  m_pIVMRSurfAllocNotify = lpIVMRSurfAllocNotify;
  HMONITOR hMonitor = g_Windowing.Get3DObject()->GetAdapterMonitor(GetAdapter(g_Windowing.Get3DObject()));
  hr = m_pIVMRSurfAllocNotify->SetD3DDevice( g_Windowing.Get3DDevice(), hMonitor);
  return hr;
}

STDMETHODIMP CVMR9AllocatorPresenter::StartPresenting(DWORD_PTR dwUserID)
{
  CAutoLock cAutoLock(this);
  CAutoLock cRenderLock(&m_RenderLock);
  HRESULT hr = S_OK;
  
  ASSERT( g_Windowing.Get3DDevice() );
  if( !g_Windowing.Get3DDevice() )
    hr =  E_FAIL;
  
  return hr;
}

STDMETHODIMP CVMR9AllocatorPresenter::StopPresenting(DWORD_PTR dwUserID)
{
    return S_OK;
}

// IUnknown
STDMETHODIMP CVMR9AllocatorPresenter::QueryInterface( 
        REFIID riid,
        void** ppvObject)
{
    HRESULT hr = E_NOINTERFACE;

    if( ppvObject == NULL ) {
        hr = E_POINTER;
    } 
    else if( riid == IID_IVMRSurfaceAllocator9 ) {
        *ppvObject = static_cast<IVMRSurfaceAllocator9*>( this );
        AddRef();
        hr = S_OK;
    } 
    else if( riid == IID_IVMRImagePresenter9 ) {
        *ppvObject = static_cast<IVMRImagePresenter9*>( this );
        AddRef();
        hr = S_OK;
    }
    else if( riid == IID_IUnknown ) {
        *ppvObject = 
            static_cast<IUnknown*>( 
            static_cast<IVMRSurfaceAllocator9*>( this ) );
        AddRef();
        hr = S_OK;    
    }

    return hr;
}

ULONG CVMR9AllocatorPresenter::AddRef()
{
    return InterlockedIncrement(& m_refCount);
}

ULONG CVMR9AllocatorPresenter::Release()
{
    ULONG ret = InterlockedDecrement(& m_refCount);
    if( ret == 0 )
    {
        delete this;
    }

    return ret;
}

STDMETHODIMP CVMR9AllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
  CheckPointer(ppRenderer, E_POINTER);

  HRESULT hr;
  *ppRenderer = NULL;

  do
  {
    m_pMacrovisionKicker = new CMacrovisionKicker(NAME("CMacrovisionKicker"), NULL);
    IUnknown* pUnk = (IUnknown*)(INonDelegatingUnknown*) m_pMacrovisionKicker;

    m_pOuterVMR9 = new COuterVMR9(NAME("COuterVMR9"), pUnk, this);


    m_pMacrovisionKicker->SetInner((IUnknown*)(INonDelegatingUnknown*) m_pOuterVMR9);
    IBaseFilter* pBF = NULL;
    pUnk->QueryInterface(__uuidof(IBaseFilter), (void**)&pBF);

    IPin* pPin = DShowUtil::GetFirstPin(pBF);

    IMemInputPin* pMemInputPin;
    pPin->QueryInterface(__uuidof(IMemInputPin), (void**)&pMemInputPin);

    m_fUseInternalTimer = HookNewSegmentAndReceive((IPinC*)(IPin*)pPin, (IMemInputPinC*)(IMemInputPin*)pMemInputPin);

    IAMVideoAccelerator* pAMVA;
    if (SUCCEEDED(pPin->QueryInterface(__uuidof(IAMVideoAccelerator), (void**)&pAMVA)))
      HookAMVideoAccelerator((IAMVideoAcceleratorC*)(IAMVideoAccelerator*)pAMVA);

    SAFE_RELEASE(pPin);
    SAFE_RELEASE(pAMVA);
    SAFE_RELEASE(pMemInputPin);
    
    IVMRFilterConfig9* pConfig = NULL;
    pBF->QueryInterface(__uuidof(IVMRFilterConfig9), (void**)&pConfig);
    if(!pConfig)
      break;

    if (1)
    {
      if(FAILED(hr = pConfig->SetNumberOfStreams(1)))
      {
        SAFE_RELEASE(pConfig);
        break;
      }

      IVMRMixerControl9* pMC;
      if (SUCCEEDED(pBF->QueryInterface(__uuidof(IVMRMixerControl9), (void**)&pMC)))
      {
        DWORD dwPrefs;
        pMC->GetMixingPrefs(&dwPrefs);  

        // See http://msdn.microsoft.com/en-us/library/dd390928(VS.85).aspx
        dwPrefs |= MixerPref9_NonSquareMixing;
        dwPrefs |= MixerPref9_NoDecimation;
        dwPrefs &= ~MixerPref9_RenderTargetMask; 
        dwPrefs |= MixerPref9_RenderTargetYUV;

        pMC->SetMixingPrefs(dwPrefs); 

        SAFE_RELEASE(pMC);
      }
    }

    if(FAILED(hr = pConfig->SetRenderingMode(VMR9Mode_Renderless)))
    {
      SAFE_RELEASE(pConfig);
      break;
    }

    IVMRSurfaceAllocatorNotify9* pSAN;
    if (FAILED(pBF->QueryInterface(__uuidof(IVMRSurfaceAllocatorNotify9), (void**)&pSAN)))
    {
      SAFE_RELEASE(pConfig);
      break;
    }

    DWORD_PTR MY_USER_ID = 0xACDCACDC;
    if(FAILED(hr = pSAN->AdviseSurfaceAllocator(MY_USER_ID, static_cast<IVMRSurfaceAllocator9*>(this)))
    || FAILED(hr = AdviseNotify(pSAN)))
    {
      SAFE_RELEASE(pConfig);
      SAFE_RELEASE(pSAN);
      break;
    }

    SAFE_RELEASE(pConfig);
    SAFE_RELEASE(pSAN);

    *ppRenderer = (IUnknown *) pBF;
    SAFE_RELEASE(pBF);

    return S_OK;
  }
  while(0);

  return E_FAIL;
}

STDMETHODIMP CVMR9AllocatorPresenter::PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo *lpPresInfo)
{
  HRESULT hr;
  CheckPointer(m_pIVMRSurfAllocNotify, E_UNEXPECTED);

  if (!g_renderManager.IsConfigured())
  {
    GetCurrentVideoSize();
  }

  if (!g_renderManager.IsStarted())
    return E_FAIL;
  
  if(!lpPresInfo || !lpPresInfo->lpSurf)
    return E_POINTER;

	CAutoLock Lock(&m_RenderLock);
  if (m_bNeedNewDevice)
  {
    if (SUCCEEDED(g_Windowing.GetDeviceStatus()))
      hr = ChangeD3dDev();
    else
      CLog::Log(LOGDEBUG,"Need new device but 3d device suck");
    
    return S_OK;
  }

  IDirect3DTexture9* pTexture = NULL;
  lpPresInfo->lpSurf->GetContainer(IID_IDirect3DTexture9, (void**)&pTexture);
  if(pTexture)
  {
    // When using VMR9AllocFlag_TextureSurface
    // Didnt got it working yet
    SAFE_RELEASE(pTexture);
  }
  else
  {
    hr = g_Windowing.Get3DDevice()->StretchRect(lpPresInfo->lpSurf, NULL, m_pVideoSurface[m_nCurSurface], NULL, D3DTEXF_NONE);
  }
  if (SUCCEEDED(hr))
    RenderPresent(m_pVideoTexture[m_nCurSurface], m_pVideoSurface[m_nCurSurface]);
  
  return hr;
}

HRESULT CVMR9AllocatorPresenter::ChangeD3dDev()
{
  HRESULT hr;
  DeleteVmrSurfaces();
  DeleteSurfaces();
  hr = m_pIVMRSurfAllocNotify->ChangeD3DDevice(g_Windowing.Get3DDevice(),g_Windowing.Get3DObject()->GetAdapterMonitor(GetAdapter(g_Windowing.Get3DObject())));
  if (SUCCEEDED(hr))
  {
    CLog::Log(LOGDEBUG,"%s Changed d3d device",__FUNCTION__);
    m_bNeedNewDevice = false;
  }
  return hr;
}

void CVMR9AllocatorPresenter::OnLostDevice()
{
  //CLog::Log(LOGDEBUG,"%s",__FUNCTION__);
  
}

void CVMR9AllocatorPresenter::OnDestroyDevice()
{
  //Only this one is required for changing the device
  CLog::Log(LOGDEBUG,"%s",__FUNCTION__);
  m_bNeedNewDevice = true;
}

void CVMR9AllocatorPresenter::OnCreateDevice()
{
  CLog::Log(LOGDEBUG,"%s",__FUNCTION__);
}