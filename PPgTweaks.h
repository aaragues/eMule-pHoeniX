#pragma once
#include "TreeOptionsCtrlEx.h"

class CPPgTweaks : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgTweaks)

public:
	CPPgTweaks();
	virtual ~CPPgTweaks();

// Dialog Data
	enum { IDD = IDD_PPG_TWEAKS };

	void Localize(void);

protected:
	UINT m_iFileBufferSize;
	UINT m_iQueueSize;
	int m_iMaxConnPerFive;
	int m_iMaxHalfOpen;
	int m_iAutoTakeEd2kLinks;
	int m_iVerbose;
	int m_iDebugSourceExchange;
	int m_iLogBannedClients;
	int m_iLogRatingDescReceived;
	int m_iLogSecureIdent;
	int m_iLogFilteredIPs;
	int m_iLogFileSaving;
    int m_iLogA4AF; // ZZ:DownloadManager
	int m_iLogUlDlEvents;
// [TPT] - WebCache ////////////////////////////////////////////////////////////////////////////////////
	int m_iLogWebCacheEvents;//JP log webcache events
	int m_iLogICHEvents;//JP log ICH events
	int m_iCreditSystem;
	int m_iLog2Disk;
	int m_iDebug2Disk;
	int m_iCommitFiles;
	int m_iFilterLANIPs;
	int m_iExtControls;
	UINT m_uServerKeepAliveTimeout;
	int m_iSparsePartFiles;
	int m_iCheckDiskspace;	// SLUGFILLER: checkDiskspace
	float m_fMinFreeDiskSpaceMB;
	CString m_sYourHostname;	// itsonlyme: hostnameSource
	int m_iFirewallStartup;
	int m_iLogLevel;
	int m_iDisablePeerCache;

	// [TPT] - Moved to PpgPhoenix1
	/*
	// ZZ:UploadSpeedSense -->
    int m_iDynUpEnabled;
    int m_iDynUpMinUpload;
    int m_iDynUpPingTolerance;
    int m_iDynUpPingToleranceMilliseconds;
    int m_iDynUpRadioPingTolerance;
    int m_iDynUpGoingUpDivider;
    int m_iDynUpGoingDownDivider;
    int m_iDynUpNumberOfPings;
	// ZZ:UploadSpeedSense <--*/
	// [TPT] - Moved to PpgPhoenix1

    int m_iA4AFSaveCpu; // ZZ:DownloadManager
	int m_iExtractMetaData;

	CSliderCtrl m_ctlFileBuffSize;
	CSliderCtrl m_ctlQueueSize;
    CTreeOptionsCtrlEx m_ctrlTreeOptions;
	bool m_bInitializedTreeOpts;
	HTREEITEM m_htiMaxCon5Sec;
	HTREEITEM m_htiMaxHalfOpen;
	HTREEITEM m_htiAutoTakeEd2kLinks;
	HTREEITEM m_htiVerboseGroup;
	HTREEITEM m_htiVerbose;
	HTREEITEM m_htiDebugSourceExchange;
	HTREEITEM m_htiLogBannedClients;
	HTREEITEM m_htiLogRatingDescReceived;
	HTREEITEM m_htiLogSecureIdent;
	HTREEITEM m_htiLogFilteredIPs;
	HTREEITEM m_htiLogFileSaving;
    HTREEITEM m_htiLogA4AF; // ZZ:DownloadManager
	HTREEITEM m_htiLogUlDlEvents;
// [TPT] - WebCache ////////////////////////////////////////////////////////////////////////////////////
	HTREEITEM m_htiLogWebCacheEvents; //jp log webcache events
	HTREEITEM m_htiLogICHEvents; //JP log ICH events
	HTREEITEM m_htiCreditSystem;
	HTREEITEM m_htiLog2Disk;
	HTREEITEM m_htiDebug2Disk;
	HTREEITEM m_htiCommit;
	HTREEITEM m_htiCommitNever;
	HTREEITEM m_htiCommitOnShutdown;
	HTREEITEM m_htiCommitAlways;
	HTREEITEM m_htiFilterLANIPs;
	HTREEITEM m_htiExtControls;
	HTREEITEM m_htiServerKeepAliveTimeout;
	HTREEITEM m_htiSparsePartFiles;
	HTREEITEM m_htiCheckDiskspace;	// SLUGFILLER: checkDiskspace
	HTREEITEM m_htiMinFreeDiskSpace;
	HTREEITEM m_htiYourHostname;	// itsonlyme: hostnameSource
	HTREEITEM m_htiFirewallStartup;
	HTREEITEM m_htiLogLevel;
	HTREEITEM m_htiDisablePeerCache;

	// [TPT] - Moved to PpgPhoenix1
	/*
	// ZZ:UploadSpeedSense -->
    HTREEITEM m_htiDynUp;
	HTREEITEM m_htiDynUpEnabled;
    HTREEITEM m_htiDynUpMinUpload;
    HTREEITEM m_htiDynUpPingTolerance;
    HTREEITEM m_htiDynUpPingToleranceMilliseconds;
    HTREEITEM m_htiDynUpPingToleranceGroup;
    HTREEITEM m_htiDynUpRadioPingTolerance;
    HTREEITEM m_htiDynUpRadioPingToleranceMilliseconds;
    HTREEITEM m_htiDynUpGoingUpDivider;
    HTREEITEM m_htiDynUpGoingDownDivider;
    HTREEITEM m_htiDynUpNumberOfPings;
	// ZZ:UploadSpeedSense <--*/
	// [TPT] - Moved to PpgPhoenix1

	// ZZ:DownloadManager -->
    HTREEITEM m_htiA4AFSaveCpu;
	// ZZ:DownloadManager <--
	HTREEITEM m_htiExtractMetaData;
	HTREEITEM m_htiExtractMetaDataNever;
	HTREEITEM m_htiExtractMetaDataID3Lib;
	HTREEITEM m_htiExtractMetaDataMediaDet;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnDestroy();
	afx_msg LRESULT OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam);
	afx_msg void OnHelp();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	// Added by MoNKi [MoNKi: -UPnPNAT Support-]
protected:
	int			m_iLogUPnP;
	HTREEITEM	m_htiLogUPnP;
	// End -UPnPNAT Support-
};
