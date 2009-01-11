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

#include "stdafx.h"
#include "WIN32Util.h"
#include "GUISettings.h"
#include "../Util.h"
#include "FileSystem/cdioSupport.h"
#include "PowrProf.h"
#include "WindowHelper.h"
#include "Application.h"
#include <shlobj.h>

extern HWND g_hWnd;

using namespace std;
using namespace MEDIA_DETECT;

DWORD CWIN32Util::dwDriveMask = 0;



CWIN32Util::CWIN32Util(void)
{
}

CWIN32Util::~CWIN32Util(void)
{
}


const CStdString CWIN32Util::GetNextFreeDriveLetter()
{
  for(int iDrive='a';iDrive<='z';iDrive++)
  {
    CStdString strDrive;
    strDrive.Format("%c:",iDrive);
    int iType = GetDriveType(strDrive);
    if(iType == DRIVE_NO_ROOT_DIR && iDrive != 'a' && iDrive != 'b' && iDrive != 'q' && iDrive != 'p' && iDrive != 't' && iDrive != 'z')
      return strDrive;
  }
  return StringUtils::EmptyString;
}

CStdString CWIN32Util::MountShare(const CStdString &smbPath, const CStdString &strUser, const CStdString &strPass, DWORD *dwError)
{
  NETRESOURCE nr;
  memset(&nr,0,sizeof(nr));
  CStdString strRemote = smbPath;
  CStdString strDrive = CWIN32Util::GetNextFreeDriveLetter();

  if(strDrive == StringUtils::EmptyString)
    return StringUtils::EmptyString;

  strRemote.Replace('/', '\\');

  nr.lpRemoteName = (LPTSTR)(LPCTSTR)strRemote.c_str();
  nr.lpLocalName  = (LPTSTR)(LPCTSTR)strDrive.c_str();
  nr.dwType       = RESOURCETYPE_DISK;

  DWORD dwRes = WNetAddConnection2(&nr,(LPCTSTR)strPass.c_str(), (LPCTSTR)strUser.c_str(), NULL);

  if(dwError != NULL)
    *dwError = dwRes;

  if(dwRes != NO_ERROR)
  {
    CLog::Log(LOGERROR, "Can't mount %s to %s. Error code %d",strRemote.c_str(), strDrive.c_str(),dwRes);
    return StringUtils::EmptyString;
  }

  return strDrive;
}

DWORD CWIN32Util::UmountShare(const CStdString &strPath)
{
  return WNetCancelConnection2((LPCTSTR)strPath.c_str(),NULL,true);
}

CStdString CWIN32Util::MountShare(const CStdString &strPath, DWORD *dwError)
{
  CStdString strURL = strPath;
  CURL url(strURL);
  url.GetURL(strURL);
  CStdString strPassword = url.GetPassWord();
  CStdString strUserName = url.GetUserName();
  CStdString strPathToShare = "\\\\"+url.GetHostName() + "\\" + url.GetShareName();
  if(!url.GetUserName().IsEmpty())
    return CWIN32Util::MountShare(strPathToShare, strUserName, strPassword, dwError);
  else
    return CWIN32Util::MountShare(strPathToShare, "", "", dwError);
}

CStdString CWIN32Util::URLEncode(const CURL &url)
{
  /* due to smb wanting encoded urls we have to build it manually */

  CStdString flat = "smb://";

  if(url.GetDomain().length() > 0)
  {
    flat += url.GetDomain();
    flat += ";";
  }

  /* samba messes up of password is set but no username is set. don't know why yet */
  /* probably the url parser that goes crazy */
  if(url.GetUserName().length() > 0 /* || url.GetPassWord().length() > 0 */)
  {
    flat += url.GetUserName();
    flat += ":";
    flat += url.GetPassWord();
    flat += "@";
  }
  else if( !url.GetHostName().IsEmpty() && !g_guiSettings.GetString("smb.username").IsEmpty() )
  {
    /* okey this is abit uggly to do this here, as we don't really only url encode */
    /* but it's the simplest place to do so */
    flat += g_guiSettings.GetString("smb.username");
    flat += ":";
    flat += g_guiSettings.GetString("smb.password");
    flat += "@";
  }

  flat += url.GetHostName();

  /* okey sadly since a slash is an invalid name we have to tokenize */
  std::vector<CStdString> parts;
  std::vector<CStdString>::iterator it;
  CUtil::Tokenize(url.GetFileName(), parts, "/");
  for( it = parts.begin(); it != parts.end(); it++ )
  {
    flat += "/";
    flat += (*it);
  }

  /* okey options should go here, thou current samba doesn't support any */

  return flat;
}

int CWIN32Util::GetDriveStatus(const CStdString &strPath)
{
  HANDLE hDevice;               // handle to the drive to be examined
  int iResult;                  // results flag
  DWORD junk;                   // discard results
  ULONG ulChanges=0;

  hDevice = CreateFile(strPath.c_str(),  // drive
                    0,                // no access to the drive
                    FILE_SHARE_READ,  // share mode
                    NULL,             // default security attributes
                    OPEN_EXISTING,    // disposition
                    FILE_ATTRIBUTE_READONLY,                // file attributes
                    NULL);            // do not copy file attributes

  if (hDevice == INVALID_HANDLE_VALUE) // cannot open the drive
  {
    return -1;
  }
  iResult = DeviceIoControl(
                    (HANDLE) hDevice,            // handle to device
                    IOCTL_STORAGE_CHECK_VERIFY2,  // dwIoControlCode
                    NULL,                        // lpInBuffer
                    0,                           // nInBufferSize
                    &ulChanges,                  // lpOutBuffer
                    sizeof(ULONG),               // nOutBufferSize
                    &junk ,                      // number of bytes returned
                    NULL );                      // OVERLAPPED structure

  CloseHandle(hDevice);

  return iResult;

}

CStdString CWIN32Util::GetLocalPath(const CStdString &strPath)
{
  CURL url(strPath);
  CStdString strLocalPath = url.GetFileName();
  strLocalPath.Replace(url.GetShareName()+"/","");
  return strLocalPath;
}

char CWIN32Util::FirstDriveFromMask (ULONG unitmask)
{
    char i;
    for (i = 0; i < 26; ++i)
    {
        if (unitmask & 0x1) break;
        unitmask = unitmask >> 1;
    }
    return (i + 'A');
}


// Workaround to get the added and removed drives
// Seems to be that the lParam in SDL is empty
// MS way: http://msdn.microsoft.com/en-us/library/aa363215(VS.85).aspx

void CWIN32Util::UpdateDriveMask()
{
  dwDriveMask = GetLogicalDrives();
}

CStdString CWIN32Util::GetChangedDrive()
{
  CStdString strDrive;
  DWORD dwDriveMask2 = GetLogicalDrives();
  DWORD dwDriveMaskResult = dwDriveMask ^ dwDriveMask2;
  if(dwDriveMaskResult == 0)
    return "";
  dwDriveMask = dwDriveMask2;
  strDrive.Format("%c:",FirstDriveFromMask(dwDriveMaskResult));
  return strDrive;
}

// End Workaround

bool CWIN32Util::PowerManagement(PowerState State)
{
// SetSuspendState not available in vs2003
#if _MSC_VER > 1400
  HANDLE hToken;
  TOKEN_PRIVILEGES tkp;
  // Get a token for this process.
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
  {
    return false;
  }
  // Get the LUID for the shutdown privilege.
  LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
  tkp.PrivilegeCount = 1;  // one privilege to set   
  tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  // Get the shutdown privilege for this process.
  AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
  if (GetLastError() != ERROR_SUCCESS)
  {
    return false;
  }

  switch (State)
  {
  case POWERSTATE_HIBERNATE:
    CLog::Log(LOGINFO, "Asking Windows to hibernate...");
    return SetSuspendState(true,true,false) == TRUE;
    break;
  case POWERSTATE_SUSPEND:
    CLog::Log(LOGINFO, "Asking Windows to suspend...");
    return SetSuspendState(false,true,false) == TRUE;
    break;
  case POWERSTATE_SHUTDOWN:
    CLog::Log(LOGINFO, "Shutdown Windows...");
    return ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_UPGRADE | SHTDN_REASON_FLAG_PLANNED) == TRUE;
    break;
  case POWERSTATE_REBOOT:
    CLog::Log(LOGINFO, "Rebooting Windows...");
    return ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_UPGRADE | SHTDN_REASON_FLAG_PLANNED) == TRUE;
    break;
  default:
    CLog::Log(LOGERROR, "Unknown PowerState called.");
    return false;
    break;
  }
#else
  return false;
#endif
}

bool CWIN32Util::XBMCShellExecute(const CStdString &strPath, bool bWaitForScriptExit)
{
  CStdString strCommand = strPath;
  CStdString strExe = strPath;
  CStdString strParams;
  strCommand.Trim();
  if (strCommand.IsEmpty())
  {
    return false;
  }
  int iIndex = -1;
  char split = ' ';
  if (strCommand[0] == '\"')
  {
    split = '\"';
  }
  iIndex = strCommand.Find(split, 1);
  if (iIndex != -1)
  {
    strExe = strCommand.substr(0, iIndex + 1);
    strParams = strCommand.substr(iIndex + 1);
  }

  strExe.Replace("\"","");

  bool ret;
  SHELLEXECUTEINFO ShExecInfo = {0};
  ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
  ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  ShExecInfo.hwnd = NULL;
  ShExecInfo.lpVerb = NULL;
  ShExecInfo.lpFile = strExe.c_str();		
  ShExecInfo.lpParameters = strParams.c_str();	
  ShExecInfo.lpDirectory = NULL;
  ShExecInfo.nShow = SW_SHOW;
  ShExecInfo.hInstApp = NULL;	

  g_windowHelper.StopThread();

  LockSetForegroundWindow(LSFW_UNLOCK);
  ShowWindow(g_hWnd,SW_MINIMIZE);
  ret = ShellExecuteEx(&ShExecInfo) == TRUE;
  g_windowHelper.SetHANDLE(ShExecInfo.hProcess);

  // ShellExecute doesn't return the window of the started process
  // we need to gather it from somewhere to allow switch back to XBMC
  // when a program is minimized instead of stopped.
  //g_windowHelper.SetHWND(ShExecInfo.hwnd);
  g_windowHelper.Create();

  if(bWaitForScriptExit)
  {
    // Todo: Pause music and video playback
    WaitForSingleObject(ShExecInfo.hProcess,INFINITE);
  }

  return ret;
}

std::vector<CStdString> CWIN32Util::GetDiskUsage()
{
  CStdString strRet;
  vector<CStdString> result;
  ULARGE_INTEGER ULTotal= { { 0 } };
  ULARGE_INTEGER ULTotalFree= { { 0 } };

  char* pcBuffer= NULL;
  DWORD dwStrLength= GetLogicalDriveStrings( 0, pcBuffer );
  if( dwStrLength != 0 )
  {
    dwStrLength+= 1;
    pcBuffer= new char [dwStrLength];
    GetLogicalDriveStrings( dwStrLength, pcBuffer );
    int iPos= 0;
    do 
    {
      CStdString strDrive = pcBuffer + iPos;
      if( DRIVE_FIXED == GetDriveType( strDrive.c_str()  ) &&
        GetDiskFreeSpaceEx( ( strDrive.c_str() ), NULL, &ULTotal, &ULTotalFree ) )
      {
        strRet.Format("%s %d MB %s",strDrive.c_str(), int(ULTotalFree.QuadPart/(1024*1024)),g_localizeStrings.Get(160));
        result.push_back(strRet);
      }
      iPos += (strlen( pcBuffer + iPos) + 1 );
    }while( strlen( pcBuffer + iPos ) > 0 );
  }
  free( pcBuffer );
  return result;
}

void CWIN32Util::MaximizeWindow(bool bRemoveBorder)
{
  /*int w=0,h=0;
  g_videoConfig.GetDesktopResolution(&w,&h);*/
}

CStdString CWIN32Util::GetResInfoString()
{
  CStdString strRes;
  DEVMODE devmode;
  ZeroMemory(&devmode, sizeof(devmode));
  devmode.dmSize = sizeof(devmode);
  EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
  strRes.Format("Desktop Resolution: %dx%d %dBit at %dHz",devmode.dmPelsWidth,devmode.dmPelsHeight,devmode.dmBitsPerPel,devmode.dmDisplayFrequency);
  return strRes;
}

int CWIN32Util::GetDesktopColorDepth()
{
  DEVMODE devmode;
  ZeroMemory(&devmode, sizeof(devmode));
  devmode.dmSize = sizeof(devmode);
  EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
  return (int)devmode.dmBitsPerPel;
}

CStdString CWIN32Util::GetProfilePath()
{
  CStdString strProfilePath;
  WCHAR szPath[MAX_PATH];
  bool bpDirs = g_application.PlatformDirectoriesEnabled();

  if(bpDirs && SUCCEEDED(SHGetFolderPathW(NULL,CSIDL_APPDATA|CSIDL_FLAG_CREATE,NULL,0,szPath)))
  {
    g_charsetConverter.wToUTF8(szPath, strProfilePath);
    CUtil::AddFileToFolder(strProfilePath, "XBMC\\", strProfilePath);
  }  
  else
    CUtil::GetHomePath(strProfilePath);

  return strProfilePath;
}

extern "C" {
  FILE *fopen_utf8(const char *_Filename, const char *_Mode)
  {
    CStdStringW wfilename, wmode;
    g_charsetConverter.utf8ToW(_Filename, wfilename, false);
    wmode = _Mode;
    return _wfopen(wfilename.c_str(), wmode.c_str());
  }
}

