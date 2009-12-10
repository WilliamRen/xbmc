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
  Common data structures shared between XBMC and XBMC's visualisations
 */

#ifndef __VISUALISATION_TYPES_H__
#define __VISUALISATION_TYPES_H__

#include "libvisualisation.h"

extern "C"
{
  struct VIS_INFO
  {
    int bWantsFreq;
    int iSyncDelay;
  };

  struct VIS_PROPS
  {
    void *device;
    int x;
    int y;
    int width;
    int height;
    float pixelRatio;
    const char *name;
    const char *presets;
    const char *datastore;
  };

  enum VIS_ACTION
  { 
    VIS_ACTION_NONE = 0,
    VIS_ACTION_NEXT_PRESET,
    VIS_ACTION_PREV_PRESET,
    VIS_ACTION_LOAD_PRESET,
    VIS_ACTION_RANDOM_PRESET,
    VIS_ACTION_LOCK_PRESET,
    VIS_ACTION_RATE_PRESET_PLUS,
    VIS_ACTION_RATE_PRESET_MINUS,
    VIS_ACTION_UPDATE_ALBUMART,
    VIS_ACTION_UPDATE_TRACK
  };

  struct Visualisation
  {
    void (__cdecl* Start)(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName);
    void (__cdecl* AudioData)(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
    void (__cdecl* Render) ();
    void (__cdecl* Stop)();
    void (__cdecl* GetInfo)(VIS_INFO *info);
    bool (__cdecl* OnAction)(long flags, const void *param);
    int (__cdecl* HasPresets)();
    viz_preset_list_t (__cdecl *GetPresets)();
    unsigned (__cdecl *GetPreset)();
    bool (__cdecl* IsLocked)();
  };
}

#endif //__VISUALISATION_TYPES_H__
