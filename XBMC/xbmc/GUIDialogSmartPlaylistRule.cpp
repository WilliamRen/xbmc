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
#include "GUIDialogSmartPlaylistRule.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogFileBrowser.h"
#include "Util.h"
#include "MusicDatabase.h"
#include "VideoDatabase.h"
#include "GUIWindowManager.h"
#include "GUIDialogSelect.h"
#include "FileItem.h"

#define CONTROL_FIELD           15
#define CONTROL_OPERATOR        16
#define CONTROL_VALUE           17
#define CONTROL_OK              18
#define CONTROL_CANCEL          19
#define CONTROL_BROWSE          20

using namespace std;
using namespace PLAYLIST;

CGUIDialogSmartPlaylistRule::CGUIDialogSmartPlaylistRule(void)
    : CGUIDialog(WINDOW_DIALOG_SMART_PLAYLIST_RULE, "SmartPlaylistRule.xml")
{
  m_cancelled = false;
}

CGUIDialogSmartPlaylistRule::~CGUIDialogSmartPlaylistRule()
{
}

bool CGUIDialogSmartPlaylistRule::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
    m_cancelled = true;
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogSmartPlaylistRule::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_OK)
        OnOK();
      else if (iControl == CONTROL_CANCEL)
        OnCancel();
      else if (iControl == CONTROL_VALUE)
        OnValue();
      else if (iControl == CONTROL_OPERATOR)
        OnOperator();
      else if (iControl == CONTROL_FIELD)
        OnField();
      else if (iControl == CONTROL_BROWSE)
        OnBrowse();
      return true;
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogSmartPlaylistRule::OnOK()
{
  m_cancelled = false;
  Close();
}

void CGUIDialogSmartPlaylistRule::OnBrowse()
{
  CFileItemList items;
  CMusicDatabase database;
  database.Open();
  CVideoDatabase videodatabase;
  videodatabase.Open();

  VIDEODB_CONTENT_TYPE type = VIDEODB_CONTENT_MOVIES;
  if (m_type.Equals("tvshows"))
    type = VIDEODB_CONTENT_TVSHOWS;
  else if (m_type.Equals("musicvideos"))
    type = VIDEODB_CONTENT_MUSICVIDEOS;
  else if (m_type.Equals("episodes"))
    type = VIDEODB_CONTENT_EPISODES;

  int iLabel = 0;
  if (m_rule.m_field == CSmartPlaylistRule::FIELD_GENRE)
  {
    if (m_type.Equals("tvshows") || m_type.Equals("episodes") || m_type.Equals("movies"))
      videodatabase.GetGenresNav("videodb://2/1/",items,type);
    else if (m_type.Equals("songs") || m_type.Equals("albums") || m_type.Equals("mixed"))
      database.GetGenresNav("musicdb://4/",items);
    if (m_type.Equals("musicvideos") || m_type.Equals("mixed"))
    {
      CFileItemList items2;
      videodatabase.GetGenresNav("videodb://3/1/",items2,VIDEODB_CONTENT_MUSICVIDEOS);
      items.Append(items2);
      items2.ClearKeepPointer();
    }
    iLabel = 515;
  }
  else if (m_rule.m_field == CSmartPlaylistRule::FIELD_ARTIST || m_rule.m_field == CSmartPlaylistRule::FIELD_ALBUMARTIST)
  {
    if (m_type.Equals("songs") || m_type.Equals("mixed") || m_type.Equals("albums"))
      database.GetArtistsNav("musicdb://5/",items,-1,m_rule.m_field == CSmartPlaylistRule::FIELD_ALBUMARTIST);
    if (m_type.Equals("musicvideos") || m_type.Equals("mixed"))
    {
      CFileItemList items2;
      videodatabase.GetMusicVideoArtistsByName("",items2);
      items.Append(items2);
    }
    iLabel = 557;
  }
  else if (m_rule.m_field == CSmartPlaylistRule::FIELD_ALBUM)
  {
    if (m_type.Equals("songs") || m_type.Equals("mixed") || m_type.Equals("albums"))
      database.GetAlbumsNav("musicdb://6/",items,-1,-1);
    if (m_type.Equals("musicvideos") || m_type.Equals("mixed"))
    {
      CFileItemList items2;
      videodatabase.GetMusicVideoAlbumsByName("",items2);
      items.Append(items2);
    }
    iLabel = 558;
  }
  else if (m_rule.m_field == CSmartPlaylistRule::FIELD_ACTOR)
  {
    videodatabase.GetActorsNav("",items,type);
    iLabel = 20337;
  }
  else if (m_rule.m_field == CSmartPlaylistRule::FIELD_DIRECTOR)
  {
    videodatabase.GetDirectorsNav("",items,type);
  }
  else if (m_rule.m_field == CSmartPlaylistRule::FIELD_STUDIO)
  {
    videodatabase.GetStudiosNav("",items,type);
  }
  else if (m_rule.m_field == CSmartPlaylistRule::FIELD_WRITER)
  {
    videodatabase.GetWritersNav("",items,type);
  }
  else if (m_rule.m_field == CSmartPlaylistRule::FIELD_TVSHOWTITLE)
  {
    videodatabase.GetTvShowsNav("",items);
  }
  else
  { // TODO: Add browseability in here.
    assert(false);
  }

  CGUIDialogSelect* pDialog = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
  pDialog->Reset();
  pDialog->SetItems(&items);
  CStdString strHeading;
  strHeading.Format(g_localizeStrings.Get(13401),g_localizeStrings.Get(iLabel));
  pDialog->SetHeading(strHeading);
  pDialog->DoModal();
  if (pDialog->GetSelectedLabel() > -1)
  {
    m_rule.m_parameter = pDialog->GetSelectedLabelText();
    UpdateButtons();
  }
  pDialog->Reset();
}

void CGUIDialogSmartPlaylistRule::OnCancel()
{
  m_cancelled = true;
  Close();
}

void CGUIDialogSmartPlaylistRule::OnValue()
{
  CStdString value(m_rule.m_parameter);
  switch (CSmartPlaylistRule::GetFieldType(m_rule.m_field))
  {
  case CSmartPlaylistRule::TEXT_FIELD:
  case CSmartPlaylistRule::BROWSEABLE_FIELD:
    if (CGUIDialogKeyboard::ShowAndGetInput(value, g_localizeStrings.Get(21420), false))
      m_rule.m_parameter = value;
    break;
  case CSmartPlaylistRule::DATE_FIELD:
    if (m_rule.m_operator == CSmartPlaylistRule::OPERATOR_IN_THE_LAST)
    {
      if (CGUIDialogKeyboard::ShowAndGetInput(value, g_localizeStrings.Get(21420), false))
        m_rule.m_parameter = value;
    }
    else
    {
      CDateTime dateTime;
      dateTime.SetFromDBDate(m_rule.m_parameter);
      if (dateTime < CDateTime(2000,1, 1, 0, 0, 0))
        dateTime = CDateTime(2000, 1, 1, 0, 0, 0);
      SYSTEMTIME date;
      dateTime.GetAsSystemTime(date);
      if (CGUIDialogNumeric::ShowAndGetDate(date, g_localizeStrings.Get(21420)))
      {
        dateTime = CDateTime(date);
        m_rule.m_parameter = dateTime.GetAsDBDate();
      }
    }
    break;
  case CSmartPlaylistRule::SECONDS_FIELD:
    if (CGUIDialogNumeric::ShowAndGetSeconds(value, g_localizeStrings.Get(21420)))
      m_rule.m_parameter = value;
    break;
  case CSmartPlaylistRule::NUMERIC_FIELD:
    if (CGUIDialogNumeric::ShowAndGetNumber(value, g_localizeStrings.Get(21420)))
      m_rule.m_parameter = value;
    break;
  case CSmartPlaylistRule::PLAYLIST_FIELD:
    // use filebrowser to grab another smart playlist

    // Note: This can cause infinite loops (playlist that refers to the same playlist) but I don't
    //       think there's any decent way to deal with this, as the infinite loop may be an arbitrary
    //       number of playlists deep, eg playlist1 -> playlist2 -> playlist3 ... -> playlistn -> playlist1
    CStdString path = "special://videoplaylists/";
    if (m_type.Equals("songs") || m_type.Equals("albums"))
      path = "special://musicplaylists/";
    if (CGUIDialogFileBrowser::ShowAndGetFile(path, ".xsp", g_localizeStrings.Get(656), path))
    {
      CSmartPlaylist playlist;
      if (playlist.Load(path))
        m_rule.m_parameter = !playlist.GetName().IsEmpty() ? playlist.GetName() : CUtil::GetTitleFromPath(path);
    }
    break;
  }
  UpdateButtons();
}

void CGUIDialogSmartPlaylistRule::OnField()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_FIELD);
  OnMessage(msg);
  m_rule.m_field = (CSmartPlaylistRule::DATABASE_FIELD)msg.GetParam1();

  UpdateButtons();
}

void CGUIDialogSmartPlaylistRule::OnOperator()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_OPERATOR);
  OnMessage(msg);
  m_rule.m_operator = (CSmartPlaylistRule::SEARCH_OPERATOR)msg.GetParam1();

  UpdateButtons();
}

void CGUIDialogSmartPlaylistRule::UpdateButtons()
{
  if (m_rule.m_parameter.size() == 0)
  {
    CONTROL_DISABLE(CONTROL_OK)
  }
  else
  {
    CONTROL_ENABLE(CONTROL_OK)
  }

  // update the field control
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_FIELD, m_rule.m_field);
  OnMessage(msg);
  CGUIMessage msg2(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_FIELD);
  OnMessage(msg2);
  m_rule.m_field = (CSmartPlaylistRule::DATABASE_FIELD)msg2.GetParam1();

  // and now update the operator set
  CGUIMessage reset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_OPERATOR);
  OnMessage(reset);

  CONTROL_DISABLE(CONTROL_BROWSE);
  switch (CSmartPlaylistRule::GetFieldType(m_rule.m_field))
  {
  case CSmartPlaylistRule::BROWSEABLE_FIELD:
    CONTROL_ENABLE(CONTROL_BROWSE);
    // fall through...
  case CSmartPlaylistRule::TEXT_FIELD:
    // text fields - add the usual comparisons
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_EQUALS);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_DOES_NOT_EQUAL);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_CONTAINS);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_DOES_NOT_CONTAIN);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_STARTS_WITH);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_ENDS_WITH);
    break;

  case CSmartPlaylistRule::NUMERIC_FIELD:
  case CSmartPlaylistRule::SECONDS_FIELD:
    // numerical fields - less than greater than
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_EQUALS);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_DOES_NOT_EQUAL);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_GREATER_THAN);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_LESS_THAN);
    break;

  case CSmartPlaylistRule::DATE_FIELD:
    // date field
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_AFTER);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_BEFORE);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_IN_THE_LAST);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_NOT_IN_THE_LAST);
    break;

  case CSmartPlaylistRule::PLAYLIST_FIELD:
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_EQUALS);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_DOES_NOT_EQUAL);
    break;
  }

  // check our operator is valid, and update if not
  CGUIMessage select(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_OPERATOR, m_rule.m_operator);
  OnMessage(select);
  CGUIMessage selected(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_OPERATOR);
  OnMessage(selected);
  m_rule.m_operator = (CSmartPlaylistRule::SEARCH_OPERATOR)selected.GetParam1();

  SET_CONTROL_LABEL(CONTROL_VALUE, m_rule.m_parameter);
}

void CGUIDialogSmartPlaylistRule::AddOperatorLabel(CSmartPlaylistRule::SEARCH_OPERATOR op)
{
  CGUIMessage select(GUI_MSG_LABEL_ADD, GetID(), CONTROL_OPERATOR, op);
  select.SetLabel(CSmartPlaylistRule::GetLocalizedOperator(op));
  OnMessage(select);
}

void CGUIDialogSmartPlaylistRule::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
  // add the fields to the field spincontrol
  vector<CSmartPlaylistRule::DATABASE_FIELD> fields = CSmartPlaylistRule::GetFields(m_type);
  for (unsigned int i = 0; i < fields.size(); i++)
  {
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_FIELD, fields[i]);
    msg.SetLabel(CSmartPlaylistRule::GetLocalizedField(fields[i]));
    OnMessage(msg);
  }
  UpdateButtons();
}

bool CGUIDialogSmartPlaylistRule::EditRule(CSmartPlaylistRule &rule, const CStdString& type)
{
  CGUIDialogSmartPlaylistRule *editor = (CGUIDialogSmartPlaylistRule *)m_gWindowManager.GetWindow(WINDOW_DIALOG_SMART_PLAYLIST_RULE);
  if (!editor) return false;
  
  editor->m_rule = rule;
  editor->m_type = type;
  editor->DoModal(m_gWindowManager.GetActiveWindow());
  rule = editor->m_rule;
  return !editor->m_cancelled;
}
