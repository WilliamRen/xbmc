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

#include "DSPlayer.h"
#include "dshowutil/dshowutil.h" // unload loaded filters
#include "DShowUtil/smartptr.h"
#include "Filters/RendererSettings.h"

#include "winsystemwin32.h" //Important needed to get the right hwnd
#include "utils/GUIInfoManager.h"
#include "utils/SystemInfo.h"
#include "MouseStat.h"
#include "GUISettings.h"
#include "Settings.h"
#include "FileItem.h"
#include "utils/log.h"
#include "URL.h"

#include "GUIWindowManager.h"
#include "GUIDialogBusy.h"
#include "WindowingFactory.h"
#include "GUIDialogOK.h"
#include "PixelShaderList.h"

using namespace std;

DSPLAYER_STATE CDSPlayer::PlayerState = DSPLAYER_CLOSED;
CFileItem CDSPlayer::currentFileItem;
CGUIDialogBoxBase *CDSPlayer::errorWindow = NULL;

CDSPlayer::CDSPlayer(IPlayerCallback& callback)
    : IPlayer(callback), CThread(), m_hReadyEvent(true), m_bSpeedChanged(false),
    m_callSetFileFromThread(true), m_bDoNotUseDSFF(false)
{
  // Change DVD Clock, time base
  CDVDClock::SetTimeBase((int64_t) DS_TIME_BASE);
  m_pClock.GetClock(); // Reset the clock

  g_dsGraph = new CDSGraph(&m_pClock, callback);
}

CDSPlayer::~CDSPlayer()
{
  if (PlayerState != DSPLAYER_CLOSED)
    CloseFile();

  StopThread();

  delete g_dsGraph;
  g_dsGraph = NULL;

  // Save Shader settings
  g_dsSettings.pixelShaderList->SaveXML();

  DShowUtil::UnloadExternalObjects();
  CLog::Log(LOGDEBUG, "%s External objects unloaded", __FUNCTION__);

  CLog::Log(LOGNOTICE, "%s DSPlayer is now closed", __FUNCTION__);

  // Restore DVD Player time base clock
  CDVDClock::SetTimeBase(DVD_TIME_BASE);
}

bool CDSPlayer::OpenFile(const CFileItem& file,const CPlayerOptions &options)
{
  if(PlayerState != DSPLAYER_CLOSED)
    CloseFile();

  PlayerState = DSPLAYER_LOADING;

  currentFileItem = file;
  m_Filename = file.GetAsUrl();
  m_PlayerOptions = options;
  m_currentRate = 1;
  
  m_hReadyEvent.Reset();

  if ( g_Windowing.IsFullScreen() && !g_guiSettings.GetBool("videoscreen.fakefullscreen") &&  (
    (g_sysinfo.IsVistaOrHigher() && g_guiSettings.GetBool("dsplayer.forcenondefaultrenderer")) ||
    (!g_sysinfo.IsVistaOrHigher() && !g_guiSettings.GetBool("dsplayer.forcenondefaultrenderer")) ) ) // The test was broken, it doesn't work on Win7 either
  {
    // Using VMR in true fullscreen. Calling SetFile() in Process makes XBMC freeze
    // We're no longer waiting indefinitly if a crash occured when trying to load the file
    m_callSetFileFromThread = false;
    START_PERFORMANCE_COUNTER
      if (FAILED(g_dsGraph->SetFile(currentFileItem, m_PlayerOptions)))
        PlayerState = DSPLAYER_ERROR;
    END_PERFORMANCE_COUNTER
  }
  Create();

  /* Show busy dialog while SetFile() not returned */
  if(!m_hReadyEvent.WaitMSec(100))
  {
    CGUIDialogBusy* dialog = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
    dialog->Show();
    while(!m_hReadyEvent.WaitMSec(1))
      g_windowManager.Process(true);
    dialog->Close();
  }

  // Starts playback
  if (PlayerState != DSPLAYER_ERROR)
  {
    g_dsGraph->Play();
    if (CGraphFilters::Get()->IsDVD())
      CStreamsManager::Get()->LoadDVDStreams();
  }

  return (PlayerState != DSPLAYER_ERROR);
}
bool CDSPlayer::CloseFile()
{
  if (PlayerState == DSPLAYER_CLOSED)
    return true;

  if (PlayerState == DSPLAYER_ERROR)
  {
    // Something to show?
    if (errorWindow)
    {
      errorWindow->DoModal();
      errorWindow = NULL;
    } else {
      CGUIDialogOK *dialog = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
      if (dialog)
      {
        dialog->SetHeading("Error");
        dialog->SetLine(0, "An error occured when trying to render the file.");
        dialog->SetLine(1, "Please look at the debug log for more informations.");
        dialog->DoModal();
      }
    }
  }

  PlayerState = DSPLAYER_CLOSING;

  g_dsGraph->CloseFile();
  
  CLog::Log(LOGDEBUG, "%s File closed", __FUNCTION__);
  
  PlayerState = DSPLAYER_CLOSED;

  return true;
}

bool CDSPlayer::IsPlaying() const
{
  return !m_bStop;
}

bool CDSPlayer::HasVideo() const
{
  return true;
}
bool CDSPlayer::HasAudio() const
{
  return true;
}

void CDSPlayer::GetAudioInfo(CStdString& strAudioInfo)
{
  CSingleLock lock(m_StateSection);
  strAudioInfo = g_dsGraph->GetAudioInfo();
}

void CDSPlayer::GetVideoInfo(CStdString& strVideoInfo)
{
  CSingleLock lock(m_StateSection);
  strVideoInfo = g_dsGraph->GetVideoInfo();
}

void CDSPlayer::GetGeneralInfo(CStdString& strGeneralInfo)
{
  CSingleLock lock(m_StateSection);
  strGeneralInfo = g_dsGraph->GetGeneralInfo();
}

//CThread
void CDSPlayer::OnStartup()
{
  CThread::SetName("CDSPlayer");
}

void CDSPlayer::OnExit()
{
  if (PlayerState == DSPLAYER_LOADING)
    PlayerState = DSPLAYER_ERROR;

  // In case of, set the ready event
  // Prevent a dead loop
  m_hReadyEvent.Set();

  if (PlayerState == DSPLAYER_CLOSING)
    m_callback.OnPlayBackStopped();
  else
    m_callback.OnPlayBackEnded();

  m_bStop = true;
}

void CDSPlayer::HandleStart()
{
  if (m_PlayerOptions.starttime > 0)
    SendMessage(g_hWnd, WM_COMMAND, ID_SEEK_TO, ((LPARAM) m_PlayerOptions.starttime * 1000));
}

void CDSPlayer::Process()
{

#define CHECK_PLAYER_STATE if (PlayerState == DSPLAYER_CLOSING || PlayerState == DSPLAYER_CLOSED) \
  break;

  if (m_callSetFileFromThread)
  {
    START_PERFORMANCE_COUNTER
    if (FAILED(g_dsGraph->SetFile(currentFileItem, m_PlayerOptions)))
      PlayerState = DSPLAYER_ERROR;
    END_PERFORMANCE_COUNTER
  }

  m_hReadyEvent.Set(); // Start playback

  if (PlayerState == DSPLAYER_ERROR)
    return;

  HandleStart();
  
  HRESULT hr = S_OK;

  while (PlayerState != DSPLAYER_CLOSING && PlayerState != DSPLAYER_CLOSED)
  {
    CHECK_PLAYER_STATE

    if (m_bSpeedChanged)
    {
      m_pClock.SetSpeed(m_currentRate * 1000);
      m_clockStart = m_pClock.GetClock();
      m_bSpeedChanged = false;

      if (m_currentRate == 1)
        g_dsGraph->Play();
      else if (((m_currentRate < 1) || (m_bDoNotUseDSFF && (m_currentRate > 1)))
        && (!g_dsGraph->IsPaused()))
        g_dsGraph->Pause(); // Pause only if not using SetRate

      // Fast forward. DirectShow does all the hard work for us.
      if (m_currentRate >= 1)
      {
        HRESULT hr = S_OK;
        Com::SmartQIPtr<IMediaSeeking> pSeeking = g_dsGraph->pFilterGraph;
        if (pSeeking)
        {
          if (m_currentRate == 1)
            pSeeking->SetRate(m_currentRate);
          else if (!m_bDoNotUseDSFF)
          {
            HRESULT hr = pSeeking->SetRate(m_currentRate);
            if (FAILED(hr))
            {
              m_bDoNotUseDSFF = true;
              pSeeking->SetRate(1);
              m_bSpeedChanged = true;
            }
          }
        }
      }
    }

    CHECK_PLAYER_STATE

    g_dsGraph->HandleGraphEvent();

    CHECK_PLAYER_STATE

    g_dsGraph->UpdateTime();

    CHECK_PLAYER_STATE

    // Handle Rewind
    if ((m_currentRate < 1) || ( m_bDoNotUseDSFF && (m_currentRate > 1)))
    {
      double clock = m_pClock.GetClock() - m_clockStart; // Time elapsed since the rate change
      //CLog::Log(LOGDEBUG, "Seeking time : %f", DS_TIME_TO_MSEC(clock));

      // New position
      uint64_t newPos = g_dsGraph->GetTime() + (uint64_t) clock;
      //CLog::Log(LOGDEBUG, "New position : %f", DS_TIME_TO_SEC(newPos));

      // Check boundaries
      if (newPos <= 0)
      {
        newPos = 0;
        m_currentRate = 1;
        m_callback.OnPlayBackSpeedChanged(1);
        m_bSpeedChanged = true;
      } else if (newPos >= g_dsGraph->GetTotalTime())
      {
        CloseFile();
        break;
      }

      g_dsGraph->Seek(newPos);

      m_clockStart = m_pClock.GetClock();
    }

    CHECK_PLAYER_STATE

    if (m_currentRate == 1)
    {
      CChaptersManager::Get()->UpdateChapters();
      if (CGraphFilters::Get()->IsDVD())
        CStreamsManager::Get()->UpdateDVDStream();
    }
    //Handle fastforward stuff
   
    Sleep(250);
    CHECK_PLAYER_STATE

    if (g_dsGraph->FileReachedEnd())
    { 
      CLog::Log(LOGDEBUG,"%s Graph detected end of video file",__FUNCTION__);
      CloseFile();
      break;
    }
  }  
}

void CDSPlayer::Stop()
{
  SendMessage(g_hWnd,WM_COMMAND, ID_STOP_DSPLAYER,0);
}

void CDSPlayer::Pause()
{
  m_bSpeedChanged = true;
  if ( PlayerState == DSPLAYER_PAUSED )
  {
    m_currentRate = 1;
    m_callback.OnPlayBackResumed();    
  } 
  else
  {
    m_currentRate = 0;
    m_callback.OnPlayBackPaused();
  }

  SendMessage(g_hWnd, WM_COMMAND, ID_PLAY_PAUSE, 0);
}
void CDSPlayer::ToFFRW(int iSpeed)
{
  if (iSpeed != 1)
    g_infoManager.SetDisplayAfterSeek();

  m_currentRate = iSpeed;
  m_bSpeedChanged = true;
}

void CDSPlayer::Seek(bool bPlus, bool bLargeStep)
{
  if (bPlus)
  {
    if (!bLargeStep)
      SendMessage(g_hWnd, WM_COMMAND, ID_SEEK_FORWARDSMALL, 0);
    else
      SendMessage(g_hWnd, WM_COMMAND, ID_SEEK_FORWARDLARGE, 0);
  }
  else
  {
    if (!bLargeStep)
      SendMessage(g_hWnd, WM_COMMAND, ID_SEEK_BACKWARDSMALL, 0);
    else
      SendMessage(g_hWnd, WM_COMMAND, ID_SEEK_BACKWARDLARGE, 0);
  }
}

void CDSPlayer::SeekPercentage(float iPercent)
{
  SendMessage(g_hWnd,WM_COMMAND, ID_SEEK_PERCENT,(LPARAM)iPercent);
}

bool CDSPlayer::OnAction(const CAction &action)
{
  if ( g_dsGraph->IsDvd() )
  {
    if ( action.GetID() == ACTION_SHOW_VIDEOMENU )
    {
      SendMessage(g_hWnd, WM_COMMAND, ID_DVD_MENU_ROOT,0);
      return true;
    }
    if ( g_dsGraph->IsInMenu() )
    {
      switch (action.GetID())
      {
        case ACTION_PREVIOUS_MENU:
          SendMessage(g_hWnd, WM_COMMAND, ID_DVD_MENU_BACK,0);
        break;
        case ACTION_MOVE_LEFT:
          SendMessage(g_hWnd, WM_COMMAND, ID_DVD_NAV_LEFT,0);
        break;
        case ACTION_MOVE_RIGHT:
          SendMessage(g_hWnd, WM_COMMAND, ID_DVD_NAV_RIGHT,0);
        break;
        case ACTION_MOVE_UP:
          SendMessage(g_hWnd, WM_COMMAND, ID_DVD_NAV_UP,0);
        break;
        case ACTION_MOVE_DOWN:
          SendMessage(g_hWnd, WM_COMMAND, ID_DVD_NAV_DOWN,0);
        break;
        /*case ACTION_MOUSE_MOVE:
        case ACTION_MOUSE_LEFT_CLICK:
        {
          CRect rs, rd;
          GetVideoRect(rs, rd);
          CPoint pt(action.GetAmount(), action.GetAmount(1));
          if (!rd.PtInRect(pt))
            return false;
          pt -= CPoint(rd.x1, rd.y1);
          pt.x *= rs.Width() / rd.Width();
          pt.y *= rs.Height() / rd.Height();
          pt += CPoint(rs.x1, rs.y1);
          if (action.GetID() == ACTION_MOUSE_LEFT_CLICK)
            SendMessage(g_hWnd, WM_COMMAND, ID_DVD_MOUSE_CLICK,MAKELPARAM(pt.x,pt.y));
          else
            SendMessage(g_hWnd, WM_COMMAND, ID_DVD_MOUSE_MOVE,MAKELPARAM(pt.x,pt.y));
          return true;
        }
        break;*/
      case ACTION_SELECT_ITEM:
        {
          // show button pushed overlay
          SendMessage(g_hWnd, WM_COMMAND, ID_DVD_MENU_SELECT,0);
        }
        break;
      case REMOTE_0:
      case REMOTE_1:
      case REMOTE_2:
      case REMOTE_3:
      case REMOTE_4:
      case REMOTE_5:
      case REMOTE_6:
      case REMOTE_7:
      case REMOTE_8:
      case REMOTE_9:
      {
        // Offset from key codes back to button number
        // int button = action.actionId - REMOTE_0;
        //CLog::Log(LOGDEBUG, " - button pressed %d", button);
        //pStream->SelectButton(button);
      }
      break;
      default:
        return false;
        break;
      }
      return true; // message is handled
    }
  }

  switch(action.GetID())
  {
    case ACTION_NEXT_ITEM:
    case ACTION_PAGE_UP:
      if(GetChapterCount() > 0)
      {
        SeekChapter( GetChapter() + 1 );
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      else
        break;
    case ACTION_PREV_ITEM:
    case ACTION_PAGE_DOWN:
      if(GetChapterCount() > 0)
      {
        SeekChapter( GetChapter() - 1 );
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      else
        break;
  }
  
  // return false to inform the caller we didn't handle the message
  return false;
}

// Time is in millisecond
void CDSPlayer::SeekTime(__int64 iTime)
{
  g_dsGraph->SeekInMilliSec(iTime);
  m_callback.OnPlayBackSeek((int)iTime);
}

#endif