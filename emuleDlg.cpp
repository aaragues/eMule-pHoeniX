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
#include <math.h>
#include <afxinet.h>
#define MMNODRV			// mmsystem: Installable driver support
//#define MMNOSOUND		// mmsystem: Sound support
#define MMNOWAVE		// mmsystem: Waveform support
#define MMNOMIDI		// mmsystem: MIDI support
#define MMNOAUX			// mmsystem: Auxiliary audio support
#define MMNOMIXER		// mmsystem: Mixer support
#define MMNOTIMER		// mmsystem: Timer support
#define MMNOJOY			// mmsystem: Joystick support
#define MMNOMCI			// mmsystem: MCI support
#define MMNOMMIO		// mmsystem: Multimedia file I/O support
#define MMNOMMSYSTEM	// mmsystem: General MMSYSTEM functions
#include <Mmsystem.h>
#include <HtmlHelp.h>
#include <share.h>
#include "emule.h"
#include "emuleDlg.h"
#include "ServerWnd.h"
#include "KademliaWnd.h"
#include "TransferWnd.h"
#include "SearchResultsWnd.h"
#include "SearchDlg.h"
#include "SharedFilesWnd.h"
#include "ChatWnd.h"
#include "IrcWnd.h"
#include "StatisticsDlg.h"
#include "CreditsDlg.h"
#include "PreferencesDlg.h"
#include "Sockets.h"
#include "KnownFileList.h"
#include "ServerList.h"
#include "Opcodes.h"
#include "SharedFileList.h"
#include "ED2KLink.h"
#include "Splashscreen.h"
#include "PartFileConvert.h"
#include "EnBitmap.h"
#include "Wizard.h"
#include "Exceptions.h"
#include "SearchList.h"
#include "HTRichEditCtrl.h"
#include "FrameGrabThread.h"
#include "kademlia/kademlia/kademlia.h"
#include "kademlia/kademlia/SearchManager.h"
#include "kademlia/routing/RoutingZone.h"
#include "kademlia/routing/contact.h"
#include "kademlia/kademlia/prefs.h"
#include "KadSearchListCtrl.h"
#include "KadContactListCtrl.h"
#include "PerfLog.h"
#include "version.h"
#include "DropTarget.h"
#include "LastCommonRouteFinder.h"
#include "WebServer.h"
#include "MMServer.h"
#include "DownloadQueue.h"
#include "ClientUDPSocket.h"
#include "UploadQueue.h"
#include "ClientList.h"
#include "UploadBandwidthThrottler.h"
#include "FriendList.h"
#include "IPFilter.h"
#include "Statistics.h"
#include "MuleToolbarCtrl.h"
#include "TaskbarNotifier.h"
#include "MuleStatusbarCtrl.h"
#include "ListenSocket.h"
#include "Server.h"
#include "PartFile.h"
#include "Scheduler.h"
#include "ClientCredits.h"
#include "MenuCmds.h"
#include "MuleSystrayDlg.h"
#include "IPFilterDlg.h"
#include "WebServices.h"
#include "DirectDownloadDlg.h"
#include "PeerCacheFinder.h"
#include "Statistics.h"
#include "FirewallOpener.h"
#include "StringConversion.h"
#include "aichsyncthread.h"
#include "Log.h"
#include "UserMsgs.h"

#include "IniNotifier.h" // [TPT] - enkeyDEV(th1) -notifier-
#include "BandWidthControl.h" // [TPT]
#include "ip2country.h" // [TPT] - IP Country
#include "fakecheck.h" // [TPT] - Fakecheck
#include "mod_version.h" // [TPT]- modID
// [TPT] - Announ: -Friend eLinks-
#include "Friend.h"
// End -Friend eLinks-

#ifndef RBBS_USECHEVRON
#define RBBS_USECHEVRON     0x00000200  // display drop-down button for this band if it's sized smaller than ideal width
#endif

#ifndef RBN_CHEVRONPUSHED
#define RBN_CHEVRONPUSHED   (RBN_FIRST - 10)
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


#define	SYS_TRAY_ICON_COOKIE_FORCE_UPDATE	(UINT)-1

BOOL (WINAPI *_TransparentBlt)(HDC, int, int, int, int, HDC, int, int, int, int, UINT)= NULL;
const static UINT UWM_ARE_YOU_EMULE = RegisterWindowMessage(EMULE_GUID);
UINT _uMainThreadId = 0;

// emulEspa�a: added by [TPT]-MoNKi [MoNKi: -invisible mode-]
// Allows "invisible mode" on multiple instances of eMule
#ifdef _DEBUG
#define EMULE_GUID_INVMODE				"EMULE-{4EADC6FC-516F-4b7c-9066-97D893649569}-DEBUG-INVISIBLEMODE"
#else
#define EMULE_GUID_INVMODE				"EMULE-{4EADC6FC-516F-4b7c-9066-97D893649569}-INVISIBLEMODE"
#endif
const static UINT UWM_RESTORE_WINDOW_IM=RegisterWindowMessage(_T(EMULE_GUID_INVMODE));
// End emulEspa�a


///////////////////////////////////////////////////////////////////////////
// CemuleDlg Dialog

IMPLEMENT_DYNAMIC(CMsgBoxException, CException)

BEGIN_MESSAGE_MAP(CemuleDlg, CTrayDialog)
	///////////////////////////////////////////////////////////////////////////
	// Windows messages
	//
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_ENDSESSION()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_MENUCHAR()
	ON_WM_QUERYENDSESSION()
	ON_WM_SYSCOLORCHANGE()
	ON_MESSAGE(WM_COPYDATA, OnWMData)
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_WM_SHOWWINDOW()
	ON_MESSAGE(WM_HOTKEY, OnHotKey) // emulEspa�a: added by [TPT]-MoNKi [MoNKi: -invisible mode-]
	// [TPT] - MFCK [addon] - New Tooltips [Rayita]
	ON_NOTIFY_EX_RANGE(UDM_TOOLTIP_DISPLAY, 0, 0xFFFF, OnToolTipNotify)
	// [TPT] - MFCK [addon] - New Tooltips [Rayita]
	///////////////////////////////////////////////////////////////////////////
	// WM_COMMAND messages
	//
	ON_COMMAND(MP_CONNECT, StartConnection)
	ON_COMMAND(MP_DISCONNECT, CloseConnection)
	ON_COMMAND(MP_EXIT, OnClose)
	ON_COMMAND(MP_RESTORE, RestoreWindow)
	ON_COMMAND(MP_TBN_QUICKDISABLE, SwitchNotifierStatus) //<<-- [TPT] - enkeyDEV(th1) -notifier-
	ON_COMMAND(MP_TBN_FORCEPOPUP, ForceNotifierPopup)     //<<-- [TPT] - enkeyDEV(th1) -notifier-

	// quick-speed changer -- 
	ON_COMMAND_RANGE(MP_QS_U10, MP_QS_UP10, QuickSpeedUpload)
	ON_COMMAND_RANGE(MP_QS_D10, MP_QS_DC, QuickSpeedDownload)
	//--- quickspeed - paralize all ---
	ON_COMMAND_RANGE(MP_QS_PA, MP_QS_UA, QuickSpeedOther)
	// quick-speed changer -- based on xrmb	
	ON_NOTIFY_EX_RANGE(RBN_CHEVRONPUSHED, 0, 0xFFFF, OnChevronPushed)

	ON_REGISTERED_MESSAGE(UWM_ARE_YOU_EMULE, OnAreYouEmule)
	
	ON_WM_MEASUREITEM() // [TPT] - New Menu Styles

	// emulEspa�a: added by [TPT]-MoNKi [MoNKi: -invisible mode-]
	// Allows "invisible mode" on multiple instances of eMule
	ON_REGISTERED_MESSAGE(UWM_RESTORE_WINDOW_IM, OnRestoreWindowInvisibleMode)
	// End emulEspa�a


	ON_BN_CLICKED(IDC_HOTMENU, OnBnClickedHotmenu)

	///////////////////////////////////////////////////////////////////////////
	// WM_USER messages
	//
	ON_MESSAGE(UM_TASKBARNOTIFIERCLICKED, OnTaskbarNotifierClicked)
	ON_MESSAGE(UM_WEB_CONNECT_TO_SERVER, OnWebServerConnect)
	ON_MESSAGE(UM_WEB_DISCONNECT_SERVER, OnWebServerDisonnect)
	ON_MESSAGE(UM_WEB_REMOVE_SERVER, OnWebServerRemove)
	ON_MESSAGE(UM_WEB_SHARED_FILES_RELOAD, OnWebSharedFilesReload)

	// Version Check DNS
	ON_MESSAGE(UM_VERSIONCHECK_RESPONSE, OnVersionCheckResponse)

	// PeerCache DNS
	ON_MESSAGE(UM_PEERCHACHE_RESPONSE, OnPeerCacheResponse)

	///////////////////////////////////////////////////////////////////////////
	// WM_APP messages
	//
	ON_MESSAGE(TM_FINISHEDHASHING, OnFileHashed)
	ON_MESSAGE(TM_FILEOPPROGRESS, OnFileOpProgress)
	ON_MESSAGE(TM_HASHFAILED, OnHashFailed)
	ON_MESSAGE(TM_FRAMEGRABFINISHED, OnFrameGrabFinished)
	ON_MESSAGE(TM_FILEALLOCEXC, OnFileAllocExc)
	ON_MESSAGE(TM_FILECOMPLETED, OnFileCompleted)
		
	// [TPT]
	// Maella -New Timer Management- (quick-n-dirty)
	ON_WM_TIMER()
	// Maella end
END_MESSAGE_MAP()

CemuleDlg::CemuleDlg(CWnd* pParent /*=NULL*/)
	: CTrayDialog(CemuleDlg::IDD, pParent)
{
	_uMainThreadId = GetCurrentThreadId();
	preferenceswnd = new CPreferencesDlg;
	serverwnd = new CServerWnd;
	kademliawnd = new CKademliaWnd;
	transferwnd = new CTransferWnd;
	sharedfileswnd = new CSharedFilesWnd;
	searchwnd = new CSearchDlg;
	chatwnd = new CChatWnd;
	ircwnd = new CIrcWnd;
	statisticswnd = new CStatisticsDlg;
	toolbar = new CMuleToolbarCtrl;
	statusbar = new CMuleStatusBarCtrl;
	m_wndTaskbarNotifier = new CTaskbarNotifier;

	// NOTE: the application icon name is prefixed with "AAA" to make sure it's alphabetically sorted by the
	// resource compiler as the 1st icon in the resource table!
	m_hIcon = AfxGetApp()->LoadIcon(_T("AAAEMULEAPP"));
	theApp.m_app_state = APP_STATE_RUNNING;
	ready = false; 
	m_bStartMinimizedChecked = false;
	m_bStartMinimized = false;
	MEMZERO(&m_wpFirstRestore, sizeof m_wpFirstRestore);
	//m_uUpDatarate = 0; // [TPT]
	//m_uDownDatarate = 0; // [TPT]
	status = 0;
	activewnd = NULL;
	for (int i = 0; i < ARRSIZE(connicons); i++)
		connicons[i] = NULL;
	transicons[0] = NULL;
	transicons[1] = NULL;
	transicons[2] = NULL;
	transicons[3] = NULL;
	imicons[0] = NULL;
	imicons[1] = NULL;
	imicons[2] = NULL;
	m_iMsgIcon = 0;
	m_icoSysTrayConnected = NULL;
	m_icoSysTrayDisconnected = NULL;
	m_icoSysTrayLowID = NULL;
	sourceTrayIconMail = NULL; // [TPT] - Notify message
	usericon = NULL;
	m_icoSysTrayCurrent = NULL;
	m_hTimer = 0;
	notifierenabled = false;
	m_pDropTarget = new CMainFrameDropTarget;
	m_pSplashWnd = NULL;
	m_dwSplashTime = (DWORD)-1;
	m_pSystrayDlg = NULL;
	m_uLastSysTrayIconCookie = SYS_TRAY_ICON_COOKIE_FORCE_UPDATE;
	m_dwMSNtime = 0;//[TPT] - Show in MSN7
}

CemuleDlg::~CemuleDlg()
{
	if (m_icoSysTrayCurrent) VERIFY( DestroyIcon(m_icoSysTrayCurrent) );
	if (m_hIcon) VERIFY( ::DestroyIcon(m_hIcon) );
	for (int i = 0; i < ARRSIZE(connicons); i++){
		if (connicons[i]) VERIFY( ::DestroyIcon(connicons[i]) );
	}
	if (transicons[0]) VERIFY( ::DestroyIcon(transicons[0]) );
	if (transicons[1]) VERIFY( ::DestroyIcon(transicons[1]) );
	if (transicons[2]) VERIFY( ::DestroyIcon(transicons[2]) );
	if (transicons[3]) VERIFY( ::DestroyIcon(transicons[3]) );
	if (imicons[0]) VERIFY( ::DestroyIcon(imicons[0]) );
	if (imicons[1]) VERIFY( ::DestroyIcon(imicons[1]) );
	if (imicons[2]) VERIFY( ::DestroyIcon(imicons[2]) );
	if (m_icoSysTrayConnected) VERIFY( ::DestroyIcon(m_icoSysTrayConnected) );
	if (m_icoSysTrayDisconnected) VERIFY( ::DestroyIcon(m_icoSysTrayDisconnected) );
	if (m_icoSysTrayLowID) VERIFY( ::DestroyIcon(m_icoSysTrayLowID) );
	if (sourceTrayIconMail) VERIFY( ::DestroyIcon(sourceTrayIconMail) ); // [TPT] - Notify message
	if (usericon) VERIFY( ::DestroyIcon(usericon) );

	// already destroyed by windows?
	//VERIFY( m_menuUploadCtrl.DestroyMenu() );
	//VERIFY( m_menuDownloadCtrl.DestroyMenu() );
	//VERIFY( m_SysMenuOptions.DestroyMenu() );

	delete preferenceswnd;
	delete serverwnd;
	delete kademliawnd;
	delete transferwnd;
	delete sharedfileswnd;
	delete chatwnd;
	delete ircwnd;
	delete statisticswnd;
	delete toolbar;
	delete statusbar;
	delete m_wndTaskbarNotifier;
	delete m_pDropTarget;
}

void CemuleDlg::DoDataExchange(CDataExchange* pDX)
{
	CTrayDialog::DoDataExchange(pDX);
}

LRESULT CemuleDlg::OnAreYouEmule(WPARAM, LPARAM)
{
	return UWM_ARE_YOU_EMULE;
} 

BOOL CemuleDlg::OnInitDialog()
{
	m_bStartMinimized = thePrefs.GetStartMinimized();
	if( !m_bStartMinimized )
		m_bStartMinimized = theApp.DidWeAutoStart();

	// temporary disable the 'startup minimized' option, otherwise no window will be shown at all
	if (thePrefs.IsFirstStart())
		m_bStartMinimized = false;

	// show splashscreen as early as possible to "entertain" user while starting emule up
	if (thePrefs.UseSplashScreen() && !m_bStartMinimized )
		ShowSplash();

	// Create global GUI objects
	theApp.CreateAllFonts();
	theApp.CreateBackwardDiagonalBrush();
	//CTitleMenu::Init();//[TPT] - pHoenix doesn�t use CTitleMenu class

	CTrayDialog::OnInitDialog();
	InitWindowStyles(this);
	//CreateMenuCmdIconMap();

	//START - [TPT] - enkeyDEV(th1) -notifier-
	m_wndTaskbarNotifier->Create(this);
	LoadNotifier();
	//END - [TPT] - enkeyDEV(th1) -notifier-
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL){
		pSysMenu->AppendMenu(MF_SEPARATOR);

		ASSERT( (MP_ABOUTBOX & 0xFFF0) == MP_ABOUTBOX && MP_ABOUTBOX < 0xF000);
		pSysMenu->AppendMenu(MF_STRING, MP_ABOUTBOX, GetResString(IDS_ABOUTBOX));

		ASSERT( (MP_VERSIONCHECK & 0xFFF0) == MP_VERSIONCHECK && MP_VERSIONCHECK < 0xF000);
		pSysMenu->AppendMenu(MF_STRING, MP_VERSIONCHECK, GetResString(IDS_VERSIONCHECK));

		// remaining system menu entries are created later...
	}

	SetIcon(m_hIcon, TRUE);			
	// this scales the 32x32 icon down to 16x16, does not look nice at least under WinXP
	//SetIcon(m_hIcon, FALSE);	

	CWnd* pwndToolbarX = toolbar;
	if (toolbar->Create(WS_CHILD | WS_VISIBLE, CRect(0,0,0,0), this, IDC_TOOLBAR))
	{
	toolbar->Init();
		if (thePrefs.GetUseReBarToolbar())
		{
		    if (m_ctlMainTopReBar.Create(WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
									     RBS_BANDBORDERS | RBS_AUTOSIZE | CCS_NODIVIDER, 
									     CRect(0, 0, 0, 0), this, AFX_IDW_REBAR))
		    {
			    CSize sizeBar;
			    VERIFY( toolbar->GetMaxSize(&sizeBar) );
			    REBARBANDINFO rbbi = {0};
			    rbbi.cbSize = sizeof(rbbi);
				rbbi.fMask = RBBIM_STYLE | RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_IDEALSIZE | RBBIM_ID;
			    rbbi.fStyle = RBBS_NOGRIPPER | RBBS_BREAK | RBBS_USECHEVRON;
			    rbbi.hwndChild = toolbar->m_hWnd;
			    rbbi.cxMinChild = sizeBar.cy;
			    rbbi.cyMinChild = sizeBar.cy;
			    rbbi.cxIdeal = sizeBar.cx;
			    rbbi.cx = rbbi.cxIdeal;
				rbbi.wID = 0;
			    VERIFY( m_ctlMainTopReBar.InsertBand((UINT)-1, &rbbi) );
				toolbar->SaveCurHeight();
		    	toolbar->UpdateBackground();
    
			    pwndToolbarX = &m_ctlMainTopReBar;
		    }
		}
	}

	//set title
	CString buffer = _T("eMule v"); 
	// [TPT]- modID
	buffer += theApp.m_strCurVersionLong + _T(" ") + MOD_VERSION; 
	SetWindowText(buffer);
	
	// [TPT] - Remove enkeyDEV(kei-kun) -TaskbarNotifier-

	// [TPT] - TBH: minimule
	if (theApp.minimule != NULL)
	{
		theApp.minimule->Create(IDD_MINIMULE,this);
		theApp.minimule->ShowWindow(SW_HIDE);
	}
	// [TPT] - TBH: minimule	

	// set statusbar
	statusbar->Create(WS_CHILD|WS_VISIBLE|CCS_BOTTOM,CRect(0,0,0,0), this, IDC_STATUSBAR);
	statusbar->EnableToolTips(true);
	SetStatusBarPartsSize();

	// create main window dialog pages
	serverwnd->Create(IDD_SERVER);
	sharedfileswnd->Create(IDD_FILES);
	searchwnd->Create(this);
	chatwnd->Create(IDD_CHAT);
	transferwnd->Create(IDD_TRANSFER);
	statisticswnd->Create(IDD_STATISTICS);
	kademliawnd->Create(IDD_KADEMLIAWND);
	ircwnd->Create(IDD_IRC);

	// with the top rebar control, some XP themes look better with some additional lite borders.. some not..
	//serverwnd->ModifyStyleEx(0, WS_EX_STATICEDGE);
	//sharedfileswnd->ModifyStyleEx(0, WS_EX_STATICEDGE);
	//searchwnd->ModifyStyleEx(0, WS_EX_STATICEDGE);
	//chatwnd->ModifyStyleEx(0, WS_EX_STATICEDGE);
	//transferwnd->ModifyStyleEx(0, WS_EX_STATICEDGE);
	//statisticswnd->ModifyStyleEx(0, WS_EX_STATICEDGE);
	//kademliawnd->ModifyStyleEx(0, WS_EX_STATICEDGE);
	//ircwnd->ModifyStyleEx(0, WS_EX_STATICEDGE);

	// optional: restore last used main window dialog
	if (thePrefs.GetRestoreLastMainWndDlg()){
		switch (thePrefs.GetLastMainWndDlgID()){
		case IDD_SERVER:
			SetActiveDialog(serverwnd);
			break;
		case IDD_FILES:
			SetActiveDialog(sharedfileswnd);
			break;
		case IDD_SEARCH:
			SetActiveDialog(searchwnd);
			break;
		case IDD_CHAT:
			SetActiveDialog(chatwnd);
			break;
		case IDD_TRANSFER:
			SetActiveDialog(transferwnd);
			break;
		case IDD_STATISTICS:
			SetActiveDialog(statisticswnd);
			break;
		case IDD_KADEMLIAWND:
			SetActiveDialog(kademliawnd);
			break;
		case IDD_IRC:
			SetActiveDialog(ircwnd);
			break;
		}
	}

	// if still no active window, activate server window
	if (activewnd == NULL)
		SetActiveDialog(serverwnd);

	SetAllIcons();
	Localize();

	// set updateintervall of graphic rate display (in seconds)
	//ShowConnectionState(false);

	// adjust all main window sizes for toolbar height and maximize the child windows
	CRect rcClient, rcToolbar, rcStatusbar;
	GetClientRect(&rcClient);
	pwndToolbarX->GetWindowRect(&rcToolbar);
	statusbar->GetWindowRect(&rcStatusbar);
	rcClient.top += rcToolbar.Height();
	rcClient.bottom -= rcStatusbar.Height();

	CWnd* apWnds[] =
	{
		serverwnd,
		kademliawnd,
		transferwnd,
		sharedfileswnd,
		searchwnd,
		chatwnd,
		ircwnd,
		statisticswnd
	};
	for (int i = 0; i < ARRSIZE(apWnds); i++)
		apWnds[i]->SetWindowPos(NULL, rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height(), SWP_NOZORDER);

	// anchors
	AddAnchor(*serverwnd,		TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*kademliawnd,		TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*transferwnd,		TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*sharedfileswnd,	TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(*searchwnd,		TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(*chatwnd,			TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*ircwnd,			TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*statisticswnd,	TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*pwndToolbarX,	TOP_LEFT, TOP_RIGHT);
	AddAnchor(*statusbar,		BOTTOM_LEFT, BOTTOM_RIGHT);

	statisticswnd->ShowInterval();

	// tray icon
	TraySetMinimizeToTray(thePrefs.GetMinTrayPTR());
	TrayMinimizeToTrayChange();

	ShowTransferRate(true);
    ShowPing();
	//searchwnd->UpdateCatTabs(); // [TPT] - khaos::categorymod

	///////////////////////////////////////////////////////////////////////////
	// Restore saved window placement
	//
	WINDOWPLACEMENT wp = {0};
	wp.length = sizeof(wp);
	wp = thePrefs.GetEmuleWindowPlacement();
	if (m_bStartMinimized)
	{
		// To avoid the window flickering during startup we try to set the proper window show state right here.
		if (*thePrefs.GetMinTrayPTR())
		{
			// Minimize to System Tray
			//
			// Unfortunately this does not work. The eMule main window is a modal dialog which is invoked
			// by CDialog::DoModal which eventually calls CWnd::RunModalLoop. Look at 'MLF_SHOWONIDLE' and
			// 'bShowIdle' in the above noted functions to see why it's not possible to create the window
			// right in hidden state.

			//--- attempt #1
			//wp.showCmd = SW_HIDE;
			//TrayShow();
			//--- doesn't work at all

			//--- attempt #2
			//if (wp.showCmd == SW_SHOWMAXIMIZED)
			//	wp.flags = WPF_RESTORETOMAXIMIZED;
			//m_bStartMinimizedChecked = false; // post-hide the window..
			//--- creates window flickering

			//--- attempt #3
			// Minimize the window into the task bar and later move it into the tray bar
			if (wp.showCmd == SW_SHOWMAXIMIZED)
				wp.flags = WPF_RESTORETOMAXIMIZED;
			wp.showCmd = SW_MINIMIZE;
			m_bStartMinimizedChecked = false;

			// to get properly restored from tray bar (after attempt #3) we have to use a patched 'restore' window cmd..
			m_wpFirstRestore = thePrefs.GetEmuleWindowPlacement();
			m_wpFirstRestore.length = sizeof(m_wpFirstRestore);
			if (m_wpFirstRestore.showCmd != SW_SHOWMAXIMIZED)
				m_wpFirstRestore.showCmd = SW_SHOWNORMAL;
		}
		else {
			// Minimize to System Taskbar
			//
			if (wp.showCmd == SW_SHOWMAXIMIZED)
				wp.flags = WPF_RESTORETOMAXIMIZED;
			wp.showCmd = SW_MINIMIZE; // Minimize window but do not activate it.
			m_bStartMinimizedChecked = true;
		}
	}
	else
	{
		// Allow only SW_SHOWNORMAL and SW_SHOWMAXIMIZED. Ignore SW_SHOWMINIMIZED to make sure the window
		// becomes visible. If user wants SW_SHOWMINIMIZED, we already have an explicit option for this (see above).
		if (wp.showCmd != SW_SHOWMAXIMIZED)
			wp.showCmd = SW_SHOWNORMAL;
		m_bStartMinimizedChecked = true;
	}
	SetWindowPlacement(&wp);

	if (thePrefs.GetWSIsEnabled())
		theApp.webserver->StartServer();
	theApp.mmserver->Init();

	VERIFY( (m_hTimer = ::SetTimer(NULL, NULL, 300, StartupTimer)) != NULL );
	if (thePrefs.GetVerbose() && !m_hTimer)
		AddDebugLogLine(true,_T("Failed to create 'startup' timer - %s"),GetErrorMessage(GetLastError()));

	theStats.starttime = GetTickCount();

	if (thePrefs.IsFirstStart())
	{
		// temporary disable the 'startup minimized' option, otherwise no window will be shown at all
		m_bStartMinimized = false;

		//[TPT] - Webcache 1.9 beta3
		thePrefs.detectWebcacheOnStart = true; // [TPT] - WebCache //jp detect webcache on startup

		DestroySplash();

		extern BOOL FirstTimeWizard();
		if (FirstTimeWizard()){
			// start connection wizard
			CConnectionWizardDlg conWizard;
			conWizard.DoModal();
		}
	}

	VERIFY( m_pDropTarget->Register(this) );

	// initalize PeerCache
	theApp.m_pPeerCache->Init(thePrefs.GetPeerCacheLastSearch(), thePrefs.WasPeerCacheFound(), thePrefs.IsPeerCacheDownloadEnabled(), thePrefs.GetPeerCachePort());

	// [TPT] - MFCK [addon] - New Tooltips [Rayita]
	m_ttip.Create(this);
	SetTTDelay();
	m_ttip.AddTool(statusbar, _T(""));
	// [TPT] - MFCK [addon] - New Tooltips [Rayita]
	
	// start aichsyncthread
	AfxBeginThread(RUNTIME_CLASS(CAICHSyncThread), THREAD_PRIORITY_BELOW_NORMAL,0);
	
	if(thePrefs.GetInvisibleMode()) 
		RegisterInvisibleHotKey();	// emulEspa�a: added by [TPT]-MoNKi [MoNKi: -invisible mode-]

	return TRUE;
}

// modders: dont remove or change the original versioncheck! (additionals are ok)
void CemuleDlg::DoVersioncheck(bool manual) {

	if (!manual && thePrefs.GetLastVC()!=0) {
		CTime last(thePrefs.GetLastVC());
		time_t tLast=safe_mktime(last.GetLocalTm());
		time_t tNow=safe_mktime(CTime::GetCurrentTime().GetLocalTm());

		if ( (difftime(tNow,tLast) / 86400)<thePrefs.GetUpdateDays() )
			return;
	}
	if (WSAAsyncGetHostByName(m_hWnd, UM_VERSIONCHECK_RESPONSE, "vcdns2.emule-project.org", m_acVCDNSBuffer, sizeof(m_acVCDNSBuffer)) == 0){
		AddLogLine(true,GetResString(IDS_NEWVERSIONFAILED));
	}
}


void CALLBACK CemuleDlg::StartupTimer(HWND hwnd, UINT uiMsg, UINT idEvent, DWORD dwTime)
{
	// NOTE: Always handle all type of MFC exceptions in TimerProcs - otherwise we'll get mem leaks
	// [TPT] - SLUGFILLER: doubleLucas - not ready to init, come back next cycle
	if (!::IsWindow(theApp.emuledlg->m_hWnd))
		return;
	if (!::IsWindow(theApp.emuledlg->sharedfileswnd->sharedfilesctrl.m_hWnd))
		return;
	if (!::IsWindow(theApp.emuledlg->serverwnd->serverlistctrl.m_hWnd))
		return;
	if (!::IsWindow(theApp.emuledlg->transferwnd->downloadlistctrl.m_hWnd))
		return;
	// [TPT] - SLUGFILLER: doubleLucas
	try
	{
		switch(theApp.emuledlg->status){
			case 0:
				theApp.emuledlg->status++;
				theApp.emuledlg->ready = true;
				theApp.sharedfiles->SetOutputCtrl(&theApp.emuledlg->sharedfileswnd->sharedfilesctrl);
				theApp.emuledlg->status++;
				break;
			case 1:
				break;
			case 2:
				theApp.emuledlg->status++;
				try{
					theApp.serverlist->Init();
				}
				catch(...){
					ASSERT(0);
					LogError(LOG_STATUSBAR,_T("Failed to initialize server list - Unknown exception"));
				}
				theApp.emuledlg->status++;
				break;
			case 3:
				break;
			case 4:{
				bool bError = false;
				theApp.emuledlg->status++;

				// NOTE: If we have an unhandled exception in CDownloadQueue::Init, MFC will silently catch it
				// and the creation of the TCP and the UDP socket will not be done -> client will get a LowID!
				try{
					theApp.downloadqueue->Init();
				}
				catch(...){
					ASSERT(0);
					LogError(LOG_STATUSBAR,_T("Failed to initialize download queue - Unknown exception"));
					bError = true;
				}
				if(!theApp.listensocket->StartListening()){
					LogError(LOG_STATUSBAR, GetResString(IDS_MAIN_SOCKETERROR),thePrefs.GetPort());
					bError = true;
				}
				if(!theApp.clientudp->Create()){
				    LogError(LOG_STATUSBAR, GetResString(IDS_MAIN_SOCKETERROR),thePrefs.GetUDPPort());
					bError = true;
				}
				
				theApp.emuledlg->serverwnd->UpdateMyInfo();

				if (!bError) // show the success msg, only if we had no serious error
					{
					// [TPT] - eWombat [WINSOCK2]
					AddLogLine(false, _T("Winsock: Version %d.%d [%.40s] %.40s"), HIBYTE( theApp.m_wsaData.wVersion ),LOBYTE(theApp.m_wsaData.wVersion ),
					(CString)theApp.m_wsaData.szDescription, (CString)theApp.m_wsaData.szSystemStatus);
					if (theApp.m_wsaData.iMaxSockets!=0)
						AddLogLine(false,_T("Winsock: max. sockets %d"), theApp.m_wsaData.iMaxSockets);
					else
						AddLogLine(false, GetResString(IDS_UNLIMITED_SOCKETS));
					// [TPT] - eWombat [WINSOCK2]
					AddLogLine(true, GetResString(IDS_MAIN_READY),theApp.m_strCurVersionLong + _T(" ") + MOD_VERSION); // [TPT]- modID
					}
				if(thePrefs.DoAutoConnect())
					theApp.emuledlg->OnBnClickedButton2();
				theApp.emuledlg->status++;
				break;
			}
			case 5:
				break;
			default:
				theApp.emuledlg->StopTimer();
		}
	}
	CATCH_DFLT_EXCEPTIONS(_T("CemuleDlg::StartupTimer"))
}

void CemuleDlg::StopTimer(){
	try	{
		if (m_hTimer){
			::KillTimer(NULL, m_hTimer);
			m_hTimer = 0;
		}
	}
	catch (...){
		ASSERT(0);
	}
	if (thePrefs.UpdateNotify()) DoVersioncheck(false);
	if (theApp.pendinglink){
		OnWMData(NULL,(LPARAM) &theApp.sendstruct);//changed by Cax2 28/10/02
		delete theApp.pendinglink;
	}

	// [TPT]
	// Maella -New Timer Management- (quick-n-dirty)
	StartMainTimer();
	// Maella end
}

void CemuleDlg::OnSysCommand(UINT nID, LPARAM lParam){
	// Systemmenu-Speedselector
	if (nID>=MP_QS_U10 && nID<=10512) {
		QuickSpeedUpload(nID);
		return;
	}
	if (nID>=MP_QS_D10 && nID<=10531) {
		QuickSpeedDownload(nID);
		return;
	}
	if (nID==MP_QS_PA || nID==MP_QS_UA) {
		QuickSpeedOther(nID);
		return;
	}
	
	switch (nID /*& 0xFFF0*/){
		case MP_ABOUTBOX : {
			CCreditsDlg dlgAbout;
			dlgAbout.DoModal();
			break;
		}
		case MP_VERSIONCHECK:
			DoVersioncheck(true);
			break;
		case MP_CONNECT : {
			StartConnection();
			break;
		}
		case MP_DISCONNECT : {
			CloseConnection();
			break;
		}
		default:{
			CTrayDialog::OnSysCommand(nID, lParam);
		}
	}

	if (
		(nID & 0xFFF0) == SC_MINIMIZE ||
		(nID & 0xFFF0) == SC_MINIMIZETRAY ||
		(nID & 0xFFF0) == SC_RESTORE ||
		(nID & 0xFFF0) == SC_MAXIMIZE ) { 
		ShowTransferRate(true);
		ShowPing();
		transferwnd->UpdateCatTabTitles();
	}
}

void CemuleDlg::PostStartupMinimized()
{
	if (!m_bStartMinimizedChecked)
{
		//TODO: Use full initialized 'WINDOWPLACEMENT' and remove the 'OnCancel' call...
		// Isn't that easy.. Read comments in OnInitDialog..
		m_bStartMinimizedChecked = true;
		if(m_bStartMinimized)
		{
			if(theApp.DidWeAutoStart())
			{
				if (!thePrefs.mintotray) {
					thePrefs.mintotray = true;
					OnCancel();
					thePrefs.mintotray = false;
				}
				else
					OnCancel();
			}
			else
			OnCancel();
	}
		}
	}

void CemuleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
		CTrayDialog::OnPaint();
	}

HCURSOR CemuleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CemuleDlg::OnBnClickedButton2(){
	if (!theApp.IsConnected())
		//connect if not currently connected
		if (!theApp.serverconnect->IsConnecting() && !Kademlia::CKademlia::isRunning() ){
			StartConnection();
		}
		else {
			CloseConnection();
		}
	else{
		//disconnect if currently connected
		CloseConnection();
	}
}

void CemuleDlg::ResetLog(){
	serverwnd->logbox->Reset();
}

void CemuleDlg::ResetDebugLog(){
	serverwnd->debuglog->Reset();
}

// [TPT] - Debug log
void CemuleDlg::ResetPhoenixLog(){
	serverwnd->phoenixlog->Reset();
}
// [TPT] - Debug log

void CemuleDlg::AddLogText(TbnMsg msg, UINT uFlags, LPCTSTR pszText) { //<<-- [TPT] - enkeyDEV(th1) -notifier-
	if (GetCurrentThreadId() != _uMainThreadId)
	{
		theApp.QueueLogLineEx(uFlags, _T("%s"), pszText);
		return;
	}

	if (uFlags & LOG_STATUSBAR)
	{
        if (statusbar->m_hWnd /*&& ready*/)
		{
			if (theApp.m_app_state != APP_STATE_SHUTINGDOWN)
				statusbar->SetText(pszText, SBarLog, 0);
		}
		else
			AfxMessageBox(pszText);
	}
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	Debug(_T("%s\n"), pszText);
#endif

	if ((uFlags & LOG_DEBUG) && !thePrefs.GetVerbose())
		return;

	TCHAR temp[1060];
	int iLen = _sntprintf(temp, ARRSIZE(temp), _T("%s: %s\r\n"), CTime::GetCurrentTime().Format(thePrefs.GetDateTimeFormat4Log()), pszText);
	if (iLen >= 0)
	{
		if (!(uFlags & LOG_DEBUG))
		{
			serverwnd->logbox->AddTyped(temp, iLen, uFlags);
			if (IsWindow(serverwnd->StatusSelector) && serverwnd->StatusSelector.GetCurSel() != CServerWnd::PaneLog)
				serverwnd->StatusSelector.HighlightItem(CServerWnd::PaneLog, TRUE);
			if (ready)
				ShowNotifier(pszText, msg, false);	//<<-- [TPT] - enkeyDEV(th1) -notifier-
			if (thePrefs.GetLog2Disk())
				theLog.Log(temp, iLen);
		}

		if (thePrefs.GetVerbose() && ((uFlags & LOG_DEBUG) || thePrefs.GetFullVerbose()))
		{
			serverwnd->debuglog->AddTyped(temp, iLen, uFlags);
			if (IsWindow(serverwnd->StatusSelector) && serverwnd->StatusSelector.GetCurSel() != CServerWnd::PaneVerboseLog)
				serverwnd->StatusSelector.HighlightItem(CServerWnd::PaneVerboseLog, TRUE);

			if (thePrefs.GetDebug2Disk())
				theVerboseLog.Log(temp, iLen);
		}
	}
}

//>> [TPT] - Debug log
void CemuleDlg::AddPhoenixText(TbnMsg msg, UINT uFlags, LPCTSTR pszText) { //<<-- [TPT] - enkeyDEV(th1) -notifier-
	if (GetCurrentThreadId() != _uMainThreadId)
	{
		theApp.QueueLogLine(uFlags, _T("%s"), pszText);
		return;
	}

	if (uFlags & LOG_STATUSBAR)
	{
        if (statusbar->m_hWnd /*&& ready*/)
		{
			if (theApp.m_app_state != APP_STATE_SHUTINGDOWN)
				statusbar->SetText(pszText, SBarLog, 0);
		}
		else
			AfxMessageBox(pszText);
	}
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	Debug(_T("%s\n"), pszText);
#endif

	if ((uFlags & LOG_DEBUG) && !thePrefs.GetVerbose())
		return;

	TCHAR temp[1060];
	int iLen = _sntprintf(temp, ARRSIZE(temp), _T("%s: %s\r\n"), CTime::GetCurrentTime().Format(thePrefs.GetDateTimeFormat4Log()), pszText);
	if (iLen >= 0)
	{
		if (!(uFlags & LOG_DEBUG))
		{
			serverwnd->logbox->AddTyped(temp, iLen, uFlags);
			if (IsWindow(serverwnd->StatusSelector) && serverwnd->StatusSelector.GetCurSel() != CServerWnd::PaneLog)
				serverwnd->StatusSelector.HighlightItem(CServerWnd::PaneLog, TRUE);
			if (ready)
				ShowNotifier(pszText, msg, false);	//<<-- [TPT] - enkeyDEV(th1) -notifier-
			if (thePrefs.GetLog2Disk())
				theLog.Log(temp, iLen);
		}

		if (thePrefs.GetVerbose() && ((uFlags & LOG_DEBUG) || thePrefs.GetFullVerbose()))
		{
			serverwnd->phoenixlog->AddTyped(temp, iLen, uFlags);
			if (IsWindow(serverwnd->StatusSelector) && serverwnd->StatusSelector.GetCurSel() != CServerWnd::PanePhoenixLog)
				serverwnd->StatusSelector.HighlightItem(CServerWnd::PanePhoenixLog, TRUE);

			if (thePrefs.GetDebug2Disk())
				theVerboseLog.Log(temp, iLen);
		}
	}
}
//<< [TPT] - Debug log




CString CemuleDlg::GetLastLogEntry()
{
	return serverwnd->logbox->GetLastLogEntry();
}

CString CemuleDlg::GetAllLogEntries()
{
	return serverwnd->logbox->GetAllLogEntries();
}

CString CemuleDlg::GetLastDebugLogEntry()
{
	return serverwnd->debuglog->GetLastLogEntry();
}

CString CemuleDlg::GetAllDebugLogEntries()
{
	return serverwnd->debuglog->GetAllLogEntries();
}

void CemuleDlg::AddServerMessageLine(LPCTSTR pszLine)
{
	serverwnd->servermsgbox->AppendText(pszLine + CString(_T('\n')));
	if (IsWindow(serverwnd->StatusSelector) && serverwnd->StatusSelector.GetCurSel() != CServerWnd::PaneServerInfo)
		serverwnd->StatusSelector.HighlightItem(CServerWnd::PaneServerInfo, TRUE);
}

void CemuleDlg::ShowConnectionStateIcon()
{
	if (theApp.serverconnect->IsConnected() && !Kademlia::CKademlia::isConnected())
	{
		if (theApp.serverconnect->IsLowID())
			statusbar->SetIcon(SBarConnected, connicons[3]); // LowNot
		else
			statusbar->SetIcon(SBarConnected, connicons[6]); // HighNot
	}
	else if (!theApp.serverconnect->IsConnected() && Kademlia::CKademlia::isConnected())
	{
		if (Kademlia::CKademlia::isFirewalled())
			statusbar->SetIcon(SBarConnected, connicons[1]); // NotLow
		else
			statusbar->SetIcon(SBarConnected, connicons[2]); // NotHigh
	}
	else if (theApp.serverconnect->IsConnected() && Kademlia::CKademlia::isConnected())
	{
		if (theApp.serverconnect->IsLowID() && Kademlia::CKademlia::isFirewalled())
			statusbar->SetIcon(SBarConnected, connicons[4]); // LowLow
		else if (theApp.serverconnect->IsLowID())
			statusbar->SetIcon(SBarConnected, connicons[5]); // LowHigh
		else if (Kademlia::CKademlia::isFirewalled())
			statusbar->SetIcon(SBarConnected, connicons[7]); // HighLow
		else
			statusbar->SetIcon(SBarConnected, connicons[8]); // HighHigh
	}
	else
	{
		statusbar->SetIcon(SBarConnected, connicons[0]); // NotNot
	}
}

CString CemuleDlg::GetConnectionStateString()
{
	CString status;
	if (theApp.serverconnect->IsConnected())
		status = _T("eD2K:") + GetResString(IDS_CONNECTED);
	else if (theApp.serverconnect->IsConnecting())
		status = _T("eD2K:") + GetResString(IDS_CONNECTING);
	else
		status = _T("eD2K:") + GetResString(IDS_NOTCONNECTED);

	if (Kademlia::CKademlia::isConnected())
		status += _T("|Kad:") + GetResString(IDS_CONNECTED);
	else if (Kademlia::CKademlia::isRunning())
		status += _T("|Kad:") + GetResString(IDS_CONNECTING);
	else
		status += _T("|Kad:") + GetResString(IDS_NOTCONNECTED);
	return status;
}

void CemuleDlg::ShowConnectionState()
{
	theApp.downloadqueue->OnConnectionState(theApp.IsConnected());
	serverwnd->UpdateMyInfo();
	serverwnd->UpdateControlsState();
	kademliawnd->UpdateControlsState();

	ShowConnectionStateIcon();
	statusbar->SetText(GetConnectionStateString(), SBarConnected, 0);

	if (theApp.IsConnected())
	{
		CString strPane(GetResString(IDS_MAIN_BTN_DISCONNECT));
		TBBUTTONINFO tbi;
		tbi.cbSize = sizeof(TBBUTTONINFO);
		tbi.dwMask = TBIF_IMAGE | TBIF_TEXT;
		tbi.iImage = 1;
		tbi.pszText = const_cast<LPTSTR>((LPCTSTR)strPane);
		toolbar->SetButtonInfo(IDC_TOOLBARBUTTON+0, &tbi);
	}
	else
	{
		if (theApp.serverconnect->IsConnecting() || Kademlia::CKademlia::isRunning()) 
		{
			CString strPane(GetResString(IDS_MAIN_BTN_CANCEL));
			TBBUTTONINFO tbi;
			tbi.cbSize = sizeof(TBBUTTONINFO);
			tbi.dwMask = TBIF_IMAGE | TBIF_TEXT;
			tbi.iImage = 2;
			tbi.pszText = const_cast<LPTSTR>((LPCTSTR)strPane);
			toolbar->SetButtonInfo(IDC_TOOLBARBUTTON+0, &tbi);
			ShowUserCount();
		} 
		else 
		{
			CString strPane(GetResString(IDS_MAIN_BTN_CONNECT));
			TBBUTTONINFO tbi;
			tbi.cbSize = sizeof(TBBUTTONINFO);
			tbi.dwMask = TBIF_IMAGE | TBIF_TEXT;
			tbi.iImage = 0;
			tbi.pszText = const_cast<LPTSTR>((LPCTSTR)strPane);
			toolbar->SetButtonInfo(IDC_TOOLBARBUTTON+0, &tbi);
			ShowUserCount();
		}

	}
}

void CemuleDlg::ShowUserCount()
{
	uint32 totaluser, totalfile;
	totaluser = totalfile = 0;
	theApp.serverlist->GetUserFileStatus( totaluser, totalfile );
	CString buffer;
	buffer.Format(_T("%s:%s(%s)|%s:%s(%s)"), GetResString(IDS_UUSERS), CastItoIShort(totaluser, false, 1), CastItoIShort(Kademlia::CKademlia::getKademliaUsers(), false, 1), GetResString(IDS_FILES), CastItoIShort(totalfile, false, 1), CastItoIShort(Kademlia::CKademlia::getKademliaFiles(), false, 1));
	statusbar->SetText(buffer, SBarUsers, 0);
}

void CemuleDlg::ShowMessageState(uint8 iconnr)
{
	m_iMsgIcon = iconnr;
	statusbar->SetIcon(SBarChatMsg, imicons[m_iMsgIcon]);
	// [TPT] - Now used to fix the tooltip minibug
	message = (iconnr==2);
}

void CemuleDlg::ShowTransferStateIcon()
{
	// [TPT]
	// Retrieve the current datarates
	uint32 eMuleIn;
	uint32 eMuleOut;
	uint32 notUsed;
	theApp.pBandWidthControl->GetDatarates(thePrefs.GetDatarateSamples(),
										   eMuleIn, notUsed,
										   eMuleOut, notUsed,
										   notUsed, notUsed);

	const float uploadRate = (float)eMuleOut / 1024.0f;
	const float downloadRate = (float)eMuleIn / 1024.0f;
	if(uploadRate > 0.0f && downloadRate > 0.0f)
		statusbar->SetIcon(SBarUpDown, transicons[3]);
	else if (uploadRate > 0.0f)
		statusbar->SetIcon(SBarUpDown, transicons[2]);
	else if (downloadRate > 0.0f)
		statusbar->SetIcon(SBarUpDown, transicons[1]);
	else
		statusbar->SetIcon(SBarUpDown, transicons[0]);
	// [TPT]
}



// [TPT]
// Maella [FAF] -Allow Bandwidth Settings in <1KB Incremements-
void CemuleDlg::ShowTransferRate(bool forceAll){
	TCHAR buffer[60];
	
	if (forceAll)
		m_uLastSysTrayIconCookie = SYS_TRAY_ICON_COOKIE_FORCE_UPDATE;
		
	// Retrieve the current datarates
	uint32 eMuleIn;	uint32 eMuleInOverall;
	uint32 eMuleOut; uint32 eMuleOutOverall;
	uint32 notUsed;
	theApp.pBandWidthControl->GetDatarates(thePrefs.GetDatarateSamples(),
										   eMuleIn, eMuleInOverall,
										   eMuleOut, eMuleOutOverall,
										   notUsed, notUsed);

	const float uploadRate = (float)eMuleOut / 1024.0f;
	const float downloadRate = (float)eMuleIn / 1024.0f;
	const float uploadOverheadRate = (float)(eMuleOutOverall - eMuleOut) / 1024.0f;
	const float downloadOverheadRate = (float)(eMuleInOverall - eMuleIn) / 1024.0f;

	// Update StatusBar text
	if(thePrefs.ShowOverhead() == false){
		_stprintf(buffer,GetResString(IDS_UPDOWNSMALL), uploadRate, downloadRate);			
	}
	else {
		_stprintf(buffer,GetResString(IDS_UPDOWN), uploadRate, uploadOverheadRate, downloadRate, downloadOverheadRate);
	}
	
	if (TrayIsVisible() || forceAll){
		TCHAR buffer2[100];
		// set trayicon-icon
		int DownRateProcent = (int)(100.0f * downloadRate / thePrefs.GetMaxGraphDownloadRate());
		if (DownRateProcent>100)
			DownRateProcent=100;
		UpdateTrayIcon(DownRateProcent);

		// [TPT] - New tray tooltip
		if (ExtensionsEnabled())
			if (theApp.IsConnected()) 
				// [TPT]- modID
				_sntprintf(buffer2,ARRSIZE(buffer2),_T("eMule v%s (%s)\r\n%s"),theApp.m_strCurVersionLong + _T(" ") + MOD_VERSION,GetResString(IDS_CONNECTED),buffer);
			else
				_sntprintf(buffer2,ARRSIZE(buffer2),_T("eMule v%s (%s)\r\n%s"),theApp.m_strCurVersionLong + _T(" ") + MOD_VERSION,GetResString(IDS_DISCONNECTED),buffer);
				// [TPT]- modID
			else
				_sntprintf(buffer2,ARRSIZE(buffer2),_T("%s"),buffer);
				

		if (ExtensionsEnabled())
			buffer2[127]=0;
		else
		buffer2[63]= _T('\0');
		// [TPT] - New tray tooltip
		TraySetToolTip(buffer2);
	}

	if (IsWindowVisible() || forceAll) {
		statusbar->SetText(buffer, SBarUpDown, 0);
		ShowTransferStateIcon();
	}
	if (IsWindowVisible() && thePrefs.ShowRatesOnTitle()) {
		// [TPT] - New tray tooltip
		if (ExtensionsEnabled())
			_sntprintf(buffer,ARRSIZE(buffer),_T("(U:%.1f D:%.1f) eMule v%s"), uploadRate, downloadRate, theApp.m_strCurVersionLong + _T(" ") + MOD_VERSION); // [TPT]- modID
		else
			_sntprintf(buffer,ARRSIZE(buffer),_T("        (U:%.1f D:%.1f) eMule v%s"), uploadRate, downloadRate, theApp.m_strCurVersionLong + _T(" ") + MOD_VERSION); // [TPT]- modID
		// [TPT] - New tray tooltip
		SetWindowText(buffer);
	}
	//[TPT] - Show in MSN7
	//Only update every 5 seconds and if enabled
	if(thePrefs.GetShowMSN7() && (::GetTickCount() - m_dwMSNtime > SEC2MS(5)))
	{
		UpdateMSN(uploadRate, uploadOverheadRate, downloadRate, downloadOverheadRate);
		m_dwMSNtime = ::GetTickCount();
	}
}
// Maella end

void CemuleDlg::ShowPing()
{
    if (IsWindowVisible())
	{
        CString buffer;
        if (thePrefs.IsDynUpEnabled())
		{
        CurrentPingStruct lastPing = theApp.lastCommonRouteFinder->GetCurrentPing();
            if (lastPing.state.GetLength() == 0)
			{
                if (lastPing.lowest > 0 && !thePrefs.IsDynUpUseMillisecondPingTolerance())
                    buffer.Format(_T("%.1f | %ims | %i%%"),lastPing.currentLimit/1024.0f, lastPing.latency, lastPing.latency*100/lastPing.lowest);
                else
                    buffer.Format(_T("%.1f | %ims"),lastPing.currentLimit/1024.0f, lastPing.latency);
                }
			else
                buffer.SetString(lastPing.state);
            }
		statusbar->SetText(buffer, SBarChatMsg, 0);
        }
    }

void CemuleDlg::OnOK()
{
}

void CemuleDlg::OnCancel()
{
	if (*thePrefs.GetMinTrayPTR()){
		// emulEspa�a: modified by [TPT]-MoNKi [MoNKi: -invisible mode-]
		/*
		TrayShow();
		ShowWindow(SW_HIDE);
		*/
		if (!thePrefs.GetInvisibleMode())
		{
			if(TrayShow())
				ShowWindow(SW_HIDE);
		}
		else
			ShowWindow(SW_HIDE);
		// End emulEspa�a
	} else {
		ShowWindow(SW_MINIMIZE);
	}
	ShowTransferRate();
    ShowPing();
}

void CemuleDlg::SetActiveDialog(CWnd* dlg)
{
	if (dlg == activewnd)
		return;
	if (activewnd)
		activewnd->ShowWindow(SW_HIDE);
	dlg->ShowWindow(SW_SHOW);
	dlg->SetFocus();
	activewnd = dlg;
	if (dlg == transferwnd){
		if (thePrefs.ShowCatTabInfos())
			transferwnd->UpdateCatTabTitles();
		toolbar->PressMuleButton(IDC_TOOLBARBUTTON+3);
	}
	else if (dlg == serverwnd){
		toolbar->PressMuleButton(IDC_TOOLBARBUTTON+2);
	}
	else if (dlg == chatwnd){
		toolbar->PressMuleButton(IDC_TOOLBARBUTTON+6);
		chatwnd->chatselector.ShowChat();
	}
	else if (dlg == ircwnd){
		toolbar->PressMuleButton(IDC_TOOLBARBUTTON+7);
	}
	else if (dlg == sharedfileswnd){
		toolbar->PressMuleButton(IDC_TOOLBARBUTTON+5);
	}
	else if (dlg == searchwnd){
		toolbar->PressMuleButton(IDC_TOOLBARBUTTON+4);
	}
	else if (dlg == statisticswnd){
		toolbar->PressMuleButton(IDC_TOOLBARBUTTON+8);
		statisticswnd->ShowStatistics();
	}
	else if	(dlg == kademliawnd){
		toolbar->PressMuleButton(IDC_TOOLBARBUTTON+1);
	}
}

void CemuleDlg::SetStatusBarPartsSize()
{
	CRect rect;
	statusbar->GetClientRect(&rect);
	int ussShift = 0;
	if(thePrefs.IsDynUpEnabled())
	{
        if (thePrefs.IsDynUpUseMillisecondPingTolerance())
            ussShift = 45;
        else
            ussShift = 90;
        }
	
	int aiWidths[5] =
	{ 
		rect.right - 675 - ussShift,
		rect.right - 440 - ussShift,
		rect.right - 250 - ussShift,
		rect.right -  25 - ussShift,
		-1
	};
	statusbar->SetParts(ARRSIZE(aiWidths), aiWidths);
}

void CemuleDlg::OnSize(UINT nType, int cx, int cy)
{
	CTrayDialog::OnSize(nType, cx, cy);
	SetStatusBarPartsSize();
	transferwnd->VerifyCatTabSize();
}

void CemuleDlg::ProcessED2KLink(LPCTSTR pszData)
{
	try {
		CString link2;
		CString link;
		link2 = pszData;
		link2.Replace(_T("%7c"),_T("|"));
		link = OptUtf8ToStr(URLDecode(link2));
		CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(link);
		_ASSERT( pLink !=0 );
		switch (pLink->GetKind()) {
		case CED2KLink::kFile:
			{
				// [TPT] - khaos::categorymod+ Need to allocate memory so that our pointer
				// remains valid until the link is added and removed from queue...
				CED2KFileLink* pFileLink = (CED2KFileLink*)CED2KLink::CreateLinkFromUrl(link.Trim());
				_ASSERT(pFileLink !=0);
					
				// [TPT] - MoNKi: -Check already downloaded files-
				/*
				theApp.downloadqueue->AddFileLinkToDownload(pFileLink,searchwnd->GetSelectedCat());
				*/
				if(theApp.knownfiles->CheckAlreadyDownloadedFileQuestion(pFileLink->GetHashKey(),pFileLink->GetName()))
				{
					theApp.downloadqueue->AddFileLinkToDownload(pFileLink, true);
					// This memory will be deallocated in CDownloadQueue::AddFileLinkToDownload
					// or CDownloadQueue::PurgeED2KFileLinkQueue.
					// [TPT] - khaos::categorymod-
				}
				// [TPT] - Add sources
				else
				{
					theApp.downloadqueue->AddSources(pFileLink);
					delete pFileLink;
				}
				// [TPT] - Add sources					
				// [TPT] - MoNKi: -Check already downloaded files-
			}
			break;
		case CED2KLink::kServerList:
			{
				CED2KServerListLink* pListLink = pLink->GetServerListLink(); 
				_ASSERT( pListLink !=0 ); 
				CString strAddress = pListLink->GetAddress(); 
				if(strAddress.GetLength() != 0)
					serverwnd->UpdateServerMetFromURL(strAddress);
			}
			break;
		case CED2KLink::kServer:
			{
				CString defName;
				CED2KServerLink* pSrvLink = pLink->GetServerLink();
				_ASSERT( pSrvLink !=0 );
				CServer* pSrv = new CServer(pSrvLink->GetPort(), ipstr(pSrvLink->GetIP()));
				_ASSERT( pSrv !=0 );
				pSrvLink->GetDefaultName(defName);
				pSrv->SetListName(defName.GetBuffer());

				// Barry - Default all new servers to high priority
				if (thePrefs.GetManualHighPrio())
					pSrv->SetPreference(SRV_PR_HIGH);

				if (!serverwnd->serverlistctrl.AddServer(pSrv,true)) 
					delete pSrv; 
				else
					AddLogLine(true,GetResString(IDS_SERVERADDED), pSrv->GetListName());
			}
			break;
		// [TPT] - Announ: -Friend eLinks-
		case CED2KLink::kFriend:
			{
				// Better with dynamic_cast, but no RTTI enabled in the project
				CED2KFriendLink* pFriendLink = static_cast<CED2KFriendLink*>(pLink);
				uchar userHash[16];
				pFriendLink->GetUserHash(userHash);

				if ( ! theApp.friendlist->IsAlreadyFriend(userHash) )
					theApp.friendlist->AddFriend(userHash, 0U, 0U, 0U, 0U, pFriendLink->GetUserName(), 1U);
				else
				{
					CString msg;
					msg.Format(GetResString(IDS_USER_ALREADY_FRIEND), pFriendLink->GetUserName());
					AddLogLine(true, msg);
				}
			}
			break;
		case CED2KLink::kFriendList:
			{
				// Better with dynamic_cast, but no RTTI enabled in the project
				CED2KFriendListLink* pFrndLstLink = static_cast<CED2KFriendListLink*>(pLink);
				CString sAddress = pFrndLstLink->GetAddress(); 
				if ( !sAddress.IsEmpty() )
					chatwnd->UpdateEmfriendsMetFromURL(sAddress);
			}
			break;
		// End -Friend eLinks-
		default:
			break;
		}
		delete pLink;
	}
	catch(...){
		LogWarning(LOG_STATUSBAR, GetResString(IDS_LINKNOTADDED));
		ASSERT(0);
	}
}

LRESULT CemuleDlg::OnWMData(WPARAM wParam,LPARAM lParam)
{
	PCOPYDATASTRUCT data = (PCOPYDATASTRUCT)lParam;
	if (data->dwData == OP_ED2KLINK)
	{
		FlashWindow(true);
		if (thePrefs.IsBringToFront())
		{
			if (IsIconic())
				ShowWindow(SW_SHOWNORMAL);
			else if (TrayHide())
				RestoreWindow();
			else
				SetForegroundWindow();
		}
		ProcessED2KLink((LPCTSTR)data->lpData);
	}
	else if (data->dwData == OP_CLCOMMAND){
		// command line command received
		CString clcommand((LPCTSTR)data->lpData);
		clcommand.MakeLower();
		AddLogLine(true,_T("CLI: %s"),clcommand);

		if (clcommand==_T("connect")) {StartConnection(); return true;}
		if (clcommand==_T("disconnect")) {theApp.serverconnect->Disconnect(); return true;}
		if (clcommand==_T("resume")) {theApp.downloadqueue->StartNextFile(); return true;}
		if (clcommand==_T("exit")) {OnClose(); return true;}
		if (clcommand==_T("restore")) {RestoreWindow();return true;}
		if (clcommand==_T("reloadipf")) {theApp.ipfilter->LoadFromDefaultFile(); return true;}
		if (clcommand.Left(7).MakeLower()==_T("limits=") && clcommand.GetLength()>8) {
			CString down;
			CString up=clcommand.Mid(7);
			int pos=up.Find(_T(','));
			if (pos>0) {
				down=up.Mid(pos+1);
				up=up.Left(pos);
			}
		
			// [TPT] - Maella [FAF] -Allow Bandwidth Settings in <1KB Incremements-
			if (down.GetLength()>0) thePrefs.SetMaxDownload(_tstof(down));
			if (up.GetLength()>0) thePrefs.SetMaxUpload(_tstof(up));									
			// Maella end

			return true;
		}

		if (clcommand==_T("help") || clcommand==_T("/?")) {
			// show usage
			return true;
		}

		if (clcommand==_T("status")) {
			CString strBuff;
			strBuff.Format(_T("%sstatus.log"),thePrefs.GetAppDir());
			FILE* file = _tfsopen(strBuff, _T("wt"), _SH_DENYWR);
			if (file){
				if (theApp.serverconnect->IsConnected())
					strBuff = GetResString(IDS_CONNECTED);
				else if (theApp.serverconnect->IsConnecting())
					strBuff = GetResString(IDS_CONNECTING);
				else
					strBuff = GetResString(IDS_DISCONNECTED);
				_ftprintf(file, _T("%s\n"), strBuff);

				// [TPT]
				// Maella -Accurate measure of bandwidth: eDonkey data + control, network adapter-
				uint32 eMuleIn;
				uint32 eMuleOut;
				uint32 notUsed;
				theApp.pBandWidthControl->GetDatarates(thePrefs.GetDatarateSamples(),
														eMuleIn, notUsed,
													   eMuleOut, notUsed,
													   notUsed, notUsed);
				

				strBuff.Format(GetResString(IDS_UPDOWNSMALL), (float)eMuleOut/1024, (float)eMuleIn/1024);
				// Maella end
				_ftprintf(file, _T("%s"), strBuff); // next string (getTextList) is already prefixed with '\n'!
				_ftprintf(file, _T("%s\n"), transferwnd->downloadlistctrl.getTextList());
				
				fclose(file);
			}
			return true;
		}
		// show "unknown command";
	}
	return true;
}

LRESULT CemuleDlg::OnFileHashed(WPARAM wParam, LPARAM lParam)
{
	if (theApp.m_app_state == APP_STATE_SHUTINGDOWN)
		return FALSE;

	CKnownFile* result = (CKnownFile*)lParam;
	ASSERT( result->IsKindOf(RUNTIME_CLASS(CKnownFile)) );

	if (wParam)
	{
		// File hashing finished for a part file when:
		// - part file just completed
		// - part file was rehashed at startup because the file date of part.met did not match the part file date

		CPartFile* requester = (CPartFile*)wParam;
		ASSERT( requester->IsKindOf(RUNTIME_CLASS(CPartFile)) );

		// SLUGFILLER: SafeHash - could have been canceled
		if (theApp.downloadqueue->IsPartFile(requester))
			requester->PartFileHashFinished(result);
		else
			delete result;
		// SLUGFILLER: SafeHash
	}
	else
	{
		ASSERT( !result->IsKindOf(RUNTIME_CLASS(CPartFile)) );

		// File hashing finished for a shared file (none partfile)
		//	- reading shared directories at startup and hashing files which were not found in known.met
		//	- reading shared directories during runtime (user hit Reload button, added a shared directory, ...)
		theApp.sharedfiles->FileHashingFinished(result);
	}
	return TRUE;
}

LRESULT CemuleDlg::OnFileOpProgress(WPARAM wParam, LPARAM lParam)
{
	if (theApp.m_app_state == APP_STATE_SHUTINGDOWN)
		return FALSE;

	CKnownFile* pKnownFile = (CKnownFile*)lParam;
	ASSERT( pKnownFile->IsKindOf(RUNTIME_CLASS(CKnownFile)) );

	if (pKnownFile->IsKindOf(RUNTIME_CLASS(CPartFile)))
	{
		CPartFile* pPartFile = static_cast<CPartFile*>(pKnownFile);
		pPartFile->SetFileOpProgress(wParam);
		pPartFile->UpdateDisplayedInfo(true);
	}

	return 0;
}

// SLUGFILLER: SafeHash
LRESULT CemuleDlg::OnHashFailed(WPARAM wParam, LPARAM lParam)
{
	theApp.sharedfiles->HashFailed((UnknownFile_Struct*)lParam);
	return 0;
}
// SLUGFILLER: SafeHash

LRESULT CemuleDlg::OnFileAllocExc(WPARAM wParam,LPARAM lParam)
{
	if (lParam == 0)
		((CPartFile*)wParam)->FlushBuffersExceptionHandler();
	else
		((CPartFile*)wParam)->FlushBuffersExceptionHandler((CFileException*)lParam);
	return 0;
}

LRESULT CemuleDlg::OnFileCompleted(WPARAM wParam, LPARAM lParam)
{
	CPartFile* partfile = (CPartFile*)lParam;
	ASSERT( partfile != NULL );
	if (partfile)
		partfile->PerformFileCompleteEnd(wParam);
	return 0;
}

bool CemuleDlg::CanClose()
{
	if (theApp.m_app_state == APP_STATE_RUNNING && thePrefs.IsConfirmExitEnabled())
	{
		//[TPT] - Fade Window on Exit Rework
		CFadeWnd *fadeWnd = NULL;
		if ((DetectWinVersion()==_WINVER_XP_) && thePrefs.GetFadeOut()){
			fadeWnd = new CFadeWnd(this);
		}

		int msgboxResult = AfxMessageBox(GetResString(IDS_MAIN_EXIT), MB_YESNO | MB_DEFBUTTON2);
		if(fadeWnd){
			fadeWnd->WaitUntilFinish();
			delete fadeWnd;
		}
		if (msgboxResult == IDNO)
			return false;
	}
	return true;
}

void CemuleDlg::OnClose()
{
	if (!CanClose())
		return;

	m_pDropTarget->Revoke();
	// [TPT]		
	// Maella -New Timer Management- (quick-n-dirty)
	StopMainTimer();
	// Maella end

	//[TPT] - Show in MSN7
	//if we have enabled on close, we must send the kill message
	if(thePrefs.GetShowMSN7())
		UpdateMSN(0,0,0,0, true);

	theApp.m_app_state = APP_STATE_SHUTINGDOWN;
	// [TPT] - Moonlight: Global shutdown event to wake up threads.
	ShutdownEvent.Signal();     // Moonlight: Global shutdown event signalled on exit.
    Sleep(100);                 // Moonlight: Give the threads 100ms to get a clue.
	// [TPT] - Moonlight: Global shutdown event to wake up threads.
	theApp.serverconnect->Disconnect();
	theApp.OnlineSig(); // Added By Bouc7 

	// get main window placement
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	GetWindowPlacement(&wp);
	ASSERT( wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_SHOWMINIMIZED || wp.showCmd == SW_SHOWNORMAL );
	if (wp.showCmd == SW_SHOWMINIMIZED && (wp.flags & WPF_RESTORETOMAXIMIZED))
		wp.showCmd = SW_SHOWMAXIMIZED;
	wp.flags = 0;
	thePrefs.SetWindowLayout(wp);

	// get active main window dialog
	if (activewnd){
		if (activewnd->IsKindOf(RUNTIME_CLASS(CServerWnd)))
			thePrefs.SetLastMainWndDlgID(IDD_SERVER);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CSharedFilesWnd)))
			thePrefs.SetLastMainWndDlgID(IDD_FILES);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CSearchDlg)))
			thePrefs.SetLastMainWndDlgID(IDD_SEARCH);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CChatWnd)))
			thePrefs.SetLastMainWndDlgID(IDD_CHAT);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CTransferWnd)))
			thePrefs.SetLastMainWndDlgID(IDD_TRANSFER);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CStatisticsDlg)))
			thePrefs.SetLastMainWndDlgID(IDD_STATISTICS);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CKademliaWnd)))
			thePrefs.SetLastMainWndDlgID(IDD_KADEMLIAWND);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CIrcWnd)))
			thePrefs.SetLastMainWndDlgID(IDD_IRC);
		else{
			ASSERT(0);
			thePrefs.SetLastMainWndDlgID(0);
		}
	}

	Kademlia::CKademlia::stop();

	preferenceswnd->CloseDialog();	// [TPT] - SLUGFILLER: modelessDialogs
	
	// try to wait untill the hashing thread notices that we are shutting down
	CSingleLock sLock1(&theApp.hashing_mut); // only one filehash at a time
	sLock1.Lock(2000);

	//[TPT] - Unlimited upload with no downloads
	//We need to save the temp var we have used
	if(thePrefs.GetUnlimitedUp() && theApp.downloadqueue->unlimitedFlag)
		thePrefs.SetMaxUpload(theApp.downloadqueue->upTemp);

	// saving data & stuff	
	theApp.emuledlg->preferenceswnd->m_wndSecurity.DeleteDDB();
	theApp.knownfiles->Save();
	transferwnd->downloadlistctrl.SaveSettings(CPreferences::tableDownload);
	transferwnd->downloadclientsctrl.SaveSettings(CPreferences::tableDownloadClients);// [TPT] - TBH Transfers Window Style
	transferwnd->uploadlistctrl.SaveSettings(CPreferences::tableUpload);
	transferwnd->queuelistctrl.SaveSettings(CPreferences::tableQueue);
	transferwnd->clientlistctrl.SaveSettings(CPreferences::tableClientList);
	chatwnd->m_FriendListCtrl.SaveSettings(CPreferences::tableFriendList); // [TPT] - Friend State Column
	searchwnd->SaveAllSettings();
	sharedfileswnd->sharedfilesctrl.SaveSettings(CPreferences::tableShared);
	sharedfileswnd->historylistctrl.SaveSettings(CPreferences::tableHistory); // [TPT] - MoNKi: -Downloaded History-	
	serverwnd->SaveAllSettings();
	kademliawnd->SaveAllSettings();

	theApp.m_pPeerCache->Save();
	theApp.scheduler->RestoreOriginals();
	thePrefs.Save();
	thePerfLog.Shutdown();

	// [TPT]- TBH-AutoBackup Begin
	if (thePrefs.GetAutoBackup2())
		preferenceswnd->m_wndPhoenix1.Backup3();
	if (thePrefs.GetAutoBackup())
	{
		preferenceswnd->m_wndPhoenix1.Backup(_T("*.ini"), false);
		preferenceswnd->m_wndPhoenix1.Backup(_T("*.dat"), false);
		preferenceswnd->m_wndPhoenix1.Backup(_T("*.met"), false);
	}
	// [TPT] - TBH-AutoBackup End

	if(thePrefs.GetInvisibleMode()) 
		UnRegisterInvisibleHotKey();	// emulEspa�a: added by [TPT]-MoNKi [MoNKi: -invisible mode-]

	// explicitly delete all listview items which may hold ptrs to objects which will get deleted
	// by the dtors (some lines below) to avoid potential problems during application shutdown.
	transferwnd->downloadlistctrl.DeleteAllItems();
	transferwnd->downloadclientsctrl.DeleteAllItems();
	chatwnd->chatselector.DeleteAllItems();
	theApp.clientlist->DeleteAll();
	searchwnd->DeleteAllSearchListCtrlItems();
	sharedfileswnd->sharedfilesctrl.DeleteAllItems();
    transferwnd->queuelistctrl.DeleteAllItems();
	transferwnd->clientlistctrl.DeleteAllItems();
	transferwnd->uploadlistctrl.DeleteAllItems();
	
	CPartFileConvert::CloseGUI();
	CPartFileConvert::RemoveAllJobs();

	// [TPT] - [MoNKi: -UPnPNAT Support-]
	theApp.m_UPnPNat->RemoveAllMappings();
	theApp.m_UPnPNat->RemoveInstance();

    theApp.uploadBandwidthThrottler->EndThread();
    theApp.lastCommonRouteFinder->EndThread();

	theApp.sharedfiles->DeletePartFileInstances();

	searchwnd->SendMessage(WM_CLOSE);

    // NOTE: Do not move those dtors into 'CemuleApp::InitInstance' (althought they should be there). The
	// dtors are indirectly calling functions which access several windows which would not be available 
	// after we have closed the main window -> crash!
	delete theApp.mmserver;			theApp.mmserver = NULL;
	delete theApp.listensocket;		theApp.listensocket = NULL;
	delete theApp.clientudp;		theApp.clientudp = NULL;
	delete theApp.sharedfiles;		theApp.sharedfiles = NULL;
	delete theApp.serverconnect;	theApp.serverconnect = NULL;
	delete theApp.serverlist;		theApp.serverlist = NULL;
	delete theApp.knownfiles;		theApp.knownfiles = NULL;
	delete theApp.searchlist;		theApp.searchlist = NULL;
	delete theApp.clientcredits;	theApp.clientcredits = NULL;
	delete theApp.downloadqueue;	theApp.downloadqueue = NULL;
	delete theApp.uploadqueue;		theApp.uploadqueue = NULL;
	// [TPT] - TBH: minimule
	theApp.minimule->DestroyWindow();
	delete theApp.minimule;			theApp.minimule = NULL;
	// [TPT] - TBH: minimule
	delete theApp.clientlist;		theApp.clientlist = NULL;
	delete theApp.friendlist;		theApp.friendlist = NULL;
	delete theApp.scheduler;		theApp.scheduler = NULL;
	delete theApp.ipfilter;			theApp.ipfilter = NULL;
	delete theApp.webserver;		theApp.webserver = NULL;
	delete theApp.m_pPeerCache;		theApp.m_pPeerCache = NULL;
	delete theApp.m_pFirewallOpener;theApp.m_pFirewallOpener = NULL;
	delete theApp.wombatlist;// [TPT] - eWombat SNAFU v2	
	// [TPT] - Maella [patch] -Bandwidth: overall bandwidth measure-	
	delete theApp.pBandWidthControl;theApp.pBandWidthControl = NULL;
	// Maella end
	delete theApp.ip2country;		theApp.ip2country = NULL;// [TPT] - IP Country
	delete theApp.FakeCheck;		theApp.FakeCheck = NULL; // [TPT] - Fakecheck

	// ZZ:UploadSpeedSense -->
	delete theApp.uploadBandwidthThrottler; theApp.uploadBandwidthThrottler = NULL;
	delete theApp.lastCommonRouteFinder; theApp.lastCommonRouteFinder = NULL;
	// ZZ:UploadSpeedSense <--

	thePrefs.Uninit();
	//CTitleMenu::FreeAPI();
	theApp.m_app_state = APP_STATE_DONE;
	CTrayDialog::OnCancel();
}

void CemuleDlg::OnTrayRButtonUp(CPoint pt)
{
	if(m_pSystrayDlg)
	{
		m_pSystrayDlg->BringWindowToTop();
		return;
	}
	
	m_pSystrayDlg = new CMuleSystrayDlg(this,pt, 
									thePrefs.GetMaxGraphUploadRate(), 
									thePrefs.GetMaxGraphDownloadRate(),
									// [TPT] - NAFC upload
									thePrefs.GetMaxUpload(),
									// [TPT] - NAFC upload
									thePrefs.GetMaxDownload());
	if(m_pSystrayDlg)
	{
		UINT nResult = m_pSystrayDlg->DoModal();
		delete m_pSystrayDlg;
		m_pSystrayDlg = NULL;
		switch(nResult)
		{
			case IDC_TOMAX:
				QuickSpeedOther(MP_QS_UA);
				break;
			case IDC_TOMIN:
				QuickSpeedOther(MP_QS_PA);
				break;
			case IDC_RESTORE:
				RestoreWindow();
				break;
			case IDC_CONNECT:
				StartConnection();
				break;
			case IDC_DISCONNECT:
				CloseConnection();
				break;
			case IDC_EXIT:
				OnClose();
				break;
			case IDC_PREFERENCES:
				ShowPreferences();
				break;
			// [TPT] - TBH: minimule
			case IDC_MINIMULE:
				{	
					if (thePrefs.GetMiniMuleLives())
						theApp.minimule->RunMiniMule();
	else
						theApp.minimule->RunMiniMule(true);
					theApp.minimule->ShowWindow(SW_SHOW);
					break;
				}
			// [TPT] - TBH: minimule
			default:
				break;
		}
	}
}

// [TPT] - enkeyDEV(th1) -notifier-
void CemuleDlg::ForceNotifierPopup() { //<<-- enkeyDEV(kei-kun) -Manual notifier popup-
	if (notifierenabled)
		m_wndTaskbarNotifier->ShowLastHistoryMessage();
}

void CemuleDlg::SwitchNotifierStatus() { 
	if (!notifierenabled)
		LoadNotifier(); //<<-- reload notifier to take care of eventually moved/changed skin
	else
		notifierenabled=false;
}; 
// [TPT] - enkeyDEV(th1) -notifier-


// Maella [FAF] -Allow Bandwidth Settings in <1KB Incremements-
void CemuleDlg::AddSpeedSelectorSys(CMenu* addToMenu)
{
	CString text;

	// creating UploadPopup Menu
	ASSERT( m_menuUploadCtrl.m_hMenu == NULL );
	if (m_menuUploadCtrl.CreateMenu())
	{
		//m_menuUploadCtrl.AddMenuTitle(GetResString(IDS_PW_TIT_UP));
		text.Format(_T("20%%\t%0.1f %s"),  (0.2f * thePrefs.GetMaxGraphUploadRate()*0.2),GetResString(IDS_KBYTESEC));	m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_U20,  text);
		text.Format(_T("40%%\t%0.1f %s"),  (0.4f * thePrefs.GetMaxGraphUploadRate()*0.4),GetResString(IDS_KBYTESEC));	m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_U40,  text);
		text.Format(_T("60%%\t%0.1f %s"),  (0.6f * thePrefs.GetMaxGraphUploadRate()*0.6),GetResString(IDS_KBYTESEC));	m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_U60,  text);
		text.Format(_T("80%%\t%0.1f %s"),  (0.8f * thePrefs.GetMaxGraphUploadRate()*0.8),GetResString(IDS_KBYTESEC));	m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_U80,  text);
		text.Format(_T("100%%\t%0.1f %s"), (1.0f * thePrefs.GetMaxGraphUploadRate()),GetResString(IDS_KBYTESEC));		m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_U100, text);
		m_menuUploadCtrl.AppendMenu(MF_SEPARATOR);
		
		if (GetRecMaxUpload() > 0.0f) {
			text.Format(GetResString(IDS_PW_MINREC)+GetResString(IDS_KBYTESEC),GetRecMaxUpload());
			m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_UP10, text );
		}

		//m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_UPC, GetResString(IDS_PW_UNLIMITED));

		text.Format(_T("%s:"), GetResString(IDS_PW_UPL));
		addToMenu->AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)m_menuUploadCtrl.m_hMenu, text);
	}

	// creating DownloadPopup Menu
	ASSERT( m_menuDownloadCtrl.m_hMenu == NULL );
	if (m_menuDownloadCtrl.CreateMenu())
	{
		//m_menuDownloadCtrl.AddMenuTitle(GetResString(IDS_PW_TIT_DOWN));
		text.Format(_T("20%%\t%0.1f %s"),  (0.2f * thePrefs.GetMaxGraphDownloadRate()*0.2),GetResString(IDS_KBYTESEC));	m_menuDownloadCtrl.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D20,  text);
		text.Format(_T("40%%\t%0.1f %s"),  (0.4f * thePrefs.GetMaxGraphDownloadRate()*0.4),GetResString(IDS_KBYTESEC));	m_menuDownloadCtrl.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D40,  text);
		text.Format(_T("60%%\t%0.1f %s"),  (0.6f * thePrefs.GetMaxGraphDownloadRate()*0.6),GetResString(IDS_KBYTESEC));	m_menuDownloadCtrl.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D60,  text);
		text.Format(_T("80%%\t%0.1f %s"),  (0.8f * thePrefs.GetMaxGraphDownloadRate()*0.8),GetResString(IDS_KBYTESEC));	m_menuDownloadCtrl.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D80,  text);
		text.Format(_T("100%%\t%0.1f %s"), (1.0f * thePrefs.GetMaxGraphDownloadRate()),GetResString(IDS_KBYTESEC));		m_menuDownloadCtrl.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D100, text);
		//m_menuDownloadCtrl.AppendMenu(MF_SEPARATOR);
		//m_menuDownloadCtrl.AppendMenu(MF_STRING, MP_QS_DC, GetResString(IDS_PW_UNLIMITED));

		// Show DownloadPopup Menu
		text.Format(_T("%s:"), GetResString(IDS_PW_DOWNL));
		addToMenu->AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)m_menuDownloadCtrl.m_hMenu, text);
	}
	addToMenu->AppendMenu(MF_SEPARATOR);
	addToMenu->AppendMenu(MF_STRING, MP_CONNECT, GetResString(IDS_MAIN_BTN_CONNECT));
	addToMenu->AppendMenu(MF_STRING, MP_DISCONNECT, GetResString(IDS_MAIN_BTN_DISCONNECT)); 
}
// Maella end


void CemuleDlg::StartConnection(){
	if (!Kademlia::CKademlia::isRunning() && !theApp.serverconnect->IsConnecting()){
		AddLogLine(true, GetResString(IDS_CONNECTING));
		if( thePrefs.GetNetworkED2K() ){
			if ( serverwnd->serverlistctrl.GetSelectedCount()>1 )
			{
				serverwnd->serverlistctrl.PostMessage(WM_COMMAND,MP_CONNECTTO,0L);
			}
			else
			{
				theApp.serverconnect->ConnectToAnyServer();
			}
		}
		if( thePrefs.GetNetworkKademlia() )
		{
			Kademlia::CKademlia::start();
		}
		ShowConnectionState();
	}
}

void CemuleDlg::CloseConnection()
{
	if (theApp.serverconnect->IsConnected()){
		theApp.serverconnect->Disconnect();
	}

	if (theApp.serverconnect->IsConnecting()){
		theApp.serverconnect->StopConnectionTry();
	}
	Kademlia::CKademlia::stop();
	theApp.OnlineSig(); // Added By Bouc7 
	ShowConnectionState();
}

void CemuleDlg::RestoreWindow()
{
	// [TPT] - SLUGFILLER: modelessDialogs remove - the state of the preferences dialog doesn't matter since it's modeless
	if (TrayIsVisible())
		TrayHide();	
	
	if (m_wpFirstRestore.length)
	{
		SetWindowPlacement(&m_wpFirstRestore);
		MEMZERO(&m_wpFirstRestore, sizeof m_wpFirstRestore);
		SetForegroundWindow();
		BringWindowToTop();
	}
	else
		CTrayDialog::RestoreWindow();
}

void CemuleDlg::UpdateTrayIcon(int iPercent)
{
	// compute an id of the icon to be generated
	UINT uSysTrayIconCookie = (iPercent > 0) ? (16 - ((iPercent*15/100) + 1)) : 0;
	if (theApp.IsConnected()){
		if (!theApp.IsFirewalled())
			uSysTrayIconCookie += 50;
	}
	else
		uSysTrayIconCookie += 100;

	// dont update if the same icon as displayed would be generated
	static bool messageIcon = false;
	if ( m_uLastSysTrayIconCookie == uSysTrayIconCookie && m_iMsgIcon == 0 && messageIcon)
		return;

	m_uLastSysTrayIconCookie = uSysTrayIconCookie;
	
	// prepare it up
	//[TPT] - Blinking tray on message info - eMulespa�a
	if(m_iMsgIcon == 0 || !messageIcon){
	if (theApp.IsConnected()){
		if (theApp.IsFirewalled())
			m_TrayIcon.Init(m_icoSysTrayLowID, 100, 1, 1, 16, 16, thePrefs.GetStatsColor(11));
		else
			m_TrayIcon.Init(m_icoSysTrayConnected, 100, 1, 1, 16, 16, thePrefs.GetStatsColor(11));
	}
	else
		m_TrayIcon.Init(m_icoSysTrayDisconnected, 100, 1, 1, 16, 16, thePrefs.GetStatsColor(11));
	}
	else
		m_TrayIcon.Init(sourceTrayIconMail,100,1,1,16,16,thePrefs.GetStatsColor(11));
	messageIcon = !messageIcon;
	//[TPT] - end: Blinking Tray Icon On Message Recieve [emulEspa�a] - End

	// load our limit and color info
	static const int aiLimits[1] = { 100 }; // set the limits of where the bar color changes (low-high)
	COLORREF aColors[1] = { thePrefs.GetStatsColor(11) }; // set the corresponding color for each level
	m_TrayIcon.SetColorLevels(aiLimits, aColors, ARRSIZE(aiLimits));

	// generate the icon (do *not* destroy that icon using DestroyIcon(), that's done in 'TrayUpdate')
	int aiVals[1] = { iPercent };
	m_icoSysTrayCurrent = m_TrayIcon.Create(aiVals);
	ASSERT( m_icoSysTrayCurrent != NULL );
	if (m_icoSysTrayCurrent)
		TraySetIcon(m_icoSysTrayCurrent, true);
	TrayUpdate();
}

int CemuleDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	return CTrayDialog::OnCreate(lpCreateStruct);
}

void CemuleDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (IsRunning())
		ShowTransferRate(true);
	CTrayDialog::OnShowWindow(bShow, nStatus);
}

// [TPT] - enkeyDEV(th1) -notifier-
void CemuleDlg::LoadNotifier() {	
	CIniNotifier tbnConfig;
	notifierenabled= false;
	    
	//try to load configuration as spotted in ini file
	tbnConfig.Open(thePrefs.GetNotifierConfiguration(), true);

	if (!tbnConfig.isValid())
	{  
	   //if something wrong, try to load default config
       CString defaultTBN;
	   defaultTBN.Format(_T("%s\\tbnskin\\default.ini"), thePrefs.GetAppDir());   		 
	   tbnConfig.Open(defaultTBN, true);
	}

	if (tbnConfig.isValid())
		notifierenabled = m_wndTaskbarNotifier->LoadConfiguration((CNotifierSkin) tbnConfig);	
	else
		notifierenabled = false;

	if (notifierenabled) {
		if (thePrefs.GetNotifierLessFramerate()) 
			m_wndTaskbarNotifier->SetFramerateDiv(2); //reduce framerate
		else
			m_wndTaskbarNotifier->SetFramerateDiv(1); //standard framerate
		
		if (thePrefs.GetUseNotifierUserTimings()) {
			m_wndTaskbarNotifier->SetTimeToShow(thePrefs.GetNotifierUserTimeToShow());
			m_wndTaskbarNotifier->SetTimeToHide(thePrefs.GetNotifierUserTimeToHide());
			m_wndTaskbarNotifier->SetTimeToStay(thePrefs.GetNotifierUserTimeToStay());
		}
	}
}

void CemuleDlg::ShowNotifier(CString Text, int MsgType, LPCTSTR pszLink, bool bForceSoundOFF)
{
	if (!notifierenabled)
		return;

	LPCTSTR pszSoundEvent = NULL;
	int iSoundPrio = 0;
	bool ShowIt = false;
	switch (MsgType)
	{
		case TBN_CHAT:
            if (thePrefs.GetUseChatNotifier())
			{
				m_wndTaskbarNotifier->Show(Text, MsgType, pszLink);
				ShowIt = true;
				pszSoundEvent = _T("eMule_Chat");
				iSoundPrio = 1;
			}
			break;
		case TBN_DLOAD:
            if (thePrefs.GetUseDownloadNotifier())
			{
				m_wndTaskbarNotifier->Show(Text, MsgType, pszLink);
				ShowIt = true;			
				pszSoundEvent = _T("eMule_DownloadFinished");
				iSoundPrio = 1;
			}
			break;
		case TBN_DLOADADDED:
            if (thePrefs.GetUseNewDownloadNotifier())
			{
				m_wndTaskbarNotifier->Show(Text, MsgType, pszLink);
				ShowIt = true;			
				pszSoundEvent = _T("eMule_DownloadAdded");
				iSoundPrio = 1;
			}
			break;
		case TBN_LOG:
            if (thePrefs.GetUseLogNotifier())
			{
				m_wndTaskbarNotifier->Show(Text, MsgType, pszLink);
				ShowIt = true;			
				pszSoundEvent = _T("eMule_LogEntryAdded");
			}
			break;
		case TBN_IMPORTANTEVENT:
			if (thePrefs.GetNotifierPopOnImportantError())
			{
				m_wndTaskbarNotifier->Show(Text, MsgType, pszLink);
				ShowIt = true;			
				pszSoundEvent = _T("eMule_Urgent");
				iSoundPrio = 1;
			}
                break;
		case TBN_IRC_QUERY:
			 if (thePrefs.GetNotifierNewPvtMsg()) 
			 {
		 		m_wndTaskbarNotifier->Show(Text, MsgType, pszLink, true); // always automatically close the notifier				
				ShowIt = true;			
				pszSoundEvent = _T("IRC_Query");
				iSoundPrio = 1;
			}
                break;
                // start added by InterCeptor (notify on error) 11.11.02
                // modified by kei-kun to match notifier 1.3 specs
         case TBN_ERROR:
                if (thePrefs.GetUseErrorNotifier()) {
                    m_wndTaskbarNotifier->Show(Text, MsgType, pszLink, true); // always automatically close the notifier				
				ShowIt = true;			
				pszSoundEvent = _T("eMule_Error");
				iSoundPrio = 1;
			}
                break;
                // end added by InterCeptor (notify on error) 11.11.02
		case TBN_NEWVERSION:
			if (thePrefs.GetNotifierPopOnNewVersion()) 
			{
				m_wndTaskbarNotifier->Show(Text, MsgType, pszLink);
				ShowIt = true;			
				pszSoundEvent = _T("eMule_NewVersion");
				iSoundPrio = 1;
			}
			break;
		case TBN_NULL:
            m_wndTaskbarNotifier->Show(Text, MsgType, pszLink);
			ShowIt = true;			
			break;
		case TBN_SEARCHCOMPLETED:
			if (thePrefs.GetNotifierPopOnSearch()) 
			{
                m_wndTaskbarNotifier->Show(Text, MsgType, pszLink, true);
				ShowIt = true;			
				pszSoundEvent = _T("search_Completed");
				iSoundPrio = 1;
			}
	}
	
	if (ShowIt && !bForceSoundOFF)
	{
		if (thePrefs.GetUseSoundInNotifier())
		{
			PlaySound(thePrefs.GetNotifierWavSoundPath(), NULL, SND_FILENAME | SND_NOSTOP | SND_NOWAIT | SND_ASYNC);
		}
		else if (pszSoundEvent)
		{
			// use 'SND_NOSTOP' only for low priority events, otherwise the 'Log message' event may overrule a more important
			// event which is fired nearly at the same time.
			PlaySound(pszSoundEvent, NULL, SND_APPLICATION | SND_ASYNC | SND_NODEFAULT | SND_NOWAIT | ((iSoundPrio > 0) ? 0 : SND_NOSTOP));
		}
	}
}

LRESULT CemuleDlg::OnTaskbarNotifierClicked(WPARAM wParam,LPARAM lParam)
{
	if (lParam)
	{
		LPTSTR pszLink = (LPTSTR)lParam;
		ShellOpenFile(pszLink, NULL);
		free(pszLink);
		pszLink = NULL;
	}

	switch (m_wndTaskbarNotifier->GetMessageType())
	{
		case TBN_CHAT:
			RestoreWindow();
			SetActiveDialog(chatwnd);
			break;

		case TBN_DLOAD:
			// if we had a link and opened the downloaded file and if we currently in traybar, dont restore the app window
			if (lParam==0)
			{
				RestoreWindow();
				SetActiveDialog(transferwnd);
			}
			break;

		case TBN_DLOADADDED:
			RestoreWindow();
			SetActiveDialog(transferwnd);
			break;

		case TBN_IMPORTANTEVENT:
			RestoreWindow();
			SetActiveDialog(serverwnd);	
			break;

		case TBN_LOG:
			RestoreWindow();
			SetActiveDialog(serverwnd);	
			break;
	// start added by InterCeptor (notify on error) 11.11.02
	case TBN_ERROR:
			RestoreWindow();
			SetActiveDialog(transferwnd);
		break;
	// end added by InterCeptor (notify on error) 11.11.02
		case TBN_NEWVERSION:
		{
			CString theUrl;
			theUrl.Format(_T("/en/version_check.php?version=%i&language=%i"),theApp.m_uCurVersionCheck,thePrefs.GetLanguageID());
			theUrl = thePrefs.GetVersionCheckBaseURL()+theUrl;
			ShellExecute(NULL, NULL, theUrl, NULL, thePrefs.GetAppDir(), SW_SHOWDEFAULT);
			break;
		}
	}
    return 0;
}
//END - enkeyDEV(kei-kun) -TaskbarNotifier-

void CemuleDlg::OnSysColorChange()
{
	CTrayDialog::OnSysColorChange();
	SetAllIcons();
}

void CemuleDlg::SetAllIcons()
{
	// connection state
	for (int i = 0; i < ARRSIZE(connicons); i++){
		if (connicons[i]) VERIFY( ::DestroyIcon(connicons[i]) );
	}
	connicons[0] = theApp.LoadIcon(_T("ConnectedNotNot"), 16, 16);
	connicons[1] = theApp.LoadIcon(_T("ConnectedNotLow"), 16, 16);
	connicons[2] = theApp.LoadIcon(_T("ConnectedNotHigh"), 16, 16);
	connicons[3] = theApp.LoadIcon(_T("ConnectedLowNot"), 16, 16);
	connicons[4] = theApp.LoadIcon(_T("ConnectedLowLow"), 16, 16);
	connicons[5] = theApp.LoadIcon(_T("ConnectedLowHigh"), 16, 16);
	connicons[6] = theApp.LoadIcon(_T("ConnectedHighNot"), 16, 16);
	connicons[7] = theApp.LoadIcon(_T("ConnectedHighLow"), 16, 16);
	connicons[8] = theApp.LoadIcon(_T("ConnectedHighHigh"), 16, 16);
	ShowConnectionStateIcon();

	// transfer state
	if (transicons[0]) VERIFY( ::DestroyIcon(transicons[0]) );
	if (transicons[1]) VERIFY( ::DestroyIcon(transicons[1]) );
	if (transicons[2]) VERIFY( ::DestroyIcon(transicons[2]) );
	if (transicons[3]) VERIFY( ::DestroyIcon(transicons[3]) );
	transicons[0] = theApp.LoadIcon(_T("UP0DOWN0"), 16, 16);
	transicons[1] = theApp.LoadIcon(_T("UP0DOWN1"), 16, 16);
	transicons[2] = theApp.LoadIcon(_T("UP1DOWN0"), 16, 16);
	transicons[3] = theApp.LoadIcon(_T("UP1DOWN1"), 16, 16);
	ShowTransferStateIcon();

	// users state
	if (usericon) VERIFY( ::DestroyIcon(usericon) );
	usericon = theApp.LoadIcon(_T("StatsClients"), 16, 16);
	ShowUserStateIcon();

	// traybar icons
	if (m_icoSysTrayConnected) VERIFY( ::DestroyIcon(m_icoSysTrayConnected) );
	if (m_icoSysTrayDisconnected) VERIFY( ::DestroyIcon(m_icoSysTrayDisconnected) );
	if (m_icoSysTrayLowID) VERIFY( ::DestroyIcon(m_icoSysTrayLowID) );
	m_icoSysTrayConnected = theApp.LoadIcon(_T("TrayConnected"), 16, 16);
	m_icoSysTrayDisconnected = theApp.LoadIcon(_T("TrayNotConnected"), 16, 16);
	m_icoSysTrayLowID = theApp.LoadIcon(_T("TrayLowID"), 16, 16);
	if (sourceTrayIconMail) VERIFY( ::DestroyIcon(sourceTrayIconMail) );
	sourceTrayIconMail = theApp.LoadIcon(_T("MESSAGEPENDING"), 16, 16);
	ShowTransferRate(true);

	if (imicons[0]) VERIFY( ::DestroyIcon(imicons[0]) );
	if (imicons[1]) VERIFY( ::DestroyIcon(imicons[1]) );
	if (imicons[2]) VERIFY( ::DestroyIcon(imicons[2]) );
	imicons[0] = NULL;
	imicons[1] = theApp.LoadIcon(_T("Message"), 16, 16);
	imicons[2] = theApp.LoadIcon(_T("MessagePending"), 16, 16);
	ShowMessageState(m_iMsgIcon);
}

void CemuleDlg::Localize()
{
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu)
	{
		VERIFY( pSysMenu->ModifyMenu(MP_ABOUTBOX, MF_BYCOMMAND | MF_STRING, MP_ABOUTBOX, GetResString(IDS_ABOUTBOX)) );
		VERIFY( pSysMenu->ModifyMenu(MP_VERSIONCHECK, MF_BYCOMMAND | MF_STRING, MP_VERSIONCHECK, GetResString(IDS_VERSIONCHECK)) );

		switch (thePrefs.GetWindowsVersion()){
		case _WINVER_98_:
		case _WINVER_95_:
		case _WINVER_ME_:
			// NOTE: I think the reason why the old version of the following code crashed under Win9X was because
			// of the menus were destroyed right after they were added to the system menu. New code should work
			// under Win9X too but I can't test it.
			break;
		default:{
			// localize the 'speed control' sub menus by deleting the current menus and creating a new ones.

			// remove any already available 'speed control' menus from system menu
			UINT uOptMenuPos = pSysMenu->GetMenuItemCount() - 1;
			CMenu* pAccelMenu = pSysMenu->GetSubMenu(uOptMenuPos);
			if (pAccelMenu)
			{
				ASSERT( pAccelMenu->m_hMenu == m_SysMenuOptions.m_hMenu );
				VERIFY( pSysMenu->RemoveMenu(uOptMenuPos, MF_BYPOSITION) );
				pAccelMenu = NULL;
			}

			// destroy all 'speed control' menus
			if (m_menuUploadCtrl)
				VERIFY( m_menuUploadCtrl.DestroyMenu() );
			if (m_menuDownloadCtrl)
				VERIFY( m_menuDownloadCtrl.DestroyMenu() );
			if (m_SysMenuOptions)
				VERIFY( m_SysMenuOptions.DestroyMenu() );

			// create new 'speed control' menus
			if (m_SysMenuOptions.CreateMenu())
			{
				AddSpeedSelectorSys(&m_SysMenuOptions);
				pSysMenu->AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)m_SysMenuOptions.m_hMenu, GetResString(IDS_EM_PREFS));
			}
		  }
		}
	}

	ShowUserStateIcon();
	toolbar->Localize();
	ShowConnectionState();
	ShowTransferRate(true);
	ShowUserCount();
	CPartFileConvert::Localize();
}

void CemuleDlg::ShowUserStateIcon()
{
	statusbar->SetIcon(SBarUsers, usericon);
}

// [TPT] - Maella [FAF] -Allow Bandwidth Settings in <1KB Incremements-
void CemuleDlg::QuickSpeedOther(UINT nID)
{
	switch (nID) {
		case MP_QS_PA: 
			thePrefs.SetMaxUpload(1.0f);
			thePrefs.SetMaxDownload(1.0f);
			break ;
		case MP_QS_UA: 
			thePrefs.SetMaxUpload(thePrefs.GetMaxGraphUploadRate());
			thePrefs.SetMaxDownload(thePrefs.GetMaxGraphDownloadRate());
			break ;
	}
}
// Maella end

// Maella [FAF] -Allow Bandwidth Settings in <1KB Incremements-
void CemuleDlg::QuickSpeedUpload(UINT nID)
{
	switch (nID) {
		case MP_QS_U10: thePrefs.SetMaxUpload(0.1f * thePrefs.GetMaxGraphUploadRate()); break ;
		case MP_QS_U20: thePrefs.SetMaxUpload(0.2f * thePrefs.GetMaxGraphUploadRate()); break ;
		case MP_QS_U30: thePrefs.SetMaxUpload(0.3f * thePrefs.GetMaxGraphUploadRate()); break ;
		case MP_QS_U40: thePrefs.SetMaxUpload(0.4f * thePrefs.GetMaxGraphUploadRate()); break ;
		case MP_QS_U50: thePrefs.SetMaxUpload(0.5f * thePrefs.GetMaxGraphUploadRate()); break ;
		case MP_QS_U60: thePrefs.SetMaxUpload(0.6f * thePrefs.GetMaxGraphUploadRate()); break ;
		case MP_QS_U70: thePrefs.SetMaxUpload(0.7f * thePrefs.GetMaxGraphUploadRate()); break ;
		case MP_QS_U80: thePrefs.SetMaxUpload(0.8f * thePrefs.GetMaxGraphUploadRate()); break ;
		case MP_QS_U90: thePrefs.SetMaxUpload(0.9f * thePrefs.GetMaxGraphUploadRate()); break ;
		case MP_QS_U100: thePrefs.SetMaxUpload(thePrefs.GetMaxGraphUploadRate()); break ;
		case MP_QS_UP10: thePrefs.SetMaxUpload(GetRecMaxUpload()); break ;
	}
	}
// Maella end

// Maella [FAF] -Allow Bandwidth Settings in <1KB Incremements-
void CemuleDlg::QuickSpeedDownload(UINT nID)
{
	switch (nID) {
		case MP_QS_D10: thePrefs.SetMaxDownload(0.1f * thePrefs.GetMaxGraphDownloadRate()); break ;
		case MP_QS_D20: thePrefs.SetMaxDownload(0.2f * thePrefs.GetMaxGraphDownloadRate()); break ;
		case MP_QS_D30: thePrefs.SetMaxDownload(0.3f * thePrefs.GetMaxGraphDownloadRate()); break ;
		case MP_QS_D40: thePrefs.SetMaxDownload(0.4f * thePrefs.GetMaxGraphDownloadRate()); break ;
		case MP_QS_D50: thePrefs.SetMaxDownload(0.5f * thePrefs.GetMaxGraphDownloadRate()); break ;
		case MP_QS_D60: thePrefs.SetMaxDownload(0.6f * thePrefs.GetMaxGraphDownloadRate()); break ;
		case MP_QS_D70: thePrefs.SetMaxDownload(0.7f * thePrefs.GetMaxGraphDownloadRate()); break ;
		case MP_QS_D80: thePrefs.SetMaxDownload(0.8f * thePrefs.GetMaxGraphDownloadRate()); break ;
		case MP_QS_D90: thePrefs.SetMaxDownload(0.9f * thePrefs.GetMaxGraphDownloadRate()); break ;
		case MP_QS_D100: thePrefs.SetMaxDownload(thePrefs.GetMaxGraphDownloadRate()); break ;
//		case MP_QS_DC: thePrefs.SetMaxDownload(UNLIMITED); break ;
	}
}
// quick-speed changer -- based on xrmb
// Maella end

// Maella [FAF] -Allow Bandwidth Settings in <1KB Incremements-
float CemuleDlg::GetRecMaxUpload() const {	
	if (thePrefs.GetMaxGraphUploadRate() < 7.0f) return 0.0f;
	else if (thePrefs.GetMaxGraphUploadRate() < 15.0f) return thePrefs.GetMaxGraphUploadRate() - 3.0f;
	else return (thePrefs.GetMaxGraphUploadRate() - 4.0f);
}
// Maella end

BOOL CemuleDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{	
		case IDC_TOOLBARBUTTON + 0:
			OnBnClickedButton2();
			break;
		case MP_HM_KAD:
		case IDC_TOOLBARBUTTON +1:
			SetActiveDialog(kademliawnd);
			break;
		case IDC_TOOLBARBUTTON + 2:
		case MP_HM_SRVR:
			SetActiveDialog(serverwnd);
			break;
		case IDC_TOOLBARBUTTON + 3:
		case MP_HM_TRANSFER:
			SetActiveDialog(transferwnd);
			break;
		case IDC_TOOLBARBUTTON + 4:
		case MP_HM_SEARCH:
			SetActiveDialog(searchwnd);
			break;
		case IDC_TOOLBARBUTTON + 5:
		case MP_HM_FILES:
			SetActiveDialog(sharedfileswnd);
			break;
		case IDC_TOOLBARBUTTON + 6:
		case MP_HM_MSGS:
			SetActiveDialog(chatwnd);
			break;
		case IDC_TOOLBARBUTTON + 7:
		case MP_HM_IRC:
			SetActiveDialog(ircwnd);
			break;
		case IDC_TOOLBARBUTTON + 8:
		case MP_HM_STATS:
			SetActiveDialog(statisticswnd);
			break;
		case IDC_TOOLBARBUTTON + 9:
		case MP_HM_PREFS:
			toolbar->CheckButton(IDC_TOOLBARBUTTON+9,TRUE);
			ShowPreferences();
			toolbar->CheckButton(IDC_TOOLBARBUTTON+9,FALSE);
			break;
		case IDC_TOOLBARBUTTON + 10:
			ShowToolPopup(true);
			break;
		case MP_HM_OPENINC:
			ShellExecute(NULL, _T("open"), thePrefs.GetIncomingDir(),NULL, NULL, SW_SHOW); 
			break;
		case MP_HM_HELP:
		case IDC_TOOLBARBUTTON + 11:
			wParam = ID_HELP;
			break;
		case MP_HM_CON:
			OnBnClickedButton2();
			break;
		case MP_HM_EXIT:
			OnClose();
			break;
		case MP_HM_LINK1: // MOD: dont remove!
			ShellExecute(NULL, NULL, thePrefs.GetHomepageBaseURL(), NULL, thePrefs.GetAppDir(), SW_SHOWDEFAULT);
			break;
		case MP_HM_LINK2:
			ShellExecute(NULL, NULL, thePrefs.GetHomepageBaseURL()+ CString(_T("/faq/")), NULL, thePrefs.GetAppDir(), SW_SHOWDEFAULT);
			break;
		case MP_HM_LINK3: {
			CString theUrl;
			theUrl.Format( thePrefs.GetVersionCheckBaseURL() + CString(_T("/en/version_check.php?version=%i&language=%i")),theApp.m_uCurVersionCheck,thePrefs.GetLanguageID());
			ShellExecute(NULL, NULL, theUrl, NULL, thePrefs.GetAppDir(), SW_SHOWDEFAULT);
			break;
		}
		case MP_WEBSVC_EDIT:
			theWebServices.Edit();
			break;
		case MP_HM_LINK4: // [TPT]
			ShellExecute(NULL, NULL, _T("http://www.emulespana.net/foros/index.php?showtopic=55050"), NULL, thePrefs.GetAppDir(), SW_SHOWDEFAULT);
			break;
		case MP_HM_CONVERTPF:
			CPartFileConvert::ShowGUI();
			break;
		case MP_HM_SCHEDONOFF:
			thePrefs.SetSchedulerEnabled(!thePrefs.IsSchedulerEnabled());
			theApp.scheduler->Check(true);
			break;
		case MP_HM_1STSWIZARD:
			extern BOOL FirstTimeWizard();
			if (FirstTimeWizard()){
				// start connection wizard
				CConnectionWizardDlg conWizard;
				conWizard.DoModal();
			}
			break;
		case MP_HM_IPFILTER:{
			CIPFilterDlg dlg;
			dlg.DoModal();
			break;
		}

		//[TPT] - eWombat [SCREENSHOT]
		case MP_HM_SCREENSHOT:{
			Screenshot((CWnd*)this,TRUE); 
			break;
		}

		case MP_HM_DIRECT_DOWNLOAD:{
			CDirectDownloadDlg dlg;
			dlg.DoModal();
			break;
		}
	}	
	if (wParam>=MP_WEBURL && wParam<=MP_WEBURL+99) {
		theWebServices.RunURL(NULL, wParam);
	}
	else if (wParam>=MP_SCHACTIONS && wParam<=MP_SCHACTIONS+99) {
		theApp.scheduler->ActivateSchedule(wParam-MP_SCHACTIONS);
		theApp.scheduler->SaveOriginals(); // use the new settings as original
	}

	return CTrayDialog::OnCommand(wParam, lParam);
}

LRESULT CemuleDlg::OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu)
{
	UINT nCmdID;
	if (toolbar->MapAccelerator(nChar, &nCmdID)){
		OnCommand(nCmdID, 0);
		return MAKELONG(0,MNC_CLOSE);
	}
	return CTrayDialog::OnMenuChar(nChar, nFlags, pMenu);
}

BOOL CemuleDlg::OnQueryEndSession()
{
	if (!CTrayDialog::OnQueryEndSession())
		return FALSE;

	return TRUE;
}

void CemuleDlg::OnEndSession(BOOL bEnding)
{
	if (bEnding && theApp.m_app_state == APP_STATE_RUNNING)
	{
		theApp.m_app_state = APP_STATE_SHUTINGDOWN;
		OnClose();
	}

	CTrayDialog::OnEndSession(bEnding);
}

// Barry - To find out if app is running or shutting/shut down
bool CemuleDlg::IsRunning()
{
	return (theApp.m_app_state == APP_STATE_RUNNING);
}


void CemuleDlg::OnBnClickedHotmenu()
{
	ShowToolPopup(false);
}

//void CemuleDlg::CreateMenuCmdIconMap()
//{
//	m_mapCmdToIcon.SetAt(MP_HM_CON, _T("Connect"));
//	m_mapCmdToIcon.SetAt(MP_HM_KAD, _T("KADEMLIA"));
//	m_mapCmdToIcon.SetAt(MP_HM_SRVR, _T("SERVER"));
//	m_mapCmdToIcon.SetAt(MP_HM_TRANSFER, _T("TRANSFER"));
//	m_mapCmdToIcon.SetAt(MP_HM_SEARCH, _T("SEARCH"));
//	m_mapCmdToIcon.SetAt(MP_HM_FILES, _T("SharedFiles"));
//	m_mapCmdToIcon.SetAt(MP_HM_MSGS, _T("MESSAGES"));
//	m_mapCmdToIcon.SetAt(MP_HM_IRC, _T("IRC"));
//	m_mapCmdToIcon.SetAt(MP_HM_STATS, _T("STATISTICS"));
//	m_mapCmdToIcon.SetAt(MP_HM_PREFS, _T("PREFERENCES"));
//	m_mapCmdToIcon.SetAt(MP_HM_HELP, _T("HELP"));
//	m_mapCmdToIcon.SetAt(MP_HM_OPENINC, _T("OPENFOLDER"));
//	m_mapCmdToIcon.SetAt(MP_HM_CONVERTPF, _T("CONVERT"));
//	m_mapCmdToIcon.SetAt(MP_HM_1STSWIZARD, _T("WIZARD"));
//	m_mapCmdToIcon.SetAt(MP_HM_IPFILTER, _T("IPFILTER"));
//	m_mapCmdToIcon.SetAt(MP_HM_DIRECT_DOWNLOAD, _T("PASTELINK"));
//	m_mapCmdToIcon.SetAt(MP_HM_EXIT, _T("EXIT"));
//}
//
//LPCTSTR CemuleDlg::GetIconFromCmdId(UINT uId)
//{
//	LPCTSTR pszIconId = NULL;
//	if (m_mapCmdToIcon.Lookup(uId, pszIconId))
//		return pszIconId;
//	return NULL;
//}

void CemuleDlg::ShowToolPopup(bool toolsonly)
{
	POINT point;

	::GetCursorPos(&point);

	// [TPT] - New Menu Styles BEGIN
	//Menu Configuration
	CMenuXP	*pMenu = new CMenuXP;
	pMenu->CreatePopupMenu();
	pMenu->SetMenuStyle(CMenuXP::STYLE_STARTMENU);
	if (!toolsonly)
		pMenu->AddSideBar(new CMenuXPSideBar(17, GetResString(IDS_HOTMENU)));
	else
		pMenu->AddSideBar(new CMenuXPSideBar(17, GetResString(IDS_TOOLS)));	
	pMenu->SetSideBarStartColor(RGB(255,0,0));
	pMenu->SetSideBarEndColor(RGB(255,128,0));
	pMenu->SetSelectedBarColor(RGB(242,120,114));
	
	CMenuXP *pLinks= new CMenuXP;
	pLinks->CreatePopupMenu();
	pLinks->SetMenuStyle(CMenuXP::STYLE_STARTMENU);
	pLinks->SetSelectedBarColor(RGB(242,120,114));
	pLinks->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_LINK1,GetResString(IDS_HM_LINKHP)));
	pLinks->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_LINK2,GetResString(IDS_HM_LINKFAQ)));
	pLinks->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_LINK3,GetResString(IDS_HM_LINKVC)));
	pLinks->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_LINK4,GetResString(IDS_PHOENIX_LINKHP)));
	theWebServices.GetGeneralMenuEntries(pLinks);
	pLinks->InsertMenu(3, MF_BYPOSITION | MF_SEPARATOR);
	pLinks->AppendODMenu(MF_STRING,new CMenuXPText(MP_WEBSVC_EDIT, GetResString(IDS_WEBSVEDIT)));

	CMenuXP *pScheduler= new CMenuXP;
	pScheduler->CreatePopupMenu();
	pScheduler->SetMenuStyle(CMenuXP::STYLE_STARTMENU);
	pScheduler->SetSelectedBarColor(RGB(242,120,114));
	CString schedonoff= (!thePrefs.IsSchedulerEnabled())?GetResString(IDS_HM_SCHED_ON):GetResString(IDS_HM_SCHED_OFF);
	pScheduler->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_SCHEDONOFF,schedonoff));		
	if (theApp.scheduler->GetCount()>0) {
		pScheduler->AppendSeparator();
		for (int i=0; i<theApp.scheduler->GetCount();i++)
			pScheduler->AppendODMenu(MF_STRING,new CMenuXPText(MP_SCHACTIONS+i,theApp.scheduler->GetSchedule(i)->title));					
	}

	if (!toolsonly) {
		if (theApp.serverconnect->IsConnected())
			pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_CON,GetResString(IDS_MAIN_BTN_DISCONNECT)));			
		else if (theApp.serverconnect->IsConnecting())
			pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_CON,GetResString(IDS_MAIN_BTN_CANCEL)));			
		else
			pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_CON,GetResString(IDS_MAIN_BTN_CONNECT)));			

		pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_KAD,GetResString(IDS_EM_KADEMLIA)));			
		pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_SRVR,GetResString(IDS_EM_SERVER)));			
		pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_TRANSFER,GetResString(IDS_EM_TRANS)));			
		pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_SEARCH,GetResString(IDS_EM_SEARCH)));			
		pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_FILES,GetResString(IDS_EM_FILES)));			
		pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_MSGS,GetResString(IDS_EM_MESSAGES)));			
		pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_IRC,GetResString(IDS_IRC)));			
		pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_STATS,GetResString(IDS_EM_STATISTIC)));			
		pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_PREFS,GetResString(IDS_EM_PREFS)));			
		pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_HELP,GetResString(IDS_EM_HELP)));			
		pMenu->AppendSeparator();		
	}

	pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_OPENINC,GetResString(IDS_OPENINC) + _T("..."), theApp.LoadIcon(_T("incoming"), 16, 16)));
	pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_CONVERTPF,GetResString(IDS_IMPORTSPLPF) + _T("..."), theApp.LoadIcon(_T("import"), 16, 16)));
	pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_1STSWIZARD,GetResString(IDS_WIZ1) + _T("...")));			
	pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_IPFILTER,GetResString(IDS_IPFILTER) + _T("...")));			
	pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_DIRECT_DOWNLOAD,GetResString(IDS_SW_DIRECTDOWNLOAD) + _T("...")));			
	
	pMenu->AppendSeparator();
	pMenu->AppendODPopup(MF_STRING | MF_POPUP, pLinks, new CMenuXPText(0,GetResString(IDS_LINKS)));				
	pMenu->AppendODPopup(MF_STRING | MF_POPUP, pScheduler, new CMenuXPText(0,GetResString(IDS_SCHEDULER)));			

	pMenu->AppendSeparator();

	pMenu->AppendODMenu(MF_STRING, new CMenuXPText(MP_HM_SCREENSHOT,_T("Screenshot"), theApp.LoadIcon(_T("DISPLAY"), 16, 16)));//[TPT] - eWombat Screenshot		

	if (!toolsonly) {
		pMenu->AppendSeparator();
		pMenu->AppendODMenu(MF_STRING,new CMenuXPText(MP_HM_EXIT,GetResString(IDS_EXIT)));		
	}
	
	pMenu->TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
	
	delete pLinks;	
	delete pScheduler;
	delete pMenu;
}

// [TPT] - New Menu Styles BEGIN
void CemuleDlg::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	HMENU hMenu = AfxGetThreadState()->m_hTrackingMenu;
	CMenu	*pMenu = CMenu::FromHandle(hMenu);
	pMenu->MeasureItem(lpMeasureItemStruct);
	
	CWnd::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}
// [TPT] - New Menu Styles END

LRESULT CemuleDlg::OnWebServerConnect(WPARAM wParam, LPARAM lParam)
{
	CServer* server=(CServer*)wParam;
	if (server==NULL) theApp.serverconnect->ConnectToAnyServer();
		else theApp.serverconnect->ConnectToServer(server);
	
	return 0;
}

LRESULT CemuleDlg::OnWebServerDisonnect(WPARAM wParam, LPARAM lParam)
{
	theApp.serverconnect->Disconnect();
	
	return 0;
}

LRESULT CemuleDlg::OnWebServerRemove(WPARAM wParam, LPARAM lParam)
{
	serverwnd->serverlistctrl.RemoveServer((CServer*)wParam); // sivka's bugfix
	return 0;
}

LRESULT CemuleDlg::OnWebSharedFilesReload(WPARAM wParam, LPARAM lParam)
{
	theApp.sharedfiles->Reload();
	return 0;
}

void CemuleDlg::ApplyHyperTextFont(LPLOGFONT plf)
{
	theApp.m_fontHyperText.DeleteObject();
	if (theApp.m_fontHyperText.CreateFontIndirect(plf))
	{
		thePrefs.SetHyperTextFont(plf);
		serverwnd->servermsgbox->SetFont(&theApp.m_fontHyperText);
		chatwnd->chatselector.UpdateFonts(&theApp.m_fontHyperText);
		ircwnd->UpdateFonts(&theApp.m_fontHyperText);
	}
}

void CemuleDlg::ApplyLogFont(LPLOGFONT plf)
{
	theApp.m_fontLog.DeleteObject();
	if (theApp.m_fontLog.CreateFontIndirect(plf))
	{
		thePrefs.SetLogFont(plf);
		serverwnd->logbox->SetFont(&theApp.m_fontLog);
		serverwnd->debuglog->SetFont(&theApp.m_fontLog);
		serverwnd->phoenixlog->SetFont(&theApp.m_fontLog); // [TPT] - Debug log
	}
}

LRESULT CemuleDlg::OnFrameGrabFinished(WPARAM wParam,LPARAM lParam){
	CKnownFile* pOwner = (CKnownFile*)wParam;
	FrameGrabResult_Struct* result = (FrameGrabResult_Struct*)lParam;
	
	if (theApp.knownfiles->IsKnownFile(pOwner) || theApp.downloadqueue->IsPartFile(pOwner) ){
		pOwner->GrabbingFinished(result->imgResults,result->nImagesGrabbed, result->pSender);
	}
	else{
		ASSERT ( false );
	}

	delete result;
	return 0;
}

void StraightWindowStyles(CWnd* pWnd)
{
	CWnd* pWndChild = pWnd->GetWindow(GW_CHILD);
	while (pWndChild)
	{
		StraightWindowStyles(pWndChild);
		pWndChild = pWndChild->GetNextWindow();
	}

	CHAR szClassName[MAX_PATH];
	if (::GetClassNameA(*pWnd, szClassName, ARRSIZE(szClassName)))
	{
		bool bButton = (__ascii_stricmp(szClassName, "Button") == 0);

		if (   (__ascii_stricmp(szClassName, "EDIT") == 0 && (pWnd->GetExStyle() & WS_EX_STATICEDGE))
			|| __ascii_stricmp(szClassName, "SysListView32") == 0
			|| __ascii_stricmp(szClassName, "msctls_trackbar32") == 0
			)
		{
			pWnd->ModifyStyleEx(WS_EX_STATICEDGE, WS_EX_CLIENTEDGE);
		}

		if (bButton)
			pWnd->ModifyStyle(BS_FLAT, 0);
	}
}

void FlatWindowStyles(CWnd* pWnd)
{
	CWnd* pWndChild = pWnd->GetWindow(GW_CHILD);
	while (pWndChild)
	{
		FlatWindowStyles(pWndChild);
		pWndChild = pWndChild->GetNextWindow();
	}

	CHAR szClassName[MAX_PATH];
	if (::GetClassNameA(*pWnd, szClassName, ARRSIZE(szClassName)))
	{
		bool bButton = (__ascii_stricmp(szClassName, "Button") == 0);

//		if (   !bButton
//			//|| (__ascii_stricmp(szClassName, "SysListView32") == 0 && (pWnd->GetStyle() & WS_BORDER) == 0)
//			|| __ascii_stricmp(szClassName, "msctls_trackbar32") == 0
//			)
//		{
//			pWnd->ModifyStyleEx(WS_EX_CLIENTEDGE, WS_EX_STATICEDGE);
//		}

		if (bButton)
			pWnd->ModifyStyle(0, BS_FLAT);
	}
}

void InitWindowStyles(CWnd* pWnd)
{
	if (thePrefs.GetStraightWindowStyles() < 0)
		return;
	else if (thePrefs.GetStraightWindowStyles() > 0)
		StraightWindowStyles(pWnd);
	else
		FlatWindowStyles(pWnd);
}

LRESULT CemuleDlg::OnVersionCheckResponse(WPARAM wParam, LPARAM lParam)
{
	if (WSAGETASYNCERROR(lParam) == 0)
	{
		int iBufLen = WSAGETASYNCBUFLEN(lParam);
		if (iBufLen >= sizeof(HOSTENT))
		{
			LPHOSTENT pHost = (LPHOSTENT)m_acVCDNSBuffer;
			if (pHost->h_length == 4 && pHost->h_addr_list && pHost->h_addr_list[0])
			{
				uint32 dwResult = ((LPIN_ADDR)(pHost->h_addr_list[0]))->s_addr;		
				// last byte contains informations about mirror urls, to avoid effects of future DDoS Attacks against eMules Homepage
				thePrefs.SetWebMirrorAlertLevel((uint8)(dwResult >> 24));
				uint8 abyCurVer[4] = { VERSION_BUILD + 1, VERSION_UPDATE, VERSION_MIN, 0};
				dwResult &= 0x00FFFFFF;
				if (dwResult > *(uint32*)abyCurVer){
					thePrefs.UpdateLastVC();
					SetActiveWindow();
					Log(LOG_SUCCESS|LOG_STATUSBAR,GetResString(IDS_NEWVERSIONAVL));
					ShowNotifier(GetResString(IDS_NEWVERSIONAVLPOPUP), TBN_NEWVERSION);
					if (!thePrefs.GetNotifierPopOnNewVersion()){
						if (AfxMessageBox(GetResString(IDS_NEWVERSIONAVL)+GetResString(IDS_VISITVERSIONCHECK),MB_YESNO)==IDYES) {
							CString theUrl;
							theUrl.Format(_T("/en/version_check.php?version=%i&language=%i"),theApp.m_uCurVersionCheck,thePrefs.GetLanguageID());
							theUrl = thePrefs.GetVersionCheckBaseURL()+theUrl;
							ShellExecute(NULL, NULL, theUrl, NULL, thePrefs.GetAppDir(), SW_SHOWDEFAULT);
						}
					}
				}
				else{
					thePrefs.UpdateLastVC();
					AddLogLine(true,GetResString(IDS_NONEWERVERSION));
				}
				return 0;
			}
		}
	}
	LogWarning(LOG_STATUSBAR,GetResString(IDS_NEWVERSIONFAILED));
	return 0;
}

void CemuleDlg::ShowSplash()
{
	ASSERT( m_pSplashWnd == NULL );
	if (m_pSplashWnd == NULL)
	{
		m_pSplashWnd = new CSplashScreen;
		if (m_pSplashWnd != NULL)
		{
			ASSERT(m_hWnd);
			if (m_pSplashWnd->Create(CSplashScreen::IDD, this))
			{
				m_pSplashWnd->ShowWindow(SW_SHOW);
				m_pSplashWnd->UpdateWindow();
				m_dwSplashTime = ::GetCurrentTime();
			}
			else
			{
				delete m_pSplashWnd;
				m_pSplashWnd = NULL;
			}
		}
	}
}

void CemuleDlg::DestroySplash()
{
	if (m_pSplashWnd != NULL)
	{
		m_pSplashWnd->DestroyWindow();
		delete m_pSplashWnd;
		m_pSplashWnd = NULL;
	}
}

LRESULT CemuleDlg::OnKickIdle(UINT nWhy, long lIdleCount)
{
	LRESULT lResult = 0;

	if (m_pSplashWnd)
	{
		if (::GetCurrentTime() - m_dwSplashTime > 2500)
		{
			// timeout expired, destroy the splash window
			DestroySplash();
			UpdateWindow();
		}
		else
		{
			// check again later...
			lResult = 1;
		}
	}

	if (m_bStartMinimized)
		PostStartupMinimized();

	if (searchwnd && searchwnd->m_hWnd)
	{
		if (theApp.m_app_state != APP_STATE_SHUTINGDOWN)
		{
			//extern void Mfc_IdleUpdateCmdUiTopLevelFrameList(CWnd* pMainFrame);
			//Mfc_IdleUpdateCmdUiTopLevelFrameList(this);
			theApp.OnIdle(0/*lIdleCount*/);	// NOTE: DO **NOT** CALL THIS WITH 'lIdleCount>0'

#ifdef _DEBUG
			// We really should call this to free up the temporary object maps from MFC.
			// It may/will show bugs (wrong usage of temp. MFC data) on couple of (hidden) places,
			// therefore it's right now too dangerous to put this in 'Release' builds..
			// ---
			// The Microsoft Foundation Class (MFC) Libraries create temporary objects that are 
			// used inside of message handler functions. In MFC applications, these temporary 
			// objects are automatically cleaned up in the CWinApp::OnIdle() function that is 
			// called in between processing messages.

			// To slow to be called on each KickIdle. Need a timer
			//extern void Mfc_IdleFreeTempMaps();
			//if (lIdleCount >= 0)
			//	Mfc_IdleFreeTempMaps();
#endif
		}
	}

	return lResult;
}

// [TPT] - [addon] - New Tooltips [Rayita]
BOOL CemuleDlg::PreTranslateMessage(MSG* pMsg)
{
	m_ttip.RelayEvent(pMsg);
	BOOL bResult = CTrayDialog::PreTranslateMessage(pMsg);

	if (m_pSplashWnd && m_pSplashWnd->m_hWnd != NULL &&
		(pMsg->message == WM_KEYDOWN	   ||
		 pMsg->message == WM_SYSKEYDOWN	   ||
		 pMsg->message == WM_LBUTTONDOWN   ||
		 pMsg->message == WM_RBUTTONDOWN   ||
		 pMsg->message == WM_MBUTTONDOWN   ||
		 pMsg->message == WM_NCLBUTTONDOWN ||
		 pMsg->message == WM_NCRBUTTONDOWN ||
		 pMsg->message == WM_NCMBUTTONDOWN))
	{
		DestroySplash();
		UpdateWindow();
	} 
	return bResult;
}
// [TPT] - [addon] - New Tooltips [Rayita]

void CemuleDlg::HtmlHelp(DWORD_PTR dwData, UINT nCmd)
{
	CWinApp* pApp = AfxGetApp();
	ASSERT_VALID(pApp);
	ASSERT(pApp->m_pszHelpFilePath != NULL);
	// to call HtmlHelp the m_fUseHtmlHelp must be set in
	// the application's constructor
	ASSERT(pApp->m_eHelpType == afxHTMLHelp);

	CWaitCursor wait;

	PrepareForHelp();

	// need to use top level parent (for the case where m_hWnd is in DLL)
	CWnd* pWnd = GetTopLevelParent();

	TRACE(traceAppMsg, 0, _T("HtmlHelp: pszHelpFile = '%s', dwData: $%lx, fuCommand: %d.\n"), pApp->m_pszHelpFilePath, dwData, nCmd);

	bool bHelpError = false;
	CString strHelpError;
	int iTry = 0;
	while (iTry++ < 2)
	{
		if (!AfxHtmlHelp(pWnd->m_hWnd, pApp->m_pszHelpFilePath, nCmd, dwData))
		{
			bHelpError = true;
			strHelpError.LoadString(AFX_IDP_FAILED_TO_LAUNCH_HELP);

			typedef struct tagHH_LAST_ERROR
			{
				int      cbStruct;
				HRESULT  hr;
				BSTR     description;
			} HH_LAST_ERROR;
			HH_LAST_ERROR hhLastError = {0};
			hhLastError.cbStruct = sizeof hhLastError;
			HWND hwndResult = AfxHtmlHelp(pWnd->m_hWnd, NULL, HH_GET_LAST_ERROR, reinterpret_cast<DWORD>(&hhLastError));
			if (hwndResult != 0)
			{
				if (FAILED(hhLastError.hr))
				{
					if (hhLastError.description)
					{
						USES_CONVERSION;
						strHelpError = OLE2T(hhLastError.description);
						::SysFreeString(hhLastError.description);
					}
					if (   hhLastError.hr == 0x8004020A  /*no topics IDs available in Help file*/
						|| hhLastError.hr == 0x8004020B) /*requested Help topic ID not found*/
					{
						// try opening once again without help topic ID
						if (nCmd != HH_DISPLAY_TOC)
						{
							nCmd = HH_DISPLAY_TOC;
							dwData = 0;
							continue;
						}
					}
				}
			}
			break;
		}
		else
		{
			bHelpError = false;
			strHelpError.Empty();
			break;
		}
	}

	if (bHelpError)
	{
		if (AfxMessageBox(CString(pApp->m_pszHelpFilePath) + _T("\n\n") + strHelpError + _T("\n\n") + GetResString(IDS_ERR_NOHELP), MB_YESNO | MB_ICONERROR) == IDYES)
		{
			CString strUrl = thePrefs.GetHomepageBaseURL() + _T("/home/perl/help.cgi");
			ShellExecute(NULL, NULL, strUrl, NULL, thePrefs.GetAppDir(), SW_SHOWDEFAULT);
		}
	}
}

LRESULT CemuleDlg::OnPeerCacheResponse(WPARAM wParam, LPARAM lParam)
{
	return theApp.m_pPeerCache->OnPeerCacheCheckResponse(wParam,lParam);
}

#if (_WIN32_IE < 0x0500)

#ifndef TBIF_BYINDEX
#define TBIF_BYINDEX            0x80000000
#endif

typedef struct tagNMREBARCHEVRON
{
    NMHDR hdr;
    UINT uBand;
    UINT wID;
    LPARAM lParam;
    RECT rc;
    LPARAM lParamNM;
} NMREBARCHEVRON, *LPNMREBARCHEVRON;
#endif //(_WIN32_IE < 0x0500)

BOOL CemuleDlg::OnChevronPushed(UINT id, NMHDR* pNMHDR, LRESULT* plResult)
{
	if (!thePrefs.GetUseReBarToolbar())
		return FALSE;

	NMREBARCHEVRON* pnmrc = (NMREBARCHEVRON*)pNMHDR;

	ASSERT( id == AFX_IDW_REBAR );
	ASSERT( pnmrc->uBand == 0 );
	ASSERT( pnmrc->wID == 0 );
	//ASSERT( m_mapCmdToIcon.GetSize() != 0 );

	// get visible area of rebar/toolbar
	CRect rcVisibleButtons;
	toolbar->GetClientRect(&rcVisibleButtons);

	// search the first toolbar button which is not fully visible
	int iButtons = toolbar->GetButtonCount();
	for (int i = 0; i < iButtons; i++)
	{
		CRect rcButton;
		toolbar->GetItemRect(i, &rcButton);

		CRect rcVisible;
		if (!rcVisible.IntersectRect(&rcVisibleButtons, &rcButton) || !EqualRect(rcButton, rcVisible))
			break;
	}

	// create menu for all toolbar buttons which are not (fully) visible
	BOOL bLastMenuItemIsSep = TRUE;
	//[TPT] - No use CTitleMenu class
	CMenu menu;
	menu.CreatePopupMenu();

	while (i < iButtons)
	{
		TCHAR szString[256];
		szString[0] = _T('\0');
		TBBUTTONINFO tbbi = {0};
		tbbi.cbSize = sizeof tbbi;
		tbbi.dwMask = TBIF_BYINDEX | TBIF_COMMAND | TBIF_STYLE | TBIF_STATE | TBIF_TEXT;
		tbbi.cchText = ARRSIZE(szString);
		tbbi.pszText = szString;
		if (toolbar->GetButtonInfo(i, &tbbi) != -1)
		{
			if (tbbi.fsStyle & TBSTYLE_SEP)
			{
				if (!bLastMenuItemIsSep)
					bLastMenuItemIsSep = menu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
			}
			else
			{
				if (szString[0] != _T('\0') && menu.AppendMenu(MF_STRING, tbbi.idCommand, szString /*, GetIconFromCmdId(tbbi.idCommand)*/))
				{
					bLastMenuItemIsSep = FALSE;
					if (tbbi.fsState & TBSTATE_CHECKED)
						menu.CheckMenuItem(tbbi.idCommand, MF_BYCOMMAND | MF_CHECKED);
					if ((tbbi.fsState & TBSTATE_ENABLED) == 0)
						menu.EnableMenuItem(tbbi.idCommand, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
				}
			}
		}

		i++;
	}

	CPoint ptMenu(pnmrc->rc.left, pnmrc->rc.top);
	ClientToScreen(&ptMenu);
	ptMenu.y += rcVisibleButtons.Height();
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, ptMenu.x, ptMenu.y, this);
	*plResult = 1;
	return FALSE;
}

bool CemuleDlg::IsPreferencesDlgOpen() const
{
	return preferenceswnd->IsDialogOpen();	// [TPT] - SLUGFILLER: modelessDialogs
}

int CemuleDlg::ShowPreferences(UINT uStartPageID)
	{
	// [TPT] - SLUGFILLER: modelessDialogs
	int retval = IDOK;
	if (preferenceswnd->IsDialogOpen())
		retval = -1;
	else if (uStartPageID != (UINT)-1)
		preferenceswnd->SetStartPage(uStartPageID);
		preferenceswnd->OpenDialog();
	return retval;
	// [TPT] - SLUGFILLER: modelessDialogs
}

// [TPT]
// Maella -New Timer Management- (quick-n-dirty)
// Remark: Don't use the 'global' SetTimer
//         => avoid problem with CrashRtp.dll
void CemuleDlg::StartMainTimer() 
{
	m_nMainTimer = SetTimer(MAIN_TIMER, TIMER_PERIOD, 0);
}

void CemuleDlg::StopMainTimer() 
{
   KillTimer(m_nMainTimer);   
}

void CemuleDlg::OnTimer(UINT_PTR nIDEvent) 
{
	if(nIDEvent == MAIN_TIMER)
	{
		// Remark: This try/catch should be removed for Beta+RC versions, so
		//         it would possible to collect information about the location
		//         of diverse exceptions
		try {
				MainTimerProc();
		}
		CATCH_DFLT_EXCEPTIONS(_T("CemuleDlg::OnTimer"))
	}

   // Call base class handler.
   CTrayDialog::OnTimer(nIDEvent);
}

// Remark: this method was previously in UploadQueue
void CemuleDlg::MainTimerProc(){
	
	// NOTE: Always handle all type of MFC exceptions in TimerProcs - otherwise we'll get mem leaks
	try
	{
		// Barry - Don't do anything if the app is shutting down - can cause unhandled exceptions
		if (!theApp.emuledlg->IsRunning())
			return;    
			
		DWORD timer = ::GetTickCount();

		static uint16 counter; counter++;
		static UINT _uSaveStatistics;

		// Elandal:ThreadSafeLogging -->
		// other threads may have queued up log lines. This prints them.
		if ((counter % 2) == 0)
		{
			theApp.HandleDebugLogQueue();
			theApp.HandleLogQueue();
		}
		// Elandal: ThreadSafeLogging <--
		
		// [TPT] - Maella -Accurate measure of bandwidth: eDonkey data + control, network adapter-	
		uint32 eMuleOut;
		uint32 notUsed;
		//ikabot, only one second sample
		theApp.pBandWidthControl->GetDatarates(1/*thePrefs.GetDatarateSamples()*/,
											notUsed, notUsed,
											eMuleOut, notUsed,
											notUsed, notUsed);

		theApp.lastCommonRouteFinder->SetPrefs(thePrefs.IsDynUpEnabled(), eMuleOut, thePrefs.GetMinUpload()*1024, (thePrefs.GetMaxUpload() != 0)?thePrefs.GetMaxUpload()*1024:thePrefs.GetMaxGraphUploadRate()*1024, thePrefs.IsDynUpUseMillisecondPingTolerance(), (thePrefs.GetDynUpPingTolerance() > 100)?((thePrefs.GetDynUpPingTolerance()-100)/100.0f):0, thePrefs.GetDynUpPingToleranceMilliseconds(), thePrefs.GetDynUpGoingUpDivider(), thePrefs.GetDynUpGoingDownDivider(), thePrefs.GetDynUpNumberOfPings(), 20); // PENDING: Hard coded min pLowestPingAllowed

		theApp.pBandWidthControl->Process();
		theApp.uploadqueue->Process();
		theApp.downloadqueue->Process();

		// 1 second clock (=> CPU load balancing)	
		uchar tick_timer = counter % 10;
		if(tick_timer == 1)
		{
			// Need to be synchronized				
			theApp.downloadqueue->CompDownloadRate(); // Update GUI for Download Queue
			theApp.uploadqueue->CompUploadRate(); // Update GUI for Upload Queue
			
			if ((::GetTickCount() - timer > TIMER_PERIOD) && (thePrefs.GetBlockPhoenixMsg() == false))
				if (thePrefs.GetVerbose()) AddPhoenixLogLine(false, _T("%d: Counter 1 to slow"),  ::GetTickCount()-timer);
		}
		else if (tick_timer == 2)
		{
			// N seconds
			static showRate; showRate++;
			if(showRate >= DISPLAY_REFRESH){
				showRate = 0;
				ShowTransferRate(); // Update GUI control bar + icon tray
			}		

			if ((::GetTickCount() - timer > TIMER_PERIOD) && (thePrefs.GetBlockPhoenixMsg() == false))
				if (thePrefs.GetVerbose()) AddPhoenixLogLine(false, _T("%d: Counter 2 to slow"),  ::GetTickCount()-timer);
		}
		else if (tick_timer == 3)
		{
			theApp.clientlist->Process();					

			if ((::GetTickCount() - timer > TIMER_PERIOD) && (thePrefs.GetBlockPhoenixMsg() == false))
				if (thePrefs.GetVerbose()) AddPhoenixLogLine(false, _T("%d: Counter 3 to slow"),  ::GetTickCount()-timer);
			
		} 
		else if (tick_timer == 4)
		{			
			if( Kademlia::CKademlia::isRunning() )
			{
				Kademlia::CKademlia::process();
				if(Kademlia::CKademlia::getPrefs()->hasLostConnection())
				{
					Kademlia::CKademlia::stop();
					theApp.emuledlg->ShowConnectionState();
				}
			}

			if ((::GetTickCount() - timer > TIMER_PERIOD) && (thePrefs.GetBlockPhoenixMsg() == false))
				if (thePrefs.GetVerbose()) AddPhoenixLogLine(false, _T("%d: Counter 4 to slow"),  ::GetTickCount()-timer);
		} 
		else if (tick_timer == 5)
		{		
			if( theApp.serverconnect->IsConnecting() && !theApp.serverconnect->IsSingleConnect() )
				theApp.serverconnect->TryAnotherConnectionrequest();
			
			if ((::GetTickCount() - timer > TIMER_PERIOD) && (thePrefs.GetBlockPhoenixMsg() == false))
				if (thePrefs.GetVerbose()) AddPhoenixLogLine(false, _T("%d: Counter 5 to slow"),  ::GetTickCount()-timer);
		}
		else if (tick_timer == 6)
		{
			if (theApp.serverconnect->IsConnecting()) 
				theApp.serverconnect->CheckForTimeout();			

			if ((::GetTickCount() - timer > TIMER_PERIOD) && (thePrefs.GetBlockPhoenixMsg() == false))
				if (thePrefs.GetVerbose()) AddPhoenixLogLine(false, _T("%d: Counter 6 to slow"),  ::GetTickCount()-timer);
		}	
		else if (tick_timer == 7)
		{
			if (thePrefs.WatchClipboard4ED2KLinks()) 
				theApp.SearchClipboard();			
			
			if ((::GetTickCount() - timer > TIMER_PERIOD) && (thePrefs.GetBlockPhoenixMsg() == false))
				if (thePrefs.GetVerbose()) AddPhoenixLogLine(false, _T("%d: Counter 7 to slow"),  ::GetTickCount()-timer);
		}
		else if (tick_timer == 8)
		{
			theApp.sharedfiles->Process();
			theApp.clientlist->CleanUpClientList(); //Maella -Extended clean-up II-

			if ((::GetTickCount() - timer > TIMER_PERIOD) && (thePrefs.GetBlockPhoenixMsg() == false))
				if (thePrefs.GetVerbose()) AddPhoenixLogLine(false, _T("%d: Counter 8 to slow"),  ::GetTickCount()-timer);
		} 
		else if (tick_timer == 9)
		{
			theApp.clientlist->ProcessEx(); // [TPT] - eWombat SNAFU v2

			if ((::GetTickCount() - timer > TIMER_PERIOD) && (thePrefs.GetBlockPhoenixMsg() == false))
				if (thePrefs.GetVerbose()) AddPhoenixLogLine(false, _T("%d: Counter 9 to slow"),  ::GetTickCount()-timer);
		}
		else if (counter >= (1000/TIMER_PERIOD))
		{
			counter=0;
			static uint16 sec; sec++;
					
			theApp.emuledlg->statisticswnd->Process(); // Update GUI for Upload Queue
					
			// 1 second
			if (sec == 1)
			{
				// mobilemule sockets
				theApp.mmserver->Process();

				theApp.listensocket->UpdateConnectionsStatus();

				theApp.OnlineSig(); // Added By Bouc7 

				if (theApp.minimule->IsWindowVisible() || thePrefs.GetMiniMuleLives()) 
					theApp.minimule->RunMiniMule(); // [TPT] - TBH: minimule
				
				if ((::GetTickCount() - timer > TIMER_PERIOD) && (thePrefs.GetBlockPhoenixMsg() == false))
						if (thePrefs.GetVerbose()) AddPhoenixLogLine(false, _T("%d: Second 1 to slow"),  ::GetTickCount()-timer);

			}
			// 2 seconds		
			else if (sec == 2)
			{
				theApp.listensocket->Process();	
				// ZZ:UploadSpeedSense -->
				theApp.emuledlg->ShowPing();

				bool gotEnoughHosts = theApp.clientlist->GiveClientsForTraceRoute();
				if(gotEnoughHosts == false) {
					theApp.serverlist->GiveServersForTraceRoute();
				}
				// ZZ:UploadSpeedSense <--						

				if ((::GetTickCount() - timer > TIMER_PERIOD) && (thePrefs.GetBlockPhoenixMsg() == false))
				if (thePrefs.GetVerbose()) AddPhoenixLogLine(false, _T("%d: Second 2 to slow"),  ::GetTickCount()-timer);
			}
			// 3 seconds		
			else if (sec == 3)
			{
				// try to use different time intervals here to not create any disk-IO bottle necks by saving all files at once
				theApp.clientcredits->Process();	// 13 minutes
				theApp.serverlist->Process();		// 17 minutes			

				if ((::GetTickCount() - timer > TIMER_PERIOD) && (thePrefs.GetBlockPhoenixMsg() == false))
					if (thePrefs.GetVerbose()) AddPhoenixLogLine(false, _T("%d: Second 3 to slow"),  ::GetTickCount()-timer);
			}
			// 4 seconds		
			else if (sec == 4)
			{
				// try to use different time intervals here to not create any disk-IO bottle necks by saving all files at once
				theApp.knownfiles->Process();		// 11 minutes
				theApp.friendlist->Process();		// 19 minutes		

				if ((::GetTickCount() - timer > TIMER_PERIOD) && (thePrefs.GetBlockPhoenixMsg() == false))
					if (thePrefs.GetVerbose()) AddPhoenixLogLine(false, _T("%d: Second 4 to slow"),  ::GetTickCount()-timer);
			}
			// 5 seconds
			else if (sec>=5) {

				#ifdef _DEBUG
					if (thePrefs.m_iDbgHeap > 0 && !AfxCheckMemory())
						AfxDebugBreak();
				#endif

				sec = 0;
				// [TPT] - Maella -One-queue-per-file- (idea bloodymad)
				if (!thePrefs.TransferFullChunks())
						theApp.uploadqueue->UpdateMaxClientScore();
				// [TPT] - end
					
				// update cat-titles with downloadinfos only when needed
				if (thePrefs.ShowCatTabInfos() && 
					theApp.emuledlg->activewnd==theApp.emuledlg->transferwnd && 
					theApp.emuledlg->IsWindowVisible()) 
					theApp.emuledlg->transferwnd->UpdateCatTabTitles(false);
				
					if (thePrefs.IsSchedulerEnabled())
						theApp.scheduler->Check();

					theApp.emuledlg->transferwnd->UpdateListCount(1, -1);
				
				if ((::GetTickCount() - timer > TIMER_PERIOD) && (thePrefs.GetBlockPhoenixMsg() == false))
					if (thePrefs.GetVerbose()) AddPhoenixLogLine(false, _T("%d: Second 5 to slow"),  ::GetTickCount()-timer);
			}

			// -khaos--+++>
			// 60 seconds
			static uint16 statsave; statsave++;
			if (statsave>=60) {
				// Time to save our cumulative statistics.
				statsave=0;
				if (thePrefs.GetWSIsEnabled())
					theApp.webserver->UpdateSessionCount();
					
				theApp.serverconnect->KeepConnectionAlive();
			}

			_uSaveStatistics++;
		if (_uSaveStatistics >= thePrefs.GetStatsSaveInterval())
			{
				_uSaveStatistics = 0;
				thePrefs.SaveStats();
			}
		}

		// need more accuracy here. don't rely on the 'sec' and 'statsave' helpers.
		thePerfLog.LogSamples();

	}
	CATCH_DFLT_EXCEPTIONS(_T("CUploadQueue::UploadTimer"))

}
// Maella end


////////////////////////////////////////////////////////////////////
// BEGIN - emulEspa�a: added by [TPT]-MoNKi [MoNKi: -invisible mode-]
LRESULT CemuleDlg::OnHotKey(WPARAM wParam, LPARAM lParam)
{
	//ikabotTest
	//if(wParam == HOTKEY_INVISIBLEMODE_ID) RestoreWindow();

	// Allows "invisible mode" on multiple instances of eMule
	// Restore the rest of hidden emules
	EnumWindows(AskEmulesForInvisibleMode, INVMODE_RESTOREWINDOW);
	
	return 0;
}

BOOL CemuleDlg::RegisterInvisibleHotKey()
{
	if(m_hWnd && IsRunning()){
		bool res = RegisterHotKey( this->m_hWnd, HOTKEY_INVISIBLEMODE_ID ,
						   thePrefs.GetInvisibleModeHKKeyModifier(),
						   thePrefs.GetInvisibleModeHKKey());
		return res;
	} else
		return false;
}

BOOL CemuleDlg::UnRegisterInvisibleHotKey()
{
	if(m_hWnd){
		bool res = !(UnregisterHotKey(this->m_hWnd, HOTKEY_INVISIBLEMODE_ID));

		// Allows "invisible mode" on multiple instances of eMule
		// Only one app (eMule) can register the hotkey, if we unregister, we need
		// to register the hotkey in other emule.
		EnumWindows(AskEmulesForInvisibleMode, INVMODE_REGISTERHOTKEY);
		return res;
	} else
		return false;
}

// Allows "invisible mode" on multiple instances of eMule
// LOWORD(WPARAM) -> HotKey KeyModifier
// HIWORD(WPARAM) -> HotKey VirtualKey
// LPARAM		  -> int:	INVMODE_RESTOREWINDOW	-> Restores the window
//							INVMODE_REGISTERHOTKEY	-> Registers the hotkey
LRESULT CemuleDlg::OnRestoreWindowInvisibleMode(WPARAM wParam, LPARAM lParam)
{
	if (thePrefs.GetInvisibleMode() &&
		(UINT)LOWORD(wParam) == thePrefs.GetInvisibleModeHKKeyModifier() &&
		(TCHAR)HIWORD(wParam) == thePrefs.GetInvisibleModeHKKey()) {
			switch(lParam){
				case INVMODE_RESTOREWINDOW:
					RestoreWindow();
					break;
				case INVMODE_REGISTERHOTKEY:
					RegisterInvisibleHotKey();
					break;
			}
			return UWM_RESTORE_WINDOW_IM;
	} else
		return false;
} 

// Allows "invisible mode" on multiple instances of eMule
BOOL CALLBACK CemuleDlg::AskEmulesForInvisibleMode(HWND hWnd, LPARAM lParam){
	DWORD dwMsgResult;
	WPARAM msgwParam;

	msgwParam=MAKEWPARAM(thePrefs.GetInvisibleModeHKKeyModifier(),
				thePrefs.GetInvisibleModeHKKey());

	LRESULT res = ::SendMessageTimeout(hWnd,UWM_RESTORE_WINDOW_IM, msgwParam, lParam,
				SMTO_BLOCK |SMTO_ABORTIFHUNG,10000,&dwMsgResult);
	
	return res; 
} 
// END emulEspa�a
////////////////////////////////////////////////////////////


// [TPT] - [addon] - New Tooltips [Rayita]
BOOL CemuleDlg::OnToolTipNotify(UINT id, NMHDR *pNMH, LRESULT *pResult)
{
	NM_PPTOOLTIP_DISPLAY * pNotify = (NM_PPTOOLTIP_DISPLAY*)pNMH;
	int control_id = CWnd::FromHandle(pNotify->ti->hWnd)->GetDlgCtrlID();
	if (control_id == IDC_STATUSBAR)
		pNotify->ti->hIcon = statusbar->GetTipInfo(*((CString *)&pNotify->ti->sTooltip));

	return TRUE;
}

void CemuleDlg::SetTTDelay()
{
	m_ttip.SetDelayTime(TTDT_AUTOPOP, 40000);
	m_ttip.SetDelayTime(TTDT_INITIAL, thePrefs.GetToolTipDelay()/2);
}
// [TPT] - MFCK [addon] - New Tooltips [Rayita]
