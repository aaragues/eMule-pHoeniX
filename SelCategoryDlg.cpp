// [TPT] - khaos::categorymod+
// SelCategoryDlg.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "SelCategoryDlg.h"
#include "emuleDlg.h"
#include "transferwnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CSelCategoryDlg dialog

IMPLEMENT_DYNAMIC(CSelCategoryDlg, CDialog)

CSelCategoryDlg::CSelCategoryDlg(CWnd* pWnd)
	: CDialog(CSelCategoryDlg::IDD, pWnd)
{
	// If they have selected to use the active category as the default
	// when adding links, then set m_Return to it.  Otherwise, use 'All' (0).
	if (thePrefs.UseActiveCatForLinks())
		m_Return =	theApp.emuledlg->transferwnd->GetActiveCategory();
	else
		m_Return = 0;

	m_bCreatedNew = false;
}

CSelCategoryDlg::~CSelCategoryDlg()
{
}

void CSelCategoryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSelCategoryDlg, CDialog)
END_MESSAGE_MAP()

BOOL CSelCategoryDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	Localize();
	
	return TRUE;
}

void CSelCategoryDlg::Localize()
{
	if(m_hWnd)
	{
		// Load the language strings.
		GetDlgItem(IDC_STATIC_INS)->SetWindowText(GetResString(IDS_CAT_SELDLGTXT));
		GetDlgItem(IDCANCEL)->SetWindowText(GetResString(IDS_CANCEL));
		GetDlgItem(IDC_SEL_CAT_STATIC)->SetWindowText(GetResString(IDS_CAT_USEACTIVE));
		SetWindowText(GetResString(IDS_CAT_SELDLGCAP));

		// 'All' is always an option.
		((CComboBox*)GetDlgItem(IDC_CATCOMBO))->AddString(GetResString(IDS_ALL) + _T("/") + GetResString(IDS_CAT_UNASSIGN));
		
		// If there are more categories, add them to the list.
		if (thePrefs.GetCatCount() > 1)
			for (int i=1; i < thePrefs.GetCatCount(); i++)
						((CComboBox*)GetDlgItem(IDC_CATCOMBO))->AddString(thePrefs.GetCategory(i)->title);

		// Select the category that is currently visible in the transfer dialog as default, or 0 if they are
		// not using "New Downloads Default To Active Category"
		((CComboBox*)GetDlgItem(IDC_CATCOMBO))->SetCurSel(thePrefs.UseActiveCatForLinks()?theApp.emuledlg->transferwnd->GetActiveCategory():0);
		GetDlgItem(IDC_CAT_CHECK)->EnableWindow(TRUE);
	}
}

void CSelCategoryDlg::OnOK()
{
	int	comboIndex = ((CComboBox*)GetDlgItem(IDC_CATCOMBO))->GetCurSel();

	CString* catTitle = new CString(thePrefs.GetCategory(comboIndex)->title);
	catTitle->Trim();

	CString	comboText;
	((CComboBox*)GetDlgItem(IDC_CATCOMBO))->GetWindowText(comboText);
	comboText.Trim();

	if (catTitle->CompareNoCase(comboText) == 0 || (comboIndex == 0 && comboText.Compare(GetResString(IDS_ALL) + _T("/") + GetResString(IDS_CAT_UNASSIGN)) == 0))
		m_Return = comboIndex;
	else {
		m_bCreatedNew = true;
		m_Return = theApp.emuledlg->transferwnd->AddCategory(comboText, thePrefs.GetIncomingDir(), _T(""), _T(""));
	}

	// [TPT]
	if (IsDlgButtonChecked(IDC_CAT_CHECK))
	{
		theApp.emuledlg->transferwnd->SetActiveCategory(comboIndex);
		theApp.emuledlg->transferwnd->downloadlistctrl.ChangeCategory(comboIndex);
		thePrefs.DownloadNextInThisCat();
	}
	// [TPT]

	delete catTitle;
	catTitle = NULL;

	CDialog::OnOK();
}

void CSelCategoryDlg::OnCancel()
{
	// m_Return will still be default, so we don't have to do a darn thing here.
	CDialog::OnCancel();
}