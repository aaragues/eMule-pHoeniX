//this file is part of eMule
//Copyright (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
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
#include "stdafx.h"
#include "emule.h"
#include "CatDialog.h"
#include "Preferences.h"
#include "otherfunctions.h"
#include "SharedFileList.h"
#include "emuledlg.h"
#include "TransferWnd.h"
#include "UserMsgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

// CCatDialog dialog

IMPLEMENT_DYNAMIC(CCatDialog, CDialog)
CCatDialog::CCatDialog(int index)
	: CDialog(CCatDialog::IDD, 0)
{
	m_myCat=thePrefs.GetCategory(index);
	if (m_myCat==NULL) return;
}

BOOL CCatDialog::OnInitDialog(){
	CDialog::OnInitDialog();
	InitWindowStyles(this);
	SetIcon(theApp.LoadIcon(_T("CATEGORY"),16,16),FALSE);
	Localize();
	UpdateData();
	m_bCancelled=false;

	return true;
}

void CCatDialog::UpdateData(){
	GetDlgItem(IDC_TITLE)->SetWindowText(m_myCat->title);
	GetDlgItem(IDC_INCOMING)->SetWindowText(m_myCat->incomingpath);
	GetDlgItem(IDC_COMMENT)->SetWindowText(m_myCat->comment);

	COLORREF selcolor=m_myCat->color;
	newcolor=m_myCat->color;
	m_ctlColor.SetColor(selcolor);

	// HoaX_69: AutoCat
	// [TPT] - khaos::categorymod+
	//GetDlgItem(IDC_AUTOCATEXT)->SetWindowText(m_myCat->autocat); 
	GetDlgItem(IDC_AUTOCATEXT)->SetWindowText(m_myCat->viewfilters.sAdvancedFilterMask); // Morph

	m_prio.SetCurSel(m_myCat->prio+1);

	CString buffer;
	GetDlgItem(IDC_FS_MIN)->SetWindowText(CastItoUIXBytes(m_myCat->viewfilters.nFSizeMin));
	GetDlgItem(IDC_FS_MAX)->SetWindowText(CastItoUIXBytes(m_myCat->viewfilters.nFSizeMax));
	GetDlgItem(IDC_RS_MIN)->SetWindowText(CastItoUIXBytes(m_myCat->viewfilters.nRSizeMin));
	GetDlgItem(IDC_RS_MAX)->SetWindowText(CastItoUIXBytes(m_myCat->viewfilters.nRSizeMax));
	buffer.Format(_T("%u"), m_myCat->viewfilters.nTimeRemainingMin >= 60 ? (m_myCat->viewfilters.nTimeRemainingMin / 60) : 0);
	GetDlgItem(IDC_RT_MIN)->SetWindowText(buffer);
	buffer.Format(_T("%u"), m_myCat->viewfilters.nTimeRemainingMax >= 60 ? (m_myCat->viewfilters.nTimeRemainingMax / 60) : 0);
	GetDlgItem(IDC_RT_MAX)->SetWindowText(buffer);
	buffer.Format(_T("%u"), m_myCat->viewfilters.nSourceCountMin);
	GetDlgItem(IDC_SC_MIN)->SetWindowText(buffer);
	buffer.Format(_T("%u"), m_myCat->viewfilters.nSourceCountMax);
	GetDlgItem(IDC_SC_MAX)->SetWindowText(buffer);
	buffer.Format(_T("%u"), m_myCat->viewfilters.nAvailSourceCountMin);
	GetDlgItem(IDC_ASC_MIN)->SetWindowText(buffer);
	buffer.Format(_T("%u"), m_myCat->viewfilters.nAvailSourceCountMax);
	GetDlgItem(IDC_ASC_MAX)->SetWindowText(buffer);

	CheckDlgButton(IDC_CHECK_FS, m_myCat->selectioncriteria.bFileSize?1:0);
	CheckDlgButton(IDC_CHECK_MASK, m_myCat->selectioncriteria.bAdvancedFilterMask?1:0);
	// [TPT] - khaos::categorymod-
}

CCatDialog::~CCatDialog()
{
}

void CCatDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CATCOLOR, m_ctlColor);
	DDX_Control(pDX, IDC_PRIOCOMBO, m_prio);
}


BEGIN_MESSAGE_MAP(CCatDialog, CDialog)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_MESSAGE(UM_CPN_SELENDOK, OnSelChange) //CPN_SELCHANGE
END_MESSAGE_MAP()


// CCatDialog message handlers

void CCatDialog::Localize(){
	if(m_hWnd) {
	GetDlgItem(IDC_STATIC_TITLE)->SetWindowText(GetResString(IDS_TITLE));
	GetDlgItem(IDC_STATIC_INCOMING)->SetWindowText(GetResString(IDS_PW_INCOMING));
	GetDlgItem(IDC_STATIC_COMMENT)->SetWindowText(GetResString(IDS_COMMENT));
	GetDlgItem(IDCANCEL)->SetWindowText(GetResString(IDS_CANCEL));
	GetDlgItem(IDC_STATIC_COLOR)->SetWindowText(GetResString(IDS_COLOR));
	GetDlgItem(IDC_STATIC_PRIO)->SetWindowText(GetResString(IDS_STARTPRIO));
	//GetDlgItem(IDC_STATIC_AUTOCAT)->SetWindowText(GetResString(IDS_AUTOCAT_LABEL)); // [TPT] - khaos::categorymod-
	// [TPT] - khaos::categorymod+
	GetDlgItem(IDC_CHECK_MASK)->SetWindowText(GetResString(IDS_CAT_AUTOCAT));
	GetDlgItem(IDC_STATIC_MIN)->SetWindowText(GetResString(IDS_CAT_MINIMUM));
	GetDlgItem(IDC_STATIC_MAX)->SetWindowText(GetResString(IDS_CAT_MAXIMUM));
	GetDlgItem(IDC_STATIC_MIN2)->SetWindowText(GetResString(IDS_CAT_MINIMUM));
	GetDlgItem(IDC_STATIC_MAX2)->SetWindowText(GetResString(IDS_CAT_MAXIMUM));
	GetDlgItem(IDC_STATIC_MIN3)->SetWindowText(GetResString(IDS_CAT_MINIMUM));
	GetDlgItem(IDC_STATIC_MAX3)->SetWindowText(GetResString(IDS_CAT_MAXIMUM));
	GetDlgItem(IDC_STATIC_MIN4)->SetWindowText(GetResString(IDS_CAT_MINIMUM));
	GetDlgItem(IDC_STATIC_MAX4)->SetWindowText(GetResString(IDS_CAT_MAXIMUM));
	GetDlgItem(IDC_STATIC_MIN5)->SetWindowText(GetResString(IDS_CAT_MINIMUM));
	GetDlgItem(IDC_STATIC_MAX5)->SetWindowText(GetResString(IDS_CAT_MAXIMUM));
	GetDlgItem(IDC_STATIC_EXP)->SetWindowText(GetResString(IDS_CAT_DLGHELP));
	GetDlgItem(IDC_STATIC_FSIZE)->SetWindowText(GetResString(IDS_CAT_FS));
	GetDlgItem(IDC_STATIC_RSIZE)->SetWindowText(GetResString(IDS_CAT_RS));
	GetDlgItem(IDC_STATIC_RTIME)->SetWindowText(GetResString(IDS_CAT_RT));
	GetDlgItem(IDC_STATIC_SCOUNT)->SetWindowText(GetResString(IDS_CAT_SC));
	GetDlgItem(IDC_STATIC_ASCOUNT)->SetWindowText(GetResString(IDS_CAT_ASC));
	// [TPT] - khaos::categorymod-

	m_ctlColor.CustomText = GetResString(IDS_COL_MORECOLORS);
	m_ctlColor.DefaultText = GetResString(IDS_DEFAULT);
	m_ctlColor.SetDefaultColor(NULL);

	SetWindowText(GetResString(IDS_EDITCAT));

	while (m_prio.GetCount()>0) m_prio.DeleteString(0);
	//m_prio.AddString(GetResString(IDS_DONTCHANGE)); //ZZ:DownloadManager
	m_prio.AddString(GetResString(IDS_PRIOLOW));
	m_prio.AddString(GetResString(IDS_PRIONORMAL));
	m_prio.AddString(GetResString(IDS_PRIOHIGH));
	//m_prio.AddString(GetResString(IDS_PRIOAUTO)); //ZZ:DownloadManager
	m_prio.SetCurSel(m_myCat->prio);
}
}

void CCatDialog::OnBnClickedBrowse()
{	
	TCHAR buffer[MAX_PATH] = {0};
	GetDlgItemText(IDC_INCOMING, buffer, ARRSIZE(buffer));
	if(SelectDir(GetSafeHwnd(), buffer,GetResString(IDS_SELECT_INCOMINGDIR)))
		GetDlgItem(IDC_INCOMING)->SetWindowText(buffer);
}

void CCatDialog::OnBnClickedOk()
{
	CString oldpath = m_myCat->incomingpath;

	if (GetDlgItem(IDC_TITLE)->GetWindowTextLength()>0)
		GetDlgItem(IDC_TITLE)->GetWindowText(m_myCat->title, ARRSIZE(m_myCat->title));

	if (GetDlgItem(IDC_INCOMING)->GetWindowTextLength()>2)
		GetDlgItem(IDC_INCOMING)->GetWindowText(m_myCat->incomingpath, ARRSIZE(m_myCat->incomingpath));
	
	GetDlgItem(IDC_COMMENT)->GetWindowText(m_myCat->comment, ARRSIZE(m_myCat->comment));

	MakeFoldername(m_myCat->incomingpath);
	if (!thePrefs.IsShareableDirectory(m_myCat->incomingpath)){
		_sntprintf(m_myCat->incomingpath, ARRSIZE(m_myCat->incomingpath), _T("%s"), thePrefs.GetIncomingDir());
		MakeFoldername(m_myCat->incomingpath);
	}

	if (!PathFileExists(m_myCat->incomingpath)){
		if (!::CreateDirectory(m_myCat->incomingpath, 0)){
			AfxMessageBox(GetResString(IDS_ERR_BADFOLDER));
			_tcscpy(m_myCat->incomingpath,oldpath);
			return;
		}
	}

	if (CString(m_myCat->incomingpath).CompareNoCase(oldpath)!=0)
		theApp.sharedfiles->Reload();

	m_myCat->color=newcolor;
    m_myCat->prio=m_prio.GetCurSel();
    // [TPT] - khaos::categorymod+	
	// GetDlgItem(IDC_AUTOCATEXT)->GetWindowText(m_myCat->autocat);
	GetDlgItem(IDC_AUTOCATEXT)->GetWindowText(m_myCat->viewfilters.sAdvancedFilterMask);
	
	CString sBuffer;
	
	GetDlgItem(IDC_FS_MIN)->GetWindowText(sBuffer);
	m_myCat->viewfilters.nFSizeMin = CastXBytesToI(sBuffer);
	GetDlgItem(IDC_FS_MAX)->GetWindowText(sBuffer);
	m_myCat->viewfilters.nFSizeMax = CastXBytesToI(sBuffer);
	GetDlgItem(IDC_RS_MIN)->GetWindowText(sBuffer);
	m_myCat->viewfilters.nRSizeMin = CastXBytesToI(sBuffer);
	GetDlgItem(IDC_RS_MAX)->GetWindowText(sBuffer);
	m_myCat->viewfilters.nRSizeMax = CastXBytesToI(sBuffer);
	GetDlgItem(IDC_RT_MIN)->GetWindowText(sBuffer);
	m_myCat->viewfilters.nTimeRemainingMin = (uint32) (60 * _tstoi(sBuffer));
	GetDlgItem(IDC_RT_MAX)->GetWindowText(sBuffer);
	m_myCat->viewfilters.nTimeRemainingMax = (uint32) (60 * _tstoi(sBuffer));
	GetDlgItem(IDC_SC_MIN)->GetWindowText(sBuffer);
	m_myCat->viewfilters.nSourceCountMin = _tstoi(sBuffer);
	GetDlgItem(IDC_SC_MAX)->GetWindowText(sBuffer);
	m_myCat->viewfilters.nSourceCountMax = _tstoi(sBuffer);
	GetDlgItem(IDC_ASC_MIN)->GetWindowText(sBuffer);
	m_myCat->viewfilters.nAvailSourceCountMin = _tstoi(sBuffer);
	GetDlgItem(IDC_ASC_MAX)->GetWindowText(sBuffer);
	m_myCat->viewfilters.nAvailSourceCountMax = _tstoi(sBuffer);

	m_myCat->selectioncriteria.bFileSize = IsDlgButtonChecked(IDC_CHECK_FS)?true:false;
	m_myCat->selectioncriteria.bAdvancedFilterMask = IsDlgButtonChecked(IDC_CHECK_MASK)?true:false;	
	// [TPT] - khaos::categorymod-

	theApp.emuledlg->transferwnd->downloadlistctrl.Invalidate();

	OnOK();
}


void CCatDialog::OnBnClickedCancel()
{
	m_bCancelled=true;
	
	OnCancel();
}

LONG CCatDialog::OnSelChange(UINT lParam, LONG wParam)
{
	if (lParam==CLR_DEFAULT)
		newcolor=0;		
	else
		newcolor=m_ctlColor.GetColor();
	
	return TRUE;
}
