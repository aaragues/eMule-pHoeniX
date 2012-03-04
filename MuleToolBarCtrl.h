#pragma once

#define IDC_TOOLBAR			16127
#define IDC_TOOLBARBUTTON	16129

#define	MULE_TOOLBAR_BAND_NR	0

enum EToolbarLabelType {
	NoLabels	= 0,
	LabelsBelow = 1,
	LabelsRight = 2
};

class CMuleToolbarCtrl : public CToolBarCtrl
{
	DECLARE_DYNAMIC(CMuleToolbarCtrl)

public:
	CMuleToolbarCtrl();
	virtual ~CMuleToolbarCtrl();

	void Init();
	void Localize();
	void Refresh();
	void SaveCurHeight();
	void UpdateBackground();
	void PressMuleButton(int nID);

	static int GetDefaultLabelType() { return (int)LabelsBelow; }

protected:
	CSize		m_sizBtnBmp;
	int			m_iPreviousHeight;
	int			m_iLastPressedButton;
	int			m_buttoncount;
	TBBUTTON	TBButtons[12];
	TCHAR		TBStrings[12][200];
	CStringArray m_astrToolbarPaths;
	EToolbarLabelType m_eLabelType;
	CStringArray m_astrSkinPaths;
	CBitmap		m_bmpBack;

	void ChangeToolbarBitmap(const CString& rstrPath, bool bRefresh);
	void ChangeTextLabelStyle(EToolbarLabelType eLabelType, bool bRefresh, bool bForceUpdateButtons = false);
	void UpdateIdealSize();
	void SetAllButtonsStrings();
	void SetAllButtonsWidth();
	void ForceRecalcLayout();

#ifdef _DEBUG
	void Dump();
#endif

	void AutoSize();
	virtual		BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSysColorChange();
	afx_msg void OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTbnQueryDelete(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTbnQueryInsert(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTbnGetButtonInfo(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTbnToolbarChange(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTbnReset(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTbnInitCustomize(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTbnEndAdjust(NMHDR* pNMHDR, LRESULT* pResult);
};