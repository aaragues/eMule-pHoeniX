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
#include "ListViewWalkerPropertySheet.h"
#include "UserMsgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define IDC_PREV	100
#define IDC_NEXT	101

// CListViewWalkerPropertySheet

IMPLEMENT_DYNAMIC(CListViewWalkerPropertySheet, CResizableSheet)

BEGIN_MESSAGE_MAP(CListViewWalkerPropertySheet, CResizableSheet)
	ON_BN_CLICKED(IDC_NEXT, OnNext)
	ON_BN_CLICKED(IDC_PREV, OnPrev)
END_MESSAGE_MAP()

CListViewWalkerPropertySheet::CListViewWalkerPropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CResizableSheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CListViewWalkerPropertySheet::CListViewWalkerPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CResizableSheet(pszCaption, pParentWnd, iSelectPage)
{
}

CListViewWalkerPropertySheet::~CListViewWalkerPropertySheet()
{
}

BOOL CListViewWalkerPropertySheet::OnInitDialog()
{
	BOOL bResult = CResizableSheet::OnInitDialog();

	//////////////////////////////////////////////////////////////////////////
	// Add additional controls
	//
	if (m_pListCtrl != NULL)
	{
		// switching from multi-selection to single selection is currently not supported -> disable Up/Down controls
		DWORD dwCtrlStyle = (m_aItems.GetSize()>1) ? WS_DISABLED : 0;

		const struct
		{
			CButton	   *pCtlBtn;
			UINT		uCtlId;
			LPCTSTR		pszLabel;
			LPCTSTR		pszSymbol;
			DWORD		dwStyle;
		} aCtrls[] =
		{
			{ &m_ctlPrev,  IDC_PREV, _T("&Prev"),  _T("5"), WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP },
			{ &m_ctlNext,  IDC_NEXT, _T("&Next"),  _T("6"), WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP }
		};

		CWnd* pctlOk = GetDlgItem(IDOK);
		CRect rcOk;
		pctlOk->GetWindowRect(&rcOk);
		ScreenToClient(&rcOk);
		CFont* pDefCtrlFont = pctlOk->GetFont();

		for (int i = 0; i < ARRSIZE(aCtrls); i++)
		{
			const int iNaviBtnWidth = rcOk.Width()/2;
			CRect rc;
			rc.left = rcOk.left - (8 + iNaviBtnWidth) * (ARRSIZE(aCtrls) - i);
			rc.top = rcOk.top;
			rc.right = rc.left + iNaviBtnWidth;
			rc.bottom = rc.top + rcOk.Height();
			VERIFY( aCtrls[i].pCtlBtn->Create(aCtrls[i].pszLabel, dwCtrlStyle | aCtrls[i].dwStyle, rc, this, aCtrls[i].uCtlId) );

			if (theApp.m_fontSymbol.m_hObject)
			{
				aCtrls[i].pCtlBtn->SetFont(&theApp.m_fontSymbol);
				aCtrls[i].pCtlBtn->SetWindowText(aCtrls[i].pszSymbol); // show down-arrow
			}
			else
				aCtrls[i].pCtlBtn->SetFont(pDefCtrlFont);
			AddAnchor(*aCtrls[i].pCtlBtn, BOTTOM_RIGHT);
		}
	}

	return bResult;
}

void CListViewWalkerPropertySheet::ChangeData(CObject* pObj)
{
	m_aItems.RemoveAll();
	m_aItems.Add(pObj);
	SendMessage(UM_DATA_CHANGED);

	for (int iPage = 0; iPage < GetPageCount(); iPage++)
	{
		CPropertyPage* pPage = GetPage(iPage);
		if (pPage && pPage->m_hWnd)
		{
			pPage->SendMessage(UM_DATA_CHANGED);
			pPage->SetModified(FALSE);
		}
	}
	GetActivePage()->OnSetActive();
}

void CListViewWalkerPropertySheet::OnPrev()
{
	ASSERT( m_pListCtrl != NULL );
	if (m_pListCtrl == NULL)
		return;

	CObject* pObj = m_pListCtrl->GetPrevSelectableItem(m_aItems[0]);	// [TPT] - SLUGFILLER: modelessDialogs
	if (pObj)
		ChangeData(pObj);
	else
		MessageBeep((UINT)-1);
}

void CListViewWalkerPropertySheet::OnNext()
{
	ASSERT( m_pListCtrl != NULL );
	if (m_pListCtrl == NULL)
		return;

	CObject* pObj = m_pListCtrl->GetNextSelectableItem(m_aItems[0]);	// [TPT] - SLUGFILLER: modelessDialogs
	if (pObj)
		ChangeData(pObj);
	else
		MessageBeep((UINT)-1);
}

CObject* CListCtrlItemWalk::GetPrevSelectableItem(CObject* pCurrentObj)	// [TPT] - SLUGFILLER: modelessDialogs
{
	ASSERT( m_pListCtrl != NULL );
	if (m_pListCtrl == NULL)
		return NULL;

	int iItemCount = m_pListCtrl->GetItemCount();
	if (iItemCount >= 2)
	{
		// [TPT] - SLUGFILLER: modelessDialogs
		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)pCurrentObj;
		int iItem = m_pListCtrl->FindItem(&find);
		if (iItem != (-1))
		{
		// [TPT] - SLUGFILLER: modelessDialogs
			if (iItem-1 >= 0)
			{
				m_pListCtrl->SetItemState(iItem, 0, LVIS_SELECTED | LVIS_FOCUSED);
				iItem--;
				m_pListCtrl->SetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				m_pListCtrl->SetSelectionMark(iItem);
				m_pListCtrl->EnsureVisible(iItem, FALSE);

				return STATIC_DOWNCAST(CObject, (CObject*)m_pListCtrl->GetItemData(iItem));
			}
		}
	}
	return NULL;
}

CObject* CListCtrlItemWalk::GetNextSelectableItem(CObject* pCurrentObj)	// [TPT] - SLUGFILLER: modelessDialogs
{
	ASSERT( m_pListCtrl != NULL );
	if (m_pListCtrl == NULL)
		return NULL;

	int iItemCount = m_pListCtrl->GetItemCount();
	if (iItemCount >= 2)
	{
		// [TPT] - SLUGFILLER: modelessDialogs
		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)pCurrentObj;
		int iItem = m_pListCtrl->FindItem(&find);
		if (iItem != (-1))
		{
		// [TPT] - SLUGFILLER: modelessDialogs
			if (iItem+1 < iItemCount)
			{
				m_pListCtrl->SetItemState(iItem, 0, LVIS_SELECTED | LVIS_FOCUSED);
				iItem++;
				m_pListCtrl->SetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				m_pListCtrl->SetSelectionMark(iItem);
				m_pListCtrl->EnsureVisible(iItem, FALSE);

				return STATIC_DOWNCAST(CObject, (CObject*)m_pListCtrl->GetItemData(iItem));
			}
		}
	}
	return NULL;
}
