#include "stdafx.h"
#include "fileitem.h"
#include "Util.h"
#include "picture.h"
#include "playlistfactory.h"
#include "shortcut.h"
#include "crc32.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/filecurl.h"
#include "musicInfoTagLoaderFactory.h"
#include "cuedocument.h"
#include "Utils/fstrcmp.h"
#include "videodatabase.h"
#include "musicdatabase.h"
#include "SortFileItem.h"

CFileItem::CFileItem(const CSong& song)
{
  Reset();
  m_strLabel = song.strTitle;
  m_strPath = song.strFileName;
  m_musicInfoTag.SetSong(song);
  m_lStartOffset = song.iStartOffset;
  m_lEndOffset = song.iEndOffset;
  m_strThumbnailImage = song.strThumb;
}

CFileItem::CFileItem(const CAlbum& album)
{
  Reset();
  m_strLabel = album.strAlbum;
  m_strPath = album.strPath;
  m_bIsFolder = true;
  m_strLabel2 = album.strArtist;
  m_musicInfoTag.SetAlbum(album);
  m_strThumbnailImage = album.strThumb;
}

CFileItem::CFileItem(const CArtist& artist)
{
  Reset();
  m_strLabel = artist.strArtist;
  m_strPath = artist.strArtist;
  m_bIsFolder = true;
  m_musicInfoTag.SetArtist(artist.strArtist);
}

CFileItem::CFileItem(const CGenre& genre)
{
  Reset();
  m_strLabel = genre.strGenre;
  m_strPath = genre.strGenre;
  m_bIsFolder = true;
  m_musicInfoTag.SetGenre(genre.strGenre);
}

CFileItem::CFileItem(const CFileItem& item)
{
  *this = item;
}

CFileItem::CFileItem(const CGUIListItem& item)
{
  Reset();
  // not particularly pretty, but it gets around the issue of Reset() defaulting
  // parameters in the CGUIListItem base class.
  *((CGUIListItem *)this) = item;
}

CFileItem::CFileItem(void)
{
  Reset();
}

CFileItem::CFileItem(const CStdString& strLabel)
    : CGUIListItem()
{
  Reset();
  SetLabel(strLabel);
}

CFileItem::CFileItem(const CStdString& strPath, bool bIsFolder)
{
  Reset();
  m_strPath = strPath;
  m_bIsFolder = bIsFolder;
}

CFileItem::CFileItem(const CShare& share)
{
  Reset();
  m_bIsFolder = true;
  m_bIsShareOrDrive = true;
  m_strPath = share.strPath;
  m_strLabel = share.strName;
  if (share.strStatus.size())
    m_strLabel.Format("%s (%s)", share.strName.c_str(), share.strStatus.c_str());
  m_iLockMode = share.m_iLockMode;
  m_strLockCode = share.m_strLockCode;
  m_iHasLock = share.m_iHasLock;
  m_iBadPwdCount = share.m_iBadPwdCount;
  m_iDriveType = share.m_iDriveType;
  m_strThumbnailImage = share.m_strThumbnailImage;
  SetLabelPreformated(true);
}

CFileItem::~CFileItem(void)
{
}

const CFileItem& CFileItem::operator=(const CFileItem& item)
{
  if (this == &item) return * this;
  m_strLabel2 = item.m_strLabel2;
  m_strLabel = item.m_strLabel;
  m_bLabelPreformated=item.m_bLabelPreformated;
  FreeMemory();
  m_bSelected = item.m_bSelected;
  m_strIcon = item.m_strIcon;
  m_strThumbnailImage = item.m_strThumbnailImage;
  m_overlayIcon = item.m_overlayIcon;
  m_strPath = item.m_strPath;
  m_bIsFolder = item.m_bIsFolder;
  m_bIsParentFolder = item.m_bIsParentFolder;
  m_iDriveType = item.m_iDriveType;
  m_bIsShareOrDrive = item.m_bIsShareOrDrive;
  m_dateTime = item.m_dateTime;
  m_dwSize = item.m_dwSize;
  m_musicInfoTag = item.m_musicInfoTag;
  m_lStartOffset = item.m_lStartOffset;
  m_lEndOffset = item.m_lEndOffset;
  m_fRating = item.m_fRating;
  m_strDVDLabel = item.m_strDVDLabel;
  m_strTitle = item.m_strTitle;
  m_iprogramCount = item.m_iprogramCount;
  m_idepth = item.m_idepth;
  m_iLockMode = item.m_iLockMode;
  m_strLockCode = item.m_strLockCode;
  m_iHasLock = item.m_iHasLock;
  m_iBadPwdCount = item.m_iBadPwdCount;
  m_bCanQueue=item.m_bCanQueue;
  SetInvalid();
  return *this;
}

void CFileItem::Reset()
{
  m_strLabel2.Empty();
  m_strLabel.Empty();
  m_bLabelPreformated=false;
  FreeIcons();
  m_overlayIcon = ICON_OVERLAY_NONE;
  m_musicInfoTag.Clear();
  m_bSelected = false;
  m_fRating = 0.0f;
  m_strDVDLabel.Empty();
  m_strTitle.Empty();
  m_strPath.Empty();
  m_fRating = 0.0f;
  m_dwSize = 0;
  m_bIsFolder = false;
  m_bIsParentFolder=false;
  m_bIsShareOrDrive = false;
  m_dateTime.Reset();
  m_iDriveType = SHARE_TYPE_UNKNOWN;
  m_lStartOffset = 0;
  m_lEndOffset = 0;
  m_iprogramCount = 0;
  m_idepth = 1;
  m_iLockMode = LOCK_MODE_EVERYONE;
  m_strLockCode = "";
  m_iBadPwdCount = 0;
  m_iHasLock = 0;
  m_bCanQueue=true;
  SetInvalid();
}

void CFileItem::Serialize(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_bIsFolder;
    ar << m_bIsParentFolder;
    ar << m_strLabel;
    ar << m_strLabel2;
    ar << m_bLabelPreformated;
    ar << m_strThumbnailImage;
    ar << m_strIcon;
    ar << m_bSelected;
    ar << m_overlayIcon;

    ar << m_strPath;
    ar << m_bIsShareOrDrive;
    ar << m_iDriveType;
    ar << m_dateTime;
    ar << m_dwSize;
    ar << m_fRating;
    ar << m_strDVDLabel;
    ar << m_strTitle;
    ar << m_iprogramCount;
    ar << m_idepth;
    ar << m_lStartOffset;
    ar << m_lEndOffset;
    ar << m_iLockMode;
    ar << m_strLockCode;
    ar << m_iBadPwdCount;

    ar << m_bCanQueue;

    ar << m_musicInfoTag;
  }
  else
  {
    ar >> m_bIsFolder;
    ar >> m_bIsParentFolder;
    ar >> m_strLabel;
    ar >> m_strLabel2;
    ar >> m_bLabelPreformated;
    ar >> m_strThumbnailImage;
    ar >> m_strIcon;
    ar >> m_bSelected;
    ar >> (int&)m_overlayIcon;

    ar >> m_strPath;
    ar >> m_bIsShareOrDrive;
    ar >> m_iDriveType;
    ar >> m_dateTime;
    ar >> m_dwSize;
    ar >> m_fRating;
    ar >> m_strDVDLabel;
    ar >> m_strTitle;
    ar >> m_iprogramCount;
    ar >> m_idepth;
    ar >> m_lStartOffset;
    ar >> m_lEndOffset;
    ar >> m_iLockMode;
    ar >> m_strLockCode;
    ar >> m_iBadPwdCount;

    ar >> m_bCanQueue;

    ar >> m_musicInfoTag;
    SetInvalid();
  }
}

bool CFileItem::IsVideo() const
{
  /* check preset content type */
  if( m_contenttype.Left(7).Equals("video/") )
    return true;

  CStdString extension;
  if( m_contenttype.Left(12).Equals("application/") )
  { /* check for some standard types */
    extension = m_contenttype.Mid(12);
    if( extension.Equals("ogg")
     || extension.Equals("mp4")
     || extension.Equals("mxf") )
     return true;
  }

  CUtil::GetExtension(m_strPath, extension);

  if (extension.IsEmpty())
    return false;

  extension.ToLower();

  if (g_stSettings.m_videoExtensions.Find(extension) != -1)
    return true;

  return false;
}

bool CFileItem::IsAudio() const
{
  if (IsCDDA()) return true;
  if (IsShoutCast()) return true;
  if (IsLastFM()) return true;
  
  /* check preset content type */
  if( m_contenttype.Left(7).Equals("audio/") )
    return true;

  CStdString extension;
  if( m_contenttype.Left(12).Equals("application/") )
  { /* check for some standard types */
    extension = m_contenttype.Mid(12);
    if( extension.Equals("ogg")
     || extension.Equals("mp4")
     || extension.Equals("mxf") )
     return true;
  }

  CUtil::GetExtension(m_strPath, extension);

  if (extension.IsEmpty())
    return false;

  extension.ToLower();
  if (g_stSettings.m_musicExtensions.Find(extension) != -1)
    return true;

  return false;
}

bool CFileItem::IsPicture() const
{
  if( m_contenttype.Left(7).Equals("image/") )
    return true;

  CStdString extension;
  CUtil::GetExtension(m_strPath, extension);    

  if (extension.IsEmpty())
    return false;

  extension.ToLower();
  if (g_stSettings.m_pictureExtensions.Find(extension) != -1)
    return true;

  if (extension == ".tbn") 
    return true;

  return false;
}

bool CFileItem::IsCUESheet() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath, strExtension);
  return (strExtension.CompareNoCase(".cue") == 0);
}

bool CFileItem::IsShoutCast() const
{
  if (strstr(m_strPath.c_str(), "shout:") ) return true;
  return false;
}

bool CFileItem::IsLastFM() const
{
  if (strstr(m_strPath.c_str(), "lastfm:") ) return true;
  return false;
}

bool CFileItem::IsInternetStream() const
{
  CURL url(m_strPath);
  CStdString strProtocol = url.GetProtocol();
  strProtocol.ToLower();

  if (strProtocol.size() == 0)
    return false;

  if (strProtocol == "shout" || strProtocol == "mms" ||
      strProtocol == "http" || /*strProtocol == "ftp" ||*/
      strProtocol == "rtsp" || strProtocol == "rtp" ||
      strProtocol == "udp"  || strProtocol == "lastfm" ||
      strProtocol == "https")
    return true;

  return false;
}

bool CFileItem::IsFileFolder() const
{
  return (
    m_bIsFolder && (
    IsSmartPlayList() ||
    IsPlayList() ||
    IsZIP() ||
    IsRAR() ||
    IsType("ogg") ||
    IsType("nsf") ||
    IsType("sid")
    )
    );
}


bool CFileItem::IsSmartPlayList() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath, strExtension);
  strExtension.ToLower();
  return (strExtension == ".xsp");
}

bool CFileItem::IsPlayList() const
{
  return CPlayListFactory::IsPlaylist(m_strPath);
}

bool CFileItem::IsPythonScript() const
{
  return CUtil::GetExtension(m_strPath).Equals(".py", false);  
}

bool CFileItem::IsXBE() const
{
  return CUtil::GetExtension(m_strPath).Equals(".xbe", false);  
}

bool CFileItem::IsType(const char *ext) const
{
  return CUtil::GetExtension(m_strPath).Equals(ext, false);  
}

bool CFileItem::IsDefaultXBE() const
{
  CStdString filename = CUtil::GetFileName(m_strPath);  
  if (filename.Equals("default.xbe")) return true;
  return false;
}

bool CFileItem::IsShortCut() const
{
  return CUtil::GetExtension(m_strPath).Equals(".cut", false);  
}

bool CFileItem::IsNFO() const
{
  return CUtil::GetExtension(m_strPath).Equals(".nfo", false);  
}

bool CFileItem::IsDVDImage() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath, strExtension);
  if (strExtension.Equals(".img") || strExtension.Equals(".iso")) return true;
  return false;
}

bool CFileItem::IsDVDFile(bool bVobs /*= true*/, bool bIfos /*= true*/) const
{
  CStdString strFileName = CUtil::GetFileName(m_strPath);
  if (bIfos)
  {
    if (strFileName.Equals("video_ts.ifo")) return true;
    if (strFileName.Left(4).Equals("vts_") && strFileName.Right(6).Equals("_0.ifo") && strFileName.length() == 12) return true;
  }
  if (bVobs)
  {
    if (strFileName.Equals("video_ts.vob")) return true;
    if (strFileName.Left(4).Equals("vts_") && strFileName.Right(4).Equals(".vob")) return true;
  }

  return false;
}

bool CFileItem::IsRAR() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath, strExtension);
  if ( (strExtension.CompareNoCase(".rar") == 0) || ((strExtension.Equals(".001") && m_strPath.Mid(m_strPath.length()-7,7).CompareNoCase(".ts.001"))) ) return true; // sometimes the first rar is named .001
  return false;
}

bool CFileItem::IsZIP() const
{
  return CUtil::GetExtension(m_strPath).Equals(".zip", false);  
}

bool CFileItem::IsCBZ() const
{
  return CUtil::GetExtension(m_strPath).Equals(".cbz", false);  
}

bool CFileItem::IsCBR() const
{
  return CUtil::GetExtension(m_strPath).Equals(".cbr", false);  
}

bool CFileItem::IsStack() const
{
  CURL url(m_strPath);
  return url.GetProtocol().Equals("stack");
}

bool CFileItem::IsMultiPath() const
{
  return CUtil::IsMultiPath(m_strPath);
}

bool CFileItem::IsCDDA() const
{
  return CUtil::IsCDDA(m_strPath);
}

bool CFileItem::IsDVD() const
{
  return CUtil::IsDVD(m_strPath);
}

bool CFileItem::IsOnDVD() const
{
  return CUtil::IsOnDVD(m_strPath);
}

bool CFileItem::IsISO9660() const
{
  return CUtil::IsISO9660(m_strPath);
}

bool CFileItem::IsRemote() const
{
  return CUtil::IsRemote(m_strPath);
}

bool CFileItem::IsSmb() const
{
  return CUtil::IsSmb(m_strPath);
}

bool CFileItem::IsDAAP() const
{
  return CUtil::IsDAAP(m_strPath);
}

bool CFileItem::IsHD() const
{
  return CUtil::IsHD(m_strPath);
}

bool CFileItem::IsMusicDb() const
{
  if (strstr(m_strPath.c_str(), "musicdb:") ) return true;
  return false;
}

bool CFileItem::IsVideoDb() const
{
  if (strstr(m_strPath.c_str(), "videodb:") ) return true;
  return false;
}

bool CFileItem::IsVirtualDirectoryRoot() const
{
  return (m_bIsFolder && m_strPath.IsEmpty());
}

bool CFileItem::IsMemoryUnit() const
{
  CURL url(m_strPath);
  return url.GetProtocol().Left(3).Equals("mem");
}

bool CFileItem::IsRemovable() const
{
  return IsOnDVD() || IsCDDA() || IsMemoryUnit();
}

bool CFileItem::IsReadOnly() const
{
  if (IsParentFolder()) return true;
  if (m_bIsShareOrDrive) return true;
  return !CUtil::SupportsFileOperations(m_strPath);
}

void CFileItem::FillInDefaultIcon()
{
  //CLog::Log(LOGINFO, "FillInDefaultIcon(%s)", pItem->GetLabel().c_str());
  // find the default icon for a file or folder item
  // for files this can be the (depending on the file type)
  //   default picture for photo's
  //   default picture for songs
  //   default picture for videos
  //   default picture for shortcuts
  //   default picture for playlists
  //   or the icon embedded in an .xbe
  //
  // for folders
  //   for .. folders the default picture for parent folder
  //   for other folders the defaultFolder.png

  CStdString strThumb;
  CStdString strExtension;
  if (GetIconImage() == "")
  {
    if (!m_bIsFolder)
    {
      if ( IsPlayList() )
      {
        SetIconImage("defaultPlaylist.png");
      }
      else if ( IsPicture() )
      {
        // picture
        SetIconImage("defaultPicture.png");
      }
      else if ( IsXBE() )
      {
        // xbe
        SetIconImage("defaultProgram.png");
      }
      else if ( IsAudio() )
      {
        // audio
        SetIconImage("defaultAudio.png");
      }
      else if ( IsVideo() )
      {
        // video
        SetIconImage("defaultVideo.png");
      }
      else if ( IsShortCut() && !IsLabelPreformated() )
      {
        // shortcut
        CStdString strDescription;
        CStdString strFName;
        strFName = CUtil::GetFileName(m_strPath);

        int iPos = strFName.ReverseFind(".");
        strDescription = strFName.Left(iPos);
        SetLabel(strDescription);
        SetIconImage("defaultShortcut.png");
      }
      else if ( IsPythonScript() )
      {
        SetIconImage("DefaultScript.png");
      }
      //else
      //{
      //  // default icon for unknown file type
      //  SetIconImage("defaultUnknown.png");
      //}
    }
    else
    {
      if ( IsPlayList() )
      {
        SetIconImage("defaultPlaylist.png");
      }
      else if (IsParentFolder())
      {
        SetIconImage("defaultFolderBack.png");
      }
      else
      {
        SetIconImage("defaultFolder.png");
      }
    }
  }
  // Set the icon overlays (if applicable)
  if (!HasOverlay())
  {
    if (CUtil::IsInRAR(m_strPath))
      SetOverlayImage(CGUIListItem::ICON_OVERLAY_RAR);
    else if (CUtil::IsInZIP(m_strPath))
      SetOverlayImage(CGUIListItem::ICON_OVERLAY_ZIP);
  }
}

CStdString CFileItem::GetCachedArtistThumb()
{
  Crc32 crc;
  crc.ComputeFromLowerCase("artist" + GetLabel());
  CStdString cachedThumb;
  cachedThumb.Format("%s\\%08x.tbn", g_settings.GetMusicArtistThumbFolder().c_str(), (unsigned __int32)crc);
  return cachedThumb;
}

CStdString CFileItem::GetCachedProfileThumb()
{
  Crc32 crc;
  crc.ComputeFromLowerCase("profile" + m_strPath);
  CStdString cachedThumb;
  cachedThumb.Format("%s\\Thumbnails\\Profiles\\%08x.tbn", g_settings.GetUserDataFolder().c_str(), (unsigned __int32)crc);
  return cachedThumb;
}

void CFileItem::SetCachedArtistThumb()
{
  CStdString thumb(GetCachedArtistThumb());
  if (CFile::Exists(thumb))
  {
    // found it, we are finished.
    SetThumbnailImage(thumb);
//    SetIconImage(strThumb);
  }
}

// set the album thumb for a file or folder
void CFileItem::SetMusicThumb(bool alwaysCheckRemote /* = true */)
{ 
  if (HasThumbnail()) return;
  SetCachedMusicThumb();
  if (!HasThumbnail())
    SetUserMusicThumb(alwaysCheckRemote);
}

void CFileItem::RemoveExtension()
{
  if (m_bIsFolder)
    return ;
  CStdString strLabel = GetLabel();
  CUtil::RemoveExtension(strLabel);
  SetLabel(strLabel);
}

void CFileItem::CleanFileName()
{
  if (m_bIsFolder)
    return ;
  CStdString strLabel = GetLabel();
  CUtil::CleanFileName(strLabel);
  SetLabel(strLabel);
}

void CFileItem::SetLabel(const CStdString &strLabel)
{
  m_strLabel = strLabel;
  if (strLabel=="..")
  {
    m_bIsParentFolder=true;
    m_bIsFolder=true;
    SetLabelPreformated(true);
  }
  SetInvalid();
}

void CFileItem::SetFileSizeLabel()
{
  if( m_bIsFolder && m_dwSize == 0 )
    SetLabel2("");
  else
    SetLabel2(StringUtils::SizeToString(m_dwSize));
}

CURL CFileItem::GetAsUrl() const
{
  return CURL(m_strPath);
}

bool CFileItem::CanQueue() const
{
  return m_bCanQueue;
}

void CFileItem::SetCanQueue(bool bYesNo)
{
  m_bCanQueue=bYesNo;
}

bool CFileItem::IsParentFolder() const
{
  return m_bIsParentFolder;
}

// %N - TrackNumber
// %S - DiscNumber
// %A - Artist
// %T - Titel
// %B - Album
// %G - Genre
// %Y - Year
// %F - FileName
// %D - Duration
// %% - % sign
// %I - Size
// %J - Date
// %R - Movie rating
// %C - Programs count
// %K - Movie/Game title
// %L - existing Label
void CFileItem::FormatLabel(const CStdString& strMask)
{
  if (!strMask.IsEmpty())
  {
    const CStdString& strLabel=ParseFormat(strMask);
    if (!strLabel.IsEmpty())
      SetLabel(strLabel);
    else if (!m_bIsFolder && g_guiSettings.GetBool("filelists.hideextensions"))
      RemoveExtension();
  }
  else
    SetLabel(strMask);
}

void CFileItem::FormatLabel2(const CStdString& strMask)
{
  if (!strMask.IsEmpty())
  {
    const CStdString& strLabel2=ParseFormat(strMask);
    if (!strLabel2.IsEmpty())
      SetLabel2(strLabel2);
  }
  else
    SetLabel2(strMask);
}

CStdString CFileItem::ParseFormat(const CStdString& strMask)
{
  if (strMask.IsEmpty())
    return "";

  CStdString strLabel = "";
  CMusicInfoTag& tag = m_musicInfoTag;
  int iPos1 = 0;
  int iPos2 = strMask.Find('%', iPos1);
  bool bDoneSomething = !(iPos1 == iPos2); // stuff in front should be applied - everything using this bool is added by spiff
                                           // when true, it should add in anything extra in the mask (ie the content before the next %)
  bool hasContent = false; // when true, we've actually added some content
  while (iPos2 >= 0)
  {
    if( (iPos2 > iPos1) && bDoneSomething )
    {
      strLabel += strMask.Mid(iPos1, iPos2 - iPos1);
      bDoneSomething = false;  
    }
    CStdString str;
    if (strMask[iPos2 + 1] == 'N' && tag.GetTrackNumber() > 0)
    { // track number
      str.Format("%02.2i", tag.GetTrackNumber());
      bDoneSomething = hasContent = true;
    }
    else if (strMask[iPos2 + 1] == 'S' && tag.GetDiscNumber() > 0)
    { // disc number
      str.Format("%02.2i", tag.GetDiscNumber());
      bDoneSomething = hasContent = true;
    }
    else if (strMask[iPos2 + 1] == 'A' && tag.GetArtist().size())
    { // artist
      str = tag.GetArtist();
      bDoneSomething = hasContent = true;
    }
    else if (strMask[iPos2 + 1] == 'T' && tag.GetTitle().size())
    { // title
      str = tag.GetTitle();
      bDoneSomething = hasContent = true;
    }
    else if (strMask[iPos2 + 1] == 'B' && tag.GetAlbum().size())
    { // album
      str = tag.GetAlbum();
      bDoneSomething = hasContent = true;
    }
    else if (strMask[iPos2 + 1] == 'G' && tag.GetGenre().size())
    { // genre
      str = tag.GetGenre();
      bDoneSomething = hasContent = true;
    }
    else if (strMask[iPos2 + 1] == 'Y')
    { // year
      str = tag.GetYear();
      bDoneSomething = hasContent = true;
    }
    else if (strMask[iPos2 + 1] == 'F')
    { // filename
      str = CUtil::GetTitleFromPath(m_strPath, m_bIsFolder && !IsFileFolder());
      bDoneSomething = hasContent = true;
    }
    else if (strMask[iPos2 + 1] == 'L')
    { // pre-existing label
      str = GetLabel();
      // is the label the actual file or folder name?
      if (str == m_strPath.Right(str.GetLength()))
      { // label is the same as filename, clean it up as appropriate
        str = CUtil::GetTitleFromPath(m_strPath, m_bIsFolder && !IsFileFolder());
      }
      bDoneSomething = hasContent = true;
    }
    else if (strMask[iPos2 + 1] == 'D')
    { // duration
      int nDuration = tag.GetDuration();

      if (nDuration > 0)
        StringUtils::SecondsToTimeString(nDuration, str);
      else if (m_dwSize > 0)
        str = StringUtils::SizeToString(m_dwSize);
      bDoneSomething = hasContent = true;
    }
    else if (strMask[iPos2 + 1] == 'I')
    { // size
      if( m_bIsFolder && m_dwSize == 0 )
        str="";
      else
        str=StringUtils::SizeToString(m_dwSize);
      bDoneSomething = hasContent = true;
    }
    else if (strMask[iPos2 + 1] == 'J' && m_dateTime.IsValid())
    { // date
      str = m_dateTime.GetAsLocalizedDate();
      bDoneSomething = hasContent = true;
    }
    else if (strMask[iPos2 + 1] == 'R')
    { // movie rating
      str.Format("%2.2f", m_fRating);
      bDoneSomething = hasContent = true;
    }
    else if (strMask[iPos2 + 1] == 'Q')
    { // movie Year
      str=m_musicInfoTag.GetYear();
      bDoneSomething = hasContent = true;
    }
    else if (strMask[iPos2 + 1] == 'C')
    { // programs count
      str.Format("%i", m_iprogramCount);
      bDoneSomething = hasContent = true;
    }
    else if (strMask[iPos2 + 1] == 'K')
    { // moview/game title
      str=m_strTitle;
      bDoneSomething = hasContent = true;
    }
    else if (strMask[iPos2 + 1] == '%')
    { // %% to print %
      str = '%';
      bDoneSomething = hasContent = true;
    }
    strLabel += str;
    iPos1 = iPos2 + 2;
    iPos2 = strMask.Find('%', iPos1);
  }
  if (iPos1 < (int)strMask.size())
    strLabel += strMask.Right(strMask.size() - iPos1);

  if (!hasContent)
    return "";  // no content was added (only stuff from the mask string)

  return strLabel;
}

const CStdString& CFileItem::GetContentType() const
{
  if( m_contenttype.IsEmpty() )
  {
    /* discard const qualifyier*/
    CStdString& m_ref = (CStdString&)m_contenttype;

    if( m_bIsFolder )
      m_ref = "x-directory/normal";
    else if( m_strPath.Left(8).Equals("shout://") 
          || m_strPath.Left(7).Equals("http://")
          || m_strPath.Left(7).Equals("upnp://"))
    {
      CURL url(m_strPath);
      url.SetProtocol("http");
      CFileCurl::GetContent(url, m_ref);
    }
    else if( m_strPath.Left(8).Equals("https://") )
    {
      CURL url(m_strPath);      
      CFileCurl::GetContent(url, m_ref);
    }

    /* if it's still empty set to an unknown type*/
    if( m_ref.IsEmpty() )
      m_ref = "application/octet-stream";

  }

  return m_contenttype;
}

bool CFileItem::IsSamePath(const CFileItem *item)
{
  if (!item)
    return false;

  if (item->m_strPath == m_strPath && item->m_lStartOffset == m_lStartOffset) return true;
  if (IsMusicDb() || IsVideoDb())
  {
    CFileItem dbItem(m_musicInfoTag.GetURL(), false);
    dbItem.m_lStartOffset = m_lStartOffset;
    return dbItem.IsSamePath(item);
  }
  if (item->IsMusicDb() || item->IsVideoDb())
  {
    CFileItem dbItem(item->m_musicInfoTag.GetURL(), false);
    dbItem.m_lStartOffset = item->m_lStartOffset;
    return IsSamePath(&dbItem);
  }
  return false;
}

/////////////////////////////////////////////////////////////////////////////////
/////
///// CFileItemList
/////
//////////////////////////////////////////////////////////////////////////////////

CFileItemList::CFileItemList()
{
  m_fastLookup = false;
  m_bIsFolder=true;
  m_bCacheToDisc=false;
  m_sortMethod=SORT_METHOD_NONE;
  m_sortOrder=SORT_ORDER_NONE;
}

CFileItemList::CFileItemList(const CStdString& strPath)
{
  m_strPath=strPath;
  m_fastLookup = false;
  m_bIsFolder=true;
  m_bCacheToDisc=false;
  m_sortMethod=SORT_METHOD_NONE;
  m_sortOrder=SORT_ORDER_NONE;
}

CFileItemList::~CFileItemList()
{
  Clear();
}

CFileItem* CFileItemList::operator[] (int iItem)
{
  return Get(iItem);
}

const CFileItem* CFileItemList::operator[] (int iItem) const
{
  return Get(iItem);
}

CFileItem* CFileItemList::operator[] (const CStdString& strPath)
{
  return Get(strPath);
}

const CFileItem* CFileItemList::operator[] (const CStdString& strPath) const
{
  return Get(strPath);
}

void CFileItemList::SetFastLookup(bool fastLookup)
{
  if (fastLookup && !m_fastLookup)
  { // generate the map
    m_map.clear();
    for (unsigned int i=0; i < m_items.size(); i++)
    {
      CFileItem *pItem = m_items[i];
      m_map.insert(MAPFILEITEMSPAIR(pItem->m_strPath, pItem));
    }
  }
  if (!fastLookup && m_fastLookup)
    m_map.clear();
  m_fastLookup = fastLookup;
}

bool CFileItemList::Contains(CStdString& fileName)
{
  if (m_fastLookup)
    return m_map.find(fileName) != m_map.end();
  // slow method...
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItem *pItem = m_items[i];
    if (pItem->m_strPath == fileName)
      return true;
  }
  return false;
}

void CFileItemList::Clear()
{
  if (m_items.size())
  {
    IVECFILEITEMS i;
    i = m_items.begin();
    while (i != m_items.end())
    {
      CFileItem* pItem = *i;
      delete pItem;
      i = m_items.erase(i);
    }
    m_map.clear();
  }

  m_sortMethod=SORT_METHOD_NONE;
  m_sortOrder=SORT_ORDER_NONE;
  m_bCacheToDisc=false;
}

void CFileItemList::ClearKeepPointer()
{
  if (m_items.size())
  {
    m_items.clear();
    m_map.clear();
  }

  m_sortMethod=SORT_METHOD_NONE;
  m_sortOrder=SORT_ORDER_NONE;
  m_bCacheToDisc=false;
}

void CFileItemList::Add(CFileItem* pItem)
{
  m_items.push_back(pItem);
  if (m_fastLookup)
    m_map.insert(MAPFILEITEMSPAIR(pItem->m_strPath, pItem));
}

void CFileItemList::AddFront(CFileItem* pItem)
{
  m_items.insert(m_items.begin(), pItem);
  if (m_fastLookup)
    m_map.insert(MAPFILEITEMSPAIR(pItem->m_strPath, pItem));
}

void CFileItemList::Remove(CFileItem* pItem)
{
  for (IVECFILEITEMS it = m_items.begin(); it != m_items.end(); ++it)
  {
    if (pItem == *it)
    {
      m_items.erase(it);
      if (m_fastLookup)
        m_map.erase(pItem->m_strPath);
      break;
    }
  }
}

void CFileItemList::Remove(int iItem)
{
  if (iItem >= 0 && iItem < (int)Size())
  {
    CFileItem* pItem = *(m_items.begin() + iItem);
    if (m_fastLookup)
      m_map.erase(pItem->m_strPath);
    delete pItem;
    m_items.erase(m_items.begin() + iItem);
  }
}

void CFileItemList::Append(const CFileItemList& itemlist)
{
  for (int i = 0; i < itemlist.Size(); ++i)
  {
    const CFileItem* pItem = itemlist[i];
    CFileItem* pNewItem = new CFileItem(*pItem);
    Add(pNewItem);
  }
}

void CFileItemList::AppendPointer(const CFileItemList& itemlist)
{
  for (int i = 0; i < itemlist.Size(); ++i)
  {
    CFileItem* pItem = const_cast<CFileItem*>(itemlist[i]);
    Add(pItem);
  }
}

CFileItem* CFileItemList::Get(int iItem)
{
  if (iItem > -1)
    return m_items[iItem];

  return NULL;
}

const CFileItem* CFileItemList::Get(int iItem) const
{
  if (iItem > -1)
    return m_items[iItem];

  return NULL;
}

CFileItem* CFileItemList::Get(const CStdString& strPath)
{
  if (m_fastLookup)
  {
    IMAPFILEITEMS it=m_map.find(strPath);
    if (it != m_map.end())
      return it->second;

    return NULL;
  }
  // slow method...
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItem *pItem = m_items[i];
    if (pItem->m_strPath == strPath)
      return pItem;
  }

  return NULL;
}

const CFileItem* CFileItemList::Get(const CStdString& strPath) const
{
  if (m_fastLookup)
  {
    map<CStdString, CFileItem*>::const_iterator it=m_map.find(strPath);
    if (it != m_map.end())
      return it->second;

    return NULL;
  }
  // slow method...
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItem *pItem = m_items[i];
    if (pItem->m_strPath == strPath)
      return pItem;
  }

  return NULL;
}

int CFileItemList::Size() const
{
  return (int)m_items.size();
}

bool CFileItemList::IsEmpty() const
{
  return (m_items.size() <= 0);
}

void CFileItemList::Reserve(int iCount)
{
  m_items.reserve(iCount);
}

void CFileItemList::Sort(FILEITEMLISTCOMPARISONFUNC func)
{
  DWORD dwTime=GetTickCount();
  sort(m_items.begin(), m_items.end(), func);
  CLog::DebugLog("Sorting FileItems %s, took %ld ms", m_strPath.c_str(), GetTickCount()-dwTime);
}

void CFileItemList::Sort(SORT_METHOD sortMethod, SORT_ORDER sortOrder)
{
  //  Already sorted?
  if (sortMethod==m_sortMethod && m_sortOrder==sortOrder)
    return;

  switch (sortMethod)
  {
  case SORT_METHOD_LABEL:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::LabelAscending : SSortFileItem::LabelDescending);
    break;
  case SORT_METHOD_LABEL_IGNORE_THE:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::LabelAscendingNoThe : SSortFileItem::LabelDescendingNoThe);
    break;
  case SORT_METHOD_DATE:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::DateAscending : SSortFileItem::DateDescending);
    break;
  case SORT_METHOD_SIZE:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::SizeAscending : SSortFileItem::SizeDescending);
    break;
  case SORT_METHOD_DRIVE_TYPE:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::DriveTypeAscending : SSortFileItem::DriveTypeDescending);
    break;
  case SORT_METHOD_TRACKNUM:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::SongTrackNumAscending : SSortFileItem::SongTrackNumDescending);
    break;
  case SORT_METHOD_DURATION:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::SongDurationAscending : SSortFileItem::SongDurationDescending);
    break;
  case SORT_METHOD_TITLE_IGNORE_THE:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::SongTitleAscendingNoThe : SSortFileItem::SongTitleDescendingNoThe);
    break;
  case SORT_METHOD_TITLE:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::SongTitleAscending : SSortFileItem::SongTitleDescending);
    break;
  case SORT_METHOD_ARTIST:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::SongArtistAscending : SSortFileItem::SongArtistDescending);
    break;
  case SORT_METHOD_ARTIST_IGNORE_THE:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::SongArtistAscendingNoThe : SSortFileItem::SongArtistDescendingNoThe);
    break;
  case SORT_METHOD_ALBUM:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::SongAlbumAscending : SSortFileItem::SongAlbumDescending);
    break;
  case SORT_METHOD_ALBUM_IGNORE_THE:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::SongAlbumAscendingNoThe : SSortFileItem::SongAlbumDescendingNoThe);
    break;
  case SORT_METHOD_GENRE:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::SongGenreAscending : SSortFileItem::SongGenreDescending);
    break;
  case SORT_METHOD_FILE:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::FileAscending : SSortFileItem::FileDescending);
    break;
  case SORT_METHOD_VIDEO_RATING:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::MovieRatingAscending : SSortFileItem::MovieRatingDescending);
    break;
  case SORT_METHOD_VIDEO_YEAR:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::MovieYearAscending : SSortFileItem::MovieYearDescending);
    break;
  case SORT_METHOD_PROGRAM_COUNT:
  case SORT_METHOD_PLAYLIST_ORDER:
    // TODO: Playlist order is hacked into program count variable (not nice, but ok until 2.0)
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::ProgramCountAscending : SSortFileItem::ProgramCountDescending);
    break;
  default:
    break;
  }

  m_sortMethod=sortMethod;
  m_sortOrder=sortOrder;
}

void CFileItemList::Randomize()
{
  random_shuffle(m_items.begin(), m_items.end());
}

void CFileItemList::Serialize(CArchive& ar)
{
  if (ar.IsStoring())
  {
    CFileItem::Serialize(ar);

    int i = 0;
    if (m_items.size() > 0 && m_items[0]->IsParentFolder())
      i = 1;

    ar << (int)(m_items.size() - i);

    ar << m_fastLookup;

    ar << (int)m_sortMethod;
    ar << (int)m_sortOrder;
    ar << m_bCacheToDisc;

    for (i; i < (int)m_items.size(); ++i)
    {
      CFileItem* pItem = m_items[i];
      ar << *pItem;
    }
  }
  else
  {
    CFileItem* pParent=NULL;
    if (!IsEmpty())
    {
      CFileItem* pItem=m_items[0];
      if (pItem->IsParentFolder())
        pParent = new CFileItem(*pItem);
    }

    SetFastLookup(false);
    Clear();


    CFileItem::Serialize(ar);

    int iSize = 0;
    ar >> iSize;
    if (iSize <= 0)
      return ;

    if (pParent)
    {
      m_items.reserve(iSize + 1);
      m_items.push_back(pParent);
    }
    else
      m_items.reserve(iSize);

    bool fastLookup=false;
    ar >> fastLookup;

    ar >> (int&)m_sortMethod;
    ar >> (int&)m_sortOrder;
    ar >> m_bCacheToDisc;

    for (int i = 0; i < iSize; ++i)
    {
      CFileItem* pItem = new CFileItem;
      ar >> *pItem;
      Add(pItem);
    }

    SetFastLookup(fastLookup);
  }
}

void CFileItemList::FillInDefaultIcons()
{
  for (int i = 0; i < (int)m_items.size(); ++i)
  {
    CFileItem* pItem = m_items[i];
    pItem->FillInDefaultIcon();
  }
}

void CFileItemList::SetMusicThumbs()
{
  //cache thumbnails directory
  g_directoryCache.InitMusicThumbCache();

  for (int i = 0; i < (int)m_items.size(); ++i)
  {
    CFileItem* pItem = m_items[i];
    pItem->SetMusicThumb();
  }

  g_directoryCache.ClearMusicThumbCache();
}

int CFileItemList::GetFolderCount() const
{
  int nFolderCount = 0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItem* pItem = m_items[i];
    if (pItem->m_bIsFolder)
      nFolderCount++;
  }

  return nFolderCount;
}

int CFileItemList::GetFileCount() const
{
  int nFileCount = 0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItem* pItem = m_items[i];
    if (!pItem->m_bIsFolder)
      nFileCount++;
  }

  return nFileCount;
}

int CFileItemList::GetSelectedCount() const
{
  int count = 0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItem* pItem = m_items[i];
    if (pItem->IsSelected())
      count++;
  }

  return count;
}

// Checks through our file list for the path specified in path.
// Check is done case-insensitive
bool CFileItemList::HasFileNoCase(CStdString& path)
{
  bool bFound = false;
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    if (stricmp(m_items[i]->m_strPath.c_str(), path) == 0)
    {
      bFound = true;
      path = m_items[i]->m_strPath;
      break;
    }
  }
  return bFound;
}

void CFileItemList::FilterCueItems()
{
  // Handle .CUE sheet files...
  VECSONGS itemstoadd;
  CStdStringArray itemstodelete;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItem *pItem = m_items[i];
    if (!pItem->m_bIsFolder)
    { // see if it's a .CUE sheet
      if (pItem->IsCUESheet())
      {
        CCueDocument cuesheet;
        if (cuesheet.Parse(pItem->m_strPath))
        {
          VECSONGS newitems;
          cuesheet.GetSongs(newitems);
          // queue the cue sheet and the underlying media file for deletion
          CStdString strMediaFile = cuesheet.GetMediaPath();
          bool bFoundMediaFile = CFile::Exists(strMediaFile);
          if (!bFoundMediaFile)
          {
            // try file in same dir, not matching case...
            if (HasFileNoCase(strMediaFile))
            {
              bFoundMediaFile = true;
            }
            else
            {
              // try removing the .cue extension...
              strMediaFile = pItem->m_strPath;
              CUtil::RemoveExtension(strMediaFile);
              CFileItem item(strMediaFile, false);
              if (item.IsAudio() && HasFileNoCase(strMediaFile))
              {
                bFoundMediaFile = true;
              }
              else
              { // try replacing the extension with one of our allowed ones.
                CStdStringArray extensions;
                StringUtils::SplitString(g_stSettings.m_musicExtensions, "|", extensions);
                for (unsigned int i = 0; i < extensions.size(); i++)
                {
                  CUtil::ReplaceExtension(pItem->m_strPath, extensions[i], strMediaFile);
                  CFileItem item(strMediaFile, false);
                  if (!item.IsCUESheet() && !item.IsPlayList() && HasFileNoCase(strMediaFile))
                  {
                    bFoundMediaFile = true;
                    break;
                  }
                }
              }
            }
          }
          if (bFoundMediaFile)
          {
            itemstodelete.push_back(pItem->m_strPath);
            itemstodelete.push_back(strMediaFile);
            // get the additional stuff (year, genre etc.) from the underlying media files tag.
            CMusicInfoTag tag;
            auto_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(cuesheet.GetMediaPath()));
            if (NULL != pLoader.get())
            {
              // get id3tag
              pLoader->Load(strMediaFile, tag);
            }
            // fill in any missing entries from underlying media file
            for (int j = 0; j < (int)newitems.size(); j++)
            {
              CSong song = newitems[j];
              song.strFileName = strMediaFile;
              if (tag.Loaded())
              {
                if (song.strAlbum.empty() && !tag.GetAlbum().empty()) song.strAlbum = tag.GetAlbum();
                if (song.strGenre.empty() && !tag.GetGenre().empty()) song.strGenre = tag.GetGenre();
                if (song.strArtist.empty() && !tag.GetArtist().empty()) song.strArtist = tag.GetArtist();
                SYSTEMTIME dateTime;
                tag.GetReleaseDate(dateTime);
                if (dateTime.wYear > 1900) song.iYear = dateTime.wYear;
              }
              if (!song.iDuration && tag.GetDuration() > 0)
              { // must be the last song
                song.iDuration = (tag.GetDuration() * 75 - song.iStartOffset + 37) / 75;
              }
              // add this item to the list
              itemstoadd.push_back(song);
            }
          }
          else
          { // remove the .cue sheet from the directory
            itemstodelete.push_back(pItem->m_strPath);
          }
        }
        else
        { // remove the .cue sheet from the directory (can't parse it - no point listing it)
          itemstodelete.push_back(pItem->m_strPath);
        }
      }
    }
  }
  // now delete the .CUE files and underlying media files.
  for (int i = 0; i < (int)itemstodelete.size(); i++)
  {
    for (int j = 0; j < (int)m_items.size(); j++)
    {
      CFileItem *pItem = m_items[j];
      if (stricmp(pItem->m_strPath.c_str(), itemstodelete[i].c_str()) == 0)
      { // delete this item
        delete pItem;
        m_items.erase(m_items.begin() + j);
        break;
      }
    }
  }
  // and add the files from the .CUE sheet
  for (int i = 0; i < (int)itemstoadd.size(); i++)
  {
    // now create the file item, and add to the item list.
    CFileItem *pItem = new CFileItem(itemstoadd[i]);
    m_items.push_back(pItem);
  }
}

// Remove the extensions from the filenames
void CFileItemList::RemoveExtensions()
{
  for (int i = 0; i < Size(); ++i)
    m_items[i]->RemoveExtension();
}

void CFileItemList::CleanFileNames()
{
  for (int i = 0; i < Size(); ++i)
    m_items[i]->CleanFileName();
}

void CFileItemList::Stack()
{
  // TODO: Remove nfo files before this stage?  The old routine did, but I'm not sure
  // the advantage of this (seems to me it's better just to ignore them for stacking
  // purposes).
  if (g_stSettings.m_iMyVideoStack != STACK_NONE)
  {
    // items needs to be sorted for stuff below to work properly
    Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);

    // First stack any DVD folders by removing every dvd file other than
    // the VIDEO_TS.IFO file.
    bool isDVDFolder(false);
    for (int i = 0; i < Size(); ++i)
    {
      CFileItem *item = Get(i);
      if (item->GetLabel().CompareNoCase("video_ts.ifo") == 0)
      {
        isDVDFolder = true;
        break;
      }
      if (item->m_bIsFolder && (item->GetLabel().Mid(0,2).Equals("CD") && item->GetLabel().size() == 3))
      {
        if (atoi(item->GetLabel().c_str()+2) > 0)
        {
          CFileItemList items;
          CDirectory::GetDirectory(item->m_strPath,items,g_stSettings.m_videoExtensions,true);
          if (items.Size() == 1)
          {
            if (items[0]->IsVideo())
              *item = *items[0];
          }
        }
      }
    }
    if (isDVDFolder)
    { // remove any other ifo files in this folder
      // leave the vobs stacked.
      const int num = Size() ? Size() - 1 : 0;  // Size will alter in this loop
      for (int i = num; i; --i)
      {
        CFileItem *item = Get(i);
        if (item->IsDVDFile(false, true) && item->GetLabel().CompareNoCase("video_ts.ifo"))
        {
          Remove(i);
        }
      }
    }
    // Stacking
    for (int i = 0; i < Size(); ++i)
    {
      CFileItem *item = Get(i);

      // ignore parent directories, playlists and the virtual root
      if (item->IsPlayList() || item->IsParentFolder() || item->IsNFO() || IsVirtualDirectoryRoot())
        continue;

      if( item->m_bIsFolder)
      {
        // check for any dvd directories, only on known fast types
        // i'm adding xbms even thou it really isn't fast due to
        // opening file to check for existance
        if( !item->IsRemote() 
         || item->IsSmb() 
         || CUtil::IsInRAR(item->m_strPath) 
         || CUtil::IsInZIP(item->m_strPath)
         || item->m_strPath.Left(5).Equals("xbms", false)
         )
        {
          CStdString path;
          CUtil::AddFileToFolder(item->m_strPath, "VIDEO_TS.IFO", path);
          if (CFile::Exists(path))
          {
            /* set the thumbnail based on folder */
            item->SetCachedVideoThumb();
            if (!item->HasThumbnail())
              item->SetUserVideoThumb();

            item->m_bIsFolder = false;
            item->m_strPath = path;
            item->SetLabel2("");
            item->SetLabelPreformated(true);
            m_sortMethod = SORT_METHOD_NONE; /* sorting is now broken */

            /* override the previously set thumb if video_ts.ifo has any */
            /* otherwise user can't set icon on the stacked file as that */
            /* will allways be set on the video_ts.ifo file */
            CStdString thumb(item->GetCachedVideoThumb());
            if(CFile::Exists(thumb))
              item->SetThumbnailImage(thumb);              
            else
              item->SetUserVideoThumb();
              
          }
        }
        continue;
      }

      CStdString fileName, filePath;
      CUtil::Split(item->m_strPath, filePath, fileName);
      CStdString fileTitle, volumeNumber;
      //CLog::Log(LOGDEBUG,"Trying to stack: %s", item->GetLabel().c_str());
      if (CUtil::GetVolumeFromFileName(item->GetLabel(), fileTitle, volumeNumber))
      {
        vector<int> stack;
        stack.push_back(i);
        __int64 size = item->m_dwSize;
        for (int j = i + 1; j < Size(); ++j)
        {
          CFileItem *item2 = Get(j);
          // ignore directories, nfo files and playlists
          if (item2->IsPlayList() || item2->m_bIsFolder || item2->IsNFO())
            continue;
          CStdString fileName2, filePath2;
          CUtil::Split(item2->m_strPath, filePath2, fileName2);
          CStdString fileTitle2, volumeNumber2;
          //if (CUtil::GetVolumeFromFileName(fileName2, fileTitle2, volumeNumber2))
          if (CUtil::GetVolumeFromFileName(Get(j)->GetLabel(), fileTitle2, volumeNumber2))
          {
            if (fileTitle2.Equals(fileTitle, false))
            {
              //CLog::Log(LOGDEBUG,"  adding item: [%03i] %s", j, Get(j)->GetLabel().c_str());
              stack.push_back(j);
              size += item2->m_dwSize;
            }
          }
        }
        if (stack.size() > 1)
        {
          // have a stack, remove the items and add the stacked item
          CStackDirectory dir;
          // dont actually stack a multipart rar set, just remove all items but the first
          CStdString stackPath;
          if (Get(stack[0])->IsRAR())
            stackPath = Get(stack[0])->m_strPath;
          else
            stackPath = dir.ConstructStackPath(*this, stack);
          for (unsigned int j = stack.size() - 1; j > 0; --j)
            Remove(stack[j]);
          item->m_strPath = stackPath;
          // item->m_bIsFolder = true;  // don't treat stacked files as folders
          // the label may be in a different char set from the filename (eg over smb
          // the label is converted from utf8, but the filename is not)
          CUtil::GetVolumeFromFileName(item->GetLabel(), fileTitle, volumeNumber);
          item->SetLabel(fileTitle);
          item->m_dwSize = size;
          //CLog::Log(LOGDEBUG,"  ** finalized stack: %s", fileTitle.c_str());
        }
      }
    }
  }
}

bool CFileItemList::Load()
{
  CStdString strPath=m_strPath;
  if (CUtil::HasSlashAtEnd(strPath))
    strPath.Delete(strPath.size() - 1);

  Crc32 crc;
  crc.ComputeFromLowerCase(strPath);

  CStdString strFileName;
  if (IsCDDA() || IsOnDVD())
    strFileName.Format("Z:\\r-%08x.fi", (unsigned __int32)crc);
  else
    strFileName.Format("Z:\\%08x.fi", (unsigned __int32)crc);

  CLog::Log(LOGDEBUG,"Loading fileitems [%s]",strFileName.c_str());

  CFile file;
  if (file.Open(strFileName))
  {
    CArchive ar(&file, CArchive::load);
    ar >> *this;
    CLog::Log(LOGDEBUG,"  -- items: %i, directory: %s sort method: %i, ascending: %s",Size(),m_strPath.c_str(), m_sortMethod, m_sortOrder ? "true" : "false");
    ar.Close();
    file.Close();
    return true;
  }

  return false;
}

bool CFileItemList::Save()
{
  int iSize = Size();
  if (iSize <= 0)
    return false;

  CLog::Log(LOGDEBUG,"Saving fileitems [%s]",m_strPath.c_str());

  CStdString strPath=m_strPath;
  if (CUtil::HasSlashAtEnd(strPath))
    strPath.Delete(strPath.size() - 1);

  Crc32 crc;
  crc.ComputeFromLowerCase(strPath);

  CStdString strFileName;
  if (IsCDDA() || IsOnDVD())
    strFileName.Format("Z:\\r-%08x.fi", (unsigned __int32)crc);
  else
    strFileName.Format("Z:\\%08x.fi", (unsigned __int32)crc);

  CFile file;
  if (file.OpenForWrite(strFileName, true, true)) // overwrite always
  {
    CArchive ar(&file, CArchive::store);
    ar << *this;
    CLog::Log(LOGDEBUG,"  -- items: %i, sort method: %i, ascending: %s",iSize,m_sortMethod, m_sortOrder ? "true" : "false");
    ar.Close();
    file.Close();
    return true;
  }

  return false;
}

void CFileItemList::SetCachedVideoThumbs()
{
  // TODO: Investigate caching time to see if it speeds things up
  for (unsigned int i = 0; i < m_items.size(); ++i)
  {
    CFileItem* pItem = m_items[i];
    pItem->SetCachedVideoThumb();
  }
}

void CFileItemList::SetCachedProgramThumbs()
{
  // TODO: Investigate caching time to see if it speeds things up
  for (unsigned int i = 0; i < m_items.size(); ++i)
  {
    CFileItem* pItem = m_items[i];
    pItem->SetCachedProgramThumb();
  }
}

void CFileItemList::SetCachedMusicThumbs()
{
  // TODO: Investigate caching time to see if it speeds things up
  for (unsigned int i = 0; i < m_items.size(); ++i)
  {
    CFileItem* pItem = m_items[i];
    pItem->SetCachedMusicThumb();
  }
}

CStdString CFileItem::GetCachedPictureThumb()
{
  // get the locally cached thumb
  Crc32 crc;
  crc.ComputeFromLowerCase(m_strPath);
  CStdString hex;
  hex.Format("%08x", crc);
  CStdString thumb;
  thumb.Format("%s\\%c\\%s.tbn", g_settings.GetPicturesThumbFolder().c_str(), hex[0], hex.c_str());
  return thumb;
}

void CFileItem::SetCachedMusicThumb()
{
  // if it already has a thumbnail, then return
  if (HasThumbnail() || m_bIsShareOrDrive) return ;

  // streams do not have thumbnails
  if (IsInternetStream()) return ;

  //  music db items already have thumbs or there is no thumb available
  if (IsMusicDb()) return;

  // ignore the parent dir items
  if (IsParentFolder()) return;

  CStdString cachedThumb(GetPreviouslyCachedMusicThumb());
  if (!cachedThumb.IsEmpty())
    SetThumbnailImage(cachedThumb);
    // SetIconImage(cachedThumb);
}

CStdString CFileItem::GetPreviouslyCachedMusicThumb()
{ 
  // the highest priority thumb is album name + album path
  CStdString strPath;
  if (!m_bIsFolder)
    CUtil::GetDirectory(m_strPath, strPath);
  else
  {
    strPath = m_strPath;
    CUtil::RemoveSlashAtEnd(strPath);
  }

  // look if an album thumb is available,
  // could be any file with tags loaded or
  // a directory in album window
  CStdString strAlbum;
  if (m_musicInfoTag.Loaded())
    strAlbum = m_musicInfoTag.GetAlbum();

  if (!strAlbum.IsEmpty())
  {
    // try permanent album thumb (Q:\albums\thumbs)
    // using "album name + path"
    CStdString thumb(CUtil::GetCachedAlbumThumb(strAlbum, strPath));
    if (CFile::Exists(thumb))
      return thumb;
  }

  // if a file, try to find a cached filename.tbn
  if (!m_bIsFolder)
  {
    // look for locally cached tbn
    CStdString thumb(CUtil::GetCachedMusicThumb(m_strPath));
    if (CFile::Exists(thumb))
      return thumb;
  }

  // try and find a cached folder thumb (folder.jpg or folder.tbn)
  CStdString thumb(CUtil::GetCachedMusicThumb(strPath));
  if (CFile::Exists(thumb))
    return thumb;

  return "";
}

CStdString CFileItem::GetUserMusicThumb(bool alwaysCheckRemote /* = false */)
{
  if (m_bIsShareOrDrive) return "";
  if (IsInternetStream()) return "";
  if (IsParentFolder()) return "";
  if (IsMusicDb()) return "";
  CURL url(m_strPath);
  if (url.GetProtocol() == "rar" || url.GetProtocol() == "zip") return "";
  if (url.GetProtocol() == "upnp") return "";

  // we first check for <filename>.tbn or <foldername>.tbn
  CStdString fileThumb(GetTBNFile());
  if (CFile::Exists(fileThumb))
    return fileThumb;
  // if a folder, check for folder.jpg
  if (m_bIsFolder && (!IsRemote() || alwaysCheckRemote || g_guiSettings.GetBool("musicfiles.findremotethumbs")))
  {
    CStdString folderThumb;
    CStdStringArray thumbs;
    StringUtils::SplitString(g_advancedSettings.m_musicThumbs, "|", thumbs);
    for (unsigned int i = 0; i < thumbs.size(); ++i)
    {
      CUtil::AddFileToFolder(m_strPath, thumbs[i], folderThumb);
      if (CFile::Exists(folderThumb))
      {
        return folderThumb;
      }
    }
  }
  // this adds support for files which inherit a folder.jpg icon which has not been cached yet.
  // this occurs when queueing a top-level folder which has not been traversed yet.
  else if (!IsRemote() || alwaysCheckRemote || g_guiSettings.GetBool("musicfiles.findremotethumbs"))
  {
    CStdString strFolder, strFile;
    CUtil::Split(m_strPath, strFolder, strFile);
    CFileItem folderItem(strFolder, true);
    folderItem.SetMusicThumb(alwaysCheckRemote);
    if (folderItem.HasThumbnail())
      return folderItem.GetThumbnailImage();
  }
  // No thumb found
  return "";
}

void CFileItem::SetUserMusicThumb(bool alwaysCheckRemote /* = false */)
{
  // caches as the local thumb 
  CStdString thumb(GetUserMusicThumb(alwaysCheckRemote));
  if (!thumb.IsEmpty())
  {
    CStdString cachedThumb(CUtil::GetCachedMusicThumb(m_strPath));
    CPicture pic;
    pic.DoCreateThumbnail(thumb, cachedThumb);
  }

  SetCachedMusicThumb();
}

void CFileItem::SetCachedPictureThumb()
{
  if (IsParentFolder()) return;
  CStdString cachedThumb(GetCachedPictureThumb());
  if (CFile::Exists(cachedThumb))
    SetThumbnailImage(cachedThumb);
}

CStdString CFileItem::GetCachedVideoThumb()
{
  // get the locally cached thumb
  Crc32 crc;
  if (IsStack())
  {
    CStackDirectory dir;
    crc.ComputeFromLowerCase(dir.GetFirstStackedFile(m_strPath));
  }
  else
    crc.ComputeFromLowerCase(m_strPath);
  CStdString thumb;
  thumb.Format("%s\\%08x.tbn", g_settings.GetVideoThumbFolder().c_str(), (unsigned __int32)crc);
  return thumb;
}

void CFileItem::SetCachedVideoThumb()
{
  if (IsParentFolder()) return;
  CStdString cachedThumb(GetCachedVideoThumb());
  if (CFile::Exists(cachedThumb))
    SetThumbnailImage(cachedThumb);
}

// Gets the .tbn filename from a file or folder name.
// <filename>.ext -> <filename>.tbn
// <foldername>/ -> <foldername>.tbn
CStdString CFileItem::GetTBNFile()
{
  // special case for zip/rar
  if (IsRAR() || IsZIP())
  {
    // extract the filename portion and find the tbn based on that
    CURL url(m_strPath);
    CFileItem item(url.GetFileName());
    url.SetFileName(item.GetTBNFile());
    CStdString thumbFile;
    url.GetURL(thumbFile);
    return thumbFile;
  }
  if (m_bIsFolder && !IsFileFolder())
  {
    CStdString thumbFile(m_strPath);
    CUtil::RemoveSlashAtEnd(thumbFile);
    return thumbFile + ".tbn";
  }
  else
  {
    CStdString thumbFile;
    CUtil::ReplaceExtension(m_strPath, ".tbn", thumbFile);
    return thumbFile;
  }
}

CStdString CFileItem::GetUserVideoThumb()
{
  if (m_bIsShareOrDrive) return "";
  if (IsInternetStream()) return "";
  if (IsParentFolder()) return "";
  CURL url(m_strPath);
  if (url.GetProtocol() == "rar" || url.GetProtocol() == "zip") return "";
  if (url.GetProtocol() == "upnp") return "";

  // 1. check <filename>.tbn or <foldername>.tbn
  CStdString fileThumb;
  if (IsStack())
  {
    CStackDirectory dir;
    CFileItem item(dir.GetFirstStackedFile(m_strPath), false);
    fileThumb = item.GetTBNFile();
  }
  else
    fileThumb = GetTBNFile();
  if (CFile::Exists(fileThumb))
    return fileThumb;
  // 2. if a folder, check for folder.jpg
  if (m_bIsFolder)
  {
    CStdString folderThumb;
    CUtil::AddFileToFolder(m_strPath, "folder.jpg", folderThumb);
    if (CFile::Exists(folderThumb))
      return folderThumb;
  }
  // No thumb found
  return "";
}

void CFileItem::SetVideoThumb()
{
  if (HasThumbnail()) return;
  SetCachedVideoThumb();
  if (!HasThumbnail())
    SetUserVideoThumb();
}

void CFileItem::SetUserVideoThumb()
{
  if (m_bIsShareOrDrive) return;
  if (IsParentFolder()) return;

  // caches as the local thumb 
  CStdString thumb(GetUserVideoThumb());
  if (!thumb.IsEmpty())
  {
    CStdString cachedThumb(GetCachedVideoThumb());
    CPicture pic;
    pic.DoCreateThumbnail(thumb, cachedThumb);
  }
  SetCachedVideoThumb();
}

CStdString CFileItem::GetCachedProgramThumb()
{
  // get the locally cached thumb
  Crc32 crc;
  if (IsOnDVD())
  {
    CStdString strDesc;
    CUtil::GetXBEDescription(m_strPath,strDesc);
    CStdString strCRC;
    strCRC.Format("%s%u",strDesc.c_str(),CUtil::GetXbeID(m_strPath));
    crc.ComputeFromLowerCase(strCRC);
  }
  else
    crc.ComputeFromLowerCase(m_strPath);
  CStdString thumb;
  thumb.Format("%s\\%08x.tbn", g_settings.GetProgramsThumbFolder().c_str(), (unsigned __int32)crc);
  return thumb;
}

CStdString CFileItem::GetCachedGameSaveThumb()
{
  CStdString strExt;
  CStdString fileName, filePath;
  CUtil::Split(m_strPath, filePath, fileName);
  CUtil::GetExtension(fileName,strExt);
  if (strExt.Equals(".xbx")) // savemeta.xbx - cache thumb
  {
    Crc32 crc;
    crc.ComputeFromLowerCase(m_strPath);
    CStdString thumb;
    thumb.Format("%s\\%08x.tbn", g_settings.GetGameSaveThumbFolder().c_str(),(unsigned __int32)crc);
    if (!CFile::Exists(thumb))
    {
      CStdString strTitleImage, strParent, strParentSave, strParentTitle;
      CUtil::GetDirectory(m_strPath,strTitleImage);
      CUtil::GetParentPath(strTitleImage,strParent);
      CUtil::AddFileToFolder(strTitleImage,"saveimage.xbx",strTitleImage);
      CUtil::AddFileToFolder(strParent,"saveimage.xbx",strParentSave);
      CUtil::AddFileToFolder(strParent,"titleimage.xbx",strParentTitle);
      //CUtil::AddFileToFolder(strTitleImageCur,"titleimage.xbx",m_strPath);
      if (CFile::Exists(strTitleImage))
        CUtil::CacheXBEIcon(strTitleImage, thumb);
      else if (CFile::Exists(strParentSave))
        CUtil::CacheXBEIcon(strParentSave,thumb);
      else if (CFile::Exists(strParentTitle))
        CUtil::CacheXBEIcon(strParentTitle,thumb);
      else
        thumb = "";
    }
    return thumb;
  }
  else
  {
    if (CDirectory::Exists(m_strPath))
    {
      CStdString thumb;
      thumb.Format("%s\\%s.tbn", g_settings.GetGameSaveThumbFolder().c_str(), fileName.c_str());
      CLog::Log(LOGDEBUG, "Thumb  (%s)",thumb.c_str());
      if (!CFile::Exists(thumb))
      {
        CStdString titleimageXBX;
        CStdString saveimageXBX;

        CUtil::AddFileToFolder(m_strPath, "titleimage.xbx", titleimageXBX);
        CUtil::AddFileToFolder(m_strPath,"saveimage.xbx",saveimageXBX);
        
        /*if (CFile::Exists(saveimageXBX))
        {
          CUtil::CacheXBEIcon(saveimageXBX, thumb);
          CLog::Log(LOGDEBUG, "saveimageXBX  (%s)",saveimageXBX.c_str());
        }*/
       if (CFile::Exists(titleimageXBX))
        {
          CLog::Log(LOGDEBUG, "titleimageXBX  (%s)",titleimageXBX.c_str());
          CUtil::CacheXBEIcon(titleimageXBX, thumb);
        }
      }
      return thumb;
    }
  }
  return "";
}

void CFileItem::SetCachedProgramThumb()
{
  // don't set any thumb for programs on DVD, as they're bound to be named the
  // same (D:\default.xbe).
  if (IsParentFolder()) return;
  CStdString thumb(GetCachedProgramThumb());
  if (CFile::Exists(thumb))
    SetThumbnailImage(thumb);
}

void CFileItem::SetUserProgramThumb()
{
  if (m_bIsShareOrDrive) return;
  if (IsParentFolder()) return;

  if (IsShortCut())
  {
    CShortcut shortcut;
    if ( shortcut.Create( m_strPath ) )
    {
      // use the shortcut's thumb
      if (!shortcut.m_strThumb.IsEmpty())
        m_strThumbnailImage = shortcut.m_strThumb;
      else
      {
        CFileItem item(shortcut.m_strPath,false);
        item.SetUserProgramThumb();
        m_strThumbnailImage = item.m_strThumbnailImage;
      }
      return;
    }
  }
  // 1.  Try <filename>.tbn
  CStdString fileThumb(GetTBNFile());
  CStdString thumb(GetCachedProgramThumb());
  if (CFile::Exists(fileThumb))
  { // cache
    CPicture pic;
    if (pic.DoCreateThumbnail(fileThumb, thumb))
      SetThumbnailImage(thumb);
  }
  else if (IsXBE())
  {
    // 2. check for avalaunch_icon.jpg
    CStdString directory;
    CUtil::GetDirectory(m_strPath, directory);
    CStdString avalaunchIcon;
    CUtil::AddFileToFolder(directory, "avalaunch_icon.jpg", avalaunchIcon);
    if (CFile::Exists(avalaunchIcon))
    {
      CPicture pic;
      if (pic.DoCreateThumbnail(avalaunchIcon, thumb))
        SetThumbnailImage(thumb);
    }
    else if (CUtil::CacheXBEIcon(m_strPath, thumb))
      SetThumbnailImage(thumb);
  }
  else if (m_bIsFolder)
  {
    // 3. cache the folder image
    CStdString folderThumb;
    CUtil::AddFileToFolder(m_strPath, "folder.jpg", folderThumb);
    if (CFile::Exists(folderThumb))
    {
      CPicture pic;
      if (pic.DoCreateThumbnail(folderThumb, thumb))
        SetThumbnailImage(thumb);
    }
  }
}

void CFileItemList::SetProgramThumbs()
{
  // TODO: Is there a speed up if we cache the program thumbs first?
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItem *pItem = m_items[i];
    if (pItem->IsParentFolder())
      continue;
    pItem->SetCachedProgramThumb();
    if (!pItem->HasThumbnail())
      pItem->SetUserProgramThumb();
  }
}

bool CFileItem::LoadMusicTag()
{
  // not audio
  if (!IsAudio())
    return false;
  // already loaded?
  if (m_musicInfoTag.Loaded())
    return true;
  // check db
  CMusicDatabase musicDatabase;
  CSong song;
  if (musicDatabase.GetSongByFileName(m_strPath, song))
  {
    m_musicInfoTag.SetSong(song);
    return true;
  }
  // load tag from file
  else
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": loading tag information for file: %s", m_strPath.c_str());
    CMusicInfoTagLoaderFactory factory;
    CMusicInfoTag tag;
    auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(m_strPath));
    if (NULL != pLoader.get())
    {
      if (pLoader->Load(m_strPath, m_musicInfoTag))
        return true;
    }
  }
  return false;
}

void CFileItem::SetCachedGameSavesThumb()
{
  if (IsParentFolder()) return;
  CStdString thumb(GetCachedGameSaveThumb());
  if (CFile::Exists(thumb))
    SetThumbnailImage(thumb);
}

void CFileItemList::SetCachedGameSavesThumbs()
{
  // TODO: Investigate caching time to see if it speeds things up
  for (unsigned int i = 0; i < m_items.size(); ++i)
  {
    CFileItem* pItem = m_items[i];
    pItem->SetCachedGameSavesThumb();
  }
}

void CFileItemList::SetGameSavesThumbs()
{
  // No User thumbs
  // TODO: Is there a speed up if we cache the program thumbs first?
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItem *pItem = m_items[i];
    if (pItem->IsParentFolder())
      continue;
    pItem->SetCachedGameSavesThumb();  // was  pItem->SetCachedProgramThumb(); oringally
  }
}
