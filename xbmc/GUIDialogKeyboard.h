#pragma once
#include "guiDialog.h"

enum KEYBOARD {CAPS, LOWER, SYMBOLS };

class CGUIDialogKeyboard: public CGUIDialog
{

public:
	CGUIDialogKeyboard(void);
	virtual ~CGUIDialogKeyboard(void);

	void SetHeading(CStdString& strHeading) {m_strHeading = strHeading;} ;
	void SetText(CStdString& aTextString);
	CStdString GetText() const {return m_strEdit;};
	bool IsDirty() { return m_bDirty; };

	static bool ShowAndGetInput(CStdString& aTextString, bool allowEmptyResult);
	static bool ShowAndGetInput(CStdString& aTextString, CStdString &strHeading, bool allowEmptyResult);

	virtual void	Close();

protected:
	
	virtual void OnInitWindow();
	virtual void OnAction(const CAction &action);
	virtual bool OnMessage(CGUIMessage& message);
private:

	void OnClickButton(int iButtonControl);
	void OnRemoteNumberClick(int key);
	void		UpdateButtons();
	WCHAR		GetCharacter(int iButton);

	void Character(WCHAR wch);
	void Backspace();

	CStdString	m_strEdit;
	bool m_bDirty;
	KEYBOARD m_keyType;
	int m_iMode;
	bool m_bShift;

	DWORD	m_lastRemoteClickTime;
	WORD	m_lastRemoteKeyClicked;
	int		m_indexInSeries;
	CStdString	m_strHeading;
	static const char* s_charsSeries[10];
};
