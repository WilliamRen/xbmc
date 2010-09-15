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

#ifdef HAS_DS_PLAYER

#include "FGLoader.h"
#include "DSPlayer.h"
#include "Filters/RendererSettings.h"
#include "PixelShaderList.h"
#include "streamsmanager.h"
#include "DSUtil/DSUtil.h"
#include "DSUtil/SmartPtr.h"

#include "charsetconverter.h"
#include "Log.h"
#include "GUIDialogOK.h"
#include "GUIWindowManager.h"
#include "filtercorefactory/filtercorefactory.h"
#include "Settings.h"
#include "GUISettings.h"
#include "utils/SystemInfo.h"

#include "filters/XBMCFileSource.h"
#include "filters/Splitters/LAVFDemuxer.h"
#include "filters/DsVideoDecoder/XBMCVideoDecFilter.h"
#include "filters/VMR9AllocatorPresenter.h"
#include "filters/EVRAllocatorPresenter.h"

#include "Utils/AudioEnumerator.h"
#include "DVDFileInfo.h"
#include "VideoInfoTag.h"

using namespace std;

CFGLoader::CFGLoader():
  m_pFGF(NULL)
{
}

CFGLoader::~CFGLoader()
{
  CSingleLock lock(*this);
  
  CFilterCoreFactory::Destroy();
  SAFE_DELETE(m_pFGF);

  CLog::Log(LOGDEBUG, "%s Ressources released", __FUNCTION__);
}

HRESULT CFGLoader::InsertSourceFilter(CFileItem& pFileItem, const CStdString& filterName)
{

  HRESULT hr = E_FAIL;
  
  /* XBMC SOURCE FILTER  */
  /*if (CUtil::IsInArchive(pFileItem.m_strPath))
  {
    CLog::Log(LOGNOTICE,"%s File \"%s\" need a custom source filter", __FUNCTION__, pFileItem.m_strPath.c_str());
    CXBMCAsyncStream* pXBMCStream = new CXBMCAsyncStream(pFileItem.m_strPath, &CGraphFilters::Get()->Source.pBF, &hr);
    if (SUCCEEDED(hr))
    {
      hr = g_dsGraph->pFilterGraph->AddFilter(CGraphFilters::Get()->Source.pBF, L"XBMC Source Filter");
      if (FAILED(hr))
      {
        CLog::Log(LOGERROR, "%s Failed to add xbmc source filter to the graph", __FUNCTION__);
        return hr;
      }
      CGraphFilters::Get()->Source.osdname = "XBMC File Source";
      CGraphFilters::Get()->Source.pData = (void *) pXBMCStream;
      CGraphFilters::Get()->Source.isinternal = true;
      CLog::Log(LOGNOTICE, "%s Successfully added xbmc source filter to the graph", __FUNCTION__);
    }
    return hr;
  }*/
  /* DVD NAVIGATOR */
  if (pFileItem.IsDVDFile())
  {
    hr = InsertFilter(filterName, CGraphFilters::Get()->Splitter);
    if (SUCCEEDED(hr))
    {
      if(!((CGraphFilters::Get()->DVD.dvdControl = CGraphFilters::Get()->Splitter.pBF) && (CGraphFilters::Get()->DVD.dvdInfo = CGraphFilters::Get()->Splitter.pBF)))
      {
        CGraphFilters::Get()->DVD.Clear();
        return E_NOINTERFACE;
      }
    }

    CGraphFilters::Get()->SetIsDVD(true);
    CStdString dirA;
    CStdStringW dirW;
    CUtil::GetDirectory(pFileItem.m_strPath, dirA);
    g_charsetConverter.utf8ToW(dirA, dirW);

    hr = CGraphFilters::Get()->DVD.dvdControl->SetDVDDirectory(dirW.c_str());
    if (FAILED(hr))
      CLog::Log(LOGERROR, "%s Failed loading dvd directory.", __FUNCTION__);

    CGraphFilters::Get()->DVD.dvdControl->SetOption(DVD_ResetOnStop, FALSE);
    CGraphFilters::Get()->DVD.dvdControl->SetOption(DVD_HMSF_TimeCodeEvents, TRUE);
    
    return hr;
  }

  if (filterName.Equals("internal_urlsource"))
  {
    //TODO
    //add IAMOpenProgress for requesting the status of stream without this interface the player failed if connection is too slow
    Com::SmartQIPtr<IFileSourceFilter> pSourceUrl;
    Com::SmartPtr<IUnknown> pUnk = NULL;

    pUnk.CoCreateInstance(CLSID_URLReader, NULL);
    hr = pUnk->QueryInterface(IID_IBaseFilter, (void**)&CGraphFilters::Get()->Source.pBF);
    if (SUCCEEDED(hr))
    {
      hr = g_dsGraph->pFilterGraph->AddFilter(CGraphFilters::Get()->Source.pBF, L"URLReader");
      CGraphFilters::Get()->Source.osdname = "URLReader";
      CStdStringW strUrlW; g_charsetConverter.utf8ToW(pFileItem.m_strPath, strUrlW);

      if (pSourceUrl = pUnk)
        hr = pSourceUrl->Load(strUrlW.c_str(), NULL);

      if(FAILED(hr))
      {
        g_dsGraph->pFilterGraph->RemoveFilter(CGraphFilters::Get()->Source.pBF);
        CLog::Log(LOGERROR, "%s Failed to add url source filter to the graph.", __FUNCTION__);
        CGraphFilters::Get()->Source.pBF = NULL;
        return E_FAIL;
      }

      ParseStreamingType(pFileItem, CGraphFilters::Get()->Source.pBF);

      return S_OK;
    }
  }

  /* Two cases:
  1/ The source filter is also a splitter. We insert it to the graph as a splitter and load the file
  2/ The source filter is only a source filter. Add it to the graph as a source filter
  */

  // TODO: Loading an url must be done!

  // If the source filter has more than one ouput pin, it's a splitter too.
  // Only problem, the file must be loaded in the source filter to see the
  // number of output pin
  CURL url(pFileItem.m_strPath);
  
  CStdString pWinFilePath = url.Get();
  if ( (pWinFilePath.Left(6)).Equals("smb://", false) )
    pWinFilePath.Replace("smb://", "\\\\");
  
  if (! pFileItem.IsInternetStream())
    pWinFilePath.Replace("/", "\\");

  CStdStringW strFileW;
  g_charsetConverter.utf8ToW(pWinFilePath, strFileW);

  SFilterInfos infos;
  try // Load() may crash on bad designed codec. Prevent XBMC to hang
  {
    if (FAILED(hr = InsertFilter(filterName, infos)))
    {
      if (infos.isinternal)
        delete infos.pData;
      return E_FAIL;
    }

    Com::SmartQIPtr<IFileSourceFilter> pFS = infos.pBF;
    
    if (SUCCEEDED(pFS->Load(strFileW.c_str(), NULL)))
      CLog::Log(LOGNOTICE, "%s Successfully loaded file in the splitter/source", __FUNCTION__);
    else
    {
      CLog::Log(LOGERROR, "%s Failed to load file in the splitter/source", __FUNCTION__);

      if (infos.isinternal)
        delete infos.pData;

      return E_FAIL;
    }
  } catch (...) {
    CLog::Log(LOGERROR, "%s An exception has been thrown by the codec...", __FUNCTION__);

      if (infos.isinternal)
        delete infos.pData;

      return E_FAIL;
  }

  bool isSplitterToo = IsSplitter(infos.pBF);
  if (isSplitterToo)
  {
    CLog::Log(LOGDEBUG, "%s The source filter is also a splitter.", __FUNCTION__);
    CGraphFilters::Get()->Splitter = infos;
  } else {
    CGraphFilters::Get()->Source = infos;
  }

  ParseStreamingType(pFileItem, infos.pBF);

  return hr;
}

void CFGLoader::ParseStreamingType(CFileItem& pFileItem, IBaseFilter* pBF)
{
  // Detect the type of the streaming, in order to choose the right splitter
  // Often, streaming url does not have an extension ...
  GUID guid;
  BeginEnumPins(pBF, pEP, pPin)
  {
    PIN_DIRECTION pPinDir;
    pPin->QueryDirection(& pPinDir);
    if (pPinDir != PINDIR_OUTPUT)
      continue;

    BeginEnumMediaTypes(pPin, pEMT, pMT)
    {
      if (pMT->majortype != MEDIATYPE_Stream)
        continue;

      CStdString str;
      g_charsetConverter.wToUTF8(GetMediaTypeName(pMT->subtype), str);
      CLog::Log(LOGDEBUG, __FUNCTION__" Streaming output pin media tpye: %s", str.c_str());
      guid = pMT->subtype;
    }
    EndEnumMediaTypes(pMT);
  }
  EndEnumPins;

  if (guid == MEDIASUBTYPE_MOV)
    pFileItem.m_strPath = "http://dummysite.com/dummyfile.mov";
}

HRESULT CFGLoader::InsertSplitter(const CFileItem& pFileItem, const CStdString& filterName)
{
  HRESULT hr = InsertFilter(filterName, CGraphFilters::Get()->Splitter);

  if (SUCCEEDED(hr))
  {
    if (SUCCEEDED(hr = ConnectFilters(g_dsGraph->pFilterGraph, CGraphFilters::Get()->Source.pBF, CGraphFilters::Get()->Splitter.pBF)))
      CLog::Log(LOGNOTICE, "%s Successfully connected the source to the splitter", __FUNCTION__);
    else
      CLog::Log(LOGERROR, "%s Failed to connect the source to the splitter", __FUNCTION__);
  }
  
  return hr;
}

HRESULT CFGLoader::InsertAudioRenderer(const CStdString& filterName)
{
  HRESULT hr = S_OK;
  CFGFilterRegistry* pFGF;
  CStdString currentGuid, currentName;

  if (! filterName.empty())
  {
    if (SUCCEEDED(InsertFilter(filterName, CGraphFilters::Get()->AudioRenderer)))
      return S_OK;
    else
      CLog::Log(LOGERROR, "%s Failed to insert custom audio renderer, fallback to default one", __FUNCTION__);
  }

  CAudioEnumerator p_dsound;
  std::vector<DSFilterInfo> deviceList;
  p_dsound.GetAudioRenderers(deviceList);

  //see if there a config first 
  for (std::vector<DSFilterInfo>::const_iterator iter = deviceList.begin();
    iter != deviceList.end(); ++iter)
  {
    DSFilterInfo dev = *iter;
    if (g_guiSettings.GetString("dsplayer.audiorenderer").Equals(dev.lpstrName))
    {
      currentGuid = dev.lpstrGuid;
      currentName = dev.lpstrName;
      break;
    }
  }
  if (currentName.IsEmpty())
  {
    currentGuid = StringFromGUID(CLSID_DSoundRender);
    currentName.Format("Default DirectSound Device");
  }

  pFGF = new CFGFilterRegistry(GUIDFromString(currentGuid));
  hr = pFGF->Create(&CGraphFilters::Get()->AudioRenderer.pBF);
  delete pFGF;

  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "%s Failed to create the audio renderer (%X)", __FUNCTION__, hr);
    return hr;
  }

  CGraphFilters::Get()->AudioRenderer.osdname = currentName;
  CGraphFilters::Get()->AudioRenderer.guid = GUIDFromString(currentGuid);

  hr = g_dsGraph->pFilterGraph->AddFilter(CGraphFilters::Get()->AudioRenderer.pBF, AnsiToUTF16(currentName));

  if (SUCCEEDED(hr))
    CLog::Log(LOGNOTICE, "%s Successfully added \"%s\" to the graph", __FUNCTION__, CGraphFilters::Get()->AudioRenderer.osdname.c_str());
  else
    CLog::Log(LOGNOTICE, "%s Failed to add \"%s\" to the graph (result: %X)", __FUNCTION__, CGraphFilters::Get()->AudioRenderer.osdname.c_str(), hr);

  return hr;
}

HRESULT CFGLoader::InsertVideoRenderer()
{
  HRESULT hr = S_OK;
  
  // TODO: Use a listbox instead of a checkbox on the GUI. Simpler and easier
  if (g_sysinfo.IsVistaOrHigher())
  {
    if (g_guiSettings.GetBool("dsplayer.forcenondefaultrenderer"))
      CGraphFilters::Get()->SetCurrentRenderer(DIRECTSHOW_RENDERER_VMR9);
    else 
      CGraphFilters::Get()->SetCurrentRenderer(DIRECTSHOW_RENDERER_EVR);
  }
  else
  {
    if (g_guiSettings.GetBool("dsplayer.forcenondefaultrenderer"))
      CGraphFilters::Get()->SetCurrentRenderer(DIRECTSHOW_RENDERER_EVR);
    else 
      CGraphFilters::Get()->SetCurrentRenderer(DIRECTSHOW_RENDERER_VMR9);
  }    

  // Renderers
  if (CGraphFilters::Get()->GetCurrentRenderer() == DIRECTSHOW_RENDERER_EVR)
  {
    m_pFGF = new CFGFilterVideoRenderer(CLSID_EVRAllocatorPresenter, L"Xbmc EVR");
    CGraphFilters::Get()->VideoRenderer.osdname = _T("Enhanced Video Renderer");
  }
  else
  {
    m_pFGF = new CFGFilterVideoRenderer(CLSID_VMR9AllocatorPresenter, L"Xbmc VMR9");
    CGraphFilters::Get()->VideoRenderer.osdname = _T("VMR9 (Renderless)");
  }

  
  hr = m_pFGF->Create( &CGraphFilters::Get()->VideoRenderer.pBF);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "%s Failed to create allocator presenter (hr = %X)", __FUNCTION__, hr);
    return hr;
  }
  hr = g_dsGraph->pFilterGraph->AddFilter(CGraphFilters::Get()->VideoRenderer.pBF, m_pFGF->GetName());

  /* Query IQualProp from the renderer */
  CGraphFilters::Get()->VideoRenderer.pBF->QueryInterface(IID_IQualProp, (void **) &CGraphFilters::Get()->VideoRenderer.pQualProp);

  if (SUCCEEDED(hr))
  {
    CLog::Log(LOGDEBUG, "%s Allocator presenter successfully added to the graph (Renderer: %s)",  __FUNCTION__, CGraphFilters::Get()->VideoRenderer.osdname.c_str());
  } else {
    CLog::Log(LOGERROR, "%s Failed to add allocator presenter to the graph (hr = %X)", __FUNCTION__, hr);
  }

  return hr; 
}

HRESULT CFGLoader::LoadFilterRules(const CFileItem& _pFileItem)
{
  CFileItem pFileItem = _pFileItem;

  if (!pFileItem.IsInternetStream() && !CFilterCoreFactory::SomethingMatch(pFileItem))
  {

    CLog::Log(LOGERROR, "%s Extension \"%s\" not found. Please check mediasconfig.xml",
      __FUNCTION__, CURL(pFileItem.m_strPath).GetFileType().c_str());
    CGUIDialogOK *dialog = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (dialog)
    {
      dialog->SetHeading("Extension not found");
      dialog->SetLine(0, "Impossible to play the media file : the media");
      dialog->SetLine(1, "extension \"" + CURL(pFileItem.m_strPath).GetFileType() + "\" isn't declared in mediasconfig.xml.");
      dialog->SetLine(2, "");
      CDSPlayer::errorWindow = dialog;
    }
    return E_FAIL;
  }

  CStdString filter = "";

  CFilterCoreFactory::GetAudioRendererFilter(pFileItem, filter);
  InsertAudioRenderer(filter); // First added, last connected
  InsertVideoRenderer();

  // We *need* those informations for filter loading. If the user wants it, be sure it's loaded
  // before using it.
  bool hasStreamDetails = false;
  if (g_guiSettings.GetBool("myvideos.extractflags") &&
    pFileItem.HasVideoInfoTag() && !pFileItem.GetVideoInfoTag()->HasStreamDetails())
  {
    CLog::Log(LOGDEBUG,"%s - trying to extract filestream details from video file %s", __FUNCTION__, pFileItem.m_strPath.c_str());
    hasStreamDetails = CDVDFileInfo::GetFileStreamDetails(&pFileItem);
  } else
    hasStreamDetails = pFileItem.HasVideoInfoTag() && pFileItem.GetVideoInfoTag()->HasStreamDetails();

  if (FAILED(CFilterCoreFactory::GetSourceFilter(pFileItem, filter)))
  {
    CLog::Log(LOGERROR, __FUNCTION__" Failed to get the source filter");
    return E_FAIL;
  }
  
  if (FAILED(InsertSourceFilter(pFileItem, filter)))
  {
    CLog::Log(LOGERROR, __FUNCTION__" Failed to insert the source filter");
    return E_FAIL;
  }

  if (! CGraphFilters::Get()->Splitter.pBF)
  {
    if ( FAILED(CFilterCoreFactory::GetSplitterFilter(pFileItem, filter)) )
      return E_FAIL;
    
    if ( FAILED(InsertSplitter(pFileItem, filter)))
    {
      return E_FAIL;
    }
  }

  /* Ok, the splitter is added to the graph. We need to detect the video stream codec in order to choose
  if we use or not the dxva filter. Also dump the output pins to the debug log */
  Com::SmartPtr<IBaseFilter> pBF = CGraphFilters::Get()->Splitter.pBF;
  bool fourccfinded = false;
  CStdString pinName = "", guid = "";
  int nIn = 0, nOut = 0, nInC = 0, nOutC = 0;
  PIN_DIRECTION dir;

  CountPins(pBF, nIn, nOut, nInC, nOutC);
  CLog::Log(LOGDEBUG, "%s The splitter has %d output pins", __FUNCTION__, nOut);

  BeginEnumPins(pBF, pEP, pPin)
  {
    if (SUCCEEDED(pPin->QueryDirection(&dir)) && ( dir == PINDIR_OUTPUT ))
    {
      g_charsetConverter.wToUTF8(GetPinName(pPin), pinName);
      CLog::Log(LOGDEBUG, "%s Output pin found : %s", __FUNCTION__, pinName.c_str());
    }

    BeginEnumMediaTypes(pPin, pEMT, pMT)
    {
      if (dir == PINDIR_OUTPUT)
      {
        g_charsetConverter.wToUTF8(StringFromGUID(pMT->majortype), guid);
        CLog::Log(LOGDEBUG, "%s \tOutput pin major type : %s %s", __FUNCTION__, GuidNames[pMT->majortype], guid.c_str());

        g_charsetConverter.wToUTF8(StringFromGUID(pMT->subtype), guid);
        CLog::Log(LOGDEBUG, "%s \tOutput pin sub type : %s %s", __FUNCTION__, GuidNames[pMT->subtype], guid.c_str());

        g_charsetConverter.wToUTF8(StringFromGUID(pMT->formattype), guid);
        CLog::Log(LOGDEBUG, "%s \tOutput pin format type : %s %s", __FUNCTION__, GuidNames[pMT->formattype], guid.c_str());
      }

      if (! fourccfinded && pMT->majortype == MEDIATYPE_Video)
      {
        CDSStreamDetailVideo s;
        CStreamsManager::MediaTypeToStreamDetail(pMT, s);

        CLog::Log(LOGINFO, "%s \tVideo stream fourcc : %c%c%c%c", __FUNCTION__, s.m_iFourcc >> 24 & 0xff, s.m_iFourcc >> 16 & 0xff, s.m_iFourcc >> 8 & 0xff, s.m_iFourcc & 0xff);

        fourccfinded = true;
        // CCV1 is a fake fourcc code generated by Haali in order to trick the Microsoft h264
        // decoder included in Windows 7. ffdshow handle that fourcc well, but it may not be
        // the case for others decoders
        if (s.m_iFourcc == 'H264' || s.m_iFourcc == 'AVC1' || s.m_iFourcc == 'WVC1'
          || s.m_iFourcc == 'WMV3' || s.m_iFourcc == 'WMVA' || s.m_iFourcc == 'CCV1')
          CGraphFilters::Get()->SetIsUsingDXVADecoder(true);

        if (s.m_iFourcc == 'CCV1')
        {
          CLog::Log(LOGINFO, "Haali Media Splitter is configured for using a fake fourcc code 'CCV1'. If you've got an error message, be sure your video decoder handle that fourcc well.");
        }
      }
    }
    EndEnumMediaTypes(pMT)
  }
  EndEnumPins

  // Init Streams manager, and load audio / subtitles streams
  CStreamsManager::Get()->InitManager();
  CStreamsManager::Get()->LoadStreams();

  if (! hasStreamDetails) {
    // We will use our own stream detail
    // We need to make a copy of our streams details because
    // Reset() delete the streams
    CLog::Log(LOGWARNING, __FUNCTION__" DVDPlayer failed to fetch streams details. Using DirectShow ones");

    pFileItem.GetVideoInfoTag()->m_streamDetails.AddStream(
      new CDSStreamDetailVideo((const CDSStreamDetailVideo &)(*CStreamsManager::Get()->GetVideoStreamDetail()))
    );
    std::vector<CDSStreamDetailAudio *>& streams = CStreamsManager::Get()->GetAudios();
    for (std::vector<CDSStreamDetailAudio *>::const_iterator it = streams.begin();
      it != streams.end(); ++it)
      pFileItem.GetVideoInfoTag()->m_streamDetails.AddStream(
        new CDSStreamDetailAudio((const CDSStreamDetailAudio &)(**it))
      );
  }

  std::vector<CStdString> extras;
  if (FAILED(CFilterCoreFactory::GetExtraFilters(pFileItem, extras, CGraphFilters::Get()->IsUsingDXVADecoder())))
  {
    //Dont want the loading to fail for an error there
    CLog::Log(LOGERROR, "Failed loading extras filters in filtersconfig.xml");
  }

  // Insert extra first because first added, last connected!
  for (unsigned int i = 0; i < extras.size(); i++)
  {
    SFilterInfos f;
    if (SUCCEEDED(InsertFilter(extras[i], f)))
      CGraphFilters::Get()->Extras.push_back(f);
  }
  extras.clear();

  if ( FAILED(CFilterCoreFactory::GetVideoFilter(pFileItem, filter, CGraphFilters::Get()->IsUsingDXVADecoder())))
    goto clean;

  if ( FAILED(InsertFilter(filter, CGraphFilters::Get()->Video)))
    goto clean;

  if ( FAILED(CFilterCoreFactory::GetAudioFilter(pFileItem, filter, CGraphFilters::Get()->IsUsingDXVADecoder())))
    goto clean;

  if ( FAILED(InsertFilter(filter, CGraphFilters::Get()->Audio)))
    goto clean;

  // Shaders
  {
    std::vector<uint32_t> shaders;
    if (SUCCEEDED(CFilterCoreFactory::GetShaders(pFileItem, shaders, CGraphFilters::Get()->IsUsingDXVADecoder())))
    {
      for (std::vector<uint32_t>::const_iterator it = shaders.begin();
        it != shaders.end(); ++it)
      {
        g_dsSettings.pixelShaderList->EnableShader(*it);
      }
    }
  }

  CLog::Log(LOGDEBUG,"%s All filters added to the graph", __FUNCTION__);
 
  // If we have add our own informations, clear it to prevent xbmc to save it to the database
  if (! hasStreamDetails) {
    pFileItem.GetVideoInfoTag()->m_streamDetails.Reset();
  }
  return S_OK;

clean:
  if (! hasStreamDetails) {
    pFileItem.GetVideoInfoTag()->m_streamDetails.Reset();
  }
  return E_FAIL;
}

HRESULT CFGLoader::LoadConfig()
{
  // Two steps

  // First, filters
  LoadFilterCoreFactorySettings(g_settings.GetUserDataItem("dsplayer/filtersconfig.xml"), FILTERS, true);
  LoadFilterCoreFactorySettings("special://xbmc/system/players/dsplayer/filtersconfig.xml", FILTERS, false);

  // Second, medias rules
  LoadFilterCoreFactorySettings(g_settings.GetUserDataItem("dsplayer/mediasconfig.xml"), MEDIAS, false);
  LoadFilterCoreFactorySettings("special://xbmc/system/players/dsplayer/mediasconfig.xml", MEDIAS, false);

  return S_OK;
}

HRESULT CFGLoader::InsertFilter(const CStdString& filterName, SFilterInfos& f)
{
  HRESULT hr = S_OK;
  f.pBF = NULL;

  CFGFilter *filter = NULL;
  if (! (filter = CFilterCoreFactory::GetFilterFromName(filterName)))
    return E_FAIL;

  if(SUCCEEDED(hr = filter->Create(&f.pBF)))
  {
    g_charsetConverter.wToUTF8(filter->GetName(), f.osdname);
    if (filter->GetType() == CFGFilter::INTERNAL)
    {
      CLog::Log(LOGDEBUG, "%s Using an internal filter", __FUNCTION__);
      f.isinternal = true;
      f.pData = filter;
    }
    if (SUCCEEDED(hr = g_dsGraph->pFilterGraph->AddFilter(f.pBF, filter->GetName().c_str())))
      CLog::Log(LOGNOTICE, "%s Successfully added \"%s\" to the graph", __FUNCTION__, f.osdname.c_str());
    else
      CLog::Log(LOGERROR, "%s Failed to add \"%s\" to the graph", __FUNCTION__, f.osdname.c_str());

    f.guid = filter->GetCLSID();
  } 
  else
  {
    CLog::Log(LOGERROR,"%s Failed to create filter \"%s\"", __FUNCTION__, filterName.c_str());
  }

  return hr;
}

bool CFGLoader::LoadFilterCoreFactorySettings( const CStdString& fileStr, ESettingsType type, bool clear )
{
  if (clear)
  {
    CFilterCoreFactory::Destroy();
  }

  CLog::Log(LOGNOTICE, "Loading filter core factory settings from %s (%s configuration).", fileStr.c_str(), (type == MEDIAS) ? "medias" : "filters");
  if (!XFILE::CFile::Exists(fileStr))
  { // tell the user it doesn't exist
    CLog::Log(LOGNOTICE, "%s does not exist. Skipping.", fileStr.c_str());
    return false;
  }

  TiXmlDocument filterCoreFactoryXML;
  if (!filterCoreFactoryXML.LoadFile(fileStr))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d (%s)", fileStr.c_str(), filterCoreFactoryXML.ErrorRow(), filterCoreFactoryXML.ErrorDesc());
    return false;
  }

  return ( (type == MEDIAS) ? SUCCEEDED(CFilterCoreFactory::LoadMediasConfiguration(filterCoreFactoryXML.RootElement()))
                            : SUCCEEDED(CFilterCoreFactory::LoadFiltersConfiguration(filterCoreFactoryXML.RootElement())) );
}

#endif