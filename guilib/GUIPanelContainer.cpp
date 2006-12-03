#include "include.h"
#include "GUIPanelContainer.h"
#include "GUIListItem.h"

CGUIPanelContainer::CGUIPanelContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime)
    : CGUIBaseContainer(dwParentID, dwControlId, posX, posY, width, height, orientation, scrollTime)
{
  ControlType = GUICONTAINER_PANEL;
//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  m_spinControl = NULL;
  m_largePanel = NULL;
//#endif
}

CGUIPanelContainer::~CGUIPanelContainer(void)
{
}

void CGUIPanelContainer::Render()
{
  if (!IsVisible()) return CGUIBaseContainer::Render();

  ValidateOffset();

  if (m_bInvalidated)
    UpdateLayout();

  m_scrollOffset += m_scrollSpeed * (m_renderTime - m_scrollLastTime);
  if ((m_scrollSpeed < 0 && m_scrollOffset < m_offset * m_layout.Size(m_orientation)) ||
      (m_scrollSpeed > 0 && m_scrollOffset > m_offset * m_layout.Size(m_orientation)))
  {
    m_scrollOffset = m_offset * m_layout.Size(m_orientation);
    m_scrollSpeed = 0;
  }
  m_scrollLastTime = m_renderTime;

  int offset = (int)(m_scrollOffset / m_layout.Size(m_orientation));

  // Free memory not used on screen at the moment, do this first so there's more memory for the new items.
  FreeMemory(CorrectOffset(offset * m_itemsPerRow), CorrectOffset((offset + m_itemsPerPage) * m_itemsPerRow));

  g_graphicsContext.SetViewPort(m_posX, m_posY, m_width, m_height);
  float posX = m_posX;
  float posY = m_posY;
  if (m_orientation == VERTICAL)
    posY += (offset * m_layout.Size(m_orientation) - m_scrollOffset);
  else
    posX += (offset * m_layout.Size(m_orientation) - m_scrollOffset);;

  float focusedPosX = 0;
  float focusedPosY = 0;
  CGUIListItem *focusedItem = NULL;
  int current = offset * m_itemsPerRow;
  int row = 1;
  while (posX < m_posX + m_width && posY < m_posY + m_height && m_items.size())
  {
    if (current >= (int)m_items.size())
      break;
    CGUIListItem *item = m_items[current];
    bool focused = (current == m_offset * m_itemsPerRow + m_cursor) && m_bHasFocus;
    // render our item
    if (focused)
    {
      focusedPosX = posX;
      focusedPosY = posY;
      focusedItem = item;
    }
    else
      RenderItem(posX, posY, item, focused);

    // increment our position
    if (row < m_itemsPerRow)
    {
      if (m_orientation == VERTICAL)
        posX += m_layout.Size(HORIZONTAL);
      else
        posY += m_layout.Size(VERTICAL);
      row++;
    }
    else
    {
      if (m_orientation == VERTICAL)
      {
        posY += m_layout.Size(VERTICAL);
        posX -= m_layout.Size(HORIZONTAL) * (m_itemsPerRow - 1);
      }
      else
      {
        posX += m_layout.Size(HORIZONTAL);
        posX -= m_layout.Size(VERTICAL) * (m_itemsPerRow - 1);
      }
      row = 1;
    }
    current++;
  }
  // and render the focused item last (for overlapping purposes)
  if (focusedItem)
    RenderItem(focusedPosX, focusedPosY, focusedItem, true);

  g_graphicsContext.RestoreViewPort();

  if (m_pageControl)
  { // tell our pagecontrol (scrollbar or whatever) to update
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, offset);
    SendWindowMessage(msg);
  }
  CGUIBaseContainer::Render();
}

bool CGUIPanelContainer::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_PAGE_UP:
    {
      if (m_offset == 0)
      { // already on the first page, so move to the first item
        m_cursor = 0;
      }
      else
      { // scroll up to the previous page
        Scroll( -m_itemsPerPage);
      }
      return true;
    }
    break;
  case ACTION_PAGE_DOWN:
    {
      if ((m_offset + m_itemsPerPage) * m_itemsPerRow >= (int)m_items.size() || (int)m_items.size() < m_itemsPerPage)
      { // already at the last page, so move to the last item.
        m_cursor = m_items.size() - m_offset * m_itemsPerRow - 1;
        if (m_cursor < 0) m_cursor = 0;
      }
      else
      { // scroll down to the next page
        Scroll(m_itemsPerPage);
      }
      return true;
    }
    break;
    // smooth scrolling (for analog controls)
  case ACTION_SCROLL_UP:
    {
      m_analogScrollCount += action.fAmount1 * action.fAmount1;
      bool handled = false;
      while (m_analogScrollCount > 0.4)
      {
        handled = true;
        m_analogScrollCount -= 0.4f;
        if (m_offset > 0 && m_cursor <= m_itemsPerPage * m_itemsPerRow / 2)
        {
          Scroll(-1);
        }
        else if (m_cursor > 0)
        {
          m_cursor--;
        }
      }
      return handled;
    }
    break;
  case ACTION_SCROLL_DOWN:
    {
      m_analogScrollCount += action.fAmount1 * action.fAmount1;
      bool handled = false;
      while (m_analogScrollCount > 0.4)
      {
        handled = true;
        m_analogScrollCount -= 0.4f;
        if ((m_offset + m_itemsPerPage) * m_itemsPerRow < (int)m_items.size() && m_cursor >= m_itemsPerPage * m_itemsPerRow / 2)
        {
          Scroll(1);
        }
        else if (m_cursor < m_itemsPerPage - 1 && m_offset * m_itemsPerRow + m_cursor < (int)m_items.size() - 1)
        {
          m_cursor++;
        }
      }
      return handled;
    }
    break;
  }
  return CGUIBaseContainer::OnAction(action);
}

bool CGUIPanelContainer::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_ADD)
    {
      // from baseclass
      CGUIListItem* item = (CGUIListItem*)message.GetLPVOID();
      m_items.push_back(item);
      if (m_pageControl)
      {
        CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, GetRows());
        SendWindowMessage(msg);
      }
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_cursor = 0;
      // from baseclass
      m_items.clear();
      if (m_pageControl)
      {
        CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, GetRows());
        SendWindowMessage(msg);
      }
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
    {
      // from baseclass
      message.SetParam1(CorrectOffset(m_offset * m_itemsPerRow + m_cursor));
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
    {
      // Check that m_offset is valid
      ValidateOffset();
      // only select an item if it's in a valid range
      if (message.GetParam1() >= 0 && message.GetParam1() < (int)m_items.size())
      {
        // Select the item requested
        int item = message.GetParam1();
        if (item >= m_offset * m_itemsPerRow && item < (m_offset + m_itemsPerPage) * m_itemsPerRow)
        { // the item is on the current page, so don't change it.
          m_cursor = item - m_offset * m_itemsPerRow;
        }
        else if (item < m_offset * m_itemsPerRow)
        { // item is on a previous page - make it the first item on the page
          m_cursor = item % m_itemsPerRow;
          ScrollToOffset((item - m_cursor) / m_itemsPerRow);
        }
        else // (item >= m_offset+m_itemsPerPage)
        { // item is on a later page - make it the last row on the page
          m_cursor = item % m_itemsPerRow + m_itemsPerRow * (m_itemsPerPage - 1);
          ScrollToOffset((item - m_cursor) / m_itemsPerRow);
        }
      }
      return true;
    }
  }
  return CGUIBaseContainer::OnMessage(message);
}

void CGUIPanelContainer::OnLeft()
{
  if (m_orientation == VERTICAL && MoveLeft(m_dwControlLeft))
    return;
  if (m_orientation == HORIZONTAL && MoveUp(m_dwControlLeft))
    return;
  CGUIControl::OnLeft();
}

void CGUIPanelContainer::OnRight()
{
  if (m_orientation == VERTICAL && MoveRight(m_dwControlRight))
    return;
  if (m_orientation == HORIZONTAL && MoveDown(m_dwControlRight))
    return;
  return CGUIControl::OnRight();
}

void CGUIPanelContainer::OnUp()
{
  if (m_orientation == VERTICAL && MoveUp(m_dwControlUp))
    return;
  if (m_orientation == HORIZONTAL && MoveLeft(m_dwControlUp))
    return;
  CGUIControl::OnUp();
}

void CGUIPanelContainer::OnDown()
{
  if (m_orientation == VERTICAL && MoveDown(m_dwControlDown))
    return;
  if (m_orientation == HORIZONTAL && MoveRight(m_dwControlDown))
    return;
  return CGUIControl::OnDown();
}

bool CGUIPanelContainer::MoveDown(DWORD nextControl)
{
  if (m_cursor + m_itemsPerRow < m_itemsPerPage * m_itemsPerRow && (m_offset + 1) * m_itemsPerRow + m_cursor < (int)m_items.size())
    m_cursor += m_itemsPerRow;
  else if ((m_offset + 1) * m_itemsPerRow + m_cursor < (int)m_items.size())
    ScrollToOffset(m_offset + 1);
  else if (!nextControl || nextControl == GetID())
  { // move first item in list
    m_cursor %= m_itemsPerRow;
    ScrollToOffset(0);
  }
  else
    return false;
  return true;
}

bool CGUIPanelContainer::MoveUp(DWORD nextControl)
{
  if (m_cursor >= m_itemsPerRow)
    m_cursor -= m_itemsPerRow;
  else if (m_offset > 0)
    ScrollToOffset(m_offset - 1);
  else if (!nextControl || nextControl == GetID())
  { // move last item in list in this column
    m_cursor %= m_itemsPerRow;
    m_cursor += (m_itemsPerPage - 1) * m_itemsPerRow;
    int offset = (int)GetRows() - m_itemsPerPage;
    // should check here whether cursor is actually allowed here, and reduce accordingly
    if (offset * m_itemsPerRow + m_cursor >= (int)m_items.size())
      m_cursor = (int)m_items.size() - offset * m_itemsPerRow;
    ScrollToOffset(offset);
  }
  else
    return false;
  return true;
}

bool CGUIPanelContainer::MoveLeft(DWORD nextControl)
{
  int col = m_cursor % m_itemsPerRow;
  if (col > 0)
    m_cursor--;
  else if (!nextControl || nextControl == GetID())
  { // wrap around
    m_cursor += (m_itemsPerRow - 1);
    if (m_offset * m_itemsPerRow + m_cursor >= (int)m_items.size())
      m_cursor = (int)m_items.size() - m_offset * m_itemsPerRow;
  }
  else
    return false;
  return true;
}

bool CGUIPanelContainer::MoveRight(DWORD nextControl)
{
  int col = m_cursor % m_itemsPerRow;
  if (col + 1 < m_itemsPerRow && m_offset * m_itemsPerRow + m_cursor + 1 < (int)m_items.size())
    m_cursor++;
  else if (!nextControl || nextControl == GetID()) // move first item in row
    m_cursor -= col;
  else
    return false;
  return true;
}

// scrolls the said amount
void CGUIPanelContainer::Scroll(int amount)
{
  // increase or decrease the offset
  m_offset += amount;
  if (m_offset > ((int)GetRows() - m_itemsPerPage) * m_itemsPerRow)
  {
    m_offset = ((int)GetRows() - m_itemsPerPage) * m_itemsPerRow;
  }
  if (m_offset < 0) m_offset = 0;
  ScrollToOffset(m_offset);
}

void CGUIPanelContainer::ValidateOffset()
{ // first thing is we check the range of m_offset
  if (m_offset > (int)GetRows() - m_itemsPerPage)
  {
    m_offset = (int)GetRows() - m_itemsPerPage;
    m_scrollOffset = m_offset * m_layout.Size(m_orientation);
  }
  if (m_offset < 0)
  {
    m_offset = 0;
    m_scrollOffset = 0;
  }
}

void CGUIPanelContainer::UpdateLayout()
{
  // calculate the number of items to display
  if (m_orientation == HORIZONTAL)
  {
    m_itemsPerRow = (int)(m_height / m_layout.Size(VERTICAL));
    m_itemsPerPage = (int)(m_width / m_layout.Size(HORIZONTAL));
  }
  else
  {
    m_itemsPerRow = (int)(m_width / m_layout.Size(HORIZONTAL));
    m_itemsPerPage = (int)(m_height / m_layout.Size(VERTICAL));
  }
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, GetRows());
  SendWindowMessage(msg);
}

unsigned int CGUIPanelContainer::GetRows()
{
  return (m_items.size() + m_itemsPerRow - 1) / m_itemsPerRow;
}

//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
CGUIPanelContainer::CGUIPanelContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
                         const CImage& imageNoFocus, const CImage& imageFocus,
                         float itemWidth, float itemHeight,
                         float textureWidth, float textureHeight,
                         float thumbPosX, float thumbPosY, float thumbWidth, float thumbHeight, DWORD thumbAlign, CGUIImage::GUIIMAGE_ASPECT_RATIO thumbAspect,
                         const CLabelInfo& labelInfo, bool hideLabels,
                         CGUIControl *pSpin, CGUIControl *pPanel)
: CGUIBaseContainer(dwParentID, dwControlId, posX, posY, width, height, VERTICAL, 200) 
{
  m_layout.CreateThumbnailPanelLayouts(itemWidth, itemHeight, false, imageNoFocus, textureWidth, textureHeight, thumbPosX, thumbPosY, thumbWidth, thumbHeight, thumbAlign, thumbAspect, labelInfo, hideLabels);
  m_focusedLayout.CreateThumbnailPanelLayouts(itemWidth, itemHeight, true, imageFocus, textureWidth, textureHeight, thumbPosX, thumbPosY, thumbWidth, thumbHeight, thumbAlign, thumbAspect, labelInfo, hideLabels);
  m_height -= 5;
  int numItems = (int)(m_height / itemHeight);
  m_height = numItems * itemHeight;
  m_spinControl = pSpin;
  m_largePanel = pPanel;
  ControlType = GUICONTAINER_PANEL;
}
//#endif
