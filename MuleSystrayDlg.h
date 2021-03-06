#if !defined(AFX_INPUTBOX_H__25CB82F9_AFE9_4640_9574_3E211C6D0452__INCLUDED_)
#define AFX_INPUTBOX_H__25CB82F9_AFE9_4640_9574_3E211C6D0452__INCLUDED_
#if _MSC_VER > 1000
#pragma once
#endif 

class CInputBox : public CEdit
{
protected:
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

    DECLARE_MESSAGE_MAP()
};

#endif

#if !defined(AFX_MULESYSTRAYDLG_H__A3BFC8BE_562C_4838_936D_C3D7CF647DA9__INCLUDED_)
#define AFX_MULESYSTRAYDLG_H__A3BFC8BE_562C_4838_936D_C3D7CF647DA9__INCLUDED_


#include "TrayMenuBtn.h"		// Added by ClassView
#include "GradientStatic.h"	// Added by ClassView
#include "resource.h"
#include "NumEdit.h" // [TPT]
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MuleSystrayDlg.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CMuleSystrayDlg dialog

class CMuleSystrayDlg : public CDialog
{
// Construction
public:
	CMuleSystrayDlg(CWnd* pParent, CPoint pt, float iMaxUp, float iMaxDown, float iCurUp, float iCurDown);
	~CMuleSystrayDlg();
    
// Dialog Data
	//{{AFX_DATA(CMuleSystrayDlg)
	enum { IDD = IDD_MULETRAYDLG };
	CStatic	m_ctrlUpArrow;
	CStatic	m_ctrlDownArrow;
	CGradientStatic	m_ctrlSidebar;
	CSliderCtrl	m_ctrlUpSpeedSld;
	CSliderCtrl	m_ctrlDownSpeedSld;
	CNumEdit	m_DownSpeedInput;
	CNumEdit	m_UpSpeedInput;
	float		m_nDownSpeedTxt;
	float		m_nUpSpeedTxt;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMuleSystrayDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CTrayMenuBtn m_ctrlSpeed;
	CTrayMenuBtn m_ctrlAllToMax;
	CTrayMenuBtn m_ctrlAllToMin;
	CTrayMenuBtn m_ctrlRestore;
	CTrayMenuBtn m_ctrlDisconnect;
	CTrayMenuBtn m_ctrlConnect;
	CTrayMenuBtn m_ctrlExit;
	CTrayMenuBtn m_ctrlPreferences;
	CTrayMenuBtn m_ctrlMiniMule;

	bool m_bClosingDown;
	
	int m_iMaxUp;
	int m_iMaxDown;
	CPoint m_ptInitialPosition;

	HICON m_hUpArrow;
	HICON m_hDownArrow;

	UINT m_nExitCode;

	// Generated message map functions
	//{{AFX_MSG(CMuleSystrayDlg)
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeDowntxt();
	afx_msg void OnChangeUptxt();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MULESYSTRAYDLG_H__A3BFC8BE_562C_4838_936D_C3D7CF647DA9__INCLUDED_)
