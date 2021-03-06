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
#include "SearchDlg.h"
#include "PPgTweaks.h"
#include "Scheduler.h"
#include "DownloadQueue.h"
#include "Preferences.h"
#include "OtherFunctions.h"
#include "TransferWnd.h"
#include "emuledlg.h"
#include "SharedFilesWnd.h"
#include "ServerWnd.h"
#include "HelpIDs.h"
#include "Log.h"
#include "UserMsgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


#define	DFLT_MAXCONPERFIVE	20
#define DFLT_MAXHALFOPEN 9

///////////////////////////////////////////////////////////////////////////////
// CPPgTweaks dialog

IMPLEMENT_DYNAMIC(CPPgTweaks, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgTweaks, CPropertyPage)
	ON_WM_HSCROLL()	
	ON_WM_DESTROY()
	ON_MESSAGE(UM_TREEOPTSCTRL_NOTIFY, OnTreeOptsCtrlNotify)
	ON_WM_HELPINFO()
END_MESSAGE_MAP()

CPPgTweaks::CPPgTweaks()
	: CPropertyPage(CPPgTweaks::IDD)
	, m_ctrlTreeOptions(theApp.m_iDfltImageListColorFlags)
{
	m_iFileBufferSize = 0;
	m_iQueueSize = 0;
	m_iMaxConnPerFive = 0;
	m_iMaxHalfOpen = 0;
	m_iAutoTakeEd2kLinks = 0;
	m_iVerbose = 0;
	m_iDebugSourceExchange = 0;
	m_iLogBannedClients = 0;
	m_iLogRatingDescReceived = 0;
	m_iLogSecureIdent = 0;
	m_iLogFilteredIPs = 0;
	m_iLogFileSaving = 0;
	m_iLogA4AF = 0; 
	m_iLogUlDlEvents = 0;
// [TPT] - WebCache ////////////////////////////////////////////////////////////////////////////////////
	m_iLogWebCacheEvents = 0; //JP log webcache events
	m_iLogICHEvents = 0; //JP log ICH events
	m_iCreditSystem = 0;
	m_iLog2Disk = 0;
	m_iDebug2Disk = 0;
	m_iCommitFiles = 0;
	m_iFilterLANIPs = 0;
	m_iExtControls = 0;
	m_uServerKeepAliveTimeout = 0;
	m_iSparsePartFiles = 0;
	m_iCheckDiskspace = 0;
	m_fMinFreeDiskSpaceMB = 0.0F;
	(void)m_sYourHostname;
	m_iFirewallStartup = 0;
	m_iLogLevel = 0;
	m_iDisablePeerCache = 0;

	// [TPT] - Moved to PpgPhoenix1
	/*// ZZ:UploadSpeedSense -->
    m_iDynUpEnabled = 0;
    m_iDynUpMinUpload = 0.0f;
    m_iDynUpPingTolerance = 0;
    m_iDynUpGoingUpDivider = 0;
    m_iDynUpGoingDownDivider = 0;
    m_iDynUpNumberOfPings = 0;*/
    // [TPT] - Moved to PpgPhoenix1

	// ZZ:DownloadManager
    m_iA4AFSaveCpu = 0;
	m_iExtractMetaData = 0;

	m_bInitializedTreeOpts = false;
	m_htiMaxCon5Sec = NULL;
	m_htiMaxHalfOpen = NULL;
	m_htiAutoTakeEd2kLinks = NULL;
	m_htiVerboseGroup = NULL;
	m_htiVerbose = NULL;
	m_htiDebugSourceExchange = NULL;
	m_htiLogBannedClients = NULL;
	m_htiLogRatingDescReceived = NULL;
	m_htiLogSecureIdent = NULL;
	m_htiLogFilteredIPs = NULL;
	m_htiLogFileSaving = NULL;
	m_htiLogUlDlEvents = NULL;
// [TPT] - WebCache ////////////////////////////////////////////////////////////////////////////////////
	m_htiLogWebCacheEvents = NULL; //jp log webcache events
	m_htiLogICHEvents = NULL; //JP log ICH events
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
	m_htiSparsePartFiles = NULL;
	m_htiCheckDiskspace = NULL;	// SLUGFILLER: checkDiskspace
	m_htiMinFreeDiskSpace = NULL;
	m_htiYourHostname = NULL;	// itsonlyme: hostnameSource
	m_htiFirewallStartup = NULL;
	m_htiLogLevel = NULL;
	m_htiDisablePeerCache = NULL;
	
	// [TPT] - Moved to PpgPhoenix1
	/*// ZZ:UploadSpeedSense -->
    m_htiDynUp = NULL;
	m_htiDynUpEnabled = NULL;
    m_htiDynUpMinUpload = NULL;
    m_htiDynUpPingTolerance = NULL;
    m_htiDynUpPingToleranceMilliseconds = NULL;
    m_htiDynUpPingToleranceGroup = NULL;
    m_htiDynUpRadioPingTolerance = NULL;
    m_htiDynUpRadioPingToleranceMilliseconds = NULL;
    m_htiDynUpGoingUpDivider = NULL;
    m_htiDynUpGoingDownDivider = NULL;
    m_htiDynUpNumberOfPings = NULL;*/
    // [TPT] - Moved to PpgPhoenix1

    // ZZ:DownloadManager
    m_htiA4AFSaveCpu = NULL;
	m_htiLogA4AF = NULL;
	m_htiExtractMetaData = NULL;

	// emulEspaņa. Added by MoNKi [MoNKi: -UPnPNAT Support-]
	m_iLogUPnP = 0;
	m_htiLogUPnP = NULL;
	// End emulEspaņa
}

CPPgTweaks::~CPPgTweaks()
{
}

void CPPgTweaks::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILEBUFFERSIZE, m_ctlFileBuffSize);
	DDX_Control(pDX, IDC_QUEUESIZE, m_ctlQueueSize);
	DDX_Control(pDX, IDC_EXT_OPTS, m_ctrlTreeOptions);
	if (!m_bInitializedTreeOpts)
	{
		int iImgBackup = 8; // default icon
		int iImgLog = 8; // default icon
		// [TPT] - Moved to PpgPhoenix1
		/*// ZZ:UploadSpeedSense -->
		int iImgDynyp = 8; // default icon
		// ZZ:UploadSpeedSense <--*/
		// [TPT] - Moved to PpgPhoenix1
		int iImgA4AF = 8; // default icon // ZZ:DownloadManager
		int iImgMetaData = 8; // default icon
        CImageList* piml = m_ctrlTreeOptions.GetImageList(TVSIL_NORMAL);
		if (piml){
			iImgBackup = piml->Add(CTempIconLoader(_T("Harddisk")));
			iImgLog = piml->Add(CTempIconLoader(_T("Log")));
			// [TPT] - Moved to PpgPhoenix1
            /*// ZZ:UploadSpeedSense -->
			iImgDynyp = piml->Add(CTempIconLoader(_T("upload")));
			// ZZ:UploadSpeedSense <--*/
			// [TPT] - Moved to PpgPhoenix1
            iImgA4AF =  piml->Add(CTempIconLoader(_T("Download"))); // ZZ:DownloadManager 
            iImgMetaData =  piml->Add(CTempIconLoader(_T("MediaInfo"))); // ZZ:DownloadManager 
		}

		m_htiMaxCon5Sec = m_ctrlTreeOptions.InsertItem(GetResString(IDS_MAXCON5SECLABEL), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, TVI_ROOT);
		m_ctrlTreeOptions.AddEditBox(m_htiMaxCon5Sec, RUNTIME_CLASS(CNumTreeOptionsEdit));
		m_htiMaxHalfOpen = m_ctrlTreeOptions.InsertItem(GetResString(IDS_MAXHALFOPENCONS), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, TVI_ROOT);
		m_ctrlTreeOptions.AddEditBox(m_htiMaxHalfOpen, RUNTIME_CLASS(CNumTreeOptionsEdit));
		m_htiAutoTakeEd2kLinks = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_AUTOTAKEED2KLINKS), TVI_ROOT, m_iAutoTakeEd2kLinks);
		m_htiFirewallStartup = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_FO_PREF_STARTUP), TVI_ROOT, m_iFirewallStartup);

		m_htiCreditSystem = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_USECREDITSYSTEM), TVI_ROOT, m_iCreditSystem);
		m_htiFilterLANIPs = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_PW_FILTER), TVI_ROOT, m_iFilterLANIPs);
		m_htiExtControls = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_SHOWEXTSETTINGS), TVI_ROOT, m_iExtControls);
        m_htiA4AFSaveCpu = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_A4AF_SAVE_CPU), TVI_ROOT, m_iA4AFSaveCpu); // ZZ:DownloadManager

		m_htiSparsePartFiles = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_SPARSEPARTFILES), TVI_ROOT, m_iSparsePartFiles);
		m_htiCheckDiskspace = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_CHECKDISKSPACE), TVI_ROOT, m_iCheckDiskspace);	// SLUGFILLER: checkDiskspace
		m_htiMinFreeDiskSpace = m_ctrlTreeOptions.InsertItem(GetResString(IDS_MINFREEDISKSPACE), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiCheckDiskspace);
		m_ctrlTreeOptions.AddEditBox(m_htiMinFreeDiskSpace, RUNTIME_CLASS(CNumTreeOptionsEdit));
		m_htiYourHostname = m_ctrlTreeOptions.InsertItem(GetResString(IDS_YOURHOSTNAME), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, TVI_ROOT);
		m_ctrlTreeOptions.AddEditBox(m_htiYourHostname, RUNTIME_CLASS(CTreeOptionsEditEx));
		m_htiDisablePeerCache = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_DISABLEPEERACHE), TVI_ROOT, m_iDisablePeerCache);

		m_htiLog2Disk = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_LOG2DISK), TVI_ROOT, m_iLog2Disk);
		if (thePrefs.GetEnableVerboseOptions())
		{
			m_htiVerboseGroup = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_VERBOSE), iImgLog, TVI_ROOT);
			m_htiVerbose = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_ENABLED), m_htiVerboseGroup, m_iVerbose);
			m_htiLogLevel = m_ctrlTreeOptions.InsertItem(GetResString(IDS_LOG_LEVEL), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiVerboseGroup);
			m_ctrlTreeOptions.AddEditBox(m_htiLogLevel, RUNTIME_CLASS(CNumTreeOptionsEdit));
			m_htiDebug2Disk = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_LOG2DISK), m_htiVerboseGroup, m_iDebug2Disk);
			m_htiDebugSourceExchange = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_DEBUG_SOURCE_EXCHANGE), m_htiVerboseGroup, m_iDebugSourceExchange);
			m_htiLogBannedClients = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_LOG_BANNED_CLIENTS), m_htiVerboseGroup, m_iLogBannedClients);
			m_htiLogRatingDescReceived = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_LOG_RATING_RECV), m_htiVerboseGroup, m_iLogRatingDescReceived);
			m_htiLogSecureIdent = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_LOG_SECURE_IDENT), m_htiVerboseGroup, m_iLogSecureIdent);
			m_htiLogFilteredIPs = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_LOG_FILTERED_IPS), m_htiVerboseGroup, m_iLogFilteredIPs);
			m_htiLogFileSaving = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_LOG_FILE_SAVING), m_htiVerboseGroup, m_iLogFileSaving);
			m_htiLogA4AF = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_LOG_A4AF), m_htiVerboseGroup, m_iLogA4AF); // ZZ:DownloadManager
			m_htiLogUlDlEvents = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_LOG_ULDL_EVENTS), m_htiVerboseGroup, m_iLogUlDlEvents);
// [TPT] - WebCache ////////////////////////////////////////////////////////////////////////////////////
			m_htiLogWebCacheEvents = m_ctrlTreeOptions.InsertCheckBox(_T("Log WebCache Events:"), m_htiVerboseGroup, m_iLogWebCacheEvents); //JP log webcache events
			m_htiLogICHEvents = m_ctrlTreeOptions.InsertCheckBox(_T("Log IACH Events:"), m_htiVerboseGroup, m_iLogICHEvents); //JP log ICH events
			// emulEspaņa. Added by MoNKi [MoNKi: -UPnPNAT Support-]
			m_htiLogUPnP = m_ctrlTreeOptions.InsertCheckBox(_T("Log UPnP"), m_htiVerboseGroup, m_iLogUPnP);
			// End emulEspaņa
		}

		m_htiCommit = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_COMMITFILES), iImgBackup, TVI_ROOT);
		m_htiCommitNever = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_NEVER), m_htiCommit, m_iCommitFiles == 0);
		m_htiCommitOnShutdown = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_ONSHUTDOWN), m_htiCommit, m_iCommitFiles == 1);
		m_htiCommitAlways = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_ALWAYS), m_htiCommit, m_iCommitFiles == 2);

		m_htiServerKeepAliveTimeout = m_ctrlTreeOptions.InsertItem(GetResString(IDS_SERVERKEEPALIVETIMEOUT), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, TVI_ROOT);
		m_ctrlTreeOptions.AddEditBox(m_htiServerKeepAliveTimeout, RUNTIME_CLASS(CNumTreeOptionsEdit));

		// [TPT] - Moved to PpgPhoenix1
		/*
		// ZZ:UploadSpeedSense -->
        m_htiDynUp = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_DYNUP), iImgDynyp, TVI_ROOT);
		m_htiDynUpEnabled = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_DYNUPENABLED), m_htiDynUp, m_iDynUpEnabled);

        m_htiDynUpMinUpload = m_ctrlTreeOptions.InsertItem(GetResString(IDS_DYNUP_MINUPLOAD), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUp);
		m_ctrlTreeOptions.AddEditBox(m_htiDynUpMinUpload, RUNTIME_CLASS(CNumTreeOptionsEdit));

        m_htiDynUpPingTolerance = m_ctrlTreeOptions.InsertItem(GetResString(IDS_DYNUP_PINGTOLERANCE), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUp);
		m_ctrlTreeOptions.AddEditBox(m_htiDynUpPingTolerance, RUNTIME_CLASS(CNumTreeOptionsEdit));

        m_htiDynUpPingToleranceMilliseconds = m_ctrlTreeOptions.InsertItem(GetResString(IDS_DYNUP_PINGTOLERANCE_MS), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUp);
        m_ctrlTreeOptions.AddEditBox(m_htiDynUpPingToleranceMilliseconds, RUNTIME_CLASS(CNumTreeOptionsEdit));

        m_htiDynUpPingToleranceGroup = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_DYNUP_RADIO_PINGTOLERANCE_HEADER), iImgDynyp, m_htiDynUp);
        m_htiDynUpRadioPingTolerance = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_DYNUP_RADIO_PINGTOLERANCE_PERCENT), m_htiDynUpPingToleranceGroup, m_iDynUpRadioPingTolerance == 0);
        m_htiDynUpRadioPingToleranceMilliseconds = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_DYNUP_RADIO_PINGTOLERANCE_MS), m_htiDynUpPingToleranceGroup, m_iDynUpRadioPingTolerance == 1);

        m_htiDynUpGoingUpDivider = m_ctrlTreeOptions.InsertItem(GetResString(IDS_DYNUP_GOINGUPDIVIDER), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUp);
		m_ctrlTreeOptions.AddEditBox(m_htiDynUpGoingUpDivider, RUNTIME_CLASS(CNumTreeOptionsEdit));

        m_htiDynUpGoingDownDivider = m_ctrlTreeOptions.InsertItem(GetResString(IDS_DYNUP_GOINGDOWNDIVIDER), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUp);
		m_ctrlTreeOptions.AddEditBox(m_htiDynUpGoingDownDivider, RUNTIME_CLASS(CNumTreeOptionsEdit));

        m_htiDynUpNumberOfPings = m_ctrlTreeOptions.InsertItem(GetResString(IDS_DYNUP_NUMBEROFPINGS), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUp);
		m_ctrlTreeOptions.AddEditBox(m_htiDynUpNumberOfPings, RUNTIME_CLASS(CNumTreeOptionsEdit));
		// ZZ:UploadSpeedSense <--*/
		// [TPT] - Moved to PpgPhoenix1
        	m_htiExtractMetaData = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_EXTRACT_META_DATA), iImgMetaData, TVI_ROOT);
		m_htiExtractMetaDataNever = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_NEVER), m_htiExtractMetaData, m_iExtractMetaData == 0);
		m_htiExtractMetaDataID3Lib = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_META_DATA_ID3LIB), m_htiExtractMetaData, m_iExtractMetaData == 1);
		m_htiExtractMetaDataMediaDet = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_META_DATA_MEDIADET), m_htiExtractMetaData, m_iExtractMetaData == 2);

        if (m_htiVerboseGroup)
		    m_ctrlTreeOptions.Expand(m_htiVerboseGroup, TVE_EXPAND);
		m_ctrlTreeOptions.Expand(m_htiCommit, TVE_EXPAND);
		m_ctrlTreeOptions.Expand(m_htiCheckDiskspace, TVE_EXPAND);

		// [TPT] - Moved to PpgPhoenix1
		/*
		// ZZ:UploadSpeedSense -->
		m_ctrlTreeOptions.Expand(m_htiDynUp, TVE_EXPAND);
        m_ctrlTreeOptions.Expand(m_htiDynUpPingToleranceGroup, TVE_EXPAND);
		// ZZ:UploadSpeedSense <--*/
		// [TPT] - Moved to PpgPhoenix1
		m_ctrlTreeOptions.Expand(m_htiExtractMetaData, TVE_EXPAND);
        m_ctrlTreeOptions.SendMessage(WM_VSCROLL, SB_TOP);
        m_bInitializedTreeOpts = true;
	}

	DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiMaxCon5Sec, m_iMaxConnPerFive);
	DDV_MinMaxInt(pDX, m_iMaxConnPerFive, 1, INT_MAX);
	DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiMaxHalfOpen, m_iMaxHalfOpen);
	DDV_MinMaxInt(pDX, m_iMaxHalfOpen, 1, INT_MAX);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiAutoTakeEd2kLinks, m_iAutoTakeEd2kLinks);
	if (m_htiVerbose)				DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiVerbose, m_iVerbose);
	if (m_htiDebug2Disk)			DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiDebug2Disk, m_iDebug2Disk);
	if (m_htiDebugSourceExchange)	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiDebugSourceExchange, m_iDebugSourceExchange);
	if (m_htiLogBannedClients)		DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLogBannedClients, m_iLogBannedClients);
	if (m_htiLogRatingDescReceived) DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLogRatingDescReceived, m_iLogRatingDescReceived);
	if (m_htiLogSecureIdent)		DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLogSecureIdent, m_iLogSecureIdent);
	if (m_htiLogFilteredIPs)		DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLogFilteredIPs, m_iLogFilteredIPs);
	if (m_htiLogFileSaving)			DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLogFileSaving, m_iLogFileSaving);
    if (m_htiLogA4AF)			    DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLogA4AF, m_iLogA4AF); // ZZ:DownloadManager
	if (m_htiLogUlDlEvents)			DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLogUlDlEvents, m_iLogUlDlEvents);
// [TPT] - WebCache ////////////////////////////////////////////////////////////////////////////////////
	if (m_htiLogWebCacheEvents)		DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLogWebCacheEvents, m_iLogWebCacheEvents);//jp log webcache events
	if (m_htiLogICHEvents)			DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLogICHEvents, m_iLogICHEvents);//JP log ICH events
	// emulEspaņa. Added by MoNKi [MoNKi: -UPnPNAT Support-]
	if (m_htiLogUPnP)				DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLogUPnP, m_iLogUPnP);
	// End emulEspaņa
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiCreditSystem, m_iCreditSystem);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLog2Disk, m_iLog2Disk);
	DDX_TreeRadio(pDX, IDC_EXT_OPTS, m_htiCommit, m_iCommitFiles);
	DDX_TreeRadio(pDX, IDC_EXT_OPTS, m_htiExtractMetaData, m_iExtractMetaData);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiFilterLANIPs, m_iFilterLANIPs);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiExtControls, m_iExtControls);
	DDX_Text(pDX, IDC_EXT_OPTS, m_htiServerKeepAliveTimeout, m_uServerKeepAliveTimeout);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiSparsePartFiles, m_iSparsePartFiles);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiCheckDiskspace, m_iCheckDiskspace);	// SLUGFILLER: checkDiskspace
	DDX_Text(pDX, IDC_EXT_OPTS, m_htiMinFreeDiskSpace, m_fMinFreeDiskSpaceMB);
	DDV_MinMaxFloat(pDX, m_fMinFreeDiskSpaceMB, 0.0, UINT_MAX / (1024*1024));
	DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiYourHostname, m_sYourHostname);	// itsonlyme: hostnameSource
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiFirewallStartup, m_iFirewallStartup);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiDisablePeerCache, m_iDisablePeerCache);

	if (m_htiDebug2Disk)			m_ctrlTreeOptions.SetCheckBoxEnable(m_htiDebug2Disk, m_iVerbose);
	if (m_htiDebugSourceExchange)	m_ctrlTreeOptions.SetCheckBoxEnable(m_htiDebugSourceExchange, m_iVerbose);
	if (m_htiLogBannedClients)		m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogBannedClients, m_iVerbose);
	if (m_htiLogRatingDescReceived) m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogRatingDescReceived, m_iVerbose);
	if (m_htiLogSecureIdent)		m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogSecureIdent, m_iVerbose);
	if (m_htiLogFilteredIPs)		m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogFilteredIPs, m_iVerbose);
	if (m_htiLogFileSaving)			m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogFileSaving, m_iVerbose);
	if (m_htiLogA4AF)               m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogA4AF, m_iVerbose); // ZZ:DownloadManager
	if (m_htiLogUlDlEvents)         m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogUlDlEvents, m_iVerbose);
// [TPT] - WebCache ////////////////////////////////////////////////////////////////////////////////////
	if (m_htiLogWebCacheEvents)     m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogWebCacheEvents, m_iVerbose);//jp log webcache events
	if (m_htiLogICHEvents)			m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogICHEvents, m_iVerbose);//JP log ICH events
	if (m_htiLogLevel){
		DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiLogLevel, m_iLogLevel);
		DDV_MinMaxInt(pDX, m_iLogLevel, 1, 5);
	}	
	m_ctrlTreeOptions.SetCheckBoxEnable(m_htiAutoTakeEd2kLinks, HaveEd2kRegAccess());
	m_ctrlTreeOptions.SetCheckBoxEnable(m_htiFirewallStartup, thePrefs.GetWindowsVersion() == _WINVER_XP_);


	// [TPT] - Moved to PpgPhoenix1
	/*// ZZ:UploadSpeedSense -->
    DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiDynUpEnabled, m_iDynUpEnabled);

    DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiDynUpMinUpload, m_iDynUpMinUpload);
	DDV_MinMaxInt(pDX, m_iDynUpMinUpload, 1, INT_MAX);

    DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiDynUpPingTolerance, m_iDynUpPingTolerance);
	DDV_MinMaxInt(pDX, m_iDynUpPingTolerance, 100, INT_MAX);

    DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiDynUpPingToleranceMilliseconds, m_iDynUpPingToleranceMilliseconds);
	DDV_MinMaxInt(pDX, m_iDynUpPingTolerance, 1, INT_MAX);

    DDX_TreeRadio(pDX, IDC_EXT_OPTS, m_htiDynUpPingToleranceGroup, m_iDynUpRadioPingTolerance);

    DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiDynUpGoingUpDivider, m_iDynUpGoingUpDivider);
	DDV_MinMaxInt(pDX, m_iDynUpGoingUpDivider, 1, INT_MAX);

    DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiDynUpGoingDownDivider, m_iDynUpGoingDownDivider);
	DDV_MinMaxInt(pDX, m_iDynUpGoingDownDivider, 1, INT_MAX);

    DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiDynUpNumberOfPings, m_iDynUpNumberOfPings);
	DDV_MinMaxInt(pDX, m_iDynUpNumberOfPings, 1, INT_MAX);
	// ZZ:UploadSpeedSense <--*/
	// [TPT] - Moved to PpgPhoenix1

    DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiA4AFSaveCpu, m_iA4AFSaveCpu); // ZZ:DownloadManager
}

BOOL CPPgTweaks::OnInitDialog()
{
	m_iMaxConnPerFive = thePrefs.GetMaxConperFive();
	m_iMaxHalfOpen = thePrefs.GetMaxHalfConnections();
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
        m_iLogA4AF = thePrefs.m_bLogA4AF;                   		    // do *not* use the according 'Get...' function here! // ZZ:DownloadManager
		m_iLogUlDlEvents = thePrefs.m_bLogUlDlEvents;
// [TPT] - WebCache ////////////////////////////////////////////////////////////////////////////////////
		m_iLogWebCacheEvents = thePrefs.m_bLogWebCacheEvents;//JP log webcache events
		m_iLogICHEvents = thePrefs.m_bLogICHEvents;//JP log ICH events
		// emulEspaņa. Added by MoNKi [MoNKi: -UPnPNAT Support-]
		m_iLogUPnP = thePrefs.GetUPnPVerboseLog();
		// End emulEspaņa

		m_iLogLevel = 5 - thePrefs.m_byLogLevel;
	}
	m_iLog2Disk = thePrefs.log2disk;
	m_iCreditSystem = thePrefs.m_bCreditSystem;
	m_iCommitFiles = thePrefs.m_iCommitFiles;
	m_iExtractMetaData = thePrefs.m_iExtractMetaData;
	m_iFilterLANIPs = thePrefs.filterLANIPs;
	m_iExtControls = thePrefs.m_bExtControls;
	m_uServerKeepAliveTimeout = thePrefs.m_dwServerKeepAliveTimeout / 60000;
	m_iSparsePartFiles = thePrefs.m_bSparsePartFiles;
	m_iCheckDiskspace = thePrefs.checkDiskspace;	// SLUGFILLER: checkDiskspace
	m_fMinFreeDiskSpaceMB = (float)(thePrefs.m_uMinFreeDiskSpace / (1024.0 * 1024.0));
	m_sYourHostname = thePrefs.GetYourHostname();	// itsonlyme: hostnameSource
	m_iFirewallStartup = ((thePrefs.GetWindowsVersion() == _WINVER_XP_) ? thePrefs.m_bOpenPortsOnStartUp : 0); 
	m_iDisablePeerCache = !thePrefs.m_bPeerCacheEnabled;

	// [TPT] - Moved to PpgPhoenix1
	/*
	// ZZ:UploadSpeedSense -->
    m_iDynUpEnabled = thePrefs.IsDynUpEnabled();
    m_iDynUpMinUpload = thePrefs.GetMinUpload();
    m_iDynUpPingTolerance = thePrefs.GetDynUpPingTolerance();
    m_iDynUpPingToleranceMilliseconds = thePrefs.GetDynUpPingToleranceMilliseconds();
    m_iDynUpRadioPingTolerance = thePrefs.IsDynUpUseMillisecondPingTolerance()?1:0;
    m_iDynUpGoingUpDivider = thePrefs.GetDynUpGoingUpDivider();
    m_iDynUpGoingDownDivider = thePrefs.GetDynUpGoingDownDivider();
    m_iDynUpNumberOfPings = thePrefs.GetDynUpNumberOfPings();
	// ZZ:UploadSpeedSense <--*/
	// [TPT] - Moved to PpgPhoenix1

    m_iA4AFSaveCpu = thePrefs.GetA4AFSaveCpu(); // ZZ:DownloadManager

    CPropertyPage::OnInitDialog();
	InitWindowStyles(this);
	m_ctrlTreeOptions.SetItemHeight(m_ctrlTreeOptions.GetItemHeight() + 2);

	m_iFileBufferSize = thePrefs.m_iFileBufferSize;
	m_ctlFileBuffSize.SetRange(16, 1024+512, TRUE);
	int iMin, iMax;
	m_ctlFileBuffSize.GetRange(iMin, iMax);
	m_ctlFileBuffSize.SetPos(m_iFileBufferSize/1024);
	int iPage = 128;
	for (int i = ((iMin+iPage-1)/iPage)*iPage; i < iMax; i += iPage)
		m_ctlFileBuffSize.SetTic(i);
	m_ctlFileBuffSize.SetPageSize(iPage);

	// [TPT] - SLUGFILLER: infiniteQueue
	if (!thePrefs.infiniteQueue)
		m_iQueueSize = thePrefs.m_iQueueSize;
	else
		m_iQueueSize = 101*100;
	m_ctlQueueSize.SetRange(20, 101, TRUE);
	// [TPT] - SLUGFILLER: infiniteQueue
	m_ctlQueueSize.SetPos(m_iQueueSize/100);
	m_ctlQueueSize.SetTicFreq(10);
	m_ctlQueueSize.SetPageSize(10);

	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgTweaks::OnKillActive()
{
	// if prop page is closed by pressing ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();
	return CPropertyPage::OnKillActive();
}

BOOL CPPgTweaks::OnApply()
{
	// if prop page is closed by pressing ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();

	if (!UpdateData())
		return FALSE;

	thePrefs.SetMaxConsPerFive(m_iMaxConnPerFive ? m_iMaxConnPerFive : DFLT_MAXCONPERFIVE);
	theApp.scheduler->original_cons5s = thePrefs.GetMaxConperFive();
	thePrefs.SetMaxHalfConnections(m_iMaxHalfOpen ? m_iMaxHalfOpen : DFLT_MAXHALFOPEN);

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
        thePrefs.m_bLogA4AF = m_iLogA4AF; // ZZ:DownloadManager
		thePrefs.m_bLogUlDlEvents = m_iLogUlDlEvents;
// [TPT] - WebCache ////////////////////////////////////////////////////////////////////////////////////
		thePrefs.m_bLogWebCacheEvents = m_iLogWebCacheEvents;//JP log webcache events
		thePrefs.m_bLogICHEvents = m_iLogICHEvents;//JP log ICH events
		// emulEspaņa. Added by MoNKi [MoNKi: -UPnPNAT Support-]
		thePrefs.SetUPnPVerboseLog((bool)m_iLogUPnP);
		// End emulEspaņa

		thePrefs.m_byLogLevel = 5 - m_iLogLevel;

		thePrefs.m_bVerbose = m_iVerbose; // store after related options were stored!
	}

	thePrefs.m_bCreditSystem = m_iCreditSystem;
	thePrefs.m_iCommitFiles = m_iCommitFiles;
	thePrefs.m_iExtractMetaData = m_iExtractMetaData;
	thePrefs.filterLANIPs = m_iFilterLANIPs;
	thePrefs.m_iFileBufferSize = m_iFileBufferSize;
	// [TPT] - SLUGFILLER: infiniteQueue
	if (m_iQueueSize <= 100*100) {
		thePrefs.m_iQueueSize = m_iQueueSize;
		thePrefs.infiniteQueue = false;
	}
	else
		thePrefs.infiniteQueue = true;
	// [TPT] - SLUGFILLER: infiniteQueue
	if (thePrefs.m_bExtControls != (bool)m_iExtControls) {
		thePrefs.m_bExtControls = m_iExtControls;
		// [TPT] - New Menu Style
		//theApp.emuledlg->transferwnd->downloadlistctrl.CreateMenues();
		//theApp.emuledlg->searchwnd->CreateMenus();
		//theApp.emuledlg->sharedfileswnd->sharedfilesctrl.CreateMenues();
		// [TPT] - New Menu Style
	}
	thePrefs.m_dwServerKeepAliveTimeout = m_uServerKeepAliveTimeout * 60000;
	thePrefs.m_bSparsePartFiles = m_iSparsePartFiles;
	thePrefs.checkDiskspace = m_iCheckDiskspace;	// SLUGFILLER: checkDiskspace
	thePrefs.m_uMinFreeDiskSpace = (UINT)(m_fMinFreeDiskSpaceMB * (1024 * 1024));
	if (thePrefs.GetYourHostname() != m_sYourHostname) {
		thePrefs.SetYourHostname(m_sYourHostname);
		theApp.emuledlg->serverwnd->UpdateMyInfo();
	}
	thePrefs.m_bOpenPortsOnStartUp = m_iFirewallStartup; 
	thePrefs.m_bPeerCacheEnabled = !m_iDisablePeerCache;

	// [TPT] - Moved to PpgPhoenix1
	/*
	// ZZ:UploadSpeedSense -->	
    thePrefs.m_bDynUpEnabled = m_iDynUpEnabled;
    thePrefs.minupload = m_iDynUpMinUpload;
    thePrefs.m_iDynUpPingTolerance = m_iDynUpPingTolerance;
    thePrefs.m_iDynUpPingToleranceMilliseconds = m_iDynUpPingToleranceMilliseconds;
    thePrefs.m_bDynUpUseMillisecondPingTolerance = (m_iDynUpRadioPingTolerance == 1);
    thePrefs.m_iDynUpGoingUpDivider = m_iDynUpGoingUpDivider;
    thePrefs.m_iDynUpGoingDownDivider = m_iDynUpGoingDownDivider;
    thePrefs.m_iDynUpNumberOfPings = m_iDynUpNumberOfPings;
	// ZZ:UploadSpeedSense <--*/
	// [TPT] - Moved to PpgPhoenix1

    thePrefs.m_bA4AFSaveCpu = m_iA4AFSaveCpu; // ZZ:DownloadManager

	if (thePrefs.GetEnableVerboseOptions())
	{
	    theApp.emuledlg->serverwnd->ToggleDebugWindow();
		theApp.emuledlg->serverwnd->UpdateLogTabSelection();
	}
	theApp.downloadqueue->CheckDiskspace();

	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CPPgTweaks::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	if (pScrollBar->GetSafeHwnd() == m_ctlFileBuffSize.m_hWnd)
	{
		m_iFileBufferSize = m_ctlFileBuffSize.GetPos() * 1024;
        CString temp;
		temp.Format(_T("%s: %s"), GetResString(IDS_FILEBUFFERSIZE), CastItoXBytes(m_iFileBufferSize, false, false));
		GetDlgItem(IDC_FILEBUFFERSIZE_STATIC)->SetWindowText(temp);
		SetModified(TRUE);
	}
	else if (pScrollBar->GetSafeHwnd() == m_ctlQueueSize.m_hWnd)
	{
		m_iQueueSize = ((CSliderCtrl*)pScrollBar)->GetPos() * 100;
		CString temp;
		// [TPT] - SLUGFILLER: infiniteQueue
		if (m_iQueueSize <= 100*100)
 			temp.Format(_T("%s: %s"), GetResString(IDS_QUEUESIZE), GetFormatedUInt(m_iQueueSize));
		else
			temp = GetResString(IDS_INFINITEQUEUE);
		// [TPT] - SLUGFILLER: infiniteQueue
		GetDlgItem(IDC_QUEUESIZE_STATIC)->SetWindowText(temp);
		SetModified(TRUE);
	}
}

void CPPgTweaks::Localize(void)
{	
	if(m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_TWEAK));
		GetDlgItem(IDC_WARNING)->SetWindowText(GetResString(IDS_TWEAKS_WARNING));

		if (m_htiMaxCon5Sec) m_ctrlTreeOptions.SetEditLabel(m_htiMaxCon5Sec, GetResString(IDS_MAXCON5SECLABEL));
		if (m_htiMaxHalfOpen) m_ctrlTreeOptions.SetEditLabel(m_htiMaxHalfOpen, GetResString(IDS_MAXHALFOPENCONS));
		if (m_htiAutoTakeEd2kLinks) m_ctrlTreeOptions.SetItemText(m_htiAutoTakeEd2kLinks, GetResString(IDS_AUTOTAKEED2KLINKS));
		if (m_htiCreditSystem) m_ctrlTreeOptions.SetItemText(m_htiCreditSystem, GetResString(IDS_USECREDITSYSTEM));
		if (m_htiLog2Disk) m_ctrlTreeOptions.SetItemText(m_htiLog2Disk, GetResString(IDS_LOG2DISK));

		if (m_htiVerboseGroup) m_ctrlTreeOptions.SetItemText(m_htiVerboseGroup, GetResString(IDS_VERBOSE));
		if (m_htiVerbose) m_ctrlTreeOptions.SetItemText(m_htiVerbose, GetResString(IDS_ENABLED));
		if (m_htiDebug2Disk) m_ctrlTreeOptions.SetItemText(m_htiDebug2Disk, GetResString(IDS_LOG2DISK));
		if (m_htiDebugSourceExchange) m_ctrlTreeOptions.SetItemText(m_htiDebugSourceExchange, GetResString(IDS_DEBUG_SOURCE_EXCHANGE));
		if (m_htiLogBannedClients) m_ctrlTreeOptions.SetItemText(m_htiLogBannedClients, GetResString(IDS_LOG_BANNED_CLIENTS));
		if (m_htiLogRatingDescReceived) m_ctrlTreeOptions.SetItemText(m_htiLogRatingDescReceived, GetResString(IDS_LOG_RATING_RECV));
		if (m_htiLogSecureIdent) m_ctrlTreeOptions.SetItemText(m_htiLogSecureIdent, GetResString(IDS_LOG_SECURE_IDENT));
		if (m_htiLogFilteredIPs) m_ctrlTreeOptions.SetItemText(m_htiLogFilteredIPs, GetResString(IDS_LOG_FILTERED_IPS));
		if (m_htiLogFileSaving) m_ctrlTreeOptions.SetItemText(m_htiLogFileSaving, GetResString(IDS_LOG_FILE_SAVING));
		if (m_htiLogLevel) m_ctrlTreeOptions.SetEditLabel(m_htiLogLevel, GetResString(IDS_LOG_LEVEL));
		if (m_htiLogA4AF) m_ctrlTreeOptions.SetItemText(m_htiLogA4AF, GetResString(IDS_LOG_A4AF));
		if (m_htiLogUlDlEvents) m_ctrlTreeOptions.SetItemText(m_htiLogUlDlEvents, GetResString(IDS_LOG_ULDL_EVENTS));
// [TPT] - WebCache ////////////////////////////////////////////////////////////////////////////////////
		if (m_htiLogWebCacheEvents) m_ctrlTreeOptions.SetItemText(m_htiLogWebCacheEvents, _T("Log WebCache Events:"));//jp log webcache events
		if (m_htiLogICHEvents) m_ctrlTreeOptions.SetItemText(m_htiLogICHEvents, _T("Log IACH Events:"));//JP log ICH events

		if (m_htiCommit) m_ctrlTreeOptions.SetItemText(m_htiCommit, GetResString(IDS_COMMITFILES));
		if (m_htiCommitNever) m_ctrlTreeOptions.SetItemText(m_htiCommitNever, GetResString(IDS_NEVER));
		if (m_htiCommitOnShutdown) m_ctrlTreeOptions.SetItemText(m_htiCommitOnShutdown, GetResString(IDS_ONSHUTDOWN));
		if (m_htiCommitAlways) m_ctrlTreeOptions.SetItemText(m_htiCommitAlways, GetResString(IDS_ALWAYS));

		if (m_htiExtractMetaData) m_ctrlTreeOptions.SetItemText(m_htiExtractMetaData, GetResString(IDS_EXTRACT_META_DATA));
		if (m_htiExtractMetaDataNever) m_ctrlTreeOptions.SetItemText(m_htiExtractMetaDataNever, GetResString(IDS_NEVER));
		if (m_htiExtractMetaDataID3Lib) m_ctrlTreeOptions.SetItemText(m_htiExtractMetaDataID3Lib, GetResString(IDS_META_DATA_ID3LIB));
		if (m_htiExtractMetaDataMediaDet) m_ctrlTreeOptions.SetItemText(m_htiExtractMetaDataMediaDet, GetResString(IDS_META_DATA_MEDIADET));

		if (m_htiFilterLANIPs) m_ctrlTreeOptions.SetItemText(m_htiFilterLANIPs, GetResString(IDS_PW_FILTER));
		if (m_htiExtControls) m_ctrlTreeOptions.SetItemText(m_htiExtControls, GetResString(IDS_SHOWEXTSETTINGS));
		if (m_htiServerKeepAliveTimeout) m_ctrlTreeOptions.SetEditLabel(m_htiServerKeepAliveTimeout, GetResString(IDS_SERVERKEEPALIVETIMEOUT));
		if (m_htiSparsePartFiles) m_ctrlTreeOptions.SetItemText(m_htiSparsePartFiles, GetResString(IDS_SPARSEPARTFILES));
		if (m_htiCheckDiskspace) m_ctrlTreeOptions.SetItemText(m_htiCheckDiskspace, GetResString(IDS_CHECKDISKSPACE));	// SLUGFILLER: checkDiskspace
		if (m_htiMinFreeDiskSpace) m_ctrlTreeOptions.SetEditLabel(m_htiMinFreeDiskSpace, GetResString(IDS_MINFREEDISKSPACE));
		if (m_htiYourHostname) m_ctrlTreeOptions.SetEditLabel(m_htiYourHostname, GetResString(IDS_YOURHOSTNAME));	// itsonlyme: hostnameSource
		if (m_htiFirewallStartup) m_ctrlTreeOptions.SetItemText(m_htiFirewallStartup, GetResString(IDS_FO_PREF_STARTUP));
		if (m_htiDisablePeerCache) m_ctrlTreeOptions.SetItemText(m_htiDisablePeerCache, GetResString(IDS_DISABLEPEERACHE));

		// [TPT] - Moved to PpgPhoenix1
		/*
		// ZZ:UploadSpeedSense -->
        if (m_htiDynUp) m_ctrlTreeOptions.SetItemText(m_htiDynUp, GetResString(IDS_DYNUP));
		if (m_htiDynUpEnabled) m_ctrlTreeOptions.SetItemText(m_htiDynUpEnabled, GetResString(IDS_DYNUPENABLED));
        if (m_htiDynUpMinUpload) m_ctrlTreeOptions.SetEditLabel(m_htiDynUpMinUpload, GetResString(IDS_DYNUP_MINUPLOAD));
        if (m_htiDynUpPingTolerance) m_ctrlTreeOptions.SetEditLabel(m_htiDynUpPingTolerance, GetResString(IDS_DYNUP_PINGTOLERANCE));
        if (m_htiDynUpGoingUpDivider) m_ctrlTreeOptions.SetEditLabel(m_htiDynUpGoingUpDivider, GetResString(IDS_DYNUP_GOINGUPDIVIDER));
        if (m_htiDynUpGoingDownDivider) m_ctrlTreeOptions.SetEditLabel(m_htiDynUpGoingDownDivider, GetResString(IDS_DYNUP_GOINGDOWNDIVIDER));
        if (m_htiDynUpNumberOfPings) m_ctrlTreeOptions.SetEditLabel(m_htiDynUpNumberOfPings, GetResString(IDS_DYNUP_NUMBEROFPINGS));
		// ZZ:UploadSpeedSense <--*/
		// [TPT] - Moved to PpgPhoenix1

        // ZZ:DownloadManager -->
        if (m_htiA4AFSaveCpu) m_ctrlTreeOptions.SetItemText(m_htiA4AFSaveCpu, GetResString(IDS_A4AF_SAVE_CPU));

        CString temp;
		temp.Format(_T("%s: %s"), GetResString(IDS_FILEBUFFERSIZE), CastItoXBytes(m_iFileBufferSize, false, false));
		GetDlgItem(IDC_FILEBUFFERSIZE_STATIC)->SetWindowText(temp);
		// [TPT] - SLUGFILLER: infiniteQueue
		if (m_iQueueSize <= 100*100)
			temp.Format(_T("%s: %s"), GetResString(IDS_QUEUESIZE), GetFormatedUInt(m_iQueueSize));
		else
			temp = GetResString(IDS_INFINITEQUEUE);
		// [TPT] - SLUGFILLER: infiniteQueue
		GetDlgItem(IDC_QUEUESIZE_STATIC)->SetWindowText(temp);
	}
}

void CPPgTweaks::OnDestroy()
{
	m_ctrlTreeOptions.DeleteAllItems();
	m_ctrlTreeOptions.DestroyWindow();
	m_bInitializedTreeOpts = false;
	m_htiMaxCon5Sec = NULL;
	m_htiMaxHalfOpen = NULL;
	m_htiAutoTakeEd2kLinks = NULL;
	m_htiVerboseGroup = NULL;
	m_htiVerbose = NULL;
	m_htiDebugSourceExchange = NULL;
	m_htiLogBannedClients = NULL;
	m_htiLogRatingDescReceived = NULL;
	m_htiLogSecureIdent = NULL;
	m_htiLogFilteredIPs = NULL;
	m_htiLogFileSaving = NULL;
    m_htiLogA4AF = NULL; // ZZ:DownloadManager
	m_htiLogLevel = NULL;
	m_htiLogUlDlEvents = NULL;
// [TPT] - WebCache ////////////////////////////////////////////////////////////////////////////////////
	m_htiLogWebCacheEvents = NULL;//jp log webcache events
	m_htiLogICHEvents = NULL;//JP log ICH events
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
	m_htiSparsePartFiles = NULL;
	m_htiCheckDiskspace = NULL;	// SLUGFILLER: checkDiskspace
	m_htiMinFreeDiskSpace = NULL;
	m_htiYourHostname = NULL;	// itsonlyme: hostnameSource
	m_htiFirewallStartup = NULL;
	m_htiDisablePeerCache = NULL;

	// [TPT] - Moved to PpgPhoenix1
	/*
	// ZZ:UploadSpeedSense -->
    m_htiDynUp = NULL;
	m_htiDynUpEnabled = NULL;
    m_htiDynUpMinUpload = NULL;
    m_htiDynUpPingTolerance = NULL;
    m_htiDynUpPingToleranceMilliseconds = NULL;
    m_htiDynUpPingToleranceGroup = NULL;
    m_htiDynUpRadioPingTolerance = NULL;
    m_htiDynUpRadioPingToleranceMilliseconds = NULL;
    m_htiDynUpGoingUpDivider = NULL;
    m_htiDynUpGoingDownDivider = NULL;
    m_htiDynUpNumberOfPings = NULL;
	// ZZ:UploadSpeedSense <--*/
	// [TPT] - Moved to PpgPhoenix1

    // ZZ:DownloadManager -->
    m_htiA4AFSaveCpu = NULL;
    // ZZ:DownloadManager <--
	m_htiExtractMetaData = NULL;
	m_htiExtractMetaDataNever = NULL;
	m_htiExtractMetaDataID3Lib = NULL;
	m_htiExtractMetaDataMediaDet = NULL;
    
    CPropertyPage::OnDestroy();
}

LRESULT CPPgTweaks::OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == IDC_EXT_OPTS)
	{
		TREEOPTSCTRLNOTIFY* pton = (TREEOPTSCTRLNOTIFY*)lParam;
		if (m_htiVerbose && pton->hItem == m_htiVerbose)
		{
			BOOL bCheck;
			if (m_ctrlTreeOptions.GetCheckBox(m_htiVerbose, bCheck))
			{
				if (m_htiDebug2Disk)			m_ctrlTreeOptions.SetCheckBoxEnable(m_htiDebug2Disk, bCheck);
				if (m_htiDebugSourceExchange)	m_ctrlTreeOptions.SetCheckBoxEnable(m_htiDebugSourceExchange, bCheck);
				if (m_htiLogBannedClients)		m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogBannedClients, bCheck);
				if (m_htiLogRatingDescReceived) m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogRatingDescReceived, bCheck);
				if (m_htiLogSecureIdent)		m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogSecureIdent, bCheck);
				if (m_htiLogFilteredIPs)		m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogFilteredIPs, bCheck);
				if (m_htiLogFileSaving)			m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogFileSaving, bCheck);
                if (m_htiLogA4AF)			    m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogA4AF, bCheck); // ZZ:DownloadManager
				if (m_htiLogUlDlEvents)			m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogUlDlEvents, bCheck);
// [TPT] - WebCache ////////////////////////////////////////////////////////////////////////////////////
				if (m_htiLogWebCacheEvents)		m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogWebCacheEvents, bCheck);//jp log webcache events
				if (m_htiLogICHEvents)			m_ctrlTreeOptions.SetCheckBoxEnable(m_htiLogICHEvents, bCheck);//JP log ICH events
			}
		}
		SetModified();
	}
	return 0;
}

void CPPgTweaks::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_Extended_Settings);
}

BOOL CPPgTweaks::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgTweaks::OnHelpInfo(HELPINFO* pHelpInfo)
{
	OnHelp();
	return TRUE;
}
