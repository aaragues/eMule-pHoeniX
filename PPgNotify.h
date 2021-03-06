//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#pragma once

class CPPgNotify : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgNotify)

public:
	CPPgNotify();
	virtual ~CPPgNotify();	

	void Localize(void);

// Dialog Data
	enum { IDD = IDD_PPG_NOTIFY };

protected:
	void LoadSettings(void);

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	CListBox      tnbConfigList;

	CString CPPgNotify::DialogBrowseFile(CString Filters, CString DefaultFileName=_T(""));
	void LoadNotifierConfigurations();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnLbnSelchangeLboxTbnConfig()  { SetModified(); }; // added by enkeyDEV(kei-kun) 21/11/2002
	afx_msg void OnSettingsChange() { SetModified(); }
	afx_msg void OnBnClickedCbTbnOnchat();
	afx_msg void OnBnClickedBtnBrowseWav();
    afx_msg void OnDestroy();
	afx_msg void OnHelp();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
};

// [TPT] - enkeyDEV(th1) -notifier-
