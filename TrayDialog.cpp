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
#include "TrayDialog.h"
#include "emuledlg.h"
#include "shlwapi.h"
#include "UserMsgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CTrayDialog dialog

const UINT WM_TASKBARCREATED = ::RegisterWindowMessage(_T("TaskbarCreated"));

BEGIN_MESSAGE_MAP(CTrayDialog, CTrayDialogBase)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SYSCOMMAND()
	ON_MESSAGE(UM_TRAY_ICON_NOTIFY_MESSAGE, OnTrayNotify)
	ON_REGISTERED_MESSAGE(WM_TASKBARCREATED, OnTaskBarCreated)
	ON_WM_TIMER()
END_MESSAGE_MAP()

CTrayDialog::CTrayDialog(UINT uIDD,CWnd* pParent /*=NULL*/)
	: CTrayDialogBase(uIDD, pParent)
{
	// [TPT] - New tray tooltip
	//extensions available for >= Windows 2000 and Windows Millennium Edition
	if (GetDllVersion(TEXT("Shell32.dll")) >= PACKVERSION(5,0))	
	{
	m_nidIconData.cbSize			= sizeof(NOTIFYICONDATA);
		m_bExtensions					= true;
	}
	else
	{
		m_nidIconData.cbSize			= NOTIFYICONDATA_V1_SIZE;
		m_bExtensions					= false;
	ASSERT( m_nidIconData.cbSize == NOTIFYICONDATA_V1_SIZE );
	}	
	// [TPT] - New tray tooltip
	
	m_nidIconData.hWnd				= 0;
	m_nidIconData.uID				= 1;
	m_nidIconData.uCallbackMessage	= UM_TRAY_ICON_NOTIFY_MESSAGE;
	m_nidIconData.hIcon				= 0;
	m_nidIconData.szTip[0]			= 0;	
	m_nidIconData.uFlags			= NIF_MESSAGE;
	m_bTrayIconVisible				= FALSE;
	m_pbMinimizeToTray = NULL;
	m_nDefaultMenuItem				= 0;
	m_hPrevIconDelete = NULL;
    	m_bCurIconDelete = false;
	m_bdoubleclicked				= false;
	//m_uSingleClickTimer = 0;
}

int CTrayDialog::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CTrayDialogBase::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	ASSERT( WM_TASKBARCREATED );
	m_nidIconData.hWnd = m_hWnd;
	m_nidIconData.uID = 1;	
	return 0;
}

void CTrayDialog::OnDestroy() 
{
	//KillSingleClickTimer();
	CTrayDialogBase::OnDestroy();

	// shouldn't that be done before passing the message to DefWinProc?
	if(m_nidIconData.hWnd && m_nidIconData.uID>0 && TrayIsVisible())
	{
		VERIFY( Shell_NotifyIcon(NIM_DELETE,&m_nidIconData) );
	}
}

BOOL CTrayDialog::TrayIsVisible()
{
	return m_bTrayIconVisible;
}

void CTrayDialog::TraySetIcon(HICON hIcon, bool bDelete)
{
	ASSERT(hIcon); 
	if (hIcon){
		//ASSERT(m_hPrevIconDelete == NULL);
		if (m_bCurIconDelete){
			ASSERT( m_nidIconData.hIcon != NULL && (m_nidIconData.uFlags & NIF_ICON) );
			m_hPrevIconDelete = m_nidIconData.hIcon;
		}
		m_bCurIconDelete = bDelete;
		m_nidIconData.hIcon = hIcon;
		m_nidIconData.uFlags |= NIF_ICON;
	}
}

void CTrayDialog::TraySetIcon(UINT nResourceID, bool bDelete)
{
	TraySetIcon(AfxGetApp()->LoadIcon(nResourceID));
}

void CTrayDialog::TraySetIcon(LPCTSTR lpszResourceName, bool bDelete)
{
	TraySetIcon(AfxGetApp()->LoadIcon(lpszResourceName));
}

void CTrayDialog::TraySetToolTip(LPCTSTR lpszToolTip)
{
	ASSERT(_tcslen(lpszToolTip) > 0 && _tcslen(lpszToolTip) < ARRSIZE(m_nidIconData.szTip)); // [TPT]
	_tcsncpy(m_nidIconData.szTip,lpszToolTip,ARRSIZE(m_nidIconData.szTip));
	m_nidIconData.szTip[ARRSIZE(m_nidIconData.szTip)-1] = _T('\0');
	m_nidIconData.uFlags |= NIF_TIP;

	Shell_NotifyIcon(NIM_MODIFY,&m_nidIconData);
}

BOOL CTrayDialog::TrayShow()
{
	BOOL bSuccess = FALSE;
	if(!m_bTrayIconVisible)
	{
		bSuccess = Shell_NotifyIcon(NIM_ADD,&m_nidIconData);
		if(bSuccess)
		{
			m_bTrayIconVisible = TRUE;
			// [TPT] - TBH: minimule
			if (thePrefs.IsMiniMuleEnabled())
			{
				if (thePrefs.GetMiniMuleLives())
					theApp.minimule->RunMiniMule();
				else
					theApp.minimule->RunMiniMule(true);
				theApp.minimule->ShowWindow(SW_SHOW);
			}
			// [TPT] - TBH: minimule
		}
		
	}
	else
	{
		TRACE0("CTrayDialog::TrayShow: ICON ALREADY VISIBLE");
	}
	return bSuccess;
}

BOOL CTrayDialog::TrayHide()
{
	BOOL bSuccess = FALSE;
	if(m_bTrayIconVisible)
	{
		bSuccess = Shell_NotifyIcon(NIM_DELETE,&m_nidIconData);
		if(bSuccess)
		{
			m_bTrayIconVisible= FALSE;
			theApp.minimule->ShowWindow(SW_HIDE); // [TPT] - TBH: minimule

		}
	}
	else
	{
		TRACE0("CTrayDialog::TrayHide: ICON ALREADY HIDDEN");
	}
	return bSuccess;
}

BOOL CTrayDialog::TrayUpdate()
{
    BOOL bSuccess = FALSE;
    if(m_bTrayIconVisible)
    {
        bSuccess = Shell_NotifyIcon(NIM_MODIFY,&m_nidIconData);
        if (!bSuccess){
			//ASSERT(0);
            return FALSE; // don't delete 'm_hPrevIconDelete' because it's still attached to the tray
        }
    }
    else
    {
        //TRACE0("ICON NOT VISIBLE");
    }
   
    if (m_hPrevIconDelete != NULL)
    {
        VERIFY( ::DestroyIcon(m_hPrevIconDelete) );
        m_hPrevIconDelete = NULL;
    }        
   
	return bSuccess;
} 

BOOL CTrayDialog::TraySetMenu(UINT nResourceID, UINT nDefaultPos)
{
	BOOL bSuccess = m_mnuTrayMenu.LoadMenu(nResourceID);
	ASSERT( bSuccess );
	return bSuccess;
}

BOOL CTrayDialog::TraySetMenu(LPCTSTR lpszMenuName, UINT nDefaultPos)
{
	BOOL bSuccess = m_mnuTrayMenu.LoadMenu(lpszMenuName);
	ASSERT( bSuccess );
	return bSuccess;
}

BOOL CTrayDialog::TraySetMenu(HMENU hMenu, UINT nDefaultPos)
{
	m_mnuTrayMenu.Attach(hMenu);
	return TRUE;
}

LRESULT CTrayDialog::OnTrayNotify(WPARAM wParam, LPARAM lParam)
{
    UINT uID = (UINT)wParam;
	if (uID != 1)
		return 0;
	
	CPoint pt;	
    UINT uMsg = (UINT)lParam;
    switch (uMsg)
	{ 
		case WM_MOUSEMOVE:
			GetCursorPos(&pt);
			ClientToScreen(&pt);
			OnTrayMouseMove(pt);
			break;

		case WM_LBUTTONDOWN:
			GetCursorPos(&pt);
			ClientToScreen(&pt);
			OnTrayLButtonDown(pt);
			break;
		case WM_LBUTTONDBLCLK:
			//KillSingleClickTimer();
			GetCursorPos(&pt);
			ClientToScreen(&pt);
			OnTrayLButtonDblClk(pt);
			break;

		case WM_RBUTTONUP:
		case WM_CONTEXTMENU:
			//KillSingleClickTimer();
			GetCursorPos(&pt);
			//ClientToScreen(&pt);
			OnTrayRButtonUp(pt);//bond006: systray menu gets stuck (bugfix)
			break;

		case WM_RBUTTONDBLCLK:
			//KillSingleClickTimer();
			GetCursorPos(&pt);
			ClientToScreen(&pt);
			OnTrayRButtonDblClk(pt);
			break;
		case WM_LBUTTONUP:
			if(m_bdoubleclicked)
{
				if(TrayHide())
					RestoreWindow();
				m_bdoubleclicked=false;
	}
			break;
}
	return 1; 
}

void CTrayDialog::OnSysCommand(UINT nID, LPARAM lParam)
{
	if (m_pbMinimizeToTray != NULL && *m_pbMinimizeToTray)
	{
		if ((nID & 0xFFF0) == SC_MINIMIZE)
		{
			// emulEspaņa: modified by [TPT]-MoNKi [MoNKi: -invisible mode-]
			if (!thePrefs.GetInvisibleMode())
			{
				if(TrayShow())
					ShowWindow(SW_HIDE);
			}
			else
				ShowWindow(SW_HIDE);		
			// End emulEspaņa
		}
		else
			CTrayDialogBase::OnSysCommand(nID, lParam);	
	}
	else if ((nID & 0xFFF0) == SC_MINIMIZETRAY)
	{
		// emulEspaņa: modified by [TPT]-MoNKi [MoNKi: -invisible mode-]
		if (!thePrefs.GetInvisibleMode())
		{
			if(TrayShow())
			ShowWindow(SW_HIDE);
		}
		else
			ShowWindow(SW_HIDE);
		// End emulEspaņa
	}
	else
		CTrayDialogBase::OnSysCommand(nID, lParam);
}

void CTrayDialog::TraySetMinimizeToTray(uint8* pbMinimizeToTray)
{
	m_pbMinimizeToTray = pbMinimizeToTray;
}

void CTrayDialog::TrayMinimizeToTrayChange()
{
	if (m_pbMinimizeToTray == NULL)
		return;
	if (*m_pbMinimizeToTray)
		MinTrayBtnHide();
	else
		MinTrayBtnShow();
}

void CTrayDialog::OnTrayRButtonUp(CPoint pt)
{
}

void CTrayDialog::OnTrayLButtonDown(CPoint pt)
{
}

void CTrayDialog::OnTrayLButtonDblClk(CPoint pt)
{
	m_bdoubleclicked = true;
}

void CTrayDialog::OnTrayRButtonDblClk(CPoint pt)
{
}

void CTrayDialog::OnTrayMouseMove(CPoint pt)
{
}

LRESULT CTrayDialog::OnTaskBarCreated(WPARAM wParam, LPARAM lParam)
{
	if (m_bTrayIconVisible)
	{
		BOOL bResult = Shell_NotifyIcon(NIM_ADD, &m_nidIconData);
		ASSERT( bResult );
		return bResult;
	}
	return 1;
}

void CTrayDialog::RestoreWindow()
{
	ShowWindow(SW_SHOW);
}
