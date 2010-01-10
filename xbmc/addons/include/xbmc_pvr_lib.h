#pragma once
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

#ifndef LIBPVR_H_
#define LIBPVR_H_

#include "xbmc_pvr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void PVR_register_me(ADDON_HANDLE hdl);
void PVR_event_callback(const PVR_EVENT event, const char* msg);
void PVR_transfer_epg_entry(const PVRHANDLE handle, const PVR_PROGINFO *epgentry);
void PVR_transfer_channel_entry(const PVRHANDLE handle, const PVR_CHANNEL *chan);
void PVR_transfer_timer_entry(const PVRHANDLE handle, const PVR_TIMERINFO *timer);
void PVR_transfer_recording_entry(const PVRHANDLE handle, const PVR_RECORDINGINFO *recording);

#ifdef __cplusplus
}
#endif

#endif /* LIBPVR_H_ */
