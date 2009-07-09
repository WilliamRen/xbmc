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

/*
 * Common data structures shared between XBMC and PVR clients
 */

#ifndef __PVRCLIENT_TYPES_H__
#define __PVRCLIENT_TYPES_H__

#define MIN_XBMC_PVRDLL_API 1

//#define __cdecl
//#define __declspec(x)
#include <string.h>
#include <time.h>
#include "utils/TVEPGInfoTag.h"
#include "utils/TVChannelInfoTag.h"
#include "utils/TVRecordInfoTag.h"
#include "utils/TVTimerInfoTag.h"
#include "xbmc_addon_types.h"

#ifdef __cplusplus
extern "C" {
#endif

  typedef void*         PVRHANDLE;

  /**
  * PVR Client Error Codes
  */
  typedef enum {
    PVR_ERROR_NO_ERROR             = 0,
    PVR_ERROR_UNKOWN               = -1,
    PVR_ERROR_NOT_IMPLEMENTED      = -2,
    PVR_ERROR_SERVER_ERROR         = -3,
    PVR_ERROR_SERVER_TIMEOUT       = -4,
    PVR_ERROR_NOT_SYNC             = -5,
    PVR_ERROR_NOT_DELETED          = -6,
    PVR_ERROR_NOT_SAVED            = -7,
    PVR_ERROR_RECORDING_RUNNING    = -8,
    PVR_ERROR_ALREADY_PRESENT      = -9
  } PVR_ERROR;

  /**
  * PVR Client Event Codes
  * Sent via PVRManager callback
  */
  typedef enum {
    PVR_EVENT_UNKNOWN              = 0,
    PVR_EVENT_CLOSE                = 1,
    PVR_EVENT_RECORDINGS_CHANGE    = 2,
    PVR_EVENT_CHANNELS_CHANGE      = 3,
    PVR_EVENT_TIMERS_CHANGE        = 4
  } PVR_EVENT;

  /**
  * PVR Client Properties
  * Returned on client initialization
  */
  typedef struct PVR_SERVERPROPS {
    bool SupportChannelLogo;
    bool SupportChannelSettings;
    bool SupportTimeShift;
    bool SupportEPG;
    bool SupportRadio;
    bool SupportRecordings;
    bool SupportTimers;
    bool SupportTeletext;
    bool SupportDirector;
    bool SupportBouquets;
  } PVR_SERVERPROPS;

  /**
  * EPG Channel Definition
  * Is used by the TransferChannelEntry function to inform XBMC that this
  * channel is present.
  */
  typedef struct PVR_CHANNEL {
    int             uid;                /* Unique identifier for this channel */
    int             number;             /* The backend channel number */

    const char     *name;               /* Channel name provided by the Broadcast */
    const char     *callsign;           /* Channel name provided by the user (if present) */
    const char     *iconpath;           /* Path to the channel icon (if present) */

    bool            encrypted;          /* This is a encrypted channel */
    bool            radio;              /* This is a radio channel */
    bool            hide;               /* This channel is hidden by the user */
    bool            recording;          /* This channel is currently recording */
    bool            teletext;           /* This channel provide Teletext */

    int             bouquet;            /* Bouquet ID this channel have (if supported) */

    bool            multifeed;          /* This is a multifeed channel */
    int             multifeed_master;   /* The Master multifeed channel, multifeed_master==number for master itself */
    int             multifeed_number;   /* The own number inside multifeed channel list */

    /* The Stream URL to access this channel, it can be all types of protocol and types
     * are supported by XBMC or in case the client read the stream use pvr://client_>>ClientID<</channels/>>number<<
     * as URL.
     * Examples:
     * Open a Transport Stream over the Client reading functions where client_"1" is the Client ID and
     * 123 the channel number.
     *   pvr://client_1/channels/123.ts
     * Open a VOB file over http and use XBMC's own filereader.
     *   http://192.168.0.120:3000/PS/C-61441-10008-53621+1.vob
     */
    const char     *stream_url;
  } PVR_CHANNEL;

  /**
  * EPG Bouquet Definition
  */
  typedef struct PVR_BOUQUET {
    char*  Name;
    char*  Category;
    int    Number;
  } PVR_BOUQUET;
  typedef struct PVR_BOUQUETLIST {
    PVR_BOUQUET* bouquet;
    int          length;
  } PVR_BOUQUETLIST;

  /**
  * EPG Programme Definition
  * Used to signify an individual broadcast, whether it is also a recording, timer etc.
  */
  typedef struct PVR_PROGINFO {
    int           uid; // unique identifier, if supported will be used for
    int           channum;
    int           bouquet;
    const char   *title;
    const char   *subtitle;
    const char   *description;
    time_t        starttime;
    time_t        endtime;
    const char   *episodeid;
    const char   *seriesid;
    const char   *category;
    int           recording;
    int           rec_status;
    int           event_flags;
  } PVR_PROGINFO;
  typedef struct PVR_PROGLIST {
    PVR_PROGINFO* progInfo;
    int           length;
  } PVR_PROGLIST;

  /**
   * TV Timer Definition
   */
  typedef struct PVR_TIMERINFO {
    int           index;
    int           active;
    const char   *title;
    int           channelNum;
    time_t        starttime;
    time_t        endtime;
    time_t        firstday;
    int           recording;
    int           priority;
    int           lifetime;
    int           repeat;
    int           repeatflags;
  } PVR_TIMERINFO;
  typedef struct PVR_TIMERLIST {
    PVR_TIMERINFO* timerInfo;
    int           length;
  } PVR_TIMERLIST;
  
  // Structure to transfer the above functions to XBMC
  typedef struct PVRClient
  {
    ADDON_STATUS (__cdecl* Create)(ADDON_HANDLE hdl, int ClientID);
    PVR_ERROR (__cdecl* GetProperties)(PVR_SERVERPROPS *props);
    const char* (__cdecl* GetBackendName)();
    const char* (__cdecl* GetBackendVersion)();
    const char* (__cdecl* GetConnectionString)();
    PVR_ERROR (__cdecl* GetDriveSpace)(long long *total, long long *used);
    PVR_ERROR (__cdecl* GetEPGForChannel)(unsigned int number, EPG_DATA &epg, time_t start, time_t end);
    PVR_ERROR (__cdecl* GetEPGNowInfo)(unsigned int number, CTVEPGInfoTag *result);
    PVR_ERROR (__cdecl* GetEPGNextInfo)(unsigned int number, CTVEPGInfoTag *result);
    int (__cdecl* GetNumBouquets)();
    int (__cdecl* GetNumChannels)();
    int (__cdecl* GetNumRecordings)();
    int (__cdecl* GetNumTimers)();
    PVR_ERROR (__cdecl* GetChannelList)(VECCHANNELS *channels, bool radio);
    PVR_ERROR (__cdecl* GetChannelSettings)(CTVChannelInfoTag *result);
    PVR_ERROR (__cdecl* UpdateChannelSettings)(const CTVChannelInfoTag &chaninfo);
    PVR_ERROR (__cdecl* AddChannel)(const CTVChannelInfoTag &info);
    PVR_ERROR (__cdecl* DeleteChannel)(unsigned int number);
    PVR_ERROR (__cdecl* RenameChannel)(unsigned int number, CStdString &newname);
    PVR_ERROR (__cdecl* MoveChannel)(unsigned int number, unsigned int newnumber);
    PVR_ERROR (__cdecl* GetAllRecordings)(VECRECORDINGS *results);
    PVR_ERROR (__cdecl* DeleteRecording)(const CTVRecordingInfoTag &recinfo);
    PVR_ERROR (__cdecl* RenameRecording)(const CTVRecordingInfoTag &recinfo, CStdString &newname);
    PVR_ERROR (__cdecl* RequestTimerList)(PVRHANDLE handle);
    PVR_ERROR (__cdecl* AddTimer)(const CTVTimerInfoTag &timerinfo);
    PVR_ERROR (__cdecl* DeleteTimer)(const CTVTimerInfoTag &timerinfo, bool force);
    PVR_ERROR (__cdecl* RenameTimer)(const CTVTimerInfoTag &timerinfo, CStdString &newname);
    PVR_ERROR (__cdecl* UpdateTimer)(const CTVTimerInfoTag &timerinfo);
    bool (__cdecl* OpenLiveStream)(unsigned int channel);
    void (__cdecl* CloseLiveStream)();
    int (__cdecl* ReadLiveStream)(BYTE* buf, int buf_size);
    int (__cdecl* GetCurrentClientChannel)();
    bool (__cdecl* SwitchChannel)(unsigned int channel);
    bool (__cdecl* OpenRecordedStream)(const CTVRecordingInfoTag &recinfo);
    void (__cdecl* CloseRecordedStream)(void);
    int (__cdecl* ReadRecordedStream)(BYTE* buf, int buf_size);
    __int64 (__cdecl* SeekRecordedStream)(__int64 pos, int whence);
    __int64 (__cdecl* LengthRecordedStream)(void);
    bool (__cdecl* TeletextPagePresent)(unsigned int channel, unsigned int Page, unsigned int subPage);
    bool (__cdecl* ReadTeletextPage)(BYTE *buf, unsigned int channel, unsigned int Page, unsigned int subPage);
  } PVRClient;

#ifdef __cplusplus
}
#endif

#endif //__PVRCLIENT_TYPES_H__
