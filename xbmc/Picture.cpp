
#include "stdafx.h"
#include "picture.h"
#include "util.h"
#include "sectionloader.h"
#include "filesystem/file.h"
#include "utils/log.h"
#include "autoptrhandle.h"
#include "settings.h"

using namespace AUTOPTR;

#define QUALITY 0
#define MAX_THUMB_WIDTH 128
#define MAX_THUMB_HEIGHT 128

#define MAX_ALBUM_THUMB_WIDTH 128
#define MAX_ALBUM_THUMB_HEIGHT 128

CPicture::CPicture(void)
{
	m_bSectionLoaded=false;
}

CPicture::~CPicture(void)
{
	Free();
}

void CPicture::Free()
{

	if ( m_bSectionLoaded)
	{
		CSectionLoader::Unload("CXIMAGE");
		m_bSectionLoaded=false;
	}
}

IDirect3DTexture8* CPicture::Load(const CStdString& strFileName, int &iOriginalWidth, int &iOriginalHeight, int iMaxWidth, int iMaxHeight)
{
	IDirect3DTexture8* pTexture=NULL;
	CStdString strExtension;
	CStdString strCachedFile;
	DWORD dwImageType=0xffff;

	CUtil::GetExtension(strFileName,strExtension);
	if (!strExtension.size()) return NULL;

	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".bmp") ) dwImageType=CXIMAGE_FORMAT_BMP;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".gif") ) dwImageType=CXIMAGE_FORMAT_GIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tbn") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".jpg") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".jpeg") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".png") ) dwImageType=CXIMAGE_FORMAT_PNG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".ico") ) dwImageType=CXIMAGE_FORMAT_ICO;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tif") ) dwImageType=CXIMAGE_FORMAT_TIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tiff") ) dwImageType=CXIMAGE_FORMAT_TIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tga") ) dwImageType=CXIMAGE_FORMAT_TGA;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".pcx") ) dwImageType=CXIMAGE_FORMAT_PCX;


  if (! CUtil::IsHD(strFileName))
  {
	  strCachedFile="Z:\\cachedpic";
	  strCachedFile+= strExtension;
	  CFile file;
    ::DeleteFile(strCachedFile.c_str());
	  if ( !file.Cache(strFileName.c_str(),strCachedFile.c_str(),NULL,NULL) )
	  {
      ::DeleteFile(strCachedFile.c_str());
			CLog::Log(LOGERROR, "PICTURE::load: Unable to cache file %s\n", strFileName.c_str());
		  return NULL;
	  }
  }
  else
  {
    strCachedFile=strFileName;
  }

//	pTexture = LoadNative(strCachedFile);
//	if (pTexture) return pTexture;

	if (!m_bSectionLoaded)
	{
		CSectionLoader::Load("CXIMAGE");
		m_bSectionLoaded=true;
	}

	CxImage image(dwImageType);
  try
  {
	  if (!image.Load(strCachedFile.c_str(),dwImageType) || !image.IsValid())
	  {
			CLog::Log(LOGERROR, "PICTURE::load: Unable to open image: %s Error:%s\n", strCachedFile.c_str(), image.GetLastError());
		  return NULL;
	  }
  }
  catch(...)
  {
		CLog::Log(LOGERROR, "PICTURE::load: Unable to open image: %s\n", strCachedFile.c_str());
    if (CUtil::IsHD(strCachedFile) )
    {
      ::DeleteFile(strCachedFile.c_str());
    }
    return NULL;
  }

	m_dwWidth  = iOriginalWidth  = image.GetWidth();
	m_dwHeight = iOriginalHeight = image.GetHeight();

	bool bResize=false;
	float fAspect= ((float)m_dwWidth) / ((float)m_dwHeight);

	if (m_dwWidth > (DWORD)iMaxWidth)
    {
		bResize=true;
		m_dwWidth  = iMaxWidth;
		m_dwHeight = (DWORD)( ( (float)m_dwWidth) / fAspect);
    }

    if (m_dwHeight > (DWORD)iMaxHeight)
    {
		bResize=true;
		m_dwHeight = iMaxHeight;
		m_dwWidth  = (DWORD)(  fAspect * ( (float)m_dwHeight) );
    }

  if (bResize)
  {
		if (!image.Resample(m_dwWidth,m_dwHeight, QUALITY) || !image.IsValid())
		{
			CLog::Log(LOGERROR, "PICTURE::Load: Unable to resample picture: %s\n", strCachedFile.c_str());
			return NULL;
		}
	}

	m_dwWidth=image.GetWidth();
	m_dwHeight=image.GetHeight();

	return GetTexture(image);;
}

IDirect3DTexture8* CPicture::LoadNative(const CStdString& strFileName)
{
	CStdString strExtension;
	CUtil::GetExtension(strFileName,strExtension);
	if (!strExtension.size()) return NULL;

	// TODO: Add other known stuff that D3DCreateTextureFromFileEx supports
	if ( 0!=CUtil::cmpnocase(strExtension.c_str(),".jpg") &&
				0!=CUtil::cmpnocase(strExtension.c_str(),".jpeg") &&
				0!=CUtil::cmpnocase(strExtension.c_str(),".png") &&
				0!=CUtil::cmpnocase(strExtension.c_str(),".bmp")) return NULL;

  try
  {
		CGUIImage *newImage = new CGUIImage(WINDOW_SLIDESHOW, 100, 0, 0, 720, 576, strFileName);
		newImage->AllocResources();
		return newImage->m_vecTextures[0];
		IDirect3DTexture8 *pTexture = NULL;
		if (g_graphicsContext.Get3DDevice()->CreateTexture(1024, 1024, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture) == D3D_OK)
		{
			IDirect3DSurface8 *pSurface = NULL;
			if (pTexture->GetSurfaceLevel(0, &pSurface) == D3D_OK)
			{
				D3DXIMAGE_INFO info;
				if (D3DXLoadSurfaceFromFile(pSurface, NULL, NULL, strFileName.c_str(), NULL, D3DX_DEFAULT, 0, &info)==D3D_OK)
				{
					m_dwWidth  = info.Width;
					m_dwHeight = info.Height;
					pSurface->Release();
					return pTexture;
				}
			}
		}
		return NULL;
/*		D3DXIMAGE_INFO info;
		DWORD dwColorKey = 0;
		if (D3DXCreateTextureFromFileEx(g_graphicsContext.Get3DDevice(),strFileName.c_str(), D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
				D3DX_FILTER_NONE , D3DX_FILTER_NONE, dwColorKey, &info, NULL, &pTexture) != D3D_OK)
		{
			CLog::Log(LOGERROR, "PICTURE::load: Unable to open image: %s\n", strFileName.c_str());
		  return NULL;
	  }

		m_dwWidth  = info.Width;
		m_dwHeight = info.Height;
		return pTexture;*/
  }
  catch(...)
  {
		CLog::Log(LOGERROR, "PICTURE::load: Unable to open image: %s\n", strFileName.c_str());
    return NULL;
  }
}

DWORD	CPicture::GetWidth()  const
{
	return m_dwWidth;
}

DWORD	CPicture::GetHeight()  const
{
	return m_dwHeight;
}


IDirect3DTexture8* CPicture::GetTexture( CxImage& image  )
{
	IDirect3DTexture8* pTexture=NULL;
	if (g_graphicsContext.Get3DDevice()->CreateTexture( image.GetWidth(),
																											image.GetHeight(),
																											1, // levels
																											0, //usage
																											D3DFMT_LIN_A8R8G8B8 ,
																											0,
																											&pTexture) != D3D_OK)
	{
		return NULL;
	}
	if (!pTexture)
	{
		return NULL;
	}

	DWORD dwHeight=image.GetHeight();
	DWORD dwWidth =image.GetWidth();

	D3DLOCKED_RECT lr;
	if ( D3D_OK == pTexture->LockRect( 0, &lr, NULL, 0 ))
	{
		DWORD strideScreen=lr.Pitch;
		for (DWORD y=0; y < dwHeight; y++)
		{
			BYTE *pDest = (BYTE*)lr.pBits + strideScreen*y;
			for (DWORD x=0; x < dwWidth; x++)
			{
				RGBQUAD rgb=image.GetPixelColor( x, dwHeight-1-y);

				*pDest++ = rgb.rgbBlue;
				*pDest++ = rgb.rgbGreen;
				*pDest++ = rgb.rgbRed;
        *pDest++ = 0xff;
			}
		}
		pTexture->UnlockRect( 0 );
	}
	return pTexture;
}

IDirect3DTexture8* CPicture::GetYUY2Texture( CxImage& image  )
{
	IDirect3DTexture8* pTexture=NULL;
	if (g_graphicsContext.Get3DDevice()->CreateTexture( image.GetWidth(),
																											image.GetHeight(),
																											1, // levels
																											0, //usage
																											D3DFMT_YUY2 ,
																											0,
																											&pTexture) != D3D_OK)
	{
		return NULL;
	}
	if (!pTexture)
	{
		return NULL;
	}

	DWORD dwHeight=image.GetHeight();
	DWORD dwWidth =image.GetWidth();

	D3DLOCKED_RECT lr;
	if ( D3D_OK == pTexture->LockRect( 0, &lr, NULL, 0 ))
	{
		// translate RGB->YUV2
		DWORD strideScreen=lr.Pitch;
		for (DWORD y=0; y < dwHeight; y++)
		{
			BYTE *pDest = (BYTE*)lr.pBits + strideScreen*y;
			for (DWORD x=0; x < (dwWidth>>1); x++)
			{
				RGBQUAD rgb1=image.GetPixelColor( (x<<1), y);
				RGBQUAD yuv1=image.RGBtoYUV(rgb1);
				RGBQUAD rgb2=image.GetPixelColor((x<<1)+1,y);
				RGBQUAD yuv2=image.RGBtoYUV(rgb2);
				BYTE Y0 = yuv1.rgbRed;
				BYTE U = (yuv1.rgbGreen+yuv2.rgbGreen)>>1;
				BYTE Y1 = yuv2.rgbRed;
				BYTE V = (yuv1.rgbBlue+yuv2.rgbBlue)>>1;

				*pDest++ = Y0;
				*pDest++ = U;
				*pDest++ = Y1;
				*pDest++ = V;
			}
		}
		pTexture->UnlockRect( 0 );
	}
	return pTexture;
}

bool CPicture::CreateAlbumThumbnail(const CStdString& strFileName, const CStdString& strAlbum)
{
  CStdString strThumbnail;
	CUtil::GetAlbumFolderThumb(strAlbum, strThumbnail,true);
	return DoCreateThumbnail(strFileName, strThumbnail, MAX_ALBUM_THUMB_WIDTH, MAX_ALBUM_THUMB_HEIGHT);
}

bool CPicture::CreateThumnail(const CStdString& strFileName)
{
  CStdString strThumbnail;
  CUtil::GetThumbnail(strFileName, strThumbnail);
  bool bCacheFile=CUtil::IsRemote(strFileName);
	return DoCreateThumbnail(strFileName, strThumbnail, MAX_THUMB_WIDTH, MAX_THUMB_HEIGHT,bCacheFile);
}

bool CPicture::DoCreateThumbnail(const CStdString& strFileName, const CStdString& strThumbFileName, int nMaxWidth, int nMaxHeight, bool bCacheFile/*=true*/)
{
	CLog::Log(LOGINFO, "Creating thumb from: %s", strFileName.c_str());
	CStdString strExtension;
	CStdString strCachedFile;
	DWORD dwImageType=CXIMAGE_FORMAT_JPG;

	CUtil::GetExtension(strFileName,strExtension);
	if (!strExtension.size()) return false;

	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".bmp") ) dwImageType=CXIMAGE_FORMAT_BMP;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".gif") ) dwImageType=CXIMAGE_FORMAT_GIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".jpg") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tbn") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".jpeg") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".png") ) dwImageType=CXIMAGE_FORMAT_PNG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".ico") ) dwImageType=CXIMAGE_FORMAT_ICO;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tif") ) dwImageType=CXIMAGE_FORMAT_TIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tiff") ) dwImageType=CXIMAGE_FORMAT_TIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tga") ) dwImageType=CXIMAGE_FORMAT_TGA;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".pcx") ) dwImageType=CXIMAGE_FORMAT_PCX;


	if (bCacheFile)
	{
		strCachedFile="Z:\\cachedpic";
		strCachedFile+= strExtension;
		CFile file;
		if ( !file.Cache(strFileName.c_str(),strCachedFile.c_str(),NULL,NULL) )
		{
			CLog::Log(LOGERROR, "PICTURE::docreatethumbnail: Unable to cache file %s\n", strFileName.c_str());
			return false;
		}
	}
	else
		strCachedFile=strFileName;

	if (!m_bSectionLoaded)
	{
		CSectionLoader::Load("CXIMAGE");
		m_bSectionLoaded=true;
	}

	try
	{
		bool bNeedToConvert = dwImageType != CXIMAGE_FORMAT_JPG;
		CxImage image(dwImageType);
		if (!image.Load(strCachedFile.c_str(),dwImageType) || !image.IsValid())
		{
			CLog::Log(LOGERROR, "PICTURE::docreatethumbnail: Unable to open image: %s Error:%s\n", strCachedFile.c_str(), image.GetLastError());
			return false;
		}
		m_dwWidth=image.GetWidth();
		m_dwHeight=image.GetHeight();

		bool bResize=false;
		float fAspect= ((float)m_dwWidth) / ((float)m_dwHeight);

		if (m_dwWidth > (DWORD)nMaxWidth )
		{
			bResize=true;
			m_dwWidth  = nMaxWidth;
			m_dwHeight = (DWORD)( ( (float)m_dwWidth) / fAspect);
		}

		if (m_dwHeight > (DWORD)nMaxHeight )
		{
			bResize=true;
			m_dwHeight =nMaxHeight;
			m_dwWidth  = (DWORD)(  fAspect * ( (float)m_dwHeight) );
		}

		if (bResize)
		{
			if (!image.Resample(m_dwWidth,m_dwHeight, QUALITY) || !image.IsValid())
			{
				CLog::Log(LOGERROR, "PICTURE::docreatethumbnail: Unable to resample image: %s Error:%s\n", strCachedFile.c_str(), image.GetLastError());
				return false;
			}
			m_dwWidth=image.GetWidth();
			m_dwHeight=image.GetHeight();
			bNeedToConvert = true;
		}

		::DeleteFile(strThumbFileName.c_str());
		if ( image.GetNumColors() )
		{
			if (!image.IncreaseBpp(24) || !image.IsValid())
			{
				CLog::Log(LOGERROR, "PICTURE::docreatethumbnail: Unable to convert to 24bpp: %s Error:%s\n", strCachedFile.c_str(), image.GetLastError());
				return false;
			}
			bNeedToConvert = true;
		}
		// only resave the image if we have to (quality of the JPG saver isn't too hot!)
		if (bNeedToConvert)
		{
			// May as well have decent quality thumbs
			image.SetJpegQuality(90);
			if (!image.Save(strThumbFileName.c_str(),CXIMAGE_FORMAT_JPG))
			{
				CLog::Log(LOGERROR, "PICTURE::docreatethumbnail: Unable to save image: %s Error:%s\n", strThumbFileName.c_str(), image.GetLastError());
				::DeleteFile(strThumbFileName.c_str());
				return false;
			}
		}
		else
		{	// Don't need to convert the file - cache it instead
			CFile file;
			if ( !file.Cache(strFileName.c_str(),strThumbFileName.c_str(),NULL,NULL) )
			{
				CLog::Log(LOGERROR, "PICTURE::docreatethumbnail: Unable to copy file %s\n", strFileName.c_str());
				::DeleteFile(strThumbFileName.c_str());
				return false;
			}
		}
	}
	catch(...)
	{
		CLog::Log(LOGERROR, "PICTURE::docreatethumbnail: exception: %s\n", strCachedFile.c_str());
		return false;
	}
	return true;
}

bool CPicture::CreateAlbumThumbnailFromMemory(const BYTE* pBuffer, int nBufSize, const CStdString& strExtension, const CStdString& strThumbFileName)
{
	CLog::Log(LOGINFO, "Creating album thumb from memory: %s", strThumbFileName.c_str());
	DWORD dwImageType=CXIMAGE_FORMAT_UNKNOWN;

	if (strExtension.IsEmpty())
	{
		dwImageType=DetectFileType(pBuffer, nBufSize);
	}
	else
	{
		if ( 0==CUtil::cmpnocase(strExtension.c_str(),"bmp") ) dwImageType=CXIMAGE_FORMAT_BMP;
		if ( 0==CUtil::cmpnocase(strExtension.c_str(),"bitmap") ) dwImageType=CXIMAGE_FORMAT_BMP;
		if ( 0==CUtil::cmpnocase(strExtension.c_str(),"gif") ) dwImageType=CXIMAGE_FORMAT_GIF;
		if ( 0==CUtil::cmpnocase(strExtension.c_str(),"jpg") ) dwImageType=CXIMAGE_FORMAT_JPG;
		if ( 0==CUtil::cmpnocase(strExtension.c_str(),"tbn") ) dwImageType=CXIMAGE_FORMAT_JPG;
		if ( 0==CUtil::cmpnocase(strExtension.c_str(),"jpeg") ) dwImageType=CXIMAGE_FORMAT_JPG;
		if ( 0==CUtil::cmpnocase(strExtension.c_str(),"png") ) dwImageType=CXIMAGE_FORMAT_PNG;
		if ( 0==CUtil::cmpnocase(strExtension.c_str(),"ico") ) dwImageType=CXIMAGE_FORMAT_ICO;
		if ( 0==CUtil::cmpnocase(strExtension.c_str(),"tif") ) dwImageType=CXIMAGE_FORMAT_TIF;
		if ( 0==CUtil::cmpnocase(strExtension.c_str(),"tiff") ) dwImageType=CXIMAGE_FORMAT_TIF;
		if ( 0==CUtil::cmpnocase(strExtension.c_str(),"tga") ) dwImageType=CXIMAGE_FORMAT_TGA;
		if ( 0==CUtil::cmpnocase(strExtension.c_str(),"pcx") ) dwImageType=CXIMAGE_FORMAT_PCX;
	}

	if (dwImageType==CXIMAGE_FORMAT_UNKNOWN)
	{
		CLog::Log(LOGERROR, "PICTURE::createthumbnailfrommemory: Unknown filetype: %s\n", strExtension.c_str());
		return false;
	}

	auto_aptr<BYTE> pPicture(new BYTE[nBufSize]);
	fast_memcpy(pPicture.get(), pBuffer, nBufSize);

	if (!m_bSectionLoaded)
	{
		CSectionLoader::Load("CXIMAGE");
		m_bSectionLoaded=true;
	}

  try
	{
		CxImage image(dwImageType);
		if (!image.Decode(pPicture.get(),nBufSize,dwImageType) || !image.IsValid())
		{
			CLog::Log(LOGERROR, "PICTURE::createthumbnailfrommemory: Unable to load image from memory Error:%s\n", image.GetLastError());
			return false;
		}

		m_dwWidth=image.GetWidth();
		m_dwHeight=image.GetHeight();

    bool bResize=false;
    float fAspect= ((float)m_dwWidth) / ((float)m_dwHeight);

		if (m_dwWidth > (DWORD)MAX_ALBUM_THUMB_WIDTH )
    {
      bResize=true;
      m_dwWidth  = MAX_ALBUM_THUMB_WIDTH;
      m_dwHeight = (DWORD)( ( (float)m_dwWidth) / fAspect);
    }

    if (m_dwHeight > (DWORD)MAX_ALBUM_THUMB_HEIGHT )
    {
      bResize=true;
      m_dwHeight =MAX_ALBUM_THUMB_HEIGHT;
      m_dwWidth  = (DWORD)(  fAspect * ( (float)m_dwHeight) );
    }

    if (bResize)
    {
			if (!image.Resample(m_dwWidth,m_dwHeight, QUALITY) || !image.IsValid())
			{
				CLog::Log(LOGERROR, "PICTURE::createthumbnailfrommemory: Unable to resample image Error:%s\n", image.GetLastError());
				return false;
			}
			m_dwWidth=image.GetWidth();
			m_dwHeight=image.GetHeight();
		}

    ::DeleteFile(strThumbFileName.c_str());
    if ( image.GetNumColors() )
    {
      if (!image.IncreaseBpp(24) || !image.IsValid())
			{
				CLog::Log(LOGERROR, "PICTURE::createthumbnailfrommemory: Unable to increase bpp Error:%s\n", image.GetLastError());
				return false;
			}
    }
		image.SetJpegQuality(90);	// decent quality thumbs needed!
		if (!image.Save(strThumbFileName.c_str(),CXIMAGE_FORMAT_JPG))
    {
			CLog::Log(LOGERROR, "PICTURE::createthumbnailfrommemory: Unable to save image:%s Error:%s\n", strThumbFileName.c_str(), image.GetLastError());

      ::DeleteFile(strThumbFileName.c_str());
      return false;
    }
		return true;
	}
  catch(...)
  {
		CLog::Log(LOGERROR, "PICTURE::createthumbnailfrommemory: exception: memfile FileType: %s\n", strExtension.c_str());
  }
	return false;
}

int CPicture::DetectFileType(const BYTE* pBuffer, int nBufSize)
{
	if (nBufSize<=5)
		return CXIMAGE_FORMAT_UNKNOWN;

	if (pBuffer[1]=='P' && pBuffer[2]=='N' && pBuffer[3]=='G')
		return CXIMAGE_FORMAT_PNG;

	if (pBuffer[0]=='B' && pBuffer[1]=='M')
		return CXIMAGE_FORMAT_BMP;

	if (pBuffer[0]==0xFF && pBuffer[1]==0xD8 && pBuffer[2]==0xFF && pBuffer[3]==0xE0)
		return CXIMAGE_FORMAT_JPG;

	return CXIMAGE_FORMAT_UNKNOWN;
}

void CPicture::RenderImage(IDirect3DTexture8* pTexture,float x, float y, float width, float height, int iTextureWidth, int iTextureHeight, int iTextureLeft, int iTextureTop, DWORD dwAlpha)
{
  CPicture::VERTEX* vertex=NULL;
  LPDIRECT3DVERTEXBUFFER8 m_pVB;

	g_graphicsContext.Get3DDevice()->CreateVertexBuffer( 4*sizeof(CPicture::VERTEX), D3DUSAGE_WRITEONLY, 0L, D3DPOOL_DEFAULT, &m_pVB );
  m_pVB->Lock( 0, 0, (BYTE**)&vertex, 0L );

	float fxOff = (float)iTextureLeft;
	float fyOff = (float)iTextureTop;

  vertex[0].p = D3DXVECTOR4( x - 0.5f,	y - 0.5f,		0, 0 );
  vertex[0].tu = fxOff;
  vertex[0].tv = fyOff;

  vertex[1].p = D3DXVECTOR4( x+width - 0.5f,	y - 0.5f,		0, 0 );
  vertex[1].tu = fxOff+(float)iTextureWidth;
  vertex[1].tv = fyOff;

  vertex[2].p = D3DXVECTOR4( x+width - 0.5f,	y+height - 0.5f,	0, 0 );
  vertex[2].tu = fxOff+(float)iTextureWidth;
  vertex[2].tv = fyOff+(float)iTextureHeight;

  vertex[3].p = D3DXVECTOR4( x - 0.5f,	y+height - 0.5f,	0, 0 );
  vertex[3].tu = fxOff;
  vertex[3].tv = fyOff+iTextureHeight;

	D3DCOLOR dwColor = (dwAlpha << 24) | 0xFFFFFF;
	for (int i=0; i<4; i++)
	{
		vertex[i].col = dwColor;
	}
  m_pVB->Unlock();


	// Set state to render the image
	g_graphicsContext.Get3DDevice()->SetTexture( 0, pTexture );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_MAGFILTER,  g_stSettings.m_minFilter );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_MINFILTER,  g_stSettings.m_maxFilter );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ZENABLE,      FALSE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGENABLE,    FALSE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_CULLMODE,     D3DCULL_CCW );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_YUVENABLE, FALSE);
	g_graphicsContext.Get3DDevice()->SetVertexShader( FVF_VERTEX );
	// Render the image
	g_graphicsContext.Get3DDevice()->SetStreamSource( 0, m_pVB, sizeof(VERTEX) );
	g_graphicsContext.Get3DDevice()->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );
	// reset texture state
	g_graphicsContext.Get3DDevice()->SetTexture(0, NULL);
	m_pVB->Release();
}



bool CPicture::Convert(const CStdString& strSource,const CStdString& strDest)
{
	CStdString strExtension;
	CStdString strCachedFile;
	DWORD dwImageType;

	CUtil::GetExtension(strSource,strExtension);
	if (!strExtension.size()) return false;

	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".bmp") ) dwImageType=CXIMAGE_FORMAT_BMP;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".gif") ) dwImageType=CXIMAGE_FORMAT_GIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".jpg") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".jpeg") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".png") ) dwImageType=CXIMAGE_FORMAT_PNG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".ico") ) dwImageType=CXIMAGE_FORMAT_ICO;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tif") ) dwImageType=CXIMAGE_FORMAT_TIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tiff") ) dwImageType=CXIMAGE_FORMAT_TIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tga") ) dwImageType=CXIMAGE_FORMAT_TGA;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".pcx") ) dwImageType=CXIMAGE_FORMAT_PCX;


  if (!CUtil::IsHD(strSource))
  {
	  strCachedFile="Z:\\cachedpic";
	  strCachedFile+= strExtension;
	  CFile file;
	  if ( !file.Cache(strSource.c_str(),strCachedFile.c_str(),NULL,NULL) )
	  {
			CLog::Log(LOGERROR, "PICTURE::convert: Unable to cache image %s\n", strCachedFile.c_str());
      ::DeleteFile(strCachedFile.c_str());
		  return NULL;
	  }
  }
  else
  {
    strCachedFile=strSource;
  }

	if (!m_bSectionLoaded)
	{
		CSectionLoader::Load("CXIMAGE");
		m_bSectionLoaded=true;
	}

  try
	{
		CxImage image(dwImageType);
		if (!image.Load(strCachedFile.c_str(),dwImageType))
		{
			CLog::Log(LOGERROR, "PICTURE::convert: Unable to load image: %s Error:%s\n", strCachedFile.c_str(), image.GetLastError());
			return false;
		}

    ::DeleteFile(strDest.c_str());
    if ( image.GetNumColors() )
    {
      image.IncreaseBpp(24);
    }
		image.SetJpegQuality(90);	// decent quality thumbs needed!
		if (!image.Save(strDest.c_str(),CXIMAGE_FORMAT_JPG))
    {
			CLog::Log(LOGERROR, "PICTURE::convert: Unable to save image: %s Error:%s\n", strDest.c_str(), image.GetLastError());
      ::DeleteFile(strDest.c_str());
      return false;
    }
	}
  catch(...)
  {
			CLog::Log(LOGERROR, "PICTURE::convert: exception %s\n", strCachedFile.c_str());
    ::DeleteFile(strDest.c_str());
    return false;
  }

	return true;
}