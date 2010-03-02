#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "xbmc_addon_types.h"
#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>

class DllSetting
{
public:
  enum SETTING_TYPE { NONE=0, CHECK, SPIN };

  DllSetting(SETTING_TYPE t, const char *n, const char *l)
  {
    id = NULL;
    label = NULL;
    if (n)
    {
      id = new char[strlen(n)+1];
      strcpy(id, n);
    }
    if (l)
    {
      label = new char[strlen(l)+1];
      strcpy(label, l);
    }
    current = 0;
    type = t;
  }

  DllSetting(const DllSetting &rhs) // copy constructor
  {
    id = NULL;
    label = NULL;
    if (rhs.id)
    {
      id = new char[strlen(rhs.id)+1];
      strcpy(id, rhs.id);
    }
    if (rhs.label)
    {
      label = new char[strlen(rhs.label)+1];
      strcpy(label, rhs.label);
    }
    current = rhs.current;
    type = rhs.type;
    for (unsigned int i = 0; i < rhs.entry.size(); i++)
    {
      char *lab = new char[strlen(rhs.entry[i]) + 1];
      strcpy(lab, rhs.entry[i]);
      entry.push_back(lab);
    }
  }

  ~DllSetting()
  {
    delete[] id;
    delete[] label;
    for (unsigned int i=0; i < entry.size(); i++)
      delete[] entry[i];
  }

  void AddEntry(const char *label)
  {
    if (!label || type != SPIN) return;
    char *lab = new char[strlen(label) + 1];
    strcpy(lab, label);
    entry.push_back(lab);
  }

  // data members
  SETTING_TYPE type;
  char*        id;
  char*        label;
  int          current;
  std::vector<const char *> entry;
};

class DllUtils
{
public:

  static unsigned int VecToStruct(std::vector<DllSetting> &vecSet, StructSetting*** sSet)
  {
    *sSet = NULL;
    if(vecSet.size() == 0)
      return 0;

    unsigned int uiElements=0;

    *sSet = (StructSetting**)malloc(vecSet.size()*sizeof(StructSetting*));
    for(unsigned int i=0;i<vecSet.size();i++)
    {
      (*sSet)[i] = NULL;
      (*sSet)[i] = (StructSetting*)malloc(sizeof(StructSetting));
      (*sSet)[i]->id = NULL;
      (*sSet)[i]->label = NULL;
      uiElements++;

      if (vecSet[i].id && vecSet[i].label)
      {
        (*sSet)[i]->id = strdup(vecSet[i].id);
        (*sSet)[i]->label = strdup(vecSet[i].label);
        (*sSet)[i]->type = vecSet[i].type;
        (*sSet)[i]->current = vecSet[i].current;
        (*sSet)[i]->entry_elements = 0;
        (*sSet)[i]->entry = NULL;
        if(vecSet[i].type == DllSetting::SPIN && vecSet[i].entry.size() > 0)
        {
          (*sSet)[i]->entry = (char**)malloc(vecSet[i].entry.size()*sizeof(char**));
          for(unsigned int j=0;j<vecSet[i].entry.size();j++)
          {
            if(strlen(vecSet[i].entry[j]) > 0)
            {
              (*sSet)[i]->entry[j] = strdup(vecSet[i].entry[j]);
              (*sSet)[i]->entry_elements++;
            }
          }
        }
      }
    }
    return uiElements;
  }

  static void StructToVec(unsigned int iElements, StructSetting*** sSet, std::vector<DllSetting> *vecSet)
  {
    if(iElements == 0)
      return;

    vecSet->clear();
    for(unsigned int i=0;i<iElements;i++)
    {
      DllSetting vSet((DllSetting::SETTING_TYPE)(*sSet)[i]->type, (*sSet)[i]->id, (*sSet)[i]->label);
      if((*sSet)[i]->type == DllSetting::SPIN)
      {
        for(unsigned int j=0;j<(*sSet)[i]->entry_elements;j++)
        {
            vSet.AddEntry((*sSet)[i]->entry[j]);
        }
      }
      vSet.current = (*sSet)[i]->current;
      vecSet->push_back(vSet);
    }
  }

  static void FreeStruct(unsigned int iElements, StructSetting*** sSet)
  {
    if(iElements == 0)
      return;

    for(unsigned int i=0;i<iElements;i++)
    {
      if((*sSet)[i]->type == DllSetting::SPIN)
      {
        for(unsigned int j=0;j<(*sSet)[i]->entry_elements;j++)
        {
          free((*sSet)[i]->entry[j]);
        }
        free((*sSet)[i]->entry);
      }
      free((*sSet)[i]->id);
      free((*sSet)[i]->label);
      free((*sSet)[i]);
    }
    free(*sSet);
  }
};

typedef enum addon_log {
  LOG_DEBUG,
  LOG_INFO,
  LOG_NOTICE,
  LOG_ERROR
} addon_log_t;

typedef enum queue_msg {
  QUEUE_INFO,
  QUEUE_WARNING,
  QUEUE_ERROR
} queue_msg_t;

void XBMC_register_me(ADDON_HANDLE hdl);
bool XBMC_get_setting(std::string settingName, void *settingValue);
void XBMC_log(const addon_log_t loglevel, const char *format, ... );
void XBMC_queue_notification(const queue_msg_t type, const char *format, ... );
void XBMC_unknown_to_utf8(std::string &str);
