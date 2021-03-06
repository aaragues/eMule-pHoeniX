/*//this file is part of eMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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
#include "PPgWebCache.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////////
// CPPgWebCache dialog

IMPLEMENT_DYNAMIC(CPPgWebCache, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgWebCache, CPropertyPage)
	ON_WM_HSCROLL()	
	ON_WM_DESTROY()
	ON_MESSAGE(WM_TREEOPTSCTRL_NOTIFY, OnTreeOptsCtrlNotify)
	ON_WM_HELPINFO()
END_MESSAGE_MAP()

CPPgWebCache::CPPgWebCache()
	: CPropertyPage(CPPgWebCache::IDD)
	, m_ctrlTreeOptions(theApp.m_iDfltImageListColorFlags)
{
	m_iFileBufferSize = 0;
	m_iQueueSize = 0;
	m_iMaxConnPerFive = 0;
	m_iAutoTakeEd2kLinks = 0;
	m_iVerbose = 0;
	m_iDebugSourceExchange = 0;
	m_iLogBannedClients = 0;
	m_iLogRatingDescReceived = 0;
	m_iLogSecureIdent = 0;
	m_iLogFilteredIPs = 0;
	m_iLogFileSaving = 0;
	m_iCreditSystem = 0;
	m_iLog2Disk = 0;
	m_iDebug2Disk = 0;
	m_iCommitFiles = 0;
	m_iFilterLANIPs = 0;
	m_iExtControls = 0;
	m_uServerKeepAliveTimeout = 0;
	m_iCheckDiskspace = 0;
	m_fMinFreeDiskSpaceMB = 0.0F;
	(void)m_sYourHostname;

	// ZZ:UploadSpeedSense -->
    m_iDynUpEnabled = 0;
    m_iDynUpMinUpload = 0;
    m_iDynUpPingTolerance = 0;
    m_iDynUpGoingUpDivider = 0;
    m_iDynUpGoingDownDivider = 0;
    m_iDynUpNumberOfPings = 0;
	// ZZ:UploadSpeedSense <--

	m_bInitializedTreeOpts = false;
	m_htiMaxCon5Sec = NULL;
	m_htiAutoTakeEd2kLinks = NULL;
	m_htiVerboseGroup = NULL;
	m_htiVerbose = NULL;
	m_htiDebugSourceExchange = NULL;
	m_htiLogBannedClients = NULL;
	m_htiLogRatingDescReceived = NULL;
	m_htiLogSecureIdent = NULL;
	m_htiLogFilteredIPs = NULL;
	m_htiLogFileSaving = NULL;
	m_htiCreditSystem = NULL;
	m_htiLog2Disk = NULL;
	m_htiDebug2Disk = NULL;
	m_htiCommit = NULL;
	m_htiCommitNever = NULL;
	m_htiCommitOnShutdown = NULL;
	m_htiCommitAlways = NULL;
	m_htiFilterLANIPs = NULL;
	m_htiExtControls = NULL;
	m_htiServerKeepAliveTimeout = NULL;
	m_htiCheckDiskspace = NULL;	// SLUGFILLER: checkDiskspace
	m_htiMinFreeDiskSpace = NULL;
	m_htiYourHostname = NULL;	// itsonlyme: hostnameSource

	// ZZ:UploadSpeedSense -->
    m_htiDynUp = NULL;
	m_htiDynUpEnabled = NULL;
    m_htiDynUpMinUpload = NULL;
    m_htiDynUpPingTolerance = NULL;
    m_htiDynUpGoingUpDivider = NULL;
    m_htiDynUpGoingDownDivider = NULL;
    m_htiDynUpNumberOfPings = NULL;
	// ZZ:UploadSpeedSense <--
}

CPPgWebCache::~CPPgWebCache()
{
}

void CPPgWebCache::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EXT_OPTS, m_ctrlTreeOptions);
	if (!m_bInitializedTreeOpts)
	{
		int iImgBackup = 8; // default icon
		int iImgLog = 8; // default icon
		// ZZ:UploadSpeedSense -->
		int iImgDynyp = 8; // default icon
		// ZZ:UploadSpeedSense <--
		CImageList* piml = m_ctrlTreeOptions.GetImageList(TVSIL_NORMAL);
		if (piml){
			iImgBackup = piml->Add(CTempIconLoader("Harddisk"));
			iImgLog = piml->Add(CTempIconLoader("Log"));
            // ZZ:UploadSpeedSense -->
			iImgDynyp = piml->Add(CTempIconLoader("upload"));
			// ZZ:UploadSpeedSense <--
		}

		m_htiMaxCon5Sec = m_ctrlTreeOptions.InsertItem(GetResString(IDS_MAXCON5SECLABEL), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, TVI_ROOT);
		m_ctrlTreeOptions.AddEditBox(m_htiMaxCon5Sec, RUNTIME_CLASS(CNumTreeOptionsEdit));
		m_htiAutoTakeEd2kLinks = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_AUTOTAKEED2KLINKS), TVI_ROOT, m_iAutoTakeEd2kLinks);

		m_htiCreditSystem = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_USECREDITSYSTEM), TVI_ROOT, m_iCreditSystem);
		m_htiFilterLANIPs = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_PW_FILTER), TVI_ROOT, m_iFilterLANIPs);
		m_htiExtControls = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_SHOWEXTSETTINGS), TVI_ROOT, m_iExtControls);

		m_htiCheckDiskspace = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_CHECKDISKSPACE), TVI_ROOT, m_iCheckDiskspace);	// SLUGFILLER: checkDiskspace
		m_htiMinFreeDiskSpace = m_ctrlTreeOptions.InsertItem(GetResString(IDS_MINFREEDISKSPACE), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiCheckDiskspace);
		m_ctrlTreeOptions.AddEditBox(m_htiMinFreeDiskSpace, RUNTIME_CLASS(CNumTreeOptionsEdit));
		// itsonlyme: hostnameSource
		m_htiYourHostname = m_ctrlTreeOptions.InsertItem(GetResString(IDS_YOURHOSTNAME), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, TVI_ROOT);
		m_ctrlTreeOptions.AddEditBox(m_htiYourHostname, RUNTIME_CLASS(CTreeOptionsEdit));
		// itsonlyme: hostnameSource

		m_htiLog2Disk = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_LOG2DISK), TVI_ROOT, m_iLog2Disk);
		if (thePrefs.GetEnableVerboseOptions())
		{
			m_htiVerboseGroup = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_VERBOSE), iImgLog, TVI_ROOT);
			m_htiVerbose = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_ENABLED), m_htiVerboseGroup, m_iVerbose);
			m_htiDebug2Disk = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_LOG2DISK), m_htiVerboseGroup, m_iDebug2Disk);
			m_htiDebugSourceExchange = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_DEBUG_SOURCE_EXCHANGE), m_htiVerboseGroup, m_iDebugSourceExchange);
			m_htiLogBannedClients = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_LOG_BANNED_CLIENTS), m_htiVerboseGroup, m_iLogBannedClients);
			m_htiLogRatingDescReceived = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_LOG_RATING_RECV), m_htiVerboseGroup, m_iLogRatingDescReceived);
			m_htiLogSecureIdent = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_LOG_SECURE_IDENT), m_htiVerboseGroup, m_iLogSecureIdent);
			m_htiLogFilteredIPs = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_LOG_FILTERED_IPS), m_htiVerboseGroup, m_iLogFilteredIPs);
			m_htiLogFileSaving = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_LOG_FILE_SAVING), m_htiVerboseGroup, m_iLogFileSaving);
		}

		m_htiCommit = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_COMMITFILES), iImgBackup, TVI_ROOT);
		m_htiCommitNever = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_NEVER), m_htiCommit, m_iCommitFiles == 0);
		m_htiCommitOnShutdown = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_ONSHUTDOWN), m_htiCommit, m_iCommitFiles == 1);
		m_htiCommitAlways = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_ALWAYS), m_htiCommit, m_iCommitFiles == 2);

		m_htiServerKeepAliveTimeout = m_ctrlTreeOptions.InsertItem(GetResString(IDS_SERVERKEEPALIVETIMEOUT), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, TVI_ROOT);
		m_ctrlTreeOptions.AddEditBox(m_htiServerKeepAliveTimeout, RUNTIME_CLASS(CNumTreeOptionsEdit));

		// ZZ:UploadSpeedSense -->
        m_htiDynUp = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_DYNUP), iImgDynyp, TVI_ROOT);
		m_htiDynUpEnabled = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_DYNUPENABLED), m_htiDynUp, m_iDynUpEnabled);

        m_htiDynUpMinUpload = m_ctrlTreeOptions.InsertItem(GetResString(IDS_DYNUP_PINGTOLERANCE), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUp);
		m_ctrlTreeOptions.AddEditBox(m_htiDynUpMinUpload, RUNTIME_CLASS(CNumTreeOptionsEdit));

        m_htiDynUpPingTolerance = m_ctrlTreeOptions.InsertItem(GetResString(IDS_DYNUP_PINGTOLERANCE), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUp);
		m_ctrlTreeOptions.AddEditBox(m_htiDynUpPingTolerance, RUNTIME_CLASS(CNumTreeOptionsEdit));

        m_htiDynUpGoingUpDivider = m_ctrlTreeOptions.InsertItem(GetResString(IDS_DYNUP_GOINGUPDIVIDER), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUp);
		m_ctrlTreeOptions.AddEditBox(m_htiDynUpGoingUpDivider, RUNTIME_CLASS(CNumTreeOptionsEdit));

        m_htiDynUpGoingDownDivider = m_ctrlTreeOptions.InsertItem(GetResString(IDS_DYNUP_GOINGDOWNDIVIDER), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUp);
		m_ctrlTreeOptions.AddEditBox(m_htiDynUpGoingDownDivider, RUNTIME_CLASS(CNumTreeOptionsEdit));

        m_htiDynUpNumberOfPings = m_ctrlTreeOptions.InsertItem(GetResString(IDS_DYNUP_NUMBEROFPINGS), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUp);
		m_ctrlTreeOptions.AddEditBox(m_htiDynUpNumberOfPings, RUNTIME_CLASS(CNumTreeOptionsEdit));
		// ZZ:UploadSpeedSense <--

		if (m_htiVerboseGroup)
			m_ctrlTreeOptions.Expand(m_htiVerboseGroup, TVE_EXPAND);
		m_ctrlTreeOptions.Expand(m_htiCommit, TVE_EXPAND);
		m_ctrlTreeOptions.Expand(m_htiCheckDiskspace, TVE_EXPAND);

		// ZZ:UploadSpeedSense -->
		m_ctrlTreeOptions.Expand(m_htiDynUp, TVE_EXPAND);
		// ZZ:UploadSpeedSense <--

		m_ctrlTreeOptions.SendMessage(WM_VSCROLL, SB_TOP);

        m_bInitializedTreeOpts = true;
	}

	DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiMaxCon5Sec, m_iMaxConnPerFive);
	DDV_MinMaxInt(pDX, m_iMaxConnPerFive, 1, INT_MAX);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiAutoTakeEd2kLinks, m_iAutoTakeEd2kLinks);
	if (m_htiVerbose)				DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiVerbose, m_iVerbose);
	if (m_htiDebug2Disk)			DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiDebug2Disk, m_iDebug2Disk);
	if (m_htiDebugSourceExchange)	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiDebugSourceExchange, m_iDebugSourceExchange);
	if (m_htiLogBannedClients)		DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLogBannedClients, m_iLogBannedClients);
	if (m_htiLogRatingDescReceived) DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLogRatingDescReceived, m_iLogRatingDescReceived);
	if (m_htiLogSecureIdent)		DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLogSecureIdent, m_iLogSecureIdent);
	if (m_htiLogFilteredIPs)		DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLogFilteredIPs, m_iLogFilteredIPs);
	if (m_htiLogFileSaving)			DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLogFileSaving, m_iLogFileSaving);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiCreditSystem, m_iCreditSystem);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLog2Disk, m_iLog2Disk);
	DDX_TreeRadio(pDX, IDC_EXT_OPTS, m_htiCommit, m_iCommitFiles);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiFilterLANIPs, m_iFilterLANIPs);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiExtControls, m_iExtControls);
	DDX_Text(pDX, IDC_EXT_OPTS, m_htiServerKeepAliveTimeout, m_uServerKeepAliveTimeout);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiCheckDiskspace, m_iCheckDiskspace);	// SLUGFILLER: checkDiskspace
	DDX_Text(pDX, IDC_EXT_OPTS, m_htiMinFreeDiskSpace, m_fMinFreeDiskSpaceMB);
	DDV_MinMaxFloat(pDX, m_fMinFreeDiskSpaceMB, 0.0, UINT_MAX / (1024*1024));
	DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiYourHostname, m_sYourHostname);	// itsonlyme: hostnameSource

	if (m_htiDebug2Disk)			m_ctrlTreeOptions.SetCheckBoxEnable(m_htiDebug2Disk, m_iVerbose);
	if (m_htiDebugSourceExchange)	m_ctrlTreeOptions.SetCheckBoxEnable(m_htiDebugSourceExchange, m_iVerbose);
	if (m_htiLogBannedClients)		m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogBannedClients, m_iVerbose);
	if (m_htiLogRatingDescReceived) m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogRatingDescReceived, m_iVerbose);
	if (m_htiLogSecureIdent)		m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogSecureIdent, m_iVerbose);
	if (m_htiLogFilteredIPs)		m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogFilteredIPs, m_iVerbose);
	if (m_htiLogFileSaving)			m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogFileSaving, m_iVerbose);
	
	m_ctrlTreeOptions.SetCheckBoxEnable(m_htiAutoTakeEd2kLinks, HaveEd2kRegAccess());

	// ZZ:UploadSpeedSense -->
    DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiDynUpEnabled, m_iDynUpEnabled);

    DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiDynUpMinUpload, m_iDynUpMinUpload);
	DDV_MinMaxInt(pDX, m_iDynUpMinUpload, 1, INT_MAX);

    DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiDynUpPingTolerance, m_iDynUpPingTolerance);
	DDV_MinMaxInt(pDX, m_iDynUpPingTolerance, 100, INT_MAX);

    DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiDynUpGoingUpDivider, m_iDynUpGoingUpDivider);
	DDV_MinMaxInt(pDX, m_iDynUpGoingUpDivider, 1, INT_MAX);

    DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiDynUpGoingDownDivider, m_iDynUpGoingDownDivider);
	DDV_MinMaxInt(pDX, m_iDynUpGoingDownDivider, 1, INT_MAX);

    DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiDynUpNumberOfPings, m_iDynUpNumberOfPings);
	DDV_MinMaxInt(pDX, m_iDynUpNumberOfPings, 1, INT_MAX);
	// ZZ:UploadSpeedSense <--
}

BOOL CPPgWebCache::OnInitDialog()
{
	m_iMaxConnPerFive = thePrefs.GetMaxConperFive();
	m_iAutoTakeEd2kLinks = HaveEd2kRegAccess() ? thePrefs.AutoTakeED2KLinks() : 0;
	if (thePrefs.GetEnableVerboseOptions())
	{
		m_iVerbose = thePrefs.m_bVerbose;
		m_iDebug2Disk = thePrefs.debug2disk;							// do *not* use the according 'Get...' function here!
		m_iDebugSourceExchange = thePrefs.m_bDebugSourceExchange;		// do *not* use the according 'Get...' function here!
		m_iLogBannedClients = thePrefs.m_bLogBannedClients;				// do *not* use the according 'Get...' function here!
		m_iLogRatingDescReceived = thePrefs.m_bLogRatingDescReceived;	// do *not* use the according 'Get...' function here!
		m_iLogSecureIdent = thePrefs.m_bLogSecureIdent;					// do *not* use the according 'Get...' function here!
		m_iLogFilteredIPs = thePrefs.m_bLogFilteredIPs;					// do *not* use the according 'Get...' function here!
		m_iLogFileSaving = thePrefs.m_bLogFileSaving;					// do *not* use the according 'Get...' function here!
	}
	m_iLog2Disk = thePrefs.log2disk;
	m_iCreditSystem = thePrefs.m_bCreditSystem;
	m_iCommitFiles = thePrefs.m_iCommitFiles;
	m_iFilterLANIPs = thePrefs.filterLANIPs;
	m_iExtControls = thePrefs.m_bExtControls;
	m_uServerKeepAliveTimeout = thePrefs.m_dwServerKeepAliveTimeout / 60000;
	m_iCheckDiskspace = thePrefs.checkDiskspace;	// SLUGFILLER: checkDiskspace
	m_fMinFreeDiskSpaceMB = (float)(thePrefs.m_uMinFreeDiskSpace / (1024.0 * 1024.0));
	m_sYourHostname = thePrefs.GetYourHostname();	// itsonlyme: hostnameSource

	// ZZ:UploadSpeedSense -->
    m_iDynUpEnabled = thePrefs.IsDynUpEnabled();
    m_iDynUpMinUpload = thePrefs.GetMinUpload();
    m_iDynUpPingTolerance = thePrefs.GetDynUpPingTolerance();
    m_iDynUpGoingUpDivider = thePrefs.GetDynUpGoingUpDivider();
    m_iDynUpGoingDownDivider = thePrefs.GetDynUpGoingDownDivider();
    m_iDynUpNumberOfPings = thePrefs.GetDynUpNumberOfPings();
	// ZZ:UploadSpeedSense <--

    CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	m_iFileBufferSize = thePrefs.m_iFileBufferSize;
	((CSliderCtrl*)GetDlgItem(IDC_FILEBUFFERSIZE))->SetRange(16, 1024+512, TRUE);
	((CSliderCtrl*)GetDlgItem(IDC_FILEBUFFERSIZE))->SetPos(m_iFileBufferSize/1024);

	m_iQueueSize = thePrefs.m_iQueueSize;
	((CSliderCtrl*)GetDlgItem(IDC_QUEUESIZE))->SetRange(20, 100, TRUE);
	((CSliderCtrl*)GetDlgItem(IDC_QUEUESIZE))->SetPos(m_iQueueSize/100);

	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgWebCache::OnKillActive()
{
	// if prop page is closed by pressing VK_ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();
	return CPropertyPage::OnKillActive();
}

BOOL CPPgWebCache::OnApply()
{
	// if prop page is closed by pressing VK_ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();

	if (!UpdateData())
		return FALSE;

	thePrefs.SetMaxConsPerFive(m_iMaxConnPerFive ? m_iMaxConnPerFive : DFLT_MAXCONPERFIVE);
	theApp.scheduler->original_cons5s = thePrefs.GetMaxConperFive();

	if (HaveEd2kRegAccess() && thePrefs.AutoTakeED2KLinks() != (bool)m_iAutoTakeEd2kLinks)
	{
		thePrefs.autotakeed2klinks = m_iAutoTakeEd2kLinks;
		if (thePrefs.AutoTakeED2KLinks())
			Ask4RegFix(false, true);
		else
			RevertReg();
	}

	if (!thePrefs.log2disk && m_iLog2Disk)
		theLog.Open();
	else if (thePrefs.log2disk && !m_iLog2Disk)
		theLog.Close();
	thePrefs.log2disk = m_iLog2Disk;

	if (thePrefs.GetEnableVerboseOptions())
	{
		if (!thePrefs.GetDebug2Disk() && m_iVerbose && m_iDebug2Disk)
			theVerboseLog.Open();
		else if (thePrefs.GetDebug2Disk() && (!m_iVerbose || !m_iDebug2Disk))
			theVerboseLog.Close();
		thePrefs.debug2disk = m_iDebug2Disk;

		thePrefs.m_bDebugSourceExchange = m_iDebugSourceExchange;
		thePrefs.m_bLogBannedClients = m_iLogBannedClients;
		thePrefs.m_bLogRatingDescReceived = m_iLogRatingDescReceived;
		thePrefs.m_bLogSecureIdent = m_iLogSecureIdent;
		thePrefs.m_bLogFilteredIPs = m_iLogFilteredIPs;
		thePrefs.m_bLogFileSaving = m_iLogFileSaving;

		thePrefs.m_bVerbose = m_iVerbose; // store after related options were stored!
	}

	thePrefs.m_bCreditSystem = m_iCreditSystem;
	thePrefs.m_iCommitFiles = m_iCommitFiles;
	thePrefs.filterLANIPs = m_iFilterLANIPs;
	thePrefs.m_iFileBufferSize = m_iFileBufferSize;
	thePrefs.m_iQueueSize = m_iQueueSize;
	if (thePrefs.m_bExtControls != (bool)m_iExtControls) {
		thePrefs.m_bExtControls = m_iExtControls;
		theApp.emuledlg->transferwnd->downloadlistctrl.CreateMenues();
		theApp.emuledlg->searchwnd->CreateMenus();
		theApp.emuledlg->sharedfileswnd->sharedfilesctrl.CreateMenues();
	}
	thePrefs.m_dwServerKeepAliveTimeout = m_uServerKeepAliveTimeout * 60000;
	thePrefs.checkDiskspace = m_iCheckDiskspace;	// SLUGFILLER: checkDiskspace
	thePrefs.m_uMinFreeDiskSpace = (UINT)(m_fMinFreeDiskSpaceMB * (1024 * 1024));
	thePrefs.SetYourHostname(m_sYourHostname);	// itsonlyme: hostnameSource

	// ZZ:UploadSpeedSense -->
	if( !thePrefs.m_bDynUpEnabled && m_iDynUpEnabled )
	{
		AfxMessageBox(GetResString(IDS_USS_MIN), MB_ICONERROR);
	}
	if( m_iDynUpMinUpload < 10 )
		m_iDynUpMinUpload = 10;
    thePrefs.m_bDynUpEnabled = m_iDynUpEnabled;
    thePrefs.minupload = m_iDynUpMinUpload;
    thePrefs.m_iDynUpPingTolerance = m_iDynUpPingTolerance;
    thePrefs.m_iDynUpGoingUpDivider = m_iDynUpGoingUpDivider;
    thePrefs.m_iDynUpGoingDownDivider = m_iDynUpGoingDownDivider;
    thePrefs.m_iDynUpNumberOfPings = m_iDynUpNumberOfPings;
	// ZZ:UploadSpeedSense <--

	if (thePrefs.GetEnableVerboseOptions())
	{
	    theApp.emuledlg->serverwnd->ToggleDebugWindow();
		theApp.emuledlg->serverwnd->UpdateLogTabSelection();
	}
	theApp.downloadqueue->CheckDiskspace();

	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CPPgWebCache::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	if (pScrollBar == GetDlgItem(IDC_FILEBUFFERSIZE))
	{
		m_iFileBufferSize = ((CSliderCtrl*)pScrollBar)->GetPos() * 1024;
        CString temp;
		temp.Format(_T("%s: %s"), GetResString(IDS_FILEBUFFERSIZE), CastItoXBytes(m_iFileBufferSize));
		GetDlgItem(IDC_FILEBUFFERSIZE_STATIC)->SetWindowText(temp);
		SetModified(TRUE);
	}
	else if (pScrollBar == GetDlgItem(IDC_QUEUESIZE))
	{
		m_iQueueSize = ((CSliderCtrl*)pScrollBar)->GetPos() * 100;
		CString temp;
		temp.Format(_T("%s: %s"), GetResString(IDS_QUEUESIZE), GetFormatedUInt(m_iQueueSize));
		GetDlgItem(IDC_QUEUESIZE_STATIC)->SetWindowText(temp);
		SetModified(TRUE);
	}
}

void CPPgWebCache::Localize(void)
{	
	if(m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_TWEAK));
		GetDlgItem(IDC_WARNING)->SetWindowText(GetResString(IDS_TWEAKS_WARNING));

		if (m_htiMaxCon5Sec) m_ctrlTreeOptions.SetEditLabel(m_htiMaxCon5Sec, GetResString(IDS_MAXCON5SECLABEL));
		if (m_htiAutoTakeEd2kLinks) m_ctrlTreeOptions.SetItemText(m_htiAutoTakeEd2kLinks, GetResString(IDS_AUTOTAKEED2KLINKS));
		if (m_htiVerboseGroup) m_ctrlTreeOptions.SetItemText(m_htiVerboseGroup, GetResString(IDS_VERBOSE));
		if (m_htiVerbose) m_ctrlTreeOptions.SetItemText(m_htiVerbose, GetResString(IDS_ENABLED));
		if (m_htiDebugSourceExchange) m_ctrlTreeOptions.SetItemText(m_htiDebugSourceExchange, GetResString(IDS_DEBUG_SOURCE_EXCHANGE));
		if (m_htiLogBannedClients) m_ctrlTreeOptions.SetItemText(m_htiLogBannedClients, GetResString(IDS_LOG_BANNED_CLIENTS));
		if (m_htiLogRatingDescReceived) m_ctrlTreeOptions.SetItemText(m_htiLogRatingDescReceived, GetResString(IDS_LOG_RATING_RECV));
		if (m_htiLogSecureIdent) m_ctrlTreeOptions.SetItemText(m_htiLogSecureIdent, GetResString(IDS_LOG_SECURE_IDENT));
		if (m_htiLogFilteredIPs) m_ctrlTreeOptions.SetItemText(m_htiLogFilteredIPs, GetResString(IDS_LOG_FILTERED_IPS));
		if (m_htiLogFileSaving) m_ctrlTreeOptions.SetItemText(m_htiLogFileSaving, GetResString(IDS_LOG_FILE_SAVING));
		if (m_htiCreditSystem) m_ctrlTreeOptions.SetItemText(m_htiCreditSystem, GetResString(IDS_USECREDITSYSTEM));

		if (m_htiLog2Disk) m_ctrlTreeOptions.SetItemText(m_htiLog2Disk, GetResString(IDS_LOG2DISK));
		if (m_htiDebug2Disk) m_ctrlTreeOptions.SetItemText(m_htiDebug2Disk, GetResString(IDS_LOG2DISK));

		if (m_htiCommit) m_ctrlTreeOptions.SetItemText(m_htiCommit, GetResString(IDS_COMMITFILES));
		if (m_htiCommitNever) m_ctrlTreeOptions.SetItemText(m_htiCommitNever, GetResString(IDS_NEVER));
		if (m_htiCommitOnShutdown) m_ctrlTreeOptions.SetItemText(m_htiCommitOnShutdown, GetResString(IDS_ONSHUTDOWN));
		if (m_htiCommitAlways) m_ctrlTreeOptions.SetItemText(m_htiCommitAlways, GetResString(IDS_ALWAYS));
		if (m_htiFilterLANIPs) m_ctrlTreeOptions.SetItemText(m_htiFilterLANIPs, GetResString(IDS_PW_FILTER));
		if (m_htiExtControls) m_ctrlTreeOptions.SetItemText(m_htiExtControls, GetResString(IDS_SHOWEXTSETTINGS));
		if (m_htiServerKeepAliveTimeout) m_ctrlTreeOptions.SetEditLabel(m_htiServerKeepAliveTimeout, GetResString(IDS_SERVERKEEPALIVETIMEOUT));
		if (m_htiCheckDiskspace) m_ctrlTreeOptions.SetItemText(m_htiCheckDiskspace, GetResString(IDS_CHECKDISKSPACE));	// SLUGFILLER: checkDiskspace
		if (m_htiMinFreeDiskSpace) m_ctrlTreeOptions.SetEditLabel(m_htiMinFreeDiskSpace, GetResString(IDS_MINFREEDISKSPACE));
		if (m_htiYourHostname) m_ctrlTreeOptions.SetEditLabel(m_htiYourHostname, GetResString(IDS_YOURHOSTNAME));	// itsonlyme: hostnameSource

		// ZZ:UploadSpeedSense -->
        if (m_htiDynUp) m_ctrlTreeOptions.SetItemText(m_htiDynUp, GetResString(IDS_DYNUP));
		if (m_htiDynUpEnabled) m_ctrlTreeOptions.SetItemText(m_htiDynUpEnabled, GetResString(IDS_DYNUPENABLED));
        if (m_htiDynUpMinUpload) m_ctrlTreeOptions.SetEditLabel(m_htiDynUpMinUpload, GetResString(IDS_DYNUP_MINUPLOAD));
        if (m_htiDynUpPingTolerance) m_ctrlTreeOptions.SetEditLabel(m_htiDynUpPingTolerance, GetResString(IDS_DYNUP_PINGTOLERANCE));
        if (m_htiDynUpGoingUpDivider) m_ctrlTreeOptions.SetEditLabel(m_htiDynUpGoingUpDivider, GetResString(IDS_DYNUP_GOINGUPDIVIDER));
        if (m_htiDynUpGoingDownDivider) m_ctrlTreeOptions.SetEditLabel(m_htiDynUpGoingDownDivider, GetResString(IDS_DYNUP_GOINGDOWNDIVIDER));
        if (m_htiDynUpNumberOfPings) m_ctrlTreeOptions.SetEditLabel(m_htiDynUpNumberOfPings, GetResString(IDS_DYNUP_NUMBEROFPINGS));
		// ZZ:UploadSpeedSense <--

        CString temp;
		temp.Format(_T("%s: %s"), GetResString(IDS_FILEBUFFERSIZE), CastItoXBytes(m_iFileBufferSize));
		GetDlgItem(IDC_FILEBUFFERSIZE_STATIC)->SetWindowText(temp);
		temp.Format(_T("%s: %s"), GetResString(IDS_QUEUESIZE), GetFormatedUInt(m_iQueueSize));
		GetDlgItem(IDC_QUEUESIZE_STATIC)->SetWindowText(temp);
	}
}

void CPPgWebCache::OnDestroy()
{
	m_ctrlTreeOptions.DeleteAllItems();
	m_ctrlTreeOptions.DestroyWindow();
	m_bInitializedTreeOpts = false;
	m_htiMaxCon5Sec = NULL;
	m_htiAutoTakeEd2kLinks = NULL;
	m_htiVerboseGroup = NULL;
	m_htiVerbose = NULL;
	m_htiDebugSourceExchange = NULL;
	m_htiLogBannedClients = NULL;
	m_htiLogRatingDescReceived = NULL;
	m_htiLogSecureIdent = NULL;
	m_htiLogFilteredIPs = NULL;
	m_htiLogFileSaving = NULL;
	m_htiCreditSystem = NULL;
	m_htiLog2Disk = NULL;
	m_htiDebug2Disk = NULL;
	m_htiCommit = NULL;
	m_htiCommitNever = NULL;
	m_htiCommitOnShutdown = NULL;
	m_htiCommitAlways = NULL;
	m_htiFilterLANIPs = NULL;
	m_htiExtControls = NULL;
	m_htiServerKeepAliveTimeout = NULL;
	m_htiCheckDiskspace = NULL;	// SLUGFILLER: checkDiskspace
	m_htiMinFreeDiskSpace = NULL;
	m_htiYourHostname = NULL;	// itsonlyme: hostnameSource

	// ZZ:UploadSpeedSense -->
    m_htiDynUp = NULL;
	m_htiDynUpEnabled = NULL;
    m_htiDynUpMinUpload = NULL;
    m_htiDynUpPingTolerance = NULL;
    m_htiDynUpGoingUpDivider = NULL;
    m_htiDynUpGoingDownDivider = NULL;
    m_htiDynUpNumberOfPings = NULL;
	// ZZ:UploadSpeedSense <--
    
    CPropertyPage::OnDestroy();
}

LRESULT CPPgWebCache::OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == IDC_EXT_OPTS)
	{
		TREEOPTSCTRLNOTIFY* pton = (TREEOPTSCTRLNOTIFY*)lParam;
		if (m_htiVerbose && pton->hItem == m_htiVerbose)
		{
			BOOL bCheck;
			if (m_ctrlTreeOptions.GetCheckBox(m_htiVerbose, bCheck))
			{
				if (m_htiDebugSourceExchange)	m_ctrlTreeOptions.SetCheckBoxEnable(m_htiDebugSourceExchange, bCheck);
				if (m_htiLogBannedClients)		m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogBannedClients, bCheck);
				if (m_htiLogRatingDescReceived) m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogRatingDescReceived, bCheck);
				if (m_htiLogSecureIdent)		m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogSecureIdent, bCheck);
				if (m_htiLogFilteredIPs)		m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogFilteredIPs, bCheck);
				if (m_htiLogFileSaving)			m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogFileSaving, bCheck);
				if (m_htiDebug2Disk)			m_ctrlTreeOptions.SetCheckBoxEnable(m_htiDebug2Disk, bCheck);
			}
		}
		SetModified();
	}
	return 0;
}

void CPPgWebCache::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_Extended_Settings);
}

BOOL CPPgWebCache::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgWebCache::OnHelpInfo(HELPINFO* pHelpInfo)
{
	OnHelp();
	return TRUE;
}
*/