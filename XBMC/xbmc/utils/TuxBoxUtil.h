#pragma once

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

#include "Thread.h"

class CURL;
class TiXmlElement;
class CFileItemList;

#define TS_STREAM_PORT 31339

struct STREAMINFO
{
  CStdString frontend;
  CStdString service_name;
  CStdString service_reference;
  CStdString provider;
  CStdString vpid;
  CStdString apid;
  CStdString pcrpid;
  CStdString tpid;
  CStdString tsid;
  CStdString onid;
  CStdString sid;
  CStdString pmt;
  CStdString video_format;
  CStdString supported_crypt_systems;
  CStdString used_crypt_systems;
  CStdString satellite;
  CStdString frequency;
  CStdString symbol_rate;
  CStdString polarisation;
  CStdString inversion;
  CStdString fec;
  CStdString snr;
  CStdString agc;
  CStdString ber;
  CStdString lock;
  CStdString sync;
};
struct VIDEOSUBCHANNEL
{
  std::vector<CStdString> reference;
  std::vector<CStdString> name;
  std::vector<CStdString> selected;
  CStdString current_name;
  bool mode;
};
struct CURRENTSERVICEDATA
{
  CStdString service_name;
  CStdString service_reference;
  CStdString audio_channel_0_pid;
  CStdString audio_channel_0_selected;
  CStdString audio_channel_0_name;
  CStdString audio_channel_1_pid;
  CStdString audio_channel_1_selected;
  CStdString audio_channel_1_name;
  CStdString audio_channel_2_pid;
  CStdString audio_channel_2_selected;
  CStdString audio_channel_2_name;
  CStdString audio_track;
  CStdString current_event_date;
  CStdString current_event_time;
  CStdString current_event_start;
  CStdString current_event_duration;
  CStdString current_event_description;
  CStdString current_event_details;
  CStdString next_event_date;
  CStdString next_event_time;
  CStdString next_event_start;
  CStdString next_event_duration;
  CStdString next_event_description;
  CStdString next_event_details;
};
struct BOXSTATUS
{
  CStdString current_time;
  CStdString standby;
  CStdString recording;
  CStdString mode;
  CStdString ip;
};
struct BOXSINFO
{
  CStdString image_version;
  CStdString image_url;
  CStdString image_comment;
  CStdString image_catalog;
  CStdString firmware;
  CStdString fpfirmware;
  CStdString webinterface;
  CStdString model;
  CStdString manufacturer;
  CStdString processor;
  CStdString usbstick;
  CStdString disk;
};
struct SERVICE_EPG
{
  CStdString service_reference;
  CStdString service_name;
  CStdString image_comment;
  CStdString event;
  CStdString date;
  CStdString time;
  CStdString duration;
  CStdString descritption;
  CStdString genre;
  CStdString genrecategory;
  CStdString start;
  CStdString details;
};
class CTuxBoxUtil
{
  public:
    STREAMINFO sStrmInfo;
    CURRENTSERVICEDATA sCurSrvData;
    BOXSTATUS sBoxStatus;
    BOXSINFO sBoxInfo;
    SERVICE_EPG sServiceEPG;
    VIDEOSUBCHANNEL vVideoSubChannel;

    CTuxBoxUtil(void);
    virtual ~CTuxBoxUtil(void);
    
    bool GetZapUrl(const CStdString& strPath, CFileItem &items);
    bool ParseBouquets(TiXmlElement *root, CFileItemList &items, CURL &url, CStdString strFilter, CStdString strChild);
    bool ParseChannels(TiXmlElement *root, CFileItemList &items, CURL &url, CStdString strFilter, CStdString strChild);
    bool ZapToUrl(CURL url, CStdString strOptions, int ipoint);
    bool StreamInformations(TiXmlElement *pRootElement);
    bool CurrentServiceData(TiXmlElement *pRootElement);
    bool BoxStatus(TiXmlElement *pRootElement);
    bool BoxInfo(TiXmlElement *pRootElement);
    bool ServiceEPG(TiXmlElement *pRootElement);
    bool GetHttpXML(CURL url,CStdString strRequestType);
    bool GetAudioChannels(CStdString& strAudioChannelName, CStdString& strAudioChannelPid);
    bool GetVideoSubChannels(CStdString& strVideoSubChannelName, CStdString& strVideoSubChannelPid);
    bool GetVideoChannels(TiXmlElement *pRootElement);
    bool CreateNewItem(const CFileItem& item, CFileItem& item_new);

    CStdString GetPicon(CStdString strServiceName);
    CStdString GetSubMode(int iMode, CStdString& strXMLRootString, CStdString& strXMLChildString);
    CStdString DetectSubMode(CStdString strSubMode, CStdString& strXMLRootString, CStdString& strXMLChildString);
};
extern CTuxBoxUtil g_tuxbox;

class CTuxBoxService : public CThread
{
public:
  CTuxBoxService();
  ~CTuxBoxService();
    
  bool Start();
  void Stop();
  bool IsRunning();

  virtual void OnExit();
  virtual void OnStartup();
  virtual void Process();
};
extern CTuxBoxService g_tuxboxService;
