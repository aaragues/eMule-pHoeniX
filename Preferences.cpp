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
#include <io.h>
#include <share.h>
#include "emule.h"
#include "Preferences.h"
#include "Opcodes.h"
#include "OtherFunctions.h"
#include "Ini2.h"
#include "DownloadQueue.h"
#include "UploadQueue.h"
#include "Statistics.h"
#include "MD5Sum.h"
#include "PartFile.h"
#include "Sockets.h"
#include "ListenSocket.h"
#include "ServerList.h"
#include "SharedFileList.h"
#include "UpDownClient.h"
#include "SafeFile.h"
#include "BandWidthControl.h" // [TPT]
#include "emuledlg.h"
#include "StatisticsDlg.h"
#include "Log.h"
#include "MuleToolbarCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


#pragma pack(1)
struct Preferences_Import19c_Struct{
	uint8	version;
	char	nick[50];
	uint16	maxupload;
	uint16	maxdownload;
	uint16	port;
	uint16	maxconnections;
	uint8	reconnect;
	uint8	deadserver;
	uint8	scorsystem;
	char	incomingdir[510];
	char	tempdir[510];
	uint8	ICH;
	uint8	autoserverlist;
	uint8	updatenotify;
	uint8	mintotray;
	uchar	userhash[16];
	uint8	autoconnect;
	uint8	addserversfromserver;
	uint8	addserversfromclient;
};
#pragma pack()

#pragma pack(1)
struct Preferences_Import20a_Struct{
	uint8	version;
	char	nick[50];
	uint16	maxupload;
	uint16	maxdownload;
	uint16	port;
	uint16	maxconnections;
	uint8	reconnect;
	uint8	deadserver;
	uint16	deadserverretries;
	uint8	scorsystem;
	char	incomingdir[510];
	char	tempdir[510];
	uint8	ICH;
	uint8	autoserverlist;
	uint8	updatenotify;
	uint8	mintotray;
	uchar	userhash[16];
	uint8	autoconnect;
	uint8	addserversfromserver;
	uint8	addserversfromclient;
	uint16	maxsourceperfile;
	uint16	trafficOMeterInterval;
	uint32	totalDownloaded;
	uint32	totalUploaded;
	int		maxGraphDownloadRate;
	int		maxGraphUploadRate;
	uint8	beepOnError;
	uint8	confirmExit;
	WINDOWPLACEMENT EmuleWindowPlacement;
	int transferColumnWidths[9];
	int serverColumnWidths[8];
	uint8	splashscreen;
	uint8	filterLANIPs;
};
#pragma pack()

#pragma pack(1)
struct Preferences_Import20b_Struct{
	uint8	version;
	char	nick[50];
	uint16	maxupload;
	uint16	maxdownload;
	uint16	port;
	uint16	maxconnections;
	uint8	reconnect;
	uint8	deadserver;
	uint8	scorsystem;
	char	incomingdir[510];
	char	tempdir[510];
	uint8	ICH;
	uint8	autoserverlist;
	uint8	updatenotify;
	uint8	mintotray;
	uchar	userhash[16];
	uint8	autoconnect;
	uint8	addserversfromserver;
	uint8	addserversfromclient;
	uint16	maxsourceperfile;
	uint16	trafficOMeterInterval;
	uint32	totalDownloaded;	// outdated
	uint32	totalUploaded;		// outdated
	int		maxGraphDownloadRate;
	int		maxGraphUploadRate;
	uint8	beepOnError;
	uint8	confirmExit;
	WINDOWPLACEMENT EmuleWindowPlacement;
	int transferColumnWidths[9];
	int serverColumnWidths[8];
	uint8	splashscreen;
	uint8	filterLANIPs;
	uint64	totalDownloadedBytes;
	uint64	totalUploadedBytes;
};
#pragma pack()

CPreferences thePrefs;

int		CPreferences::m_iDbgHeap;
CString	CPreferences::strNick;
// [TPT]
float 	CPreferences::minupload;
float	CPreferences::maxupload;
float	CPreferences::maxdownload;
float   CPreferences::maxGraphDownloadRate;
float   CPreferences::maxGraphUploadRate;
bool    CPreferences::disablexs;
bool    CPreferences::reaskSourceAfterIPChange;
uint8   CPreferences::zoomFactor;
bool    CPreferences::blockUploadEndMsg;
bool    CPreferences::blockPhoenixMsg;   // [TPT] - Filter own messages	
bool    CPreferences::blockHandshakeMsg; // [TPT] - Filter Handshake messages
bool	CPreferences::blockDeadSources;  // [TPT] - Filter dead sources
bool    CPreferences::blockMaellaSpecificDebugMsg;
bool    CPreferences::blockMaellaSpecificMsg;
uint16	CPreferences::MTU;
uint16	CPreferences::minUploadSlot;
uint16	CPreferences::sendSocketBufferSize;
bool	CPreferences::NAFCEnable;
bool	CPreferences::NAFCFullControl;
float	CPreferences::NAFCNetworkOut;
uint8   CPreferences::NAFCSlider;
uint8	CPreferences::datarateSamples;
bool    CPreferences::enableMultiQueue;
bool    CPreferences::enableNewUSS;
bool    CPreferences::enableReleaseMultiQueue;
bool    CPreferences::enableA4AFTransfer;
int     CPreferences::m_iCreditSystem;
bool    CPreferences::m_bSnafu;
bool    CPreferences::m_bAntiCreditTheft;
bool    CPreferences::m_bAntiFriendshare;
bool    CPreferences::m_bMiniMule;
uint32  CPreferences::m_iMiniMuleUpdate;
bool    CPreferences::m_bMiniMuleLives;
uint8   CPreferences::m_iMiniMuleTransparency;
// [TPT] - end
uint16	CPreferences::port;
uint16	CPreferences::udpport;
uint16	CPreferences::nServerUDPPort;
uint16	CPreferences::maxconnections;
uint16	CPreferences::maxhalfconnections;
uint8	CPreferences::reconnect;
uint8	CPreferences::scorsystem;
TCHAR	CPreferences::incomingdir[MAX_PATH];
TCHAR	CPreferences::tempdir[MAX_PATH];
uint8	CPreferences::ICH;
uint8	CPreferences::autoserverlist;
uint8	CPreferences::updatenotify;
uint8	CPreferences::mintotray;
uint8	CPreferences::autoconnect;
uint8	CPreferences::autoconnectstaticonly;
uint8	CPreferences::autotakeed2klinks;
uint8	CPreferences::addnewfilespaused;
uint8	CPreferences::depth3D;
int		CPreferences::m_iStraightWindowStyles;
CString	CPreferences::m_strSkinProfile;
CString	CPreferences::m_strSkinProfileDir;
uint8	CPreferences::addserversfromserver;
uint8	CPreferences::addserversfromclient;
uint16	CPreferences::maxsourceperfile;
uint16	CPreferences::trafficOMeterInterval;
uint16	CPreferences::statsInterval;
uchar	CPreferences::userhash[16];
WINDOWPLACEMENT CPreferences::EmuleWindowPlacement;
//int		CPreferences::maxGraphDownloadRate; // [TPT]
//int		CPreferences::maxGraphUploadRate; // [TPT]
uint8	CPreferences::beepOnError;
uint8	CPreferences::confirmExit;

// [TPT] - khaos::categorymod+ +1
// [TPT] - WebCache +1
uint16	CPreferences::downloadColumnWidths[13+1+1];
BOOL	CPreferences::downloadColumnHidden[13+1+1];
INT		CPreferences::downloadColumnOrder[13+1+1];
// [TPT] - WebCache +1 end

// [TPT] - khaos::categorymod+ +1 end
// [TPT] - TBH Transfers Window Style
// [TPT] - IP Country +1
uint16	CPreferences::downloadClientsColumnWidths[8+1];
BOOL	CPreferences::downloadClientsColumnHidden[8+1];
INT		CPreferences::downloadClientsColumnOrder[8+1];
// [TPT] - IP Country +1 end
// [TPT] - TBH Transfers Window Style end
// [TPT] - itsonlyme: clientSoft +1	
// [TPT] - IP Country +1
// [TPT] - Total UL/DL +1
uint16	CPreferences::uploadColumnWidths[8+1+1+1];
BOOL	CPreferences::uploadColumnHidden[8+1+1+1];
INT		CPreferences::uploadColumnOrder[8+1+1+1];
// [TPT] - WebCache +1
uint16	CPreferences::queueColumnWidths[10+1+1+1+1];
BOOL	CPreferences::queueColumnHidden[10+1+1+1+1];
INT		CPreferences::queueColumnOrder[10+1+1+1+1];
// [TPT] - WebCache +1 end
// [TPT] - itsonlyme: clientSoft +1	end
// [TPT] - IP Country +1 end
// [TPT] - Total UL/DL +1 end
uint16	CPreferences::searchColumnWidths[14+1];// [TPT] - Fakecheck
BOOL	CPreferences::searchColumnHidden[14+1];//14 + 1
INT		CPreferences::searchColumnOrder[14+1];
// [TPT] - itsonlyme: virtualDirs +1
// [TPT] -  SLUGFILLER: Spreadbars +4
// [TPT] - xMule_MOD: showSharePermissions +1
// [TPT] - Powershare +2
uint16	CPreferences::sharedColumnWidths[12+1+1+4+2];
BOOL	CPreferences::sharedColumnHidden[12+1+1+4+2];
INT		CPreferences::sharedColumnOrder[12+1+1+4+2];
// [TPT] - Powershare +1 end
// [TPT] - xMule_MOD: showSharePermissions +1 end
// [TPT] - itsonlyme: virtualDirs +1 end
// [TPT] -  SLUGFILLER: Spreadbars +4 end
// [TPT] - Aux Ports +1
// [TPT] - serverOrder +1
uint16	CPreferences::serverColumnWidths[14+1+1];
BOOL	CPreferences::serverColumnHidden[14+1+1];
INT		CPreferences::serverColumnOrder[14+1+1];
// [TPT] - serverOrder end
// [TPT] - Aux Ports end
// [TPT] - IP Country +1
// [TPT] - eWombat SNAFU v2 +1
uint16	CPreferences::clientListColumnWidths[8+1+1];
BOOL	CPreferences::clientListColumnHidden[8+1+1];
INT		CPreferences::clientListColumnOrder[8+1+1];
// [TPT] - IP Country +1 end
// [TPT] - eWombat SNAFU v2 +1 end
// [TPT] - MoNKi: -Downloaded History-
uint16	CPreferences::m_HistoryColumnWidths[7];
BOOL	CPreferences::m_HistoryColumnHidden[7];
INT 	CPreferences::m_HistoryColumnOrder[7];
// [TPT] - MoNKi: -Downloaded History-
// [TPT] - Friend State Column
uint16	CPreferences::friendListColumnWidths[2];
BOOL	CPreferences::friendListColumnHidden[2];
INT		CPreferences::friendListColumnOrder[2];
// [TPT] - Friend State Column
uint16	CPreferences::FilenamesListColumnWidths[2];
BOOL	CPreferences::FilenamesListColumnHidden[2];
INT		CPreferences::FilenamesListColumnOrder[2];
DWORD	CPreferences::m_adwStatsColors[15];
uint8	CPreferences::splashscreen;
uint8	CPreferences::filterLANIPs;
bool	CPreferences::m_bAllocLocalHostIP;
uint8	CPreferences::onlineSig;
uint64	CPreferences::cumDownOverheadTotal;
uint64	CPreferences::cumDownOverheadFileReq;
uint64	CPreferences::cumDownOverheadSrcEx;
uint64	CPreferences::cumDownOverheadServer;
uint64	CPreferences::cumDownOverheadKad;
uint64	CPreferences::cumDownOverheadTotalPackets;
uint64	CPreferences::cumDownOverheadFileReqPackets;
uint64	CPreferences::cumDownOverheadSrcExPackets;
uint64	CPreferences::cumDownOverheadServerPackets;
uint64	CPreferences::cumDownOverheadKadPackets;
uint64	CPreferences::cumUpOverheadTotal;
uint64	CPreferences::cumUpOverheadFileReq;
uint64	CPreferences::cumUpOverheadSrcEx;
uint64	CPreferences::cumUpOverheadServer;
uint64	CPreferences::cumUpOverheadKad;
uint64	CPreferences::cumUpOverheadTotalPackets;
uint64	CPreferences::cumUpOverheadFileReqPackets;
uint64	CPreferences::cumUpOverheadSrcExPackets;
uint64	CPreferences::cumUpOverheadServerPackets;
uint64	CPreferences::cumUpOverheadKadPackets;
uint32	CPreferences::cumUpSuccessfulSessions;
uint32	CPreferences::cumUpFailedSessions;
uint32	CPreferences::cumUpAvgTime;
uint64	CPreferences::cumUpData_EDONKEY;
uint64	CPreferences::cumUpData_EDONKEYHYBRID;
uint64	CPreferences::cumUpData_EMULE;
uint64	CPreferences::cumUpData_MLDONKEY;
uint64	CPreferences::cumUpData_AMULE;
uint64	CPreferences::cumUpData_EMULECOMPAT;
uint64	CPreferences::cumUpData_SHAREAZA;
uint64	CPreferences::sesUpData_EDONKEY;
uint64	CPreferences::sesUpData_EDONKEYHYBRID;
uint64	CPreferences::sesUpData_EMULE;
uint64	CPreferences::sesUpData_MLDONKEY;
uint64	CPreferences::sesUpData_AMULE;
uint64	CPreferences::sesUpData_EMULECOMPAT;
uint64	CPreferences::sesUpData_SHAREAZA;
uint64	CPreferences::cumUpDataPort_4662;
uint64	CPreferences::cumUpDataPort_OTHER;
uint64	CPreferences::cumUpDataPort_PeerCache;
uint64	CPreferences::sesUpDataPort_4662;
uint64	CPreferences::sesUpDataPort_OTHER;
uint64	CPreferences::sesUpDataPort_PeerCache;
uint64	CPreferences::cumUpData_File;
uint64	CPreferences::cumUpData_Partfile;
uint64	CPreferences::sesUpData_File;
uint64	CPreferences::sesUpData_Partfile;
uint32	CPreferences::cumDownCompletedFiles;
uint32	CPreferences::cumDownSuccessfulSessions;
uint32	CPreferences::cumDownFailedSessions;
uint32	CPreferences::cumDownAvgTime;
uint64	CPreferences::cumLostFromCorruption;
uint64	CPreferences::cumSavedFromCompression;
uint32	CPreferences::cumPartsSavedByICH;
uint32	CPreferences::sesDownSuccessfulSessions;
uint32	CPreferences::sesDownFailedSessions;
uint32	CPreferences::sesDownAvgTime;
uint32	CPreferences::sesDownCompletedFiles;
uint64	CPreferences::sesLostFromCorruption;
uint64	CPreferences::sesSavedFromCompression;
uint32	CPreferences::sesPartsSavedByICH;
uint64	CPreferences::cumDownData_EDONKEY;
uint64	CPreferences::cumDownData_EDONKEYHYBRID;
uint64	CPreferences::cumDownData_EMULE;
uint64	CPreferences::cumDownData_MLDONKEY;
uint64	CPreferences::cumDownData_AMULE;
uint64	CPreferences::cumDownData_EMULECOMPAT;
uint64	CPreferences::cumDownData_SHAREAZA;
uint64	CPreferences::cumDownData_URL;
uint64	CPreferences::cumDownData_WEBCACHE; // [TPT] - WebCache	//jp webcache statistics
uint64	CPreferences::sesDownData_EDONKEY;
uint64	CPreferences::sesDownData_EDONKEYHYBRID;
uint64	CPreferences::sesDownData_EMULE;
uint64	CPreferences::sesDownData_MLDONKEY;
uint64	CPreferences::sesDownData_AMULE;
uint64	CPreferences::sesDownData_EMULECOMPAT;
uint64	CPreferences::sesDownData_SHAREAZA;
uint64	CPreferences::sesDownData_URL;
// [TPT] - WebCache	
uint64	CPreferences::sesDownData_WEBCACHE; //jp webcache statistics
uint32	CPreferences::ses_WEBCACHEREQUESTS; //jp webcache statistics needs to be uint32 or the statistics won't work
// [TPT] - WebCache	1.9 beta3
uint32	CPreferences::ses_PROXYREQUESTS; //jp webcache statistics
uint32	CPreferences::ses_successfullPROXYREQUESTS; //jp webcache statistics
// [TPT] - WebCache	1.9 beta3
uint32	CPreferences::ses_successfull_WCDOWNLOADS; //jp webcache statistics needs to be uint32 or the statistics won't work
// [TPT] - WebCache	
uint64	CPreferences::cumDownDataPort_4662;
uint64	CPreferences::cumDownDataPort_OTHER;
uint64	CPreferences::cumDownDataPort_PeerCache;
uint64	CPreferences::sesDownDataPort_4662;
uint64	CPreferences::sesDownDataPort_OTHER;
uint64	CPreferences::sesDownDataPort_PeerCache;
float	CPreferences::cumConnAvgDownRate;
float	CPreferences::cumConnMaxAvgDownRate;
float	CPreferences::cumConnMaxDownRate;
float	CPreferences::cumConnAvgUpRate;
float	CPreferences::cumConnMaxAvgUpRate;
float	CPreferences::cumConnMaxUpRate;
uint64	CPreferences::cumConnRunTime;
uint32	CPreferences::cumConnNumReconnects;
uint32	CPreferences::cumConnAvgConnections;
uint32	CPreferences::cumConnMaxConnLimitReached;
uint32	CPreferences::cumConnPeakConnections;
uint32	CPreferences::cumConnTransferTime;
uint32	CPreferences::cumConnDownloadTime;
uint32	CPreferences::cumConnUploadTime;
uint32	CPreferences::cumConnServerDuration;
uint32	CPreferences::cumSrvrsMostWorkingServers;
uint32	CPreferences::cumSrvrsMostUsersOnline;
uint32	CPreferences::cumSrvrsMostFilesAvail;
uint32	CPreferences::cumSharedMostFilesShared;
uint64	CPreferences::cumSharedLargestShareSize;
uint64	CPreferences::cumSharedLargestAvgFileSize;
uint64	CPreferences::cumSharedLargestFileSize;
__int64 CPreferences::stat_datetimeLastReset;
uint8	CPreferences::statsConnectionsGraphRatio;
UINT	CPreferences::statsSaveInterval;
TCHAR	CPreferences::statsExpandedTreeItems[256];
bool	CPreferences::m_bShowVerticalHourMarkers;
uint64	CPreferences::totalDownloadedBytes;
uint64	CPreferences::totalUploadedBytes;
WORD	CPreferences::m_wLanguageID;
uint8	CPreferences::transferDoubleclick;
EViewSharedFilesAccess CPreferences::m_iSeeShares;
uint8	CPreferences::m_iToolDelayTime;
uint8	CPreferences::bringtoforeground;
uint8	CPreferences::splitterbarPosition;
uint8	CPreferences::splitterbarPositionStat;
uint8	CPreferences::splitterbarPositionStat_HL;
uint8	CPreferences::splitterbarPositionStat_HR;
uint16	CPreferences::splitterbarPositionFriend;
uint16	CPreferences::splitterbarPositionIRC;
uint8	CPreferences::m_uTransferWnd2;
uint16	CPreferences::deadserverretries;
DWORD	CPreferences::m_dwServerKeepAliveTimeout;
uint16	CPreferences::statsMax;
uint8	CPreferences::statsAverageMinutes;
// [TPT] - enkeyDEV(th1) -notifier-
uint8   CPreferences::useNotifierUserTimings;
uint16  CPreferences::notifierUserTimeToShow;
uint16  CPreferences::notifierUserTimeToStay;
uint16  CPreferences::notifierUserTimeToHide;
uint8   CPreferences::notifierLessFramerate;
uint8	CPreferences::useDownloadNotifier;
uint8	CPreferences::notifierAutoClose;  
uint8	CPreferences::useErrorNotifier;	// added by InterCeptor (notify on error) 11.11.02
uint8	CPreferences::useNewDownloadNotifier;
uint8	CPreferences::useChatNotifier;
uint8	CPreferences::useLogNotifier;
uint8	CPreferences::useSoundInNotifier;
uint8	CPreferences::notifierPopsEveryChatMsg;
uint8	CPreferences::notifierImportantError;
uint8	CPreferences::notifierNewVersion;
TCHAR	CPreferences::notifierSoundFilePath[510];
uint8	CPreferences::notifierSearchCompleted;    	
uint8	CPreferences::notifierNewPvtMsg;//Rocks
// [TPT] - enkeyDEV(th1) -notifier-
TCHAR	CPreferences::m_sircserver[50];
TCHAR	CPreferences::m_sircnick[30];
TCHAR	CPreferences::m_sircchannamefilter[50];
bool	CPreferences::m_bircaddtimestamp;
bool	CPreferences::m_bircusechanfilter;
uint16	CPreferences::m_iircchanneluserfilter;
TCHAR	CPreferences::m_sircperformstring[255];
bool	CPreferences::m_bircuseperform;
bool	CPreferences::m_birclistonconnect;
bool	CPreferences::m_bircacceptlinks;
bool	CPreferences::m_bircacceptlinksfriends;
bool	CPreferences::m_bircsoundevents;
bool	CPreferences::m_bircignoremiscmessage;
bool	CPreferences::m_bircignorejoinmessage;
bool	CPreferences::m_bircignorepartmessage;
bool	CPreferences::m_bircignorequitmessage;
bool	CPreferences::m_bircignoreemuleprotoaddfriend;
bool	CPreferences::m_bircallowemuleprotoaddfriend;
bool	CPreferences::m_bircignoreemuleprotosendlink;
bool	CPreferences::m_birchelpchannel;
bool	CPreferences::m_bRemove2bin;
bool	CPreferences::m_bShowCopyEd2kLinkCmd;
bool	CPreferences::m_bpreviewprio;
bool	CPreferences::smartidcheck;
uint8	CPreferences::smartidstate;
bool	CPreferences::safeServerConnect;
bool	CPreferences::startMinimized;
bool	CPreferences::m_bAutoStart;
bool	CPreferences::m_bRestoreLastMainWndDlg;
int		CPreferences::m_iLastMainWndDlgID;
bool	CPreferences::m_bRestoreLastLogPane;
int		CPreferences::m_iLastLogPaneID;
uint16	CPreferences::MaxConperFive;
int		CPreferences::checkDiskspace;
UINT	CPreferences::m_uMinFreeDiskSpace;
bool	CPreferences::m_bSparsePartFiles;
CString	CPreferences::m_strYourHostname;
bool	CPreferences::infiniteQueue;	// [TPT] - SLUGFILLER: infiniteQueue
// [TPT] - SLUGFILLER: hideOS
uint8	CPreferences::hideOS;
bool	CPreferences::selectiveShare;
// [TPT] - SLUGFILLER: hideOS
// [TPT] - itsonlyme: displayOptions START
bool	CPreferences::showFileSystemIcon;
bool	CPreferences::showLocalRating; // [TPT] - SLUGFILLER: showComments
// [TPT] - itsonlyme: displayOptions END
DWORD	CPreferences::m_currentAdapterIndex; // [TPT] - NAFC Selection
bool	CPreferences::m_bShowInMSN7; //[TPT] - Show in MSN7
bool	CPreferences::m_bShowUpPrioInDownloadList;//[TPT]- Show upload priority in downloadlist
// [TPT] - quick start
bool	CPreferences::m_QuickStart;
uint16  CPreferences::m_QuickStartMaxCon;
uint16  CPreferences::m_QuickStartMaxConPerFive;
// [TPT] - quick start
bool	CPreferences::m_bSpreadBars; // [TPT] - SLUGFILLER: Spreadbars
bool	CPreferences::m_bEnableVerboseOptions;
bool	CPreferences::m_bVerbose;
bool	CPreferences::m_bFullVerbose;
bool	CPreferences::m_bDebugSourceExchange;
bool	CPreferences::m_bLogBannedClients;
bool	CPreferences::m_bLogRatingDescReceived;
bool	CPreferences::m_bLogSecureIdent;
bool	CPreferences::m_bLogFilteredIPs;
bool	CPreferences::m_bLogFileSaving;
bool	CPreferences::m_bLogA4AF; // ZZ:DownloadManager
bool	CPreferences::m_bLogUlDlEvents;
// [TPT] - WebCache ////////////////////////////////////////////////////////////////////////////////////
bool	CPreferences::m_bLogWebCacheEvents;//JP log webcache events
bool	CPreferences::m_bLogICHEvents;//JP log ICH events
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
bool	CPreferences::m_bUseDebugDevice = true;
#else
bool	CPreferences::m_bUseDebugDevice = false;
#endif
int		CPreferences::m_iDebugServerTCPLevel;
int		CPreferences::m_iDebugServerUDPLevel;
int		CPreferences::m_iDebugServerSourcesLevel;
int		CPreferences::m_iDebugServerSearchesLevel;
int		CPreferences::m_iDebugClientTCPLevel;
int		CPreferences::m_iDebugClientUDPLevel;
int		CPreferences::m_iDebugClientKadUDPLevel;
bool	CPreferences::m_bupdatequeuelist;
bool	CPreferences::m_bmanualhighprio;
bool	CPreferences::m_btransferfullchunks;
int		CPreferences::m_istartnextfile;
bool	CPreferences::m_bshowoverhead;
bool	CPreferences::m_bDAP;
bool	CPreferences::m_bUAP;
bool	CPreferences::m_bDisableKnownClientList;
bool	CPreferences::m_bDisableQueueList;
bool	CPreferences::m_bExtControls;
bool	CPreferences::m_bTransflstRemain;
uint8	CPreferences::versioncheckdays;
// [TPT] - SLUGFILLER: multiSort - save multiple params
// [TPT] - SLUGFILLER: DLsortFix - double, for client-only sorting
// [TPT] - khaos::categorymod+ +1
// [TPT] - WebCache +1
int		CPreferences::tableSortItemDownload[30];
bool	CPreferences::tableSortAscendingDownload[30];
// [TPT] - khaos::categorymod-
// [TPT] - WebCache +1 end
// [TPT] - IP Country +1
int		CPreferences::tableSortItemDownloadClient[18]; // [TPT] - TBH Transfers Window Style		
bool	CPreferences::tableSortAscendingDownloadClient[18]; // [TPT] - TBH Transfers Window Style	
// [TPT] - IP Country
// [TPT] - SLUGFILLER: DLsortFix

// [TPT] - itsonlyme: clientSoft +1	
// [TPT] - IP Country +1
// [TPT] - Total UL/DL +1
int		CPreferences::tableSortItemUpload[11];
bool	CPreferences::tableSortAscendingUpload[11];	
// [TPT] - WebCache +1 
int		CPreferences::tableSortItemQueue[13+1];
bool	CPreferences::tableSortAscendingQueue[13+1];
// [TPT] - WebCache +1 end
// [TPT] - IP Country
// [TPT] - itsonlyme: clientSoft

int		CPreferences::tableSortItemSearch[15];
bool	CPreferences::tableSortAscendingSearch[15];

// [TPT] - itsonlyme: virtualDirs +1
// [TPT] -  SLUGFILLER: Spreadbars +4
// [TPT] - xMule_MOD: showSharePermissions +1
int		CPreferences::tableSortItemShared[12+1+1+4+2];
bool	CPreferences::tableSortAscendingShared[12+1+1+4+2];
// [TPT] - xMule_MOD: showSharePermissions
// [TPT] - itsonlyme: virtualDirs
// [TPT] -  SLUGFILLER: Spreadbars

// [TPT] - Aux Ports +1
// [TPT] - serverOrder +1
int		CPreferences::tableSortItemServer[14+1+1];
bool	CPreferences::tableSortAscendingServer[14+1+1];
// [TPT] - serverOrder end
// [TPT] - Aux Ports end

// [TPT] - IP Country +1
// [TPT] - eWombat SNAFU v2 +1
int		CPreferences::tableSortItemClientList[8+1+1];
bool	CPreferences::tableSortAscendingClientList[8+1+1];
// [TPT] - eWombat SNAFU v2
// [TPT] - IP Country

// [TPT] - MoNKi: -Downloaded History-
int		CPreferences::tableSortItemHistory[7];
bool	CPreferences::tableSortAscendingHistory[7];
// [TPT] - MoNKi: -Downloaded History-

// [TPT] - Friend State Column
int		CPreferences::tableSortItemFriendList[2];
bool	CPreferences::tableSortAscendingFriendList[2];
// [TPT] - Friend State Column

int		CPreferences::tableSortItemFilenames[2];
bool	CPreferences::tableSortAscendingFilenames[2];

// [TPT] - SLUGFILLER: multiSort	
bool	CPreferences::showRatesInTitle;
TCHAR	CPreferences::TxtEditor[256];
TCHAR	CPreferences::VideoPlayer[256];
bool	CPreferences::moviePreviewBackup;
int		CPreferences::m_iPreviewSmallBlocks;
int		CPreferences::m_iPreviewCopiedArchives;
int		CPreferences::m_iInspectAllFileTypes;
bool	CPreferences::m_bPreviewOnIconDblClk;
bool	CPreferences::indicateratings;
bool	CPreferences::watchclipboard;
bool	CPreferences::filterserverbyip;
bool	CPreferences::m_bFirstStart;
bool	CPreferences::m_bCreditSystem;
// [TPT] - TBH: minimule
int		CPreferences::speedmetermin;
int		CPreferences::speedmetermax;
// [TPT] - TBH: minimule
bool	CPreferences::log2disk;
bool	CPreferences::debug2disk;
int		CPreferences::iMaxLogBuff;
UINT	CPreferences::uMaxLogFileSize;
bool	CPreferences::scheduler;
bool	CPreferences::dontcompressavi;
bool	CPreferences::msgonlyfriends;
bool	CPreferences::msgsecure;
uint8	CPreferences::filterlevel;
UINT	CPreferences::m_iFileBufferSize;
UINT	CPreferences::m_iQueueSize;
int		CPreferences::m_iCommitFiles;
bool	CPreferences::m_bHistoryShowShared; // [TPT] - MoNKi: -Downloaded History-
uint16	CPreferences::maxmsgsessions;
uint32	CPreferences::versioncheckLastAutomatic;
TCHAR	CPreferences::messageFilter[512];
CString	CPreferences::commentFilter;
TCHAR	CPreferences::filenameCleanups[512];
TCHAR	CPreferences::notifierConfiguration[510];
TCHAR	CPreferences::datetimeformat[64];
TCHAR	CPreferences::datetimeformat4log[64];
LOGFONT CPreferences::m_lfHyperText;
LOGFONT CPreferences::m_lfLogText;
COLORREF CPreferences::m_crLogError = RGB(255, 0, 0);
COLORREF CPreferences::m_crLogWarning = RGB(128, 0, 128);
COLORREF CPreferences::m_crLogSuccess = RGB(0, 0, 255);
int		CPreferences::m_iExtractMetaData;
bool	CPreferences::m_bAdjustNTFSDaylightFileTime = true;
TCHAR	CPreferences::m_sWebPassword[256];
TCHAR	CPreferences::m_sWebLowPassword[256];
uint16	CPreferences::m_nWebPort;
bool	CPreferences::m_bWebEnabled;
bool	CPreferences::m_bWebUseGzip;
int		CPreferences::m_nWebPageRefresh;
bool	CPreferences::m_bWebLowEnabled;
TCHAR	CPreferences::m_sWebResDir[MAX_PATH];
int		CPreferences::m_iWebTimeoutMins;
TCHAR	CPreferences::m_sTemplateFile[MAX_PATH];
ProxySettings CPreferences::proxy;
bool	CPreferences::m_bIsASCWOP;
bool	CPreferences::m_bShowProxyErrors;
bool	CPreferences::showCatTabInfos;
bool	CPreferences::resumeSameCat;
bool	CPreferences::dontRecreateGraphs;
bool	CPreferences::autofilenamecleanup;
bool	CPreferences::m_bUseAutocompl;
bool	CPreferences::m_bShowDwlPercentage;
bool	CPreferences::m_bRemoveFinishedDownloads;
uint16	CPreferences::m_iMaxChatHistory;
bool	CPreferences::m_bShowActiveDownloadsBold;
int		CPreferences::m_iSearchMethod;
bool	CPreferences::m_bAdvancedSpamfilter;
bool	CPreferences::m_bUseSecureIdent;
TCHAR	CPreferences::m_sMMPassword[256];
bool	CPreferences::m_bMMEnabled;
uint16	CPreferences::m_nMMPort;
bool	CPreferences::networkkademlia;
bool	CPreferences::networked2k;
EToolbarLabelType CPreferences::m_nToolbarLabels;
CString	CPreferences::m_sToolbarBitmap;
CString	CPreferences::m_sToolbarBitmapFolder;
CString	CPreferences::m_sToolbarSettings;
bool	CPreferences::m_bReBarToolbar;
CSize	CPreferences::m_sizToolbarIconSize;
bool	CPreferences::m_bPreviewEnabled;
bool	CPreferences::m_bDynUpEnabled;
int		CPreferences::m_iDynUpPingTolerance;
int		CPreferences::m_iDynUpGoingUpDivider;
int		CPreferences::m_iDynUpGoingDownDivider;
int		CPreferences::m_iDynUpNumberOfPings;
int		CPreferences::m_iDynUpPingToleranceMilliseconds;
bool	CPreferences::m_bDynUpUseMillisecondPingTolerance;

// [TPT] - Powershare
uint8	CPreferences::m_iPowershareMode;
uint8	CPreferences::PowerShareLimit;
// [TPT] - Powershare
bool	CPreferences::m_bSaveUploadQueueWaitTime; // [TPT] - SUQWT
bool	CPreferences::m_bUnlimitedUP;//[TPT] - Unlimited upload with no downloads
// ZZ:DownloadManager -->
bool    CPreferences::m_bA4AFSaveCpu;
// ZZ:DownloadManager <--

bool	CPreferences::m_bfadeOut;//[TPT] - Fade out on Exit

CStringList CPreferences::shareddir_list;
CStringList CPreferences::adresses_list;
bool	CPreferences::cumulateBandwidth; // [TPT]-Cumulate Bandwidth
bool	CPreferences::minimizeSlots; // [TPT] - Pawcio: MUS
bool	CPreferences::manageConnection; // [TPT] - Manage Connection
// added by [TPT]-MoNKi [MoNKi: -invisible mode-]
bool	CPreferences::m_bInvisibleMode;		
UINT	CPreferences::m_iInvisibleModeHotKeyModifier;
TCHAR	CPreferences::m_cInvisibleModeHotKey;
// End [TPT]-MoNKi
// [TPT] - IP Country
bool	CPreferences::showFlags;
bool	CPreferences::showCountryName;
bool	CPreferences::m_bDinamicNafc;
// [TPT] - IP Country
bool	CPreferences::m_bUseWindowsTextColorOnHighContrast; // [TPT] - MoNKi: -Support for High Contrast Mode-
// [TPT] - khaos::categorymod+	
bool	CPreferences::m_bShowBitmapInMenus;
bool	CPreferences::m_bShowCatNames;
bool	CPreferences::m_bActiveCatDefault;
bool	CPreferences::m_bSelCatOnAdd;
bool	CPreferences::m_bAutoSetResumeOrder;	
uint8	CPreferences::m_iStartDLInEmptyCats;
bool	CPreferences::m_bRespectMaxSources;
bool	CPreferences::m_bUseAutoCat;
bool	CPreferences::m_bShowPriorityInTab;
// [TPT] - khaos::categorymod-
bool	CPreferences::m_bDBStyle;//[TPT] - Double buffer style in lists
CString CPreferences::appdir;
CString CPreferences::configdir;
CString CPreferences::ed2klinkdir;  // [TPT] - Save ed2klinks
CString CPreferences::autoHLdir;//[TPT] - sivka autoHL
CString CPreferences::saveSourcesdir;//[TPT] - Save sources
CString CPreferences::m_strWebServerDir;
CString CPreferences::m_strLangDir;
CString CPreferences::m_strFileCommentsFilePath;
CString	CPreferences::m_strLogDir;
Preferences_Ext_Struct* CPreferences::prefsExt;
WORD	CPreferences::m_wWinVer;
bool	CPreferences::m_UseProxyListenPort;
uint16	CPreferences::ListenPort;
CArray<Category_Struct*,Category_Struct*> CPreferences::catMap;
uint8	CPreferences::m_nWebMirrorAlertLevel;
bool	CPreferences::m_bRunAsUser;
bool	CPreferences::m_bUseOldTimeRemaining;
uint32	CPreferences::m_uPeerCacheLastSearch;
bool	CPreferences::m_bPeerCacheWasFound;
bool	CPreferences::m_bPeerCacheEnabled;
uint16	CPreferences::m_nPeerCachePort;
bool	CPreferences::m_bPeerCacheShow;

bool	CPreferences::m_bOpenPortsOnStartUp;
uint8	CPreferences::m_byLogLevel;
bool	CPreferences::m_bTrustEveryHash;
// [TPT] - itsonlyme: virtualDirs
CRBMap<CString, CString>	CPreferences::m_fileToVDir_map;
CRBMap<CString, CString>	CPreferences::m_dirToVDir_map;
CRBMap<CString, CString>	CPreferences::m_dirToVDirWithSD_map;
// [TPT] - itsonlyme: virtualDirs
uint32	CPreferences::m_MainProcessPriority; // [TPT] - Select process priority 
bool	CPreferences::m_bMemoryConsumingGraph; // [TPT] - Memory consuming
bool	CPreferences::m_bNAFCGraph;
bool	CPreferences::m_bOverheadGraph;
// [TPT] - Sivka AutoHL Begin
bool	CPreferences::m_MaxSourcesPerFileTakeOver;
uint16	CPreferences::m_MaxSourcesPerFileTemp;
bool	CPreferences::m_TakeOverFileSettings;
// [TPT] - Sivka AutoHL end
// [TPT] - TBH -AutoBackup Begin
bool	CPreferences::autobackup;
bool	CPreferences::autobackup2;
// [TPT] - TBH -AutoBackup End
// [TPT] - MoNKi: -UPnPNAT Support-
bool	CPreferences::m_bUPnPNat;
bool	CPreferences::m_bUPnPNatWeb;	
uint16	CPreferences::m_iUPnPTCPExternal;
uint16	CPreferences::m_iUPnPUDPExternal;
uint16	CPreferences::m_iUPnPTCPInternal;
uint16	CPreferences::m_iUPnPUDPInternal;
bool	CPreferences::m_bUPnPVerboseLog;
uint16	CPreferences::m_iUPnPPort;
// [TPT] - MoNKi: -UPnPNAT Support-

// [TPT] - Fakecheck
TCHAR	CPreferences::UpdateURLFakeList[256];
uint32	CPreferences::m_FakesDatVersion;
bool	CPreferences::UpdateFakeStartup;
// [TPT] - Fakecheck End

// [TPT] - Manual eMfriend.met download
TCHAR	CPreferences::m_updateURLFriendList[256];
// [TPT] - Manual eMfriend.met download

// [TPT] - WebCache	
// yonatan http start 
CString	CPreferences::webcacheName;
uint16	CPreferences::webcachePort;
bool	CPreferences::webcacheReleaseAllowed; //jp webcache release
uint16	CPreferences::webcacheBlockLimit;
bool	CPreferences::PersistentConnectionsForProxyDownloads; //jp persistent proxy connections
bool	CPreferences::WCAutoupdate; //jp WCAutoupdate //[TPT] - Webcache 1.9 beta3
bool	CPreferences::webcacheExtraTimeout;
bool	CPreferences::webcacheCachesLocalTraffic;
bool	CPreferences::webcacheEnabled;
bool	CPreferences::detectWebcacheOnStart; //jp detect webcache on startup
uint32	CPreferences::webcacheLastSearch;
CString	CPreferences::webcacheLastResolvedName;
uint32	CPreferences::webcacheLastGlobalIP;
bool	CPreferences::UsesCachedTCPPort()  //jp
{
	if ((thePrefs.port==80) || (thePrefs.port==21) || (thePrefs.port==443) || (thePrefs.port==563) || (thePrefs.port==70) || (thePrefs.port==210) || ((thePrefs.port>=1025) && (thePrefs.port<=65535))) return true;
	else return false;
}
// JP detect fake HighID (from netfinity)
bool	CPreferences::m_bHighIdPossible;
//JP proxy configuration test start
bool	CPreferences::WebCacheDisabledThisSession;//jp temp disabled
uint32	CPreferences::WebCachePingSendTime;//jp check proxy config
bool	CPreferences::expectingWebCachePing;//jp check proxy config
bool	CPreferences::IsWebCacheTestPossible()//jp check proxy config
{
	return (theApp.GetPublicIP() != 0 //we have a public IP
		&& theApp.serverconnect->IsConnected() //connected to a server
		&& !theApp.serverconnect->IsLowID()//don't have LowID
		&& m_bHighIdPossible //[TPT] - Webcache 1.9 beta3
		&& !theApp.listensocket->TooManySockets());// no fake high ID
}
//JP proxy configuration test end
uint8	CPreferences::webcacheTrustLevel;//[TPT] - Webcache 1.9 beta3
//JP webcache release START
bool	CPreferences::UpdateWebcacheReleaseAllowed()
{
	webcacheReleaseAllowed = true;
	if (theApp.downloadqueue->ContainsUnstoppedFiles())
		webcacheReleaseAllowed = false;
	return webcacheReleaseAllowed;
}
//JP webcache release END
// yonatan http end
// [TPT] - WebCache	

CPreferences::CPreferences()
{
	#ifdef _DEBUG
		m_iDbgHeap = 1;
	#endif
	// [TPT] - WebCache
	//JP set standard values for stuff that doesn't need to be saved. This should probably be somewhere else START
	expectingWebCachePing = false;
	WebCachePingSendTime = 0;
	WebCacheDisabledThisSession = false;
	webcacheReleaseAllowed = true; //jp webcache release
	m_bHighIdPossible = false; // JP detect fake HighID (from netfinity)
	//JP set standard values for stuff that doesn't need to be saved. This should probably be somewhere else END
}

CPreferences::~CPreferences()
{
	delete prefsExt;
}

void CPreferences::Init()
{
	srand((uint32)time(0)); // we need random numbers sometimes

	prefsExt = new Preferences_Ext_Struct;
	MEMZERO(prefsExt, sizeof *prefsExt);

	// [TPT] - MoNKi: -UPnPNAT Support-
	m_iUPnPUDPExternal = 0;
	m_iUPnPTCPExternal = 0;
	m_iUPnPUDPInternal = 0;
	m_iUPnPTCPInternal = 0;
	// [TPT] - MoNKi: -UPnPNAT Support-

	//get application start directory
	TCHAR buffer[490];
	::GetModuleFileName(0, buffer, 490);
	LPTSTR pszFileName = _tcsrchr(buffer, _T('\\')) + 1;
	*pszFileName = _T('\0');

	appdir = buffer;
	configdir = appdir + CONFIGFOLDER;
	m_strWebServerDir = appdir + _T("webserver\\");
	m_strLangDir = appdir + _T("lang\\");
	m_strFileCommentsFilePath = configdir + _T("fileinfo.ini");
	m_strLogDir = appdir + _T("logs\\");

	///////////////////////////////////////////////////////////////////////////
	// Create 'config' directory (and optionally move files from application directory)
	//
	::CreateDirectory(GetConfigDir(), 0);
	// lets move config-files in the appdir to the configdir (for upgraders <0.29a to >=0.29a )
	if ( PathFileExists(appdir+_T("preferences.ini"))) MoveFile(appdir+_T("preferences.ini"),configdir+_T("preferences.ini"));
	if ( PathFileExists(appdir+_T("preferences.dat"))) MoveFile(appdir+_T("preferences.dat"),configdir+_T("preferences.dat"));
	if ( PathFileExists(appdir+_T("adresses.dat"))) MoveFile(appdir+_T("adresses.dat"),configdir+_T("adresses.dat"));
	if ( PathFileExists(appdir+_T("Category.ini"))) MoveFile(appdir+_T("Category.ini"),configdir+_T("Category.ini"));
	if ( PathFileExists(appdir+_T("clients.met"))) MoveFile(appdir+_T("clients.met"),configdir+_T("clients.met"));
	if ( PathFileExists(appdir+_T("emfriends.met"))) MoveFile(appdir+_T("emfriends.met"),configdir+_T("emfriends.met"));
	if ( PathFileExists(appdir+_T("fileinfo.ini"))) MoveFile(appdir+_T("fileinfo.ini"),configdir+_T("fileinfo.ini"));
	if ( PathFileExists(appdir+_T("ipfilter.dat"))) MoveFile(appdir+_T("ipfilter.dat"),configdir+_T("ipfilter.dat"));
	if ( PathFileExists(appdir+_T("known.met"))) MoveFile(appdir+_T("known.met"),configdir+_T("known.met"));
	if ( PathFileExists(appdir+_T("server.met"))) MoveFile(appdir+_T("server.met"),configdir+_T("server.met"));
	if ( PathFileExists(appdir+_T("shareddir.dat"))) MoveFile(appdir+_T("shareddir.dat"),configdir+_T("shareddir.dat"));
	if ( PathFileExists(appdir+_T("staticservers.dat"))) MoveFile(appdir+_T("staticservers.dat"),configdir+_T("staticservers.dat"));
	if ( PathFileExists(appdir+_T("webservices.dat"))) MoveFile(appdir+_T("webservices.dat"),configdir+_T("webservices.dat"));

	///////////////////////////////////////////////////////////////////////////
	// Create 'logs' directory (and optionally move files from application directory)
	//
	::CreateDirectory(GetLogDir(), 0);
	CFileFind ff;
	bool bFoundFile = ff.FindFile(GetAppDir() + _T("eMule*.log"), 0);
	while (bFoundFile)
	{
		bFoundFile = ff.FindNextFile();
		if (ff.IsDots() || ff.IsSystem() || ff.IsDirectory() || ff.IsHidden())
			continue;
		MoveFile(ff.GetFilePath(), GetLogDir() + ff.GetFileName());
	}


	CreateUserHash();

	// load preferences.dat or set standart values
	TCHAR* fullpath = new TCHAR[_tcslen(configdir)+16];
	_stprintf(fullpath,_T("%spreferences.dat"),configdir);
	FILE* preffile = _tfsopen(fullpath,_T("rb"), _SH_DENYWR);
	delete[] fullpath;
	fullpath = NULL; // [TPT] - Patch

	LoadPreferences();

	if (!preffile){
		SetStandartValues();
	}
	else{
		fread(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile);
		if (ferror(preffile))
			SetStandartValues();

		// import old pref-files
		if (prefsExt->version<20) {
			if (prefsExt->version>17) {// v0.20b+
				Preferences_Import20b_Struct* prefsImport20b;
				prefsImport20b=new Preferences_Import20b_Struct;
				MEMZERO(prefsImport20b, sizeof(Preferences_Import20b_Struct));
				fseek(preffile,0,0);
				fread(prefsImport20b,sizeof(Preferences_Import20b_Struct),1,preffile);

				md4cpy(userhash, prefsImport20b->userhash);
				MEMCOPY(incomingdir, prefsImport20b->incomingdir, 510);
				MEMCOPY(tempdir, prefsImport20b->tempdir, 510);
				strNick = prefsImport20b->nick;

				totalDownloadedBytes=prefsImport20b->totalDownloadedBytes;
				totalUploadedBytes=prefsImport20b->totalUploadedBytes;
				delete prefsImport20b;
			} else if (prefsExt->version>7) { // v0.20a
				Preferences_Import20a_Struct* prefsImport20a;
				prefsImport20a=new Preferences_Import20a_Struct;
				MEMZERO(prefsImport20a, sizeof(Preferences_Import20a_Struct));
				fseek(preffile,0,0);
				fread(prefsImport20a,sizeof(Preferences_Import20a_Struct),1,preffile);

				md4cpy(userhash, prefsImport20a->userhash);
				MEMCOPY(incomingdir, prefsImport20a->incomingdir, 510);
				MEMCOPY(tempdir, prefsImport20a->tempdir, 510);
				strNick = prefsImport20a->nick;

				totalDownloadedBytes=prefsImport20a->totalDownloaded;
				totalUploadedBytes=prefsImport20a->totalUploaded;
				delete prefsImport20a;
			} else {	//v0.19c-
				Preferences_Import19c_Struct* prefsImport19c;
				prefsImport19c=new Preferences_Import19c_Struct;
				MEMZERO(prefsImport19c, sizeof(Preferences_Import19c_Struct));

				fseek(preffile,0,0);
				fread(prefsImport19c,sizeof(Preferences_Import19c_Struct),1,preffile);

				if (prefsExt->version < 3)
					CreateUserHash();
				else
					md4cpy(userhash, prefsImport19c->userhash);
				MEMCOPY(incomingdir, prefsImport19c->incomingdir, 510);
				MEMCOPY(tempdir, prefsImport19c->tempdir, 510);
				strNick = prefsImport19c->nick;
				delete prefsImport19c;
			}
 		} else {
			md4cpy(userhash, prefsExt->userhash);
			EmuleWindowPlacement = prefsExt->EmuleWindowPlacement;
		}
		fclose(preffile);
		smartidstate = 0;
	}

	// shared directories
	fullpath = new TCHAR[_tcslen(configdir) + MAX_PATH];
	_stprintf(fullpath,_T("%sshareddir.dat"),configdir);
	CStdioFile* sdirfile = new CStdioFile();
	bool bIsUnicodeFile = IsUnicodeFile(fullpath); // check for BOM
	// open the text file either in ANSI (text) or Unicode (binary), this way we can read old and new files
	// with nearly the same code..
	if (sdirfile->Open(fullpath, CFile::modeRead | CFile::shareDenyWrite | (bIsUnicodeFile ? CFile::typeBinary : 0)))
	{
		try {
			if (bIsUnicodeFile)
				sdirfile->Seek(sizeof(WORD), SEEK_CUR); // skip BOM

		CString toadd;
		while (sdirfile->ReadString(toadd))
		{
				toadd.Trim(_T("\r\n")); // need to trim '\r' in binary mode
			TCHAR szFullPath[MAX_PATH];
			if (PathCanonicalize(szFullPath, toadd))
				toadd = szFullPath;

			if (IsInstallationDirectory(toadd))
				continue;

			if (_taccess(toadd, 0) == 0){ // only add directories which still exist
				if (toadd.Right(1) != _T('\\'))
					toadd.Append(_T("\\"));
				shareddir_list.AddHead(toadd);
			}
		}
		}
		catch (CFileException* ex) {
			ASSERT(0);
			ex->Delete();
		}
		sdirfile->Close();
	}
	delete sdirfile;
	delete[] fullpath;
	fullpath = NULL; // [TPT] - Patch
	
	//serverlist adresses
	fullpath = new TCHAR[_tcslen(configdir)+20];
	_stprintf(fullpath,_T("%sadresses.dat"),configdir);
	sdirfile = new CStdioFile();
	bIsUnicodeFile = IsUnicodeFile(fullpath);
	if (sdirfile->Open(fullpath, CFile::modeRead | CFile::shareDenyWrite | (bIsUnicodeFile ? CFile::typeBinary : 0)))
	{
		try {
			if (bIsUnicodeFile)
				sdirfile->Seek(sizeof(WORD), SEEK_CUR); // skip BOM

		CString toadd;
		while (sdirfile->ReadString(toadd))
			{
				toadd.Trim(_T("\r\n")); // need to trim '\r' in binary mode
			adresses_list.AddHead(toadd);
			}
		}
		catch (CFileException* ex) {
			ASSERT(0);
			ex->Delete();
		}
		sdirfile->Close();
	}
	delete sdirfile;
	delete[] fullpath;	
	fullpath=NULL;

	userhash[5] = 14;
	userhash[14] = 111;

	// deadlake PROXYSUPPORT
	m_UseProxyListenPort = false;
	ListenPort = 0;

	// Explicitly inform the user about errors with incoming/temp folders!
	if (!PathFileExists(GetIncomingDir()) && !::CreateDirectory(GetIncomingDir(),0)) {
		CString strError;
		strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_INCOMING), GetIncomingDir(), GetErrorMessage(GetLastError()));
		AfxMessageBox(strError, MB_ICONERROR);
		_stprintf(incomingdir,_T("%sincoming"),appdir);
		if (!PathFileExists(GetIncomingDir()) && !::CreateDirectory(GetIncomingDir(),0)){
			strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_INCOMING), GetIncomingDir(), GetErrorMessage(GetLastError()));
			AfxMessageBox(strError, MB_ICONERROR);
		}
	}
	if (!PathFileExists(GetTempDir()) && !::CreateDirectory(GetTempDir(),0)) {
		CString strError;
		strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_TEMP), GetTempDir(), GetErrorMessage(GetLastError()));
		AfxMessageBox(strError, MB_ICONERROR);
		_stprintf(tempdir,_T("%stemp"),appdir);
		if (!PathFileExists(GetTempDir()) && !::CreateDirectory(GetTempDir(),0)){
			strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_TEMP), GetTempDir(), GetErrorMessage(GetLastError()));
			AfxMessageBox(strError, MB_ICONERROR);
		}
	}

	// Create 'skins' directory
	if (!PathFileExists(GetSkinProfileDir()) && !CreateDirectory(GetSkinProfileDir(), 0)) {
		m_strSkinProfileDir = appdir + _T("skins");
		CreateDirectory(GetSkinProfileDir(), 0);
	}

	// Create 'toolbars' directory
	if (!PathFileExists(GetToolbarBitmapFolderSettings()) && !CreateDirectory(GetToolbarBitmapFolderSettings(), 0)) {
		m_sToolbarBitmapFolder = appdir + _T("skins");
		CreateDirectory(GetToolbarBitmapFolderSettings(), 0);
	}
	// [TPT] - khaos::kmod+ Source Lists directory
	saveSourcesdir = CString(GetTempDir()) + _T("\\") + SOURCESAVEFOLDER;
	if (!PathFileExists(saveSourcesdir.GetBuffer()) && !::CreateDirectory(saveSourcesdir.GetBuffer(), 0)) {		
		CString strError;
		strError.Format(_T("Failed to create source lists directory \"%s\" - %s"), saveSourcesdir, GetErrorMessage(GetLastError()));
		AfxMessageBox(strError, MB_ICONERROR);
	}
	saveSourcesdir.ReleaseBuffer();
	// [TPT] - khaos::kmod-

	// [TPT] - Save ed2klinks
	ed2klinkdir = appdir + ED2KLINKFOLDER;
	if (!PathFileExists(ed2klinkdir.GetBuffer()) && !::CreateDirectory(ed2klinkdir.GetBuffer(), 0)) {
		CString strError;
		strError.Format(_T("Failed to create ed2k link directory \"%s\" - %s"), ed2klinkdir, GetErrorMessage(GetLastError()));
		AfxMessageBox(strError, MB_ICONERROR);
	}
	ed2klinkdir.ReleaseBuffer();
	// [TPT] - Save ed2klinks

	// [TPT] - Sivka AutoHL
	autoHLdir = CString(GetTempDir()) + _T("\\") + SIVKAFOLDER;
	if (!PathFileExists(autoHLdir.GetBuffer()) && !::CreateDirectory(autoHLdir.GetBuffer(), 0)) {
		CString strError;
		strError.Format(_T("Failed to create sivka extra lists directory \"%s\" - %s"), autoHLdir, GetErrorMessage(GetLastError()));
		AfxMessageBox(strError, MB_ICONERROR);
	}
	autoHLdir.ReleaseBuffer();
	// [TPT] - Sivka AutoHL
		

	if (((int*)userhash[0]) == 0 && ((int*)userhash[1]) == 0 && ((int*)userhash[2]) == 0 && ((int*)userhash[3]) == 0)
		CreateUserHash();
}

void CPreferences::Uninit()
{
	while (!catMap.IsEmpty())
	{
		Category_Struct* delcat = catMap.GetAt(0); 
		catMap.RemoveAt(0); 
		delete delcat;
	}
}

void CPreferences::SetStandartValues()
{
	CreateUserHash();

	WINDOWPLACEMENT defaultWPM;
	defaultWPM.length = sizeof(WINDOWPLACEMENT);
	defaultWPM.rcNormalPosition.left=10;defaultWPM.rcNormalPosition.top=10;
	defaultWPM.rcNormalPosition.right=700;defaultWPM.rcNormalPosition.bottom=500;
	defaultWPM.showCmd=SW_SHOW; // [TPT] - eWombat fix
	EmuleWindowPlacement=defaultWPM;
	versioncheckLastAutomatic=0;

//	Save();
}

bool CPreferences::IsTempFile(const CString& rstrDirectory, const CString& rstrName)
{
	if (CompareDirectories(rstrDirectory, GetTempDir()))
		return false;

	// do not share a file from the temp directory, if it matches one of the following patterns
	CString strNameLower(rstrName);
	strNameLower.MakeLower();
	strNameLower += _T("|"); // append an EOS character which we can query for
	static const LPCTSTR _apszNotSharedExts[] = {
		_T("%u.part") _T("%c"), 
		_T("%u.part.met") _T("%c"), 
		_T("%u.part.met") PARTMET_BAK_EXT _T("%c"), 
		_T("%u.part.met") PARTMET_TMP_EXT _T("%c") 
	};
	for (int i = 0; i < ARRSIZE(_apszNotSharedExts); i++){
		UINT uNum;
		TCHAR iChar;
		// "misuse" the 'scanf' function for a very simple pattern scanning.
		if (_stscanf(strNameLower, _apszNotSharedExts[i], &uNum, &iChar) == 2 && iChar == _T('|'))
			return true;
	}

	return false;
}

// SLUGFILLER: SafeHash
bool CPreferences::IsConfigFile(const CString& rstrDirectory, const CString& rstrName)
{
	if (CompareDirectories(rstrDirectory, configdir))
		return false;

	// do not share a file from the config directory, if it contains one of the following extensions
	static const LPCTSTR _apszNotSharedExts[] = { _T(".met.bak"), _T(".ini.old") };
	for (int i = 0; i < ARRSIZE(_apszNotSharedExts); i++){
		int iLen = _tcslen(_apszNotSharedExts[i]);
		if (rstrName.GetLength()>=iLen && rstrName.Right(iLen).CompareNoCase(_apszNotSharedExts[i])==0)
			return true;
	}

	// do not share following files from the config directory
	static const LPCTSTR _apszNotSharedFiles[] = 
	{
		_T("AC_SearchStrings.dat"),
		_T("AC_ServerMetURLs.dat"),
		_T("adresses.dat"),
		_T("category.ini"),
		_T("clients.met"),
		_T("cryptkey.dat"),
		_T("downloads.dat"),	// [TPT] - SLUGFILLER: indexPartFiles
		_T("emfriends.met"),
		_T("fileinfo.ini"),
		_T("ipfilter.dat"),
		_T("known.met"),
		_T("preferences.dat"),
		_T("preferences.ini"),
		_T("server.met"),
		_T("server.met.new"),
		_T("server_met.download"),
		_T("server_met.old"),
		_T("shareddir.dat"),
		_T("sharedsubdir.dat"),
		_T("staticservers.dat"),
		_T("virtualdirs.ini"),	// [TPT] - itsonlyme: virtualDirs
		_T("webservices.dat")
	};
	for (int i = 0; i < ARRSIZE(_apszNotSharedFiles); i++){
		if (rstrName.CompareNoCase(_apszNotSharedFiles[i])==0)
			return true;
	}

	return false;
}
// SLUGFILLER: SafeHash

// [TPT] - Maella [FAF] -Allow Bandwidth Settings in <1KB Incremements-z
float CPreferences::GetMaxDownload(){
    return (float)GetMaxDownloadInBytesPerSec()/1024.0f;
}

uint64 CPreferences::GetMaxDownloadInBytesPerSec(bool dynamic){
//dont be a Lam3r :)
	uint64 maxup;
	// [TPT]
	// Maella -Accurate measure of bandwidth: eDonkey data + control, network adapter-	
	uint32 eMuleOut;
	uint32 notUsed;
	theApp.pBandWidthControl->GetFullHistoryDatarates(notUsed, eMuleOut,
														notUsed, notUsed);

    if(dynamic && (thePrefs.IsDynUpEnabled() || thePrefs.GetNAFCEnable() && thePrefs.GetNAFCFullControl()) && theApp.uploadqueue->GetWaitingUserCount() != 0 && eMuleOut != 0) {
    	maxup = eMuleOut;
    } else {
    	maxup = GetMaxUpload()*1024;
    }

	if( maxup < 4*1024 )
		return (( (maxup < 10*1024) && (maxup*3 < maxdownload*1024) )? maxup*3 : maxdownload*1024);
	return (( (maxup < 10*1024) && (maxup*4 < maxdownload*1024) )? maxup*4 : maxdownload*1024);
}
// Maella end

// -khaos--+++> A whole bunch of methods!  Keep going until you reach the end tag.
void CPreferences::SaveStats(int bBackUp){
	// This function saves all of the new statistics in my addon.  It is also used to
	// save backups for the Reset Stats function, and the Restore Stats function (Which is actually LoadStats)
	// bBackUp = 0: DEFAULT; save to statistics.ini
	// bBackUp = 1: Save to statbkup.ini, which is used to restore after a reset
	// bBackUp = 2: Save to statbkuptmp.ini, which is temporarily created during a restore and then renamed to statbkup.ini

	CString fullpath(configdir);
	if (bBackUp == 1)
		fullpath += _T("statbkup.ini");
	else if (bBackUp == 2)
		fullpath += _T("statbkuptmp.ini");
	else
		fullpath += _T("statistics.ini");
	
	CIni ini(fullpath, _T("Statistics"));

	// Save cumulative statistics to preferences.ini, going in order as they appear in CStatisticsDlg::ShowStatistics.
	// We do NOT SET the values in prefs struct here.

    // Save Cum Down Data
	ini.WriteUInt64(_T("TotalDownloadedBytes"), theApp.pBandWidthControl->GeteMuleIn()+GetTotalDownloaded()); // [TPT]
	ini.WriteInt(_T("DownSuccessfulSessions"), cumDownSuccessfulSessions);
	ini.WriteInt(_T("DownFailedSessions"), cumDownFailedSessions);
	ini.WriteInt(_T("DownAvgTime"), (GetDownC_AvgTime() + GetDownS_AvgTime()) / 2);
	ini.WriteUInt64(_T("LostFromCorruption"), cumLostFromCorruption + sesLostFromCorruption);
	ini.WriteUInt64(_T("SavedFromCompression"), sesSavedFromCompression + cumSavedFromCompression);
	ini.WriteInt(_T("PartsSavedByICH"), cumPartsSavedByICH + sesPartsSavedByICH);

	ini.WriteUInt64(_T("DownData_EDONKEY"), GetCumDownData_EDONKEY());
	ini.WriteUInt64(_T("DownData_EDONKEYHYBRID"), GetCumDownData_EDONKEYHYBRID());
	ini.WriteUInt64(_T("DownData_EMULE"), GetCumDownData_EMULE());
	ini.WriteUInt64(_T("DownData_MLDONKEY"), GetCumDownData_MLDONKEY());
	ini.WriteUInt64(_T("DownData_LMULE"), GetCumDownData_EMULECOMPAT());
	ini.WriteUInt64(_T("DownData_AMULE"), GetCumDownData_AMULE());
	ini.WriteUInt64(_T("DownData_SHAREAZA"), GetCumDownData_SHAREAZA());
	ini.WriteUInt64(_T("DownData_URL"), GetCumDownData_URL());
	ini.WriteUInt64(_T("DownData_WEBCACHE"), GetCumDownData_WEBCACHE()); // [TPT] - WebCache	// Superlexx - webcache - statistics
	ini.WriteUInt64(_T("DownDataPort_4662"), GetCumDownDataPort_4662());
	ini.WriteUInt64(_T("DownDataPort_OTHER"), GetCumDownDataPort_OTHER());
	ini.WriteUInt64(_T("DownDataPort_PeerCache"), GetCumDownDataPort_PeerCache());

	ini.WriteUInt64(_T("DownOverheadTotal"),theStats.GetDownDataOverheadFileRequest() +
										theStats.GetDownDataOverheadSourceExchange() +
										theStats.GetDownDataOverheadServer() +
										theStats.GetDownDataOverheadKad() +
										theStats.GetDownDataOverheadOther() +
										GetDownOverheadTotal());
	ini.WriteUInt64(_T("DownOverheadFileReq"), theStats.GetDownDataOverheadFileRequest() + GetDownOverheadFileReq());
	ini.WriteUInt64(_T("DownOverheadSrcEx"), theStats.GetDownDataOverheadSourceExchange() + GetDownOverheadSrcEx());
	ini.WriteUInt64(_T("DownOverheadServer"), theStats.GetDownDataOverheadServer() + GetDownOverheadServer());
	ini.WriteUInt64(_T("DownOverheadKad"), theStats.GetDownDataOverheadKad() + GetDownOverheadKad());
	
	ini.WriteUInt64(_T("DownOverheadTotalPackets"), theStats.GetDownDataOverheadFileRequestPackets() + 
												theStats.GetDownDataOverheadSourceExchangePackets() + 
												theStats.GetDownDataOverheadServerPackets() + 
												theStats.GetDownDataOverheadKadPackets() + 
												theStats.GetDownDataOverheadOtherPackets() + 
												GetDownOverheadTotalPackets());
	ini.WriteUInt64(_T("DownOverheadFileReqPackets"), theStats.GetDownDataOverheadFileRequestPackets() + GetDownOverheadFileReqPackets());
	ini.WriteUInt64(_T("DownOverheadSrcExPackets"), theStats.GetDownDataOverheadSourceExchangePackets() + GetDownOverheadSrcExPackets());
	ini.WriteUInt64(_T("DownOverheadServerPackets"), theStats.GetDownDataOverheadServerPackets() + GetDownOverheadServerPackets());
	ini.WriteUInt64(_T("DownOverheadKadPackets"), theStats.GetDownDataOverheadKadPackets() + GetDownOverheadKadPackets());

	// Save Cumulative Upline Statistics
	ini.WriteUInt64(_T("TotalUploadedBytes"), theApp.pBandWidthControl->GeteMuleOut()+GetTotalUploaded()); // [TPT]
	ini.WriteInt(_T("UpSuccessfulSessions"), theApp.uploadqueue->GetSuccessfullUpCount() + GetUpSuccessfulSessions());
	ini.WriteInt(_T("UpFailedSessions"), theApp.uploadqueue->GetFailedUpCount() + GetUpFailedSessions());
	ini.WriteInt(_T("UpAvgTime"), (theApp.uploadqueue->GetAverageUpTime() + GetUpAvgTime())/2);
	ini.WriteUInt64(_T("UpData_EDONKEY"), GetCumUpData_EDONKEY());
	ini.WriteUInt64(_T("UpData_EDONKEYHYBRID"), GetCumUpData_EDONKEYHYBRID());
	ini.WriteUInt64(_T("UpData_EMULE"), GetCumUpData_EMULE());
	ini.WriteUInt64(_T("UpData_MLDONKEY"), GetCumUpData_MLDONKEY());
	ini.WriteUInt64(_T("UpData_LMULE"), GetCumUpData_EMULECOMPAT());
	ini.WriteUInt64(_T("UpData_AMULE"), GetCumUpData_AMULE());
	ini.WriteUInt64(_T("UpData_SHAREAZA"), GetCumUpData_SHAREAZA());
	ini.WriteUInt64(_T("UpDataPort_4662"), GetCumUpDataPort_4662());
	ini.WriteUInt64(_T("UpDataPort_OTHER"), GetCumUpDataPort_OTHER());
	ini.WriteUInt64(_T("UpDataPort_PeerCache"), GetCumUpDataPort_PeerCache());
	ini.WriteUInt64(_T("UpData_File"), GetCumUpData_File());
	ini.WriteUInt64(_T("UpData_Partfile"), GetCumUpData_Partfile());

	ini.WriteUInt64(_T("UpOverheadTotal"), theStats.GetUpDataOverheadFileRequest() + 
										theStats.GetUpDataOverheadSourceExchange() + 
										theStats.GetUpDataOverheadServer() + 
										theStats.GetUpDataOverheadKad() + 
										theStats.GetUpDataOverheadOther() + 
										GetUpOverheadTotal());
	ini.WriteUInt64(_T("UpOverheadFileReq"), theStats.GetUpDataOverheadFileRequest() + GetUpOverheadFileReq());
	ini.WriteUInt64(_T("UpOverheadSrcEx"), theStats.GetUpDataOverheadSourceExchange() + GetUpOverheadSrcEx());
	ini.WriteUInt64(_T("UpOverheadServer"), theStats.GetUpDataOverheadServer() + GetUpOverheadServer());
	ini.WriteUInt64(_T("UpOverheadKad"), theStats.GetUpDataOverheadKad() + GetUpOverheadKad());

	ini.WriteUInt64(_T("UpOverheadTotalPackets"), theStats.GetUpDataOverheadFileRequestPackets() + 
										theStats.GetUpDataOverheadSourceExchangePackets() + 
										theStats.GetUpDataOverheadServerPackets() + 
										theStats.GetUpDataOverheadKadPackets() + 
										theStats.GetUpDataOverheadOtherPackets() + 
										GetUpOverheadTotalPackets());
	ini.WriteUInt64(_T("UpOverheadFileReqPackets"), theStats.GetUpDataOverheadFileRequestPackets() + GetUpOverheadFileReqPackets());
	ini.WriteUInt64(_T("UpOverheadSrcExPackets"), theStats.GetUpDataOverheadSourceExchangePackets() + GetUpOverheadSrcExPackets());
	ini.WriteUInt64(_T("UpOverheadServerPackets"), theStats.GetUpDataOverheadServerPackets() + GetUpOverheadServerPackets());
	ini.WriteUInt64(_T("UpOverheadKadPackets"), theStats.GetUpDataOverheadKadPackets() + GetUpOverheadKadPackets());

	// Save Cumulative Connection Statistics
	float tempRate = 0.0F;
	
	// [TPT]
	// Maella -Accurate measure of bandwidth: eDonkey data + control, network adapter-
	uint32 eMuleIn;
	uint32 eMuleOut;
	uint32 notUsed;
	theApp.pBandWidthControl->GetDatarates(GetDatarateSamples(),
										   eMuleIn, notUsed,
										   eMuleOut, notUsed,
										   notUsed, notUsed);


	// Download Rate Average
	tempRate = theStats.GetSessionAvgDownloadRate();
	ini.WriteFloat(_T("ConnAvgDownRate"), tempRate);
	
	// Max Download Rate Average
	if (tempRate > GetConnMaxAvgDownRate())
		SetConnMaxAvgDownRate(tempRate);
	ini.WriteFloat(_T("ConnMaxAvgDownRate"), GetConnMaxAvgDownRate());
	
	// Max Download Rate
	tempRate = (float)eMuleIn / 1024.0f;
	if (tempRate > GetConnMaxDownRate())
		SetConnMaxDownRate(tempRate);
	ini.WriteFloat(_T("ConnMaxDownRate"), GetConnMaxDownRate());
	
	// Upload Rate Average
	tempRate = theStats.GetSessionAvgUploadRate();
	ini.WriteFloat(_T("ConnAvgUpRate"), tempRate);
	
	// Max Upload Rate Average
	if (tempRate > GetConnMaxAvgUpRate())
		SetConnMaxAvgUpRate(tempRate);
	ini.WriteFloat(_T("ConnMaxAvgUpRate"), GetConnMaxAvgUpRate());
	
	// Max Upload Rate
	tempRate = (float)eMuleOut / 1024.0f;
	if (tempRate > GetConnMaxUpRate())
		SetConnMaxUpRate(tempRate);
	ini.WriteFloat(_T("ConnMaxUpRate"), GetConnMaxUpRate());
	
	// Overall Run Time
	ini.WriteInt(_T("ConnRunTime"), (GetTickCount() - theStats.starttime)/1000 + GetConnRunTime());
	
	// Number of Reconnects
	ini.WriteInt(_T("ConnNumReconnects"), (theStats.reconnects>0) ? (theStats.reconnects - 1 + GetConnNumReconnects()) : GetConnNumReconnects());
	
	// Average Connections
	if (theApp.serverconnect->IsConnected())
		ini.WriteInt(_T("ConnAvgConnections"), (theApp.listensocket->GetAverageConnections() + cumConnAvgConnections)/2);
	
	// Peak Connections
	if (theApp.listensocket->GetPeakConnections() > cumConnPeakConnections)
		cumConnPeakConnections = theApp.listensocket->GetPeakConnections();
	ini.WriteInt(_T("ConnPeakConnections"), cumConnPeakConnections);
	
	// Max Connection Limit Reached
	if (theApp.listensocket->GetMaxConnectionReached() + cumConnMaxConnLimitReached > cumConnMaxConnLimitReached)
		ini.WriteInt(_T("ConnMaxConnLimitReached"), theApp.listensocket->GetMaxConnectionReached() + cumConnMaxConnLimitReached);
	
	// Time Stuff...
	ini.WriteInt(_T("ConnTransferTime"), GetConnTransferTime() + theStats.GetTransferTime());
	ini.WriteInt(_T("ConnUploadTime"), GetConnUploadTime() + theStats.GetUploadTime());
	ini.WriteInt(_T("ConnDownloadTime"), GetConnDownloadTime() + theStats.GetDownloadTime());
	ini.WriteInt(_T("ConnServerDuration"), GetConnServerDuration() + theStats.GetServerDuration());
	
	// Compare and Save Server Records
	uint32 servtotal, servfail, servuser, servfile, servlowiduser, servtuser, servtfile;
	float servocc;
	theApp.serverlist->GetStatus(servtotal, servfail, servuser, servfile, servlowiduser, servtuser, servtfile, servocc);
	
	if (servtotal - servfail > cumSrvrsMostWorkingServers)
		cumSrvrsMostWorkingServers = servtotal - servfail;
	ini.WriteInt(_T("SrvrsMostWorkingServers"), cumSrvrsMostWorkingServers);

	if (servtuser > cumSrvrsMostUsersOnline)
		cumSrvrsMostUsersOnline = servtuser;
	ini.WriteInt(_T("SrvrsMostUsersOnline"), cumSrvrsMostUsersOnline);

	if (servtfile > cumSrvrsMostFilesAvail)
		cumSrvrsMostFilesAvail = servtfile;
	ini.WriteInt(_T("SrvrsMostFilesAvail"), cumSrvrsMostFilesAvail);

	// Compare and Save Shared File Records
	if (theApp.sharedfiles->GetCount() > cumSharedMostFilesShared)
		cumSharedMostFilesShared = theApp.sharedfiles->GetCount();
	ini.WriteInt(_T("SharedMostFilesShared"), cumSharedMostFilesShared);

	uint64 bytesLargestFile = 0;
	uint64 allsize = theApp.sharedfiles->GetDatasize(bytesLargestFile);
	if (allsize > cumSharedLargestShareSize)
		cumSharedLargestShareSize = allsize;
	ini.WriteUInt64(_T("SharedLargestShareSize"), cumSharedLargestShareSize);
	if (bytesLargestFile > cumSharedLargestFileSize)
		cumSharedLargestFileSize = bytesLargestFile;
	ini.WriteUInt64(_T("SharedLargestFileSize"), cumSharedLargestFileSize);

	if (theApp.sharedfiles->GetCount() != 0) {
		uint64 tempint = allsize/theApp.sharedfiles->GetCount();
		if (tempint > cumSharedLargestAvgFileSize)
			cumSharedLargestAvgFileSize = tempint;
	}

	ini.WriteUInt64(_T("SharedLargestAvgFileSize"), cumSharedLargestAvgFileSize);
	ini.WriteUInt64(_T("statsDateTimeLastReset"), stat_datetimeLastReset);

	// If we are saving a back-up or a temporary back-up, return now.
	if (bBackUp != 0)
		return;
}

void CPreferences::SetRecordStructMembers() {

	// The purpose of this function is to be called from CStatisticsDlg::ShowStatistics()
	// This was easier than making a bunch of functions to interface with the record
	// members of the prefs struct from ShowStatistics.

	// This function is going to compare current values with previously saved records, and if
	// the current values are greater, the corresponding member of prefs will be updated.
	// We will not write to INI here, because this code is going to be called a lot more often
	// than SaveStats()  - Khaos

	CString buffer;

	// Servers
	uint32 servtotal, servfail, servuser, servfile, servlowiduser, servtuser, servtfile;
	float servocc;
	theApp.serverlist->GetStatus( servtotal, servfail, servuser, servfile, servlowiduser, servtuser, servtfile, servocc );
	if ((servtotal-servfail)>cumSrvrsMostWorkingServers) cumSrvrsMostWorkingServers = (servtotal-servfail);
	if (servtuser>cumSrvrsMostUsersOnline) cumSrvrsMostUsersOnline = servtuser;
	if (servtfile>cumSrvrsMostFilesAvail) cumSrvrsMostFilesAvail = servtfile;

	// Shared Files
	if (theApp.sharedfiles->GetCount()>cumSharedMostFilesShared) cumSharedMostFilesShared = theApp.sharedfiles->GetCount();
	uint64 bytesLargestFile = 0;
	uint64 allsize=theApp.sharedfiles->GetDatasize(bytesLargestFile);
	if (allsize>cumSharedLargestShareSize) cumSharedLargestShareSize = allsize;
	if (bytesLargestFile>cumSharedLargestFileSize) cumSharedLargestFileSize = bytesLargestFile;
	if (theApp.sharedfiles->GetCount() != 0) {
		uint64 tempint = allsize/theApp.sharedfiles->GetCount();
		if (tempint>cumSharedLargestAvgFileSize) cumSharedLargestAvgFileSize = tempint;
	}
} // SetRecordStructMembers()

void CPreferences::SaveCompletedDownloadsStat(){

	// This function saves the values for the completed
	// download members to INI.  It is called from
	// CPartfile::PerformFileComplete ...   - Khaos

	TCHAR* fullpath = new TCHAR[_tcslen(configdir)+MAX_PATH]; // i_a
	_stprintf(fullpath,_T("%sstatistics.ini"),configdir);
	
	CIni ini( fullpath, _T("Statistics") );

	delete[] fullpath;
	fullpath = NULL; // [TPT] - Patch

	ini.WriteInt(_T("DownCompletedFiles"),			GetDownCompletedFiles());
	ini.WriteInt(_T("DownSessionCompletedFiles"),	GetDownSessionCompletedFiles());
} // SaveCompletedDownloadsStat()

void CPreferences::Add2SessionTransferData(UINT uClientID, UINT uClientPort, BOOL bFromPF, 
										   BOOL bUpDown, uint32 bytes, bool sentToFriend)
{
	//	This function adds the transferred bytes to the appropriate variables,
	//	as well as to the totals for all clients. - Khaos
	//	PARAMETERS:
	//	uClientID - The identifier for which client software sent or received this data, eg SO_EMULE
	//	uClientPort - The remote port of the client that sent or received this data, eg 4662
	//	bFromPF - Applies only to uploads.  True is from partfile, False is from non-partfile.
	//	bUpDown - True is Up, False is Down
	//	bytes - Number of bytes sent by the client.  Subtract header before calling.

	switch (bUpDown){
		case true:
			//	Upline Data
			switch (uClientID){
				// Update session client breakdown stats for sent bytes...
				case SO_EMULE:
				case SO_OLDEMULE:		sesUpData_EMULE+=bytes;			break;
				case SO_EDONKEYHYBRID:	sesUpData_EDONKEYHYBRID+=bytes;	break;
				case SO_EDONKEY:		sesUpData_EDONKEY+=bytes;		break;
				case SO_MLDONKEY:		sesUpData_MLDONKEY+=bytes;		break;
				case SO_AMULE:			sesUpData_AMULE+=bytes;			break;
				case SO_SHAREAZA:		sesUpData_SHAREAZA+=bytes;		break;
				case SO_CDONKEY:
				case SO_LPHANT:
				case SO_XMULE:			sesUpData_EMULECOMPAT+=bytes;	break;
			}

			switch (uClientPort){
				// Update session port breakdown stats for sent bytes...
				case 4662:				sesUpDataPort_4662+=bytes;		break;
				case (UINT)-1:			sesUpDataPort_PeerCache+=bytes;	break;
				//case (UINT)-2:		sesUpDataPort_URL+=bytes;		break;
				default:				sesUpDataPort_OTHER+=bytes;		break;
			}

			if (bFromPF)				sesUpData_Partfile+=bytes;
			else						sesUpData_File+=bytes;

			//	Add to our total for sent bytes...
			// Maella -New bandwidth control- // [TPT]
			//theApp.UpdateSentBytes(bytes, sentToFriend); 
			if (sentToFriend)
				theApp.pBandWidthControl->AddeMuleOutFriend(bytes);

			break;

		case false:
			// Downline Data
			switch (uClientID){
                // Update session client breakdown stats for received bytes...
				case SO_EMULE:
				case SO_OLDEMULE:		sesDownData_EMULE+=bytes;		break;
				case SO_EDONKEYHYBRID:	sesDownData_EDONKEYHYBRID+=bytes;break;
				case SO_EDONKEY:		sesDownData_EDONKEY+=bytes;		break;
				case SO_MLDONKEY:		sesDownData_MLDONKEY+=bytes;	break;
				case SO_AMULE:			sesDownData_AMULE+=bytes;		break;
				case SO_SHAREAZA:		sesDownData_SHAREAZA+=bytes;	break;
				case SO_CDONKEY:
				case SO_LPHANT:
				case SO_XMULE:			sesDownData_EMULECOMPAT+=bytes;	break;
				case SO_URL:			sesDownData_URL+=bytes;			break;
				case SO_WEBCACHE:		sesDownData_WEBCACHE+=bytes;	break; // [TPT] - WebCache	// Superlexx - webcache - statistics
			}

			switch (uClientPort){
				// Update session port breakdown stats for received bytes...
				// For now we are only going to break it down by default and non-default.
				// A statistical analysis of all data sent from every single port/domain is
				// beyond the scope of this add-on.
				case 4662:				sesDownDataPort_4662+=bytes;	break;
				case (UINT)-1:			sesDownDataPort_PeerCache+=bytes;break;
				//case (UINT)-2:		sesDownDataPort_URL+=bytes;		break;
				default:				sesDownDataPort_OTHER+=bytes;	break;
			}

			//	Add to our total for received bytes...
			//theApp.UpdateReceivedBytes(bytes); // Maella -New bandwidth control- // [TPT]
	}
}

// Reset Statistics by Khaos

void CPreferences::ResetCumulativeStatistics(){

	// Save a backup so that we can undo this action
	SaveStats(1);

	// SET ALL CUMULATIVE STAT VALUES TO 0  :'-(

	totalDownloadedBytes=0;
	totalUploadedBytes=0;
	cumDownOverheadTotal=0;
	cumDownOverheadFileReq=0;
	cumDownOverheadSrcEx=0;
	cumDownOverheadServer=0;
	cumDownOverheadKad=0;
	cumDownOverheadTotalPackets=0;
	cumDownOverheadFileReqPackets=0;
	cumDownOverheadSrcExPackets=0;
	cumDownOverheadServerPackets=0;
	cumDownOverheadKadPackets=0;
	cumUpOverheadTotal=0;
	cumUpOverheadFileReq=0;
	cumUpOverheadSrcEx=0;
	cumUpOverheadServer=0;
	cumUpOverheadKad=0;
	cumUpOverheadTotalPackets=0;
	cumUpOverheadFileReqPackets=0;
	cumUpOverheadSrcExPackets=0;
	cumUpOverheadServerPackets=0;
	cumUpOverheadKadPackets=0;
	cumUpSuccessfulSessions=0;
	cumUpFailedSessions=0;
	cumUpAvgTime=0;
	cumUpData_EDONKEY=0;
	cumUpData_EDONKEYHYBRID=0;
	cumUpData_EMULE=0;
	cumUpData_MLDONKEY=0;
	cumUpData_AMULE=0;
	cumUpData_EMULECOMPAT=0;
	cumUpData_SHAREAZA=0;
	cumUpDataPort_4662=0;
	cumUpDataPort_OTHER=0;
	cumUpDataPort_PeerCache=0;
	cumDownCompletedFiles=0;
	cumDownSuccessfulSessions=0;
	cumDownFailedSessions=0;
	cumDownAvgTime=0;
	cumLostFromCorruption=0;
	cumSavedFromCompression=0;
	cumPartsSavedByICH=0;
	cumDownData_EDONKEY=0;
	cumDownData_EDONKEYHYBRID=0;
	cumDownData_EMULE=0;
	cumDownData_MLDONKEY=0;
	cumDownData_AMULE=0;
	cumDownData_EMULECOMPAT=0;
	cumDownData_SHAREAZA=0;
	cumDownData_URL=0;
	cumDownData_WEBCACHE=0; // [TPT] - WebCache	// Superlexx - webcache - statistics
	cumDownDataPort_4662=0;
	cumDownDataPort_OTHER=0;
	cumDownDataPort_PeerCache=0;
	cumConnAvgDownRate=0;
	cumConnMaxAvgDownRate=0;
	cumConnMaxDownRate=0;
	cumConnAvgUpRate=0;
	cumConnRunTime=0;
	cumConnNumReconnects=0;
	cumConnAvgConnections=0;
	cumConnMaxConnLimitReached=0;
	cumConnPeakConnections=0;
	cumConnDownloadTime=0;
	cumConnUploadTime=0;
	cumConnTransferTime=0;
	cumConnServerDuration=0;
	cumConnMaxAvgUpRate=0;
	cumConnMaxUpRate=0;
	cumSrvrsMostWorkingServers=0;
	cumSrvrsMostUsersOnline=0;
	cumSrvrsMostFilesAvail=0;
    cumSharedMostFilesShared=0;
	cumSharedLargestShareSize=0;
	cumSharedLargestAvgFileSize=0;

	// Set the time of last reset...
	time_t timeNow;
	time(&timeNow);
	stat_datetimeLastReset = (__int64)timeNow;

	// Save the reset stats
	SaveStats();
	theApp.emuledlg->statisticswnd->ShowStatistics(true);
}


// Load Statistics
// This used to be integrated in LoadPreferences, but it has been altered
// so that it can be used to load the backup created when the stats are reset.
// Last Modified: 2-22-03 by Khaos
bool CPreferences::LoadStats(int loadBackUp)
{
	// loadBackUp is 0 by default
	// loadBackUp = 0: Load the stats normally like we used to do in LoadPreferences
	// loadBackUp = 1: Load the stats from statbkup.ini and create a backup of the current stats.  Also, do not initialize session variables.
	// loadBackUp = 2: Load the stats from preferences.ini.old because the version has changed.
	CString sINI;
	CFileFind findBackUp;

	switch (loadBackUp) {
		case 0:{
			// for transition...
			if(PathFileExists(configdir+_T("statistics.ini")))
				sINI.Format(_T("%sstatistics.ini"), configdir);
			else
				sINI.Format(_T("%spreferences.ini"), configdir);

			break;
			   }
		case 1:
			sINI.Format(_T("%sstatbkup.ini"), configdir);
			if (!findBackUp.FindFile(sINI))
				return false;
			SaveStats(2); // Save our temp backup of current values to statbkuptmp.ini, we will be renaming it at the end of this function.
			break;
		case 2:
			sINI.Format(_T("%spreferences.ini.old"),configdir);
			break;
	}

	bool fileex = PathFileExists(sINI);
	CIni ini(sINI, _T("Statistics"));

	totalDownloadedBytes			= ini.GetUInt64(_T("TotalDownloadedBytes"));
	totalUploadedBytes				= ini.GetUInt64(_T("TotalUploadedBytes"));

	// Load stats for cumulative downline overhead
	cumDownOverheadTotal			= ini.GetUInt64(_T("DownOverheadTotal"));
	cumDownOverheadFileReq			= ini.GetUInt64(_T("DownOverheadFileReq"));
	cumDownOverheadSrcEx			= ini.GetUInt64(_T("DownOverheadSrcEx"));
	cumDownOverheadServer			= ini.GetUInt64(_T("DownOverheadServer"));
	cumDownOverheadKad				= ini.GetUInt64(_T("DownOverheadKad"));
	cumDownOverheadTotalPackets		= ini.GetUInt64(_T("DownOverheadTotalPackets"));
	cumDownOverheadFileReqPackets	= ini.GetUInt64(_T("DownOverheadFileReqPackets"));
	cumDownOverheadSrcExPackets		= ini.GetUInt64(_T("DownOverheadSrcExPackets"));
	cumDownOverheadServerPackets	= ini.GetUInt64(_T("DownOverheadServerPackets"));
	cumDownOverheadKadPackets		= ini.GetUInt64(_T("DownOverheadKadPackets"));

	// Load stats for cumulative upline overhead
	cumUpOverheadTotal				= ini.GetUInt64(_T("UpOverHeadTotal"));
	cumUpOverheadFileReq			= ini.GetUInt64(_T("UpOverheadFileReq"));
	cumUpOverheadSrcEx				= ini.GetUInt64(_T("UpOverheadSrcEx"));
	cumUpOverheadServer				= ini.GetUInt64(_T("UpOverheadServer"));
	cumUpOverheadKad				= ini.GetUInt64(_T("UpOverheadKad"));
	cumUpOverheadTotalPackets		= ini.GetUInt64(_T("UpOverHeadTotalPackets"));
	cumUpOverheadFileReqPackets		= ini.GetUInt64(_T("UpOverheadFileReqPackets"));
	cumUpOverheadSrcExPackets		= ini.GetUInt64(_T("UpOverheadSrcExPackets"));
	cumUpOverheadServerPackets		= ini.GetUInt64(_T("UpOverheadServerPackets"));
	cumUpOverheadKadPackets			= ini.GetUInt64(_T("UpOverheadKadPackets"));

	// Load stats for cumulative upline data
	cumUpSuccessfulSessions			= ini.GetInt(_T("UpSuccessfulSessions"));
	cumUpFailedSessions				= ini.GetInt(_T("UpFailedSessions"));
	cumUpAvgTime					= ini.GetInt(_T("UpAvgTime"));

	// Load cumulative client breakdown stats for sent bytes
	cumUpData_EDONKEY				= ini.GetUInt64(_T("UpData_EDONKEY"));
	cumUpData_EDONKEYHYBRID			= ini.GetUInt64(_T("UpData_EDONKEYHYBRID"));
	cumUpData_EMULE					= ini.GetUInt64(_T("UpData_EMULE"));
	cumUpData_MLDONKEY				= ini.GetUInt64(_T("UpData_MLDONKEY"));
	cumUpData_EMULECOMPAT			= ini.GetUInt64(_T("UpData_LMULE"));
	cumUpData_AMULE					= ini.GetUInt64(_T("UpData_AMULE"));
	cumUpData_SHAREAZA				= ini.GetUInt64(_T("UpData_SHAREAZA"));

	// Load cumulative port breakdown stats for sent bytes
	cumUpDataPort_4662				= ini.GetUInt64(_T("UpDataPort_4662"));
	cumUpDataPort_OTHER				= ini.GetUInt64(_T("UpDataPort_OTHER"));
	cumUpDataPort_PeerCache			= ini.GetUInt64(_T("UpDataPort_PeerCache"));

	// Load cumulative source breakdown stats for sent bytes
	cumUpData_File					= ini.GetUInt64(_T("UpData_File"));
	cumUpData_Partfile				= ini.GetUInt64(_T("UpData_Partfile"));

	// Load stats for cumulative downline data
	cumDownCompletedFiles			= ini.GetInt(_T("DownCompletedFiles"));
	cumDownSuccessfulSessions		= ini.GetInt(_T("DownSuccessfulSessions"));
	cumDownFailedSessions			= ini.GetInt(_T("DownFailedSessions"));
	cumDownAvgTime					= ini.GetInt(_T("DownAvgTime"));

	// Cumulative statistics for saved due to compression/lost due to corruption
	cumLostFromCorruption			= ini.GetUInt64(_T("LostFromCorruption"));
	cumSavedFromCompression			= ini.GetUInt64(_T("SavedFromCompression"));
	cumPartsSavedByICH				= ini.GetInt(_T("PartsSavedByICH"));

	// Load cumulative client breakdown stats for received bytes
	cumDownData_EDONKEY				= ini.GetUInt64(_T("DownData_EDONKEY"));
	cumDownData_EDONKEYHYBRID		= ini.GetUInt64(_T("DownData_EDONKEYHYBRID"));
	cumDownData_EMULE				= ini.GetUInt64(_T("DownData_EMULE"));
	cumDownData_MLDONKEY			= ini.GetUInt64(_T("DownData_MLDONKEY"));
	cumDownData_EMULECOMPAT			= ini.GetUInt64(_T("DownData_LMULE"));
	cumDownData_AMULE				= ini.GetUInt64(_T("DownData_AMULE"));
	cumDownData_SHAREAZA			= ini.GetUInt64(_T("DownData_SHAREAZA"));
	cumDownData_URL					= ini.GetUInt64(_T("DownData_URL"));
	cumDownData_WEBCACHE			= ini.GetUInt64(_T("DownData_WEBCACHE")); // [TPT] - WebCache	// Superlexx - webcache - statistics

	// Load cumulative port breakdown stats for received bytes
	cumDownDataPort_4662			= ini.GetUInt64(_T("DownDataPort_4662"));
	cumDownDataPort_OTHER			= ini.GetUInt64(_T("DownDataPort_OTHER"));
	cumDownDataPort_PeerCache		= ini.GetUInt64(_T("DownDataPort_PeerCache"));

	// Load stats for cumulative connection data
	cumConnAvgDownRate				= ini.GetFloat(_T("ConnAvgDownRate"));
	cumConnMaxAvgDownRate			= ini.GetFloat(_T("ConnMaxAvgDownRate"));
	cumConnMaxDownRate				= ini.GetFloat(_T("ConnMaxDownRate"));
	cumConnAvgUpRate				= ini.GetFloat(_T("ConnAvgUpRate"));
	cumConnMaxAvgUpRate				= ini.GetFloat(_T("ConnMaxAvgUpRate"));
	cumConnMaxUpRate				= ini.GetFloat(_T("ConnMaxUpRate"));
	cumConnRunTime					= ini.GetUInt64(_T("ConnRunTime"));
	cumConnTransferTime				= ini.GetInt(_T("ConnTransferTime"));
	cumConnDownloadTime				= ini.GetInt(_T("ConnDownloadTime"));
	cumConnUploadTime				= ini.GetInt(_T("ConnUploadTime"));
	cumConnServerDuration			= ini.GetInt(_T("ConnServerDuration"));
	cumConnNumReconnects			= ini.GetInt(_T("ConnNumReconnects"));
	cumConnAvgConnections			= ini.GetInt(_T("ConnAvgConnections"));
	cumConnMaxConnLimitReached		= ini.GetInt(_T("ConnMaxConnLimitReached"));
	cumConnPeakConnections			= ini.GetInt(_T("ConnPeakConnections"));

	// Load date/time of last reset
	stat_datetimeLastReset			= ini.GetUInt64(_T("statsDateTimeLastReset"));

	// Smart Load For Restores - Don't overwrite records that are greater than the backed up ones
	if (loadBackUp == 1)
	{
		// Load records for servers / network
		if ((UINT)ini.GetInt(_T("SrvrsMostWorkingServers")) > cumSrvrsMostWorkingServers)
			cumSrvrsMostWorkingServers = ini.GetInt(_T("SrvrsMostWorkingServers"));

		if ((UINT)ini.GetInt(_T("SrvrsMostUsersOnline")) > cumSrvrsMostUsersOnline)
			cumSrvrsMostUsersOnline = ini.GetInt(_T("SrvrsMostUsersOnline"));

		if ((UINT)ini.GetInt(_T("SrvrsMostFilesAvail")) > cumSrvrsMostFilesAvail)
			cumSrvrsMostFilesAvail = ini.GetInt(_T("SrvrsMostFilesAvail"));

		// Load records for shared files
		if ((UINT)ini.GetInt(_T("SharedMostFilesShared")) > cumSharedMostFilesShared)
			cumSharedMostFilesShared =	ini.GetInt(_T("SharedMostFilesShared"));

		uint64 temp64 = ini.GetUInt64(_T("SharedLargestShareSize"));
		if (temp64 > cumSharedLargestShareSize)
			cumSharedLargestShareSize = temp64;

		temp64 = ini.GetUInt64(_T("SharedLargestAvgFileSize"));
		if (temp64 > cumSharedLargestAvgFileSize)
			cumSharedLargestAvgFileSize = temp64;

		temp64 = ini.GetUInt64(_T("SharedLargestFileSize"));
		if (temp64 > cumSharedLargestFileSize)
			cumSharedLargestFileSize = temp64;

		// Check to make sure the backup of the values we just overwrote exists.  If so, rename it to the backup file.
		// This allows us to undo a restore, so to speak, just in case we don't like the restored values...
		CString sINIBackUp;
		sINIBackUp.Format(_T("%sstatbkuptmp.ini"), configdir);
		if (findBackUp.FindFile(sINIBackUp)){
			CFile::Remove(sINI);				// Remove the backup that we just restored from
			CFile::Rename(sINIBackUp, sINI);	// Rename our temporary backup to the normal statbkup.ini filename.
		}

		// Since we know this is a restore, now we should call ShowStatistics to update the data items to the new ones we just loaded.
		// Otherwise user is left waiting around for the tick counter to reach the next automatic update (Depending on setting in prefs)
		theApp.emuledlg->statisticswnd->ShowStatistics();
	}
	// Stupid Load -> Just load the values.
	else
	{
		// Load records for servers / network
		cumSrvrsMostWorkingServers	= ini.GetInt(_T("SrvrsMostWorkingServers"));
		cumSrvrsMostUsersOnline		= ini.GetInt(_T("SrvrsMostUsersOnline"));
		cumSrvrsMostFilesAvail		= ini.GetInt(_T("SrvrsMostFilesAvail"));

		// Load records for shared files
		cumSharedMostFilesShared	= ini.GetInt(_T("SharedMostFilesShared"));
		cumSharedLargestShareSize	= ini.GetUInt64(_T("SharedLargestShareSize"));
		cumSharedLargestAvgFileSize = ini.GetUInt64(_T("SharedLargestAvgFileSize"));
		cumSharedLargestFileSize	= ini.GetUInt64(_T("SharedLargestFileSize"));

		// Initialize new session statistic variables...
		sesDownCompletedFiles		= 0;
		
		sesUpData_EDONKEY			= 0;
		sesUpData_EDONKEYHYBRID		= 0;
		sesUpData_EMULE				= 0;
		sesUpData_MLDONKEY			= 0;
		sesUpData_AMULE				= 0;
		sesUpData_EMULECOMPAT		= 0;
		sesUpData_SHAREAZA			= 0;
		sesUpDataPort_4662			= 0;
		sesUpDataPort_OTHER			= 0;
		sesUpDataPort_PeerCache		= 0;

		sesDownData_EDONKEY			= 0;
		sesDownData_EDONKEYHYBRID	= 0;
		sesDownData_EMULE			= 0;
		sesDownData_MLDONKEY		= 0;
		sesDownData_AMULE			= 0;
		sesDownData_EMULECOMPAT		= 0;
		sesDownData_SHAREAZA		= 0;
		sesDownData_URL				= 0;
		// [TPT] - WebCache	
		sesDownData_WEBCACHE		= 0; // Superlexx - webcache - statistics
		//[TPT] - Webcache 1.9 beta3
		ses_WEBCACHEREQUESTS		= 0; //jp webcache statistics (from proxy)
		ses_successfull_WCDOWNLOADS	= 0; //jp webcache statistics (from proxy)
		ses_PROXYREQUESTS           = 0; //jp webcache statistics (via proxy)
		ses_successfullPROXYREQUESTS= 0; //jp webcache statistics (via proxy)
		// [TPT] - WebCache	

		sesDownDataPort_4662		= 0;
		sesDownDataPort_OTHER		= 0;
		sesDownDataPort_PeerCache	= 0;

		sesDownSuccessfulSessions	= 0;
		sesDownFailedSessions		= 0;
		sesPartsSavedByICH			= 0;
	}

	if (!fileex)
	{
		time_t timeNow;
		time(&timeNow);
		stat_datetimeLastReset = (__int64)timeNow;
	}

	return true;
}

// This formats the UTC long value that is saved for stat_datetimeLastReset
// If this value is 0 (Never reset), then it returns Unknown.
CString CPreferences::GetStatsLastResetStr(bool formatLong)
{
	// formatLong dictates the format of the string returned.
	// For example...
	// true: DateTime format from the .ini
	// false: DateTime format from the .ini for the log
	CString	returnStr;
	if (GetStatsLastResetLng()) {
		tm *statsReset;
		TCHAR szDateReset[128];
		time_t lastResetDateTime = (time_t) GetStatsLastResetLng();
		statsReset = localtime(&lastResetDateTime);
		if (statsReset){
			_tcsftime(szDateReset, ARRSIZE(szDateReset), formatLong ? GetDateTimeFormat() : GetDateTimeFormat4Log(), statsReset);
			returnStr = szDateReset;
		}
	}
	if (returnStr.IsEmpty())
		returnStr = GetResString(IDS_UNKNOWN);
	return returnStr;
}

// <-----khaos-

bool CPreferences::Save(){

	bool error = false;
	TCHAR* fullpath = new TCHAR[_tcslen(configdir)+MAX_PATH]; // i_a
	_stprintf(fullpath,_T("%spreferences.dat"),configdir);

	FILE* preffile = _tfsopen(fullpath,_T("wb"), _SH_DENYWR);
	prefsExt->version = PREFFILE_VERSION;

	// -khaos--+++> Don't save stats if preferences.ini doesn't exist yet (Results in unhandled exception).
	_stprintf(fullpath,_T("%spreferences.ini"),configdir);
	bool bSaveStats = true;
	if (!PathFileExists(fullpath))
		bSaveStats = false;
	// <-----khaos-

	delete[] fullpath;
	fullpath = NULL; // [TPT] - Patch
	if (preffile){
		prefsExt->version=PREFFILE_VERSION;
		prefsExt->EmuleWindowPlacement=EmuleWindowPlacement;
		md4cpy(prefsExt->userhash, userhash);

		error = fwrite(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile);
		if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
			fflush(preffile); // flush file stream buffers to disk buffers
			(void)_commit(_fileno(preffile)); // commit disk buffers to disk
		}
		fclose(preffile);
	}
	else
		error = true;

	SavePreferences();
	SaveVirtualDirs();	// [TPT] - itsonlyme: virtualDirs
	// -khaos--+++> SaveStats is now called here instead of from SavePreferences...
	if (bSaveStats)
		SaveStats();
	// <-----khaos-
	// [TPT] - khaos::categorymod+ We need to SaveCats() each time we exit eMule.
	SaveCats();
	// [TPT] - khaos::categorymod-

	fullpath = new TCHAR[_tcslen(configdir)+14];
	_stprintf(fullpath,_T("%sshareddir.dat"),configdir);
	CStdioFile sdirfile;
	if (sdirfile.Open(fullpath, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyWrite | CFile::typeBinary))
	{
		try{
			// write Unicode byte-order mark 0xFEFF
			WORD wBOM = 0xFEFF;
			sdirfile.Write(&wBOM, sizeof(wBOM));

			for (POSITION pos = shareddir_list.GetHeadPosition();pos != 0;){
				sdirfile.WriteString(shareddir_list.GetNext(pos).GetBuffer());
				sdirfile.Write(_T("\r\n"), sizeof(TCHAR)*2);
			}
			if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
				sdirfile.Flush(); // flush file stream buffers to disk buffers
				if (_commit(_fileno(sdirfile.m_pStream)) != 0) // commit disk buffers to disk
					AfxThrowFileException(CFileException::hardIO, GetLastError(), sdirfile.GetFileName());
			}
			sdirfile.Close();
		}
		catch(CFileException* error){
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer,ARRSIZE(buffer));
			if (thePrefs.GetVerbose())
				AddDebugLogLine(true,_T("Failed to save %s - %s"), fullpath, buffer);
			error->Delete();
		}
	}
	else
		error = true;
	delete[] fullpath;
	fullpath=NULL;
	::CreateDirectory(GetIncomingDir(),0);
	::CreateDirectory(GetTempDir(),0);
	return error;
}

void CPreferences::CreateUserHash()
{
	for (int i = 0; i < 8; i++)
	{
		uint16 random = GetRandomUInt16();
		MEMCOPY(&userhash[i*2], &random, 2);
	}

	// mark as emule client. that will be need in later version
	userhash[5] = 14;
	userhash[14] = 111;
}

int CPreferences::GetColumnWidth(Table t, int index)
{
	switch(t) {
	case tableDownload:
		return downloadColumnWidths[index];
	// [TPT] - TBH Transfers Window Style
	case tableDownloadClients:
		return downloadClientsColumnWidths[index];
	case tableUpload:
		return uploadColumnWidths[index];
	case tableQueue:
		return queueColumnWidths[index];
	case tableSearch:
		return searchColumnWidths[index];
	case tableShared:
		return sharedColumnWidths[index];
	case tableServer:
		return serverColumnWidths[index];
	case tableClientList:
		return clientListColumnWidths[index];
	case tableFilenames:
		return FilenamesListColumnWidths[index];
	// [TPT] - MoNKi: -Downloaded History-
	case tableHistory:
		return m_HistoryColumnWidths[index];
	// [TPT] - MoNKi: -Downloaded History-
	// [TPT] - Friend State Column
	case tableFriendList:
		return friendListColumnWidths[index];
	// [TPT] - Friend State Column
	}
	return 0;
}

void CPreferences::SetColumnWidth(Table t, int index, int width) {
	switch(t) {
	case tableDownload:
		downloadColumnWidths[index] = width;
		break;
	// [TPT] - TBH Transfers Window Style
	case tableDownloadClients:
		downloadClientsColumnWidths[index] = width;
		break;
	case tableUpload:
		uploadColumnWidths[index] = width;
		break;
	case tableQueue:
		queueColumnWidths[index] = width;
		break;
	case tableSearch:
		searchColumnWidths[index] = width;
		break;
	case tableShared:
		sharedColumnWidths[index] = width;
		break;
	case tableServer:
		serverColumnWidths[index] = width;
		break;
	case tableClientList:
		clientListColumnWidths[index] = width;
		break;
	case tableFilenames:
		FilenamesListColumnWidths[index] = width;
		break;
	// [TPT] - MoNKi: -Downloaded History-
	case tableHistory:
		m_HistoryColumnWidths[index] = width;
		break;
	// [TPT] - MoNKi: -Downloaded History-
	// [TPT] - Friend State Column
	case tableFriendList:
		friendListColumnWidths[index] = width;
		break;
	// [TPT] - Friend State Column
	}
}

BOOL CPreferences::GetColumnHidden(Table t, int index)
{
	switch(t) {
	case tableDownload:
		return downloadColumnHidden[index];
	// [TPT] - TBH Transfers Window Style
	case tableDownloadClients:
		return downloadClientsColumnHidden[index];
	case tableUpload:
		return uploadColumnHidden[index];
	case tableQueue:
		return queueColumnHidden[index];
	case tableSearch:
		return searchColumnHidden[index];
	case tableShared:
		return sharedColumnHidden[index];
	case tableServer:
		return serverColumnHidden[index];
	case tableClientList:
		return clientListColumnHidden[index];
	case tableFilenames:
		return FilenamesListColumnHidden[index];
	// [TPT] - MoNKi: -Downloaded History-
	case tableHistory:
		return m_HistoryColumnHidden[index];
	// [TPT] - MoNKi: -Downloaded History-
	// [TPT] - Friend State Column
	case tableFriendList:
		return friendListColumnHidden[index];
	// [TPT] - Friend State Column
	}
	return FALSE;
}

void CPreferences::SetColumnHidden(Table t, int index, BOOL bHidden) {
	switch(t) {
	case tableDownload:
		downloadColumnHidden[index] = bHidden;
		break;
	// [TPT] - TBH Transfers Window Style
	case tableDownloadClients:
		downloadClientsColumnHidden[index] = bHidden;
		break;
	case tableUpload:
		uploadColumnHidden[index] = bHidden;
		break;
	case tableQueue:
		queueColumnHidden[index] = bHidden;
		break;
	case tableSearch:
		searchColumnHidden[index] = bHidden;
		break;
	case tableShared:
		sharedColumnHidden[index] = bHidden;
		break;
	case tableServer:
		serverColumnHidden[index] = bHidden;
		break;
	case tableClientList:
		clientListColumnHidden[index] = bHidden;
		break;
	case tableFilenames:
		FilenamesListColumnHidden[index] = bHidden;
		break;
	// [TPT] - MoNKi: -Downloaded History-
	case tableHistory:
		m_HistoryColumnHidden[index] = bHidden;
		break;
	// [TPT] - MoNKi: -Downloaded History-
	// [TPT] - Friend State Column
	case tableFriendList:
		friendListColumnHidden[index] = bHidden;
		break;
	// [TPT] - Friend State Column
	}
}

int CPreferences::GetColumnOrder(Table t, int index)
{
	switch(t) {
	case tableDownload:
		return downloadColumnOrder[index];
	// [TPT] - TBH Transfers Window Style
	case tableDownloadClients:
		return downloadClientsColumnOrder[index];
	case tableUpload:
		return uploadColumnOrder[index];
	case tableQueue:
		return queueColumnOrder[index];
	case tableSearch:
		return searchColumnOrder[index];
	case tableShared:
		return sharedColumnOrder[index];
	case tableServer:
		return serverColumnOrder[index];
	case tableClientList:
		return clientListColumnOrder[index];
	case tableFilenames:
		return FilenamesListColumnOrder[index];
	// [TPT] - MoNKi: -Downloaded History-
	case tableHistory:
		return m_HistoryColumnOrder[index];
	// [TPT] - MoNKi: -Downloaded History-
	// [TPT] - Friend State Column
	case tableFriendList:
		return friendListColumnOrder[index];
	// [TPT] - Friend State Column
	}
	return 0;
}

void CPreferences::SetColumnOrder(Table t, INT *piOrder) {
	switch(t) {
	case tableDownload:
		MEMCOPY(downloadColumnOrder, piOrder, sizeof(downloadColumnOrder));
		break;
	// [TPT] - TBH Transfers Window Style
	case tableDownloadClients:
		MEMCOPY(downloadClientsColumnOrder, piOrder, sizeof(downloadClientsColumnOrder));
		break;
	case tableUpload:
		MEMCOPY(uploadColumnOrder, piOrder, sizeof(uploadColumnOrder));
		break;
	case tableQueue:
		MEMCOPY(queueColumnOrder, piOrder, sizeof(queueColumnOrder));
		break;
	case tableSearch:
		MEMCOPY(searchColumnOrder, piOrder, sizeof(searchColumnOrder));
		break;
	case tableShared:
		MEMCOPY(sharedColumnOrder, piOrder, sizeof(sharedColumnOrder));
		break;
	case tableServer:
		MEMCOPY(serverColumnOrder, piOrder, sizeof(serverColumnOrder));
		break;
	case tableClientList:
		MEMCOPY(clientListColumnOrder, piOrder, sizeof(clientListColumnOrder));
		break;
	case tableFilenames:
		MEMCOPY(FilenamesListColumnOrder, piOrder, sizeof(FilenamesListColumnOrder));
		break;
	// [TPT] - MoNKi: -Downloaded History-
	case tableHistory:
		MEMCOPY(m_HistoryColumnOrder, piOrder, sizeof(m_HistoryColumnOrder));
		break;
	// [TPT] - MoNKi: -Downloaded History-
	// [TPT] - Friend State Column
	case tableFriendList:
		MEMCOPY(friendListColumnOrder, piOrder, sizeof(friendListColumnOrder));
		break;
	// [TPT] - Friend State Column
	}
}

int CPreferences::GetRecommendedMaxConnections() {
	int iRealMax = ::GetMaxWindowsTCPConnections();
	if(iRealMax == -1 || iRealMax > 520)
		return 500;

	if(iRealMax < 20)
		return iRealMax;

	if(iRealMax <= 256)
		return iRealMax - 10;

	return iRealMax - 20;
}

void CPreferences::SavePreferences()
{
	USES_CONVERSION;
	CString buffer;
	TCHAR* fullpath = new TCHAR[_tcslen(configdir)+MAX_PATH]; // i_a
	_stprintf(fullpath,_T("%spreferences.ini"),configdir);
	
	CIni ini( fullpath, _T("eMule") );
	delete[] fullpath;
	fullpath=NULL;
	//---
	ini.WriteString(_T("AppVersion"), theApp.m_strCurVersionLong);
	//---

#ifdef _DEBUG
	ini.WriteInt(_T("DebugHeap"), m_iDbgHeap);
#endif

	ini.WriteStringUTF8(_T("Nick"), strNick);
	ini.WriteString(_T("IncomingDir"), incomingdir);
	ini.WriteString(_T("TempDir"), tempdir);

	// [TPT] Maella [FAF] -Allow Bandwidth Settings in <1KB Incremements-	
    ini.WriteFloat(_T("MinUpload"), minupload);	
	ini.WriteFloat(_T("MaxUpload"),maxupload);
	ini.WriteFloat(_T("MaxDownload"),maxdownload);
	ini.WriteInt(_T("MaxConnections"),maxconnections);
	ini.WriteInt(_T("MaxHalfConnections"),maxhalfconnections);
	ini.WriteInt(_T("Port"),port);
	ini.WriteInt(_T("UDPPort"),udpport);
	ini.WriteInt(_T("ServerUDPPort"), nServerUDPPort);
	// [TPT] - WebCache		
	// yonatan http start 
	ini.WriteString(_T("webcacheName"), webcacheName);
	ini.WriteInt(_T("webcachePort"), webcachePort);
	ini.WriteInt(_T("WebCacheBlockLimit"), webcacheBlockLimit);
	ini.WriteBool(_T("PersistentConnectionsForProxyDownloads"), PersistentConnectionsForProxyDownloads); //JP persistent proxy connections
	ini.WriteBool(_T("WCAutoupdate"), WCAutoupdate); //JP WCAutoupdate
	ini.WriteBool(_T("WebCacheExtraTimeout"), webcacheExtraTimeout);
	ini.WriteBool(_T("WebCacheCachesLocalTraffic"), webcacheCachesLocalTraffic);
	ini.WriteBool(_T("WebCacheEnabled"), webcacheEnabled);
	ini.WriteBool(_T("detectWebcacheOnStart"), detectWebcacheOnStart); // jp detect webcache on startup
	ini.WriteUInt64(_T("WebCacheLastSearch"), (uint64)webcacheLastSearch);
	ini.WriteUInt64(_T("WebCacheLastGlobalIP"), (uint64)webcacheLastGlobalIP);
	ini.WriteString(_T("WebCacheLastResolvedName"), webcacheLastResolvedName);
	ini.WriteUInt64(_T("webcacheTrustLevel"), (uint64)webcacheTrustLevel);
	// yonatan http end
	// [TPT] - WebCache	
	ini.WriteInt(_T("MaxSourcesPerFile"),maxsourceperfile );
	ini.WriteWORD(_T("Language"),m_wLanguageID);
	ini.WriteInt(_T("SeeShare"),m_iSeeShares);
	ini.WriteInt(_T("ToolTipDelay"),m_iToolDelayTime);
	ini.WriteInt(_T("StatGraphsInterval"),trafficOMeterInterval);
	ini.WriteInt(_T("StatsInterval"),statsInterval);
	ini.WriteFloat(_T("DownloadCapacity"),maxGraphDownloadRate);
	ini.WriteFloat(_T("UploadCapacity"),maxGraphUploadRate);
	ini.WriteInt(_T("DeadServerRetry"),deadserverretries);
	ini.WriteInt(_T("ServerKeepAliveTimeout"),m_dwServerKeepAliveTimeout);
	ini.WriteInt(_T("SplitterbarPosition"),splitterbarPosition); // [TPT]
	ini.WriteInt(_T("SplitterbarPositionStat"),splitterbarPositionStat+1,_T("eMule"));
	ini.WriteInt(_T("SplitterbarPositionStat_HL"),splitterbarPositionStat_HL+1,_T("eMule"));
	ini.WriteInt(_T("SplitterbarPositionStat_HR"),splitterbarPositionStat_HR+1,_T("eMule"));
	ini.WriteInt(_T("SplitterbarPositionFriend"),splitterbarPositionFriend,_T("eMule"));
	ini.WriteInt(_T("SplitterbarPositionIRC"),splitterbarPositionIRC+2,_T("eMule"));
	ini.WriteInt(_T("TransferWnd2"),m_uTransferWnd2);
	ini.WriteInt(_T("VariousStatisticsMaxValue"),statsMax);
	ini.WriteInt(_T("StatsAverageMinutes"),statsAverageMinutes);
	ini.WriteInt(_T("MaxConnectionsPerFiveSeconds"),MaxConperFive);
	ini.WriteInt(_T("Check4NewVersionDelay"),versioncheckdays);

	ini.WriteBool(_T("Reconnect"),reconnect);
	ini.WriteBool(_T("Scoresystem"),scorsystem);
	ini.WriteBool(_T("Serverlist"),autoserverlist);
	ini.WriteBool(_T("UpdateNotifyTestClient"),updatenotify);
	ini.WriteBool(_T("MinToTray"),mintotray);
	ini.WriteBool(_T("AddServersFromServer"),addserversfromserver);
	ini.WriteBool(_T("AddServersFromClient"),addserversfromclient);
	ini.WriteBool(_T("Splashscreen"),splashscreen);
	ini.WriteBool(_T("BringToFront"),bringtoforeground);
	ini.WriteBool(_T("TransferDoubleClick"),transferDoubleclick);
	ini.WriteBool(_T("BeepOnError"),beepOnError);
	ini.WriteBool(_T("ConfirmExit"),confirmExit);
	ini.WriteBool(_T("FilterBadIPs"),filterLANIPs);
    ini.WriteBool(_T("Autoconnect"),autoconnect);
	ini.WriteBool(_T("OnlineSignature"),onlineSig);
	ini.WriteBool(_T("StartupMinimized"),startMinimized);
	ini.WriteBool(_T("AutoStart"),m_bAutoStart);
	ini.WriteInt(_T("LastMainWndDlgID"),m_iLastMainWndDlgID);
	ini.WriteInt(_T("LastLogPaneID"),m_iLastLogPaneID);
	ini.WriteBool(_T("SafeServerConnect"),safeServerConnect);
	ini.WriteBool(_T("ShowRatesOnTitle"),showRatesInTitle);
	ini.WriteBool(_T("IndicateRatings"),indicateratings);
	ini.WriteBool(_T("WatchClipboard4ED2kFilelinks"),watchclipboard);
	ini.WriteInt(_T("SearchMethod"),m_iSearchMethod);
	ini.WriteBool(_T("CheckDiskspace"),checkDiskspace);	// SLUGFILLER: checkDiskspace
	ini.WriteInt(_T("MinFreeDiskSpace"),m_uMinFreeDiskSpace);
	ini.WriteBool(_T("SparsePartFiles"),m_bSparsePartFiles);
	// Maella -Enable/Disable source exchange in preference- (Tarod)
	ini.WriteBool(_T("DisableXS"), disablexs);
	// Maella end

	// Maella -Reask sources after IP change- (idea Xman)
	ini.WriteBool(_T("ReaskSourceAfterIPChange"), reaskSourceAfterIPChange);
	// Maella end

	// Maella -Graph: display zoom-
	ini.WriteInt(_T("ZoomFactor"), zoomFactor);
	// Maella end

	// Maella -Filter verbose messages-
	ini.WriteBool(_T("BlockHandshakeMsg"), blockHandshakeMsg);	 // [TPT] - Filter Handshake messages
	ini.WriteBool(_T("BlockPhoenixMsg"), blockPhoenixMsg);	 // [TPT] - Filter own messages	
	ini.WriteBool(_T("BlockDeadSourcesMsg"), blockDeadSources); // [TPT] - Filter dead sources	
	ini.WriteBool(_T("BlockUploadEndMsg"), blockUploadEndMsg);		
	ini.WriteBool(_T("BlockMaellaSpecificMsg"), blockMaellaSpecificMsg);
	ini.WriteBool(_T("BlockMaellaSpecificDebugMsg"), blockMaellaSpecificDebugMsg);
	// Maella end

	// Maella -MTU Configuration-
	ini.WriteInt(_T("MTU"), MTU);
	// Maella end

	// Maella -Minimum Upload Slot-
	ini.WriteInt(_T("MinUploadSlot"), minUploadSlot);
	// Maella end

	// Maella -New bandwidth control-
	ini.WriteInt(_T("SendSocketBufferSize"), sendSocketBufferSize);
	// Maella end

	// Maella -Network Adapter Feedback Control-
	ini.WriteInt(_T("NAFCEnable"), NAFCEnable);
	ini.WriteInt(_T("NAFCFullControl"), NAFCFullControl);
	ini.WriteFloat(_T("NAFCNetworkOut"), NAFCNetworkOut);
	ini.WriteInt(_T("NAFCSlider"), NAFCSlider);
	ini.WriteBool(_T("DinamicNafc"), m_bDinamicNafc);
	// Maella end

	// Maella -Accurate measure of bandwidth: eDonkey data + control, network adapter-
	ini.WriteInt(_T("DatarateSamples"), datarateSamples);
	// Maella end

	// Maella -One-queue-per-file- (idea bloodymad)
	ini.WriteBool(_T("EnableMultiQueue"), enableMultiQueue);
	ini.WriteBool(_T("EnableReleaseMultiQueue"), enableReleaseMultiQueue);
	ini.WriteBool(_T("EnableNewSlotUpload"), enableNewUSS);
	// Maella end
				
	ini.WriteInt(_T("MainProcessPriority"), m_MainProcessPriority); // [TPT] - Select process priority 

	// added by [TPT]-MoNKi [MoNKi: -invisible mode-]
	ini.WriteBool(_T("InvisibleMode"), m_bInvisibleMode);
	ini.WriteString(_T("InvisibleModeHKKey"), CString(m_cInvisibleModeHotKey));
	ini.WriteInt(_T("InvisibleModeHKKeyModifier"), m_iInvisibleModeHotKeyModifier);
	// End [TPT]-MoNKi

	ini.WriteBool(_T("ShowSharedInHistory"), m_bHistoryShowShared); // [TPT] - MoNKi: -Downloaded History-

	ini.WriteBool(_T("WindowsTextColorOnHighContrast"), m_bUseWindowsTextColorOnHighContrast); // [TPT] - MoNKi: -Support for High Contrast Mode-

	ini.WriteInt(_T("CreditSystem"),m_iCreditSystem); // [TPT] - Credit System

	ini.WriteBool(_T("SpreadBars"), m_bSpreadBars); // [TPT] - SLUGFILLER: Spreadbars
	
	// [TPT] - IP Country
	ini.WriteBool(_T("showFlags"),showFlags);
	ini.WriteBool(_T("showCountryName"),showCountryName);
	// [TPT] - IP Country

	// [TPT] - TBH: minimule
	ini.WriteInt(_T("SpeedMeterMin"), speedmetermin);
	ini.WriteInt(_T("SpeedMeterMax"), speedmetermax);	
	ini.WriteBool(_T("ShowMiniMule"),m_bMiniMule);
	ini.WriteInt(_T("MiniMuleUpdate"),m_iMiniMuleUpdate);
	ini.WriteBool(_T("MiniMuleLives"),m_bMiniMuleLives);
	ini.WriteInt(_T("MiniMuleTransparency"),m_iMiniMuleTransparency);
	// [TPT] - TBH: minimule	
	ini.WriteBool(_T("FadeOut"),m_bfadeOut); // [TPT] - Fadeout on exit

	ini.WriteBool(_T("ShowActiveDownloadsBold"),m_bShowActiveDownloadsBold);

	// [TPT] - quick start
	ini.WriteBool(_T("QuickStart"), m_QuickStart);   
	ini.WriteInt(_T("QuickStartMaxCon"), m_QuickStartMaxCon);
	ini.WriteInt(_T("QuickStartMaxConPerFive"), m_QuickStartMaxConPerFive);
	// [TPT] - quick start
	
	// [TPT] - SLUGFILLER: hideOS
	ini.WriteInt(_T("HideOvershares"),hideOS);
	ini.WriteBool(_T("SelectiveShare"),selectiveShare);
	// [TPT] - SLUGFILLER: hideOS

	ini.WriteBool(_T("InfiniteQueue"),infiniteQueue);	// [TPT] - SLUGFILLER: infiniteQueue
	// [TPT] - itsonlyme: displayOptions START
	ini.WriteBool(_T("ShowFileSystemIcon"),showFileSystemIcon);
	ini.WriteBool(_T("ShowLocalRating"),showLocalRating); // [TPT] - SLUGFILLER: showComments
	// [TPT] - itsonlyme: displayOptions END
	
	ini.WriteInt(_T("CurrentAdapterIndex"),m_currentAdapterIndex); // [TPT] - NAFC Selection
	
	ini.WriteBool(_T("UnlimitedUp"),m_bUnlimitedUP); // //[TPT] - Unlimited upload with no downloads
	// [TPT] - eWombat SNAFU v2
	ini.WriteBool(_T("Snafu"), m_bSnafu);
	ini.WriteBool(_T("AntiCreditTheft"), m_bAntiCreditTheft);
	ini.WriteBool(_T("AntiFriendShare"), m_bAntiFriendshare);
	
	ini.WriteBool(_T("CumulateBandwidth"), cumulateBandwidth); // [TPT]-Cumulate Bandwidth
	ini.WriteBool(_T("MinimizeSlots"), minimizeSlots); // [TPT] - Pawcio: MUS
	ini.WriteBool(_T("ManageConnection"), manageConnection); // [TPT] - Manage Connection
	
	// [TPT] - khaos::categorymod+ Save Preferences	
	ini.WriteBool(_T("ShowCatName"), m_bShowCatNames);
	ini.WriteBool(_T("ActiveCatDefault"), m_bActiveCatDefault);
	ini.WriteBool(_T("SelCatOnAdd"), m_bSelCatOnAdd);
	ini.WriteBool(_T("AutoSetResumeOrder"), m_bAutoSetResumeOrder);	
	ini.WriteInt(_T("StartDLInEmptyCats"), m_iStartDLInEmptyCats);
	ini.WriteBool(_T("UseAutoCat"), m_bUseAutoCat);
	ini.WriteBool(_T("ShowPriorityInTab"), m_bShowPriorityInTab);
	// [TPT] - khaos::categorymod-

	// [TPT] - Memory Consuming
	ini.WriteBool(_T("MemoryConsuming"), m_bMemoryConsumingGraph);
	ini.WriteBool(_T("NAFCGraph"), m_bNAFCGraph);
	ini.WriteBool(_T("OverheadGraph"), m_bOverheadGraph);
	// [TPT] - TBH-AutoBackup Begin
	ini.WriteBool(_T("AutoBackup"),autobackup);
	ini.WriteBool(_T("AutoBackup2"),autobackup2);
	// [TPT] - TBH-AutoBackup End
	
	// [TPT] - MoNKi: -UPnPNAT Support-
	ini.WriteBool(_T("UPnPNAT"), m_bUPnPNat);
	ini.WriteBool(_T("UPnPNAT_Web"), m_bUPnPNatWeb);	
	ini.WriteBool(_T("UPnPVerbose"), m_bUPnPVerboseLog, _T("eMule"));
	ini.WriteInt(_T("UPnPPort"), m_iUPnPPort, _T("eMule"));



	// [TPT] - MoNKi: -UPnPNAT Support-
	ini.WriteString(_T("YourHostname"),m_strYourHostname);
	// [TPT] - Powershare
	ini.WriteInt(_T("PowershareMode"),m_iPowershareMode);
	ini.WriteInt(_T("PowerShareLimit"),PowerShareLimit);
	// [TPT] - Powershare
	ini.WriteBool(_T("SaveUploadQueueWaitTime"), m_bSaveUploadQueueWaitTime); // [TPT] - SUQWT
	// [TPT] - Fakecheck
	ini.WriteInt(_T("FakesDatVersion"),m_FakesDatVersion);
	ini.WriteBool(_T("UpdateFakeStartup"),UpdateFakeStartup);
	ini.WriteString(_T("UpdateURLFakeList"),UpdateURLFakeList);
	// [TPT] - Fakecheck End

	// [TPT] - Manual eMfriend.met download
	ini.WriteString(_T("UpdateURLFriendList"), m_updateURLFriendList);
	// [TPT] - Manual eMfriend.met download

	//[TPT] - Double buffer style in lists
	ini.WriteBool(_T("DBStyle"), m_bDBStyle);

	//[TPT] - Show bitmap in menus
	ini.WriteBool(_T("ShowBitmapInMenus"), m_bShowBitmapInMenus);

	//[TPT] - Show in MSN7
	ini.WriteBool(_T("ShowInMSN7"), m_bShowInMSN7);
	//[TPT] - Show Upload Priority in downloadlist
	ini.WriteBool(_T("ShowUpPrioInDownloadList"), m_bShowUpPrioInDownloadList);


	// Barry - New properties...
    ini.WriteBool(_T("AutoConnectStaticOnly"), autoconnectstaticonly);  
	ini.WriteBool(_T("AutoTakeED2KLinks"), autotakeed2klinks);  
    ini.WriteBool(_T("AddNewFilesPaused"), addnewfilespaused);  
    ini.WriteInt (_T("3DDepth"), depth3D);  
	//START [TPT] - enkeyDEV(kei-kun) -Notifier preferences-
	ini.WriteBool(_T("UseNotifierUserTimings"), useNotifierUserTimings);
	ini.WriteBool(_T("NotifierLessFramerate"), notifierLessFramerate);
	ini.WriteInt(_T("NotifierUserTimeToShow"), notifierUserTimeToShow);
	ini.WriteInt(_T("NotifierUserTimeToStay"), notifierUserTimeToStay);
	ini.WriteInt(_T("NotifierUserTimeToHide"), notifierUserTimeToHide);		
	ini.WriteBool(_T("NotifyOnNewDownload"),useNewDownloadNotifier);
	ini.WriteBool(_T("NotifyOnDownload"), useDownloadNotifier);
	ini.WriteBool(_T("NotifierAutoClose"),notifierAutoClose); 
	ini.WriteBool(_T("NotifyOnError"),useErrorNotifier); 	 // added by InterCeptor (notify on error) 11.11.02
	ini.WriteBool(_T("NotifyOnChat"),useChatNotifier);		  
	ini.WriteBool(_T("NotifyOnLog"),useLogNotifier);
	ini.WriteBool(_T("NotifierUseSound"),useSoundInNotifier);
	ini.WriteBool(_T("NotifierPopEveryChatMessage"),notifierPopsEveryChatMsg);
	ini.WriteBool(_T("NotifierPopNewVersion"),notifierNewVersion);
	ini.WriteBool(_T("NotifyOnImportantError"), notifierImportantError);
	ini.WriteBool(_T("NotifyNewPvtMsg"),notifierNewPvtMsg);//Rocks
	ini.WriteBool(_T("NotifierPopSearchCompleted"), notifierSearchCompleted);
	buffer.Format(_T("%s"),notifierSoundFilePath);
	ini.WriteString(_T("NotifierSoundPath"),buffer);	
	buffer.Format(_T("%s"),notifierConfiguration);		      // Added by enkeyDEV
	ini.WriteString(_T("NotifierConfiguration"),buffer);			  // Added by enkeyDEV
	//END [TPT] - enkeyDEV(kei-kun) -Notifier preferences-

	ini.WriteString(_T("TxtEditor"),TxtEditor);
	ini.WriteString(_T("VideoPlayer"),VideoPlayer);
	ini.WriteString(_T("MessageFilter"),messageFilter);
	ini.WriteString(_T("CommentFilter"),commentFilter);
	ini.WriteString(_T("DateTimeFormat"),GetDateTimeFormat());
	ini.WriteString(_T("DateTimeFormat4Log"),GetDateTimeFormat4Log());
	ini.WriteString(_T("WebTemplateFile"),m_sTemplateFile);
	ini.WriteString(_T("FilenameCleanups"),filenameCleanups);
	ini.WriteInt(_T("ExtractMetaData"),m_iExtractMetaData);

	ini.WriteString(_T("DefaultIRCServerNew"),m_sircserver);
	ini.WriteString(_T("IRCNick"),m_sircnick);
	ini.WriteBool(_T("IRCAddTimestamp"), m_bircaddtimestamp);
	ini.WriteString(_T("IRCFilterName"), m_sircchannamefilter);
	ini.WriteInt(_T("IRCFilterUser"), m_iircchanneluserfilter);
	ini.WriteBool(_T("IRCUseFilter"), m_bircusechanfilter);
	ini.WriteString(_T("IRCPerformString"), m_sircperformstring);
	ini.WriteBool(_T("IRCUsePerform"), m_bircuseperform);
	ini.WriteBool(_T("IRCListOnConnect"), m_birclistonconnect);
	ini.WriteBool(_T("IRCAcceptLink"), m_bircacceptlinks);
	ini.WriteBool(_T("IRCAcceptLinkFriends"), m_bircacceptlinksfriends);
	ini.WriteBool(_T("IRCSoundEvents"), m_bircsoundevents);
	ini.WriteBool(_T("IRCIgnoreMiscMessages"), m_bircignoremiscmessage);
	ini.WriteBool(_T("IRCIgnoreJoinMessages"), m_bircignorejoinmessage);
	ini.WriteBool(_T("IRCIgnorePartMessages"), m_bircignorepartmessage);
	ini.WriteBool(_T("IRCIgnoreQuitMessages"), m_bircignorequitmessage);
	ini.WriteBool(_T("IRCIgnoreEmuleProtoAddFriend"), m_bircignoreemuleprotoaddfriend);
	ini.WriteBool(_T("IRCAllowEmuleProtoAddFriend"), m_bircallowemuleprotoaddfriend);
	ini.WriteBool(_T("IRCIgnoreEmuleProtoSendLink"), m_bircignoreemuleprotosendlink);
	ini.WriteBool(_T("IRCHelpChannel"), m_birchelpchannel);
	ini.WriteBool(_T("SmartIdCheck"), smartidcheck);
	ini.WriteBool(_T("Verbose"), m_bVerbose);
	ini.WriteBool(_T("DebugSourceExchange"), m_bDebugSourceExchange);	// do *not* use the according 'Get...' function here!
	ini.WriteBool(_T("LogBannedClients"), m_bLogBannedClients);			// do *not* use the according 'Get...' function here!
	ini.WriteBool(_T("LogRatingDescReceived"), m_bLogRatingDescReceived);// do *not* use the according 'Get...' function here!
	ini.WriteBool(_T("LogSecureIdent"), m_bLogSecureIdent);				// do *not* use the according 'Get...' function here!
	ini.WriteBool(_T("LogFilteredIPs"), m_bLogFilteredIPs);				// do *not* use the according 'Get...' function here!
	ini.WriteBool(_T("LogFileSaving"), m_bLogFileSaving);				// do *not* use the according 'Get...' function here!
    ini.WriteBool(_T("LogA4AF"), m_bLogA4AF);                           // do *not* use the according 'Get...' function here!
	ini.WriteBool(_T("LogUlDlEvents"), m_bLogUlDlEvents);
	// [TPT] - WebCache
	ini.WriteBool(_T("LogWebCacheEvents"), m_bLogWebCacheEvents);//JP log webcache events
	ini.WriteBool(_T("LogICHEvents"), m_bLogICHEvents);//JP log ICH events
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	// following options are for debugging or when using an external debug device viewer only.
	ini.WriteInt(_T("DebugServerTCP"),m_iDebugServerTCPLevel);
	ini.WriteInt(_T("DebugServerUDP"),m_iDebugServerUDPLevel);
	ini.WriteInt(_T("DebugServerSources"),m_iDebugServerSourcesLevel);
	ini.WriteInt(_T("DebugServerSearches"),m_iDebugServerSearchesLevel);
	ini.WriteInt(_T("DebugClientTCP"),m_iDebugClientTCPLevel);
	ini.WriteInt(_T("DebugClientUDP"),m_iDebugClientUDPLevel);
	ini.WriteInt(_T("DebugClientKadUDP"),m_iDebugClientKadUDPLevel);
#endif
	ini.WriteBool(_T("PreviewPrio"), m_bpreviewprio);
	ini.WriteBool(_T("UpdateQueueListPref"), m_bupdatequeuelist);
	ini.WriteBool(_T("ManualHighPrio"), m_bmanualhighprio);
	ini.WriteBool(_T("FullChunkTransfers"), m_btransferfullchunks);
	ini.WriteBool(_T("ShowOverhead"), m_bshowoverhead);
	ini.WriteBool(_T("VideoPreviewBackupped"), moviePreviewBackup);
	ini.WriteInt(_T("StartNextFile"), m_istartnextfile);

	ini.DeleteKey(_T("FileBufferSizePref")); // delete old 'file buff size' setting
	ini.WriteInt(_T("FileBufferSize"), m_iFileBufferSize);

	ini.DeleteKey(_T("QueueSizePref")); // delete old 'queue size' setting
	ini.WriteInt(_T("QueueSize"), m_iQueueSize);

	ini.WriteInt(_T("CommitFiles"), m_iCommitFiles);
	ini.WriteBool(_T("DAPPref"), m_bDAP);
	ini.WriteBool(_T("UAPPref"), m_bUAP);
	ini.WriteBool(_T("FilterServersByIP"),filterserverbyip);
	ini.WriteBool(_T("DisableKnownClientList"),m_bDisableKnownClientList);
	ini.WriteBool(_T("DisableQueueList"),m_bDisableQueueList);
	ini.WriteBool(_T("UseCreditSystem"),m_bCreditSystem);
	ini.WriteBool(_T("SaveLogToDisk"),log2disk);
	ini.WriteBool(_T("SaveDebugToDisk"),debug2disk);
	ini.WriteBool(_T("EnableScheduler"),scheduler);
	ini.WriteBool(_T("MessagesFromFriendsOnly"),msgonlyfriends);
	ini.WriteBool(_T("MessageFromValidSourcesOnly"),msgsecure);
	ini.WriteBool(_T("ShowInfoOnCatTabs"),showCatTabInfos);
	ini.WriteBool(_T("DontRecreateStatGraphsOnResize"),dontRecreateGraphs);
	ini.WriteBool(_T("AutoFilenameCleanup"),autofilenamecleanup);
	ini.WriteBool(_T("ShowExtControls"),m_bExtControls);
	ini.WriteBool(_T("UseAutocompletion"),m_bUseAutocompl);
	ini.WriteBool(_T("NetworkKademlia"),networkkademlia);
	ini.WriteBool(_T("NetworkED2K"),networked2k);
	ini.WriteBool(_T("AutoClearCompleted"),m_bRemoveFinishedDownloads);
	ini.WriteBool(_T("TransflstRemainOrder"),m_bTransflstRemain);
	ini.WriteBool(_T("UseSimpleTimeRemainingcomputation"),m_bUseOldTimeRemaining);

	ini.WriteInt(_T("VersionCheckLastAutomatic"), versioncheckLastAutomatic);
	ini.WriteInt(_T("FilterLevel"),filterlevel);

	ini.WriteBool(_T("SecureIdent"), m_bUseSecureIdent);// change the name in future version to enable it by default
	ini.WriteBool(_T("AdvancedSpamFilter"),m_bAdvancedSpamfilter);
	ini.WriteBool(_T("ShowDwlPercentage"),m_bShowDwlPercentage);
	ini.WriteBool(_T("RemoveFilesToBin"),m_bRemove2bin);
	//ini.WriteBool(_T("ShowCopyEd2kLinkCmd"),m_bShowCopyEd2kLinkCmd);

	// Toolbar
	ini.WriteString(_T("ToolbarSetting"), m_sToolbarSettings);
	ini.WriteString(_T("ToolbarBitmap"), m_sToolbarBitmap );
	ini.WriteString(_T("ToolbarBitmapFolder"), m_sToolbarBitmapFolder);
	ini.WriteInt(_T("ToolbarLabels"), m_nToolbarLabels);
	ini.WriteInt(_T("ToolbarIconSize"), m_sizToolbarIconSize.cx);
	ini.WriteString(_T("SkinProfile"), m_strSkinProfile);
	ini.WriteString(_T("SkinProfileDir"), m_strSkinProfileDir);


	ini.SerGet(false, downloadColumnWidths,
		ARRSIZE(downloadColumnWidths), _T("DownloadColumnWidths"));
	ini.SerGet(false, downloadColumnHidden,
		ARRSIZE(downloadColumnHidden), _T("DownloadColumnHidden"));
	ini.SerGet(false, downloadColumnOrder,
		ARRSIZE(downloadColumnOrder), _T("DownloadColumnOrder"));
	// [TPT] - TBH Transfers Window Style
	ini.SerGet(false, downloadClientsColumnWidths,
		ARRSIZE(downloadClientsColumnWidths), _T("DownloadClientsColumnWidths"));
	ini.SerGet(false, downloadClientsColumnHidden,
		ARRSIZE(downloadClientsColumnHidden), _T("DownloadClientsColumnHidden"));
	ini.SerGet(false, downloadClientsColumnOrder,
		ARRSIZE(downloadClientsColumnOrder), _T("DownloadClientsColumnOrder"));
	// [TPT] - TBH Transfers Window Style
	ini.SerGet(false, uploadColumnWidths,
		ARRSIZE(uploadColumnWidths), _T("UploadColumnWidths"));
	ini.SerGet(false, uploadColumnHidden,
		ARRSIZE(uploadColumnHidden), _T("UploadColumnHidden"));
	ini.SerGet(false, uploadColumnOrder,
		ARRSIZE(uploadColumnOrder), _T("UploadColumnOrder"));
	ini.SerGet(false, queueColumnWidths,
		ARRSIZE(queueColumnWidths), _T("QueueColumnWidths"));
	ini.SerGet(false, queueColumnHidden,
		ARRSIZE(queueColumnHidden), _T("QueueColumnHidden"));
	ini.SerGet(false, queueColumnOrder,
		ARRSIZE(queueColumnOrder), _T("QueueColumnOrder"));
	ini.SerGet(false, searchColumnWidths,
		ARRSIZE(searchColumnWidths), _T("SearchColumnWidths"));
	ini.SerGet(false, searchColumnHidden,
		ARRSIZE(searchColumnHidden), _T("SearchColumnHidden"));
	ini.SerGet(false, searchColumnOrder,
		ARRSIZE(searchColumnOrder), _T("SearchColumnOrder"));
	ini.SerGet(false, sharedColumnWidths,
		ARRSIZE(sharedColumnWidths), _T("SharedColumnWidths"));
	ini.SerGet(false, sharedColumnHidden,
		ARRSIZE(sharedColumnHidden), _T("SharedColumnHidden"));
	ini.SerGet(false, sharedColumnOrder,
		ARRSIZE(sharedColumnOrder), _T("SharedColumnOrder"));
	ini.SerGet(false, serverColumnWidths,
		ARRSIZE(serverColumnWidths), _T("ServerColumnWidths"));
	ini.SerGet(false, serverColumnHidden,
		ARRSIZE(serverColumnHidden), _T("ServerColumnHidden"));
	ini.SerGet(false, serverColumnOrder,
		ARRSIZE(serverColumnOrder), _T("ServerColumnOrder"));
	ini.SerGet(false, clientListColumnWidths,
		ARRSIZE(clientListColumnWidths), _T("ClientListColumnWidths"));
	ini.SerGet(false, clientListColumnHidden,
		ARRSIZE(clientListColumnHidden), _T("ClientListColumnHidden"));
	ini.SerGet(false, clientListColumnOrder,
		ARRSIZE(clientListColumnOrder), _T("ClientListColumnOrder"));
	
	ini.SerGet(false, FilenamesListColumnWidths,
		ARRSIZE(FilenamesListColumnWidths), _T("FilenamesListColumnWidths"));
	ini.SerGet(false, FilenamesListColumnHidden,
		ARRSIZE(FilenamesListColumnHidden), _T("FilenamesListColumnHidden"));
	ini.SerGet(false, FilenamesListColumnOrder,
		ARRSIZE(FilenamesListColumnOrder), _T("FilenamesListColumnOrder"));
	// [TPT] - MoNKi: -Downloaded History-
	ini.SerGet(false, m_HistoryColumnWidths,
		ARRSIZE(m_HistoryColumnWidths), _T("m_HistoryColumnWidths"));
	ini.SerGet(false, m_HistoryColumnHidden,
		ARRSIZE(m_HistoryColumnHidden), _T("m_HistoryColumnHidden"));
	ini.SerGet(false, m_HistoryColumnOrder,
		ARRSIZE(m_HistoryColumnOrder), _T("m_HistoryColumnOrder"));
	// [TPT] - MoNKi: -Downloaded History-
	// [TPT] - Friend State Column
	ini.SerGet(false, friendListColumnWidths,
		ARRSIZE(friendListColumnWidths), _T("friendListColumnWidths"));
	ini.SerGet(false, friendListColumnHidden,
		ARRSIZE(friendListColumnHidden), _T("friendListColumnHidden"));
	ini.SerGet(false, friendListColumnOrder,
		ARRSIZE(friendListColumnOrder), _T("friendListColumnOrder"));
	// [TPT] - Friend State Column

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	// [TPT] - SLUGFILLER: multiSort - save multiple params
	ini.SerGet(false, tableSortItemDownload,
		GetColumnSortCount(tableDownload), _T("TableSortItemDownload"));
	ini.SerGet(false, (BYTE*)tableSortAscendingDownload,
		GetColumnSortCount(tableDownload), _T("TableSortAscendingDownload"));
	// [TPT] - TBH Transfers Window Style
	ini.SerGet(false, tableSortItemDownloadClient,
		GetColumnSortCount(tableDownloadClients), _T("TableSortItemDownloadClient")); 
	ini.SerGet(false, (BYTE*)tableSortAscendingDownloadClient,
		GetColumnSortCount(tableDownloadClients), _T("TableSortAscendingDownloadClient")); 
	// [TPT] - TBH Transfers Window Style	

	ini.SerGet(false, tableSortItemUpload,
		GetColumnSortCount(tableUpload), _T("TableSortItemUpload"));
	ini.SerGet(false, (BYTE*)tableSortAscendingUpload,
		GetColumnSortCount(tableUpload), _T("TableSortAscendingUpload"));
	ini.SerGet(false, tableSortItemQueue,
		GetColumnSortCount(tableQueue), _T("TableSortItemQueue"));
	ini.SerGet(false, (BYTE*)tableSortAscendingQueue,
		GetColumnSortCount(tableQueue), _T("TableSortAscendingQueue"));
	ini.SerGet(false, tableSortItemSearch,
		GetColumnSortCount(tableSearch), _T("TableSortItemSearch"));
	ini.SerGet(false, (BYTE*)tableSortAscendingSearch,
		GetColumnSortCount(tableSearch), _T("TableSortAscendingSearch"));
	ini.SerGet(false, tableSortItemShared,
		GetColumnSortCount(tableShared), _T("TableSortItemShared"));
	ini.SerGet(false, (BYTE*)tableSortAscendingShared,
		GetColumnSortCount(tableShared), _T("TableSortAscendingShared"));
	ini.SerGet(false, tableSortItemServer,
		GetColumnSortCount(tableServer), _T("TableSortItemServer"));
	ini.SerGet(false, (BYTE*)tableSortAscendingServer,
		GetColumnSortCount(tableServer), _T("TableSortAscendingServer"));
	ini.SerGet(false, tableSortItemClientList,
		GetColumnSortCount(tableClientList), _T("TableSortItemClientList"));
	ini.SerGet(false, (BYTE*)tableSortAscendingClientList,
		GetColumnSortCount(tableClientList), _T("TableSortAscendingClientList"));
	ini.SerGet(false, tableSortItemFilenames,
		GetColumnSortCount(tableFilenames), _T("TableSortItemFilenames"));
	ini.SerGet(false, (BYTE*)tableSortAscendingFilenames,
		GetColumnSortCount(tableFilenames), _T("TableSortAscendingFilenames"));	
	// [TPT] - MoNKi: -Downloaded History-
	ini.SerGet(false, tableSortItemHistory,
		GetColumnSortCount(tableHistory), _T("TableSortItemHistory"));
	ini.SerGet(false, (BYTE*)tableSortAscendingHistory,
		GetColumnSortCount(tableHistory), _T("tableSortAscendingHistory"));
	// [TPT] - MoNKi: -Downloaded History-
	// [TPT] - Friend State Column
	ini.SerGet(false, tableSortItemFriendList,
		GetColumnSortCount(tableFriendList), _T("TableSortItemFriendList"));
	ini.SerGet(false, (BYTE*)tableSortAscendingFriendList,
		GetColumnSortCount(tableFriendList), _T("tableSortAscendingFriendList"));
	// [TPT] - Friend State Column
	// [TPT] - SLUGFILLER: multiSort
	ini.WriteBinary(_T("HyperTextFont"), (LPBYTE)&m_lfHyperText, sizeof m_lfHyperText);
	ini.WriteBinary(_T("LogTextFont"), (LPBYTE)&m_lfLogText, sizeof m_lfLogText);

	// ZZ:UploadSpeedSense -->
    ini.WriteBool(_T("USSEnabled"), m_bDynUpEnabled, _T("eMule"));
    ini.WriteBool(_T("USSUseMillisecondPingTolerance"), m_bDynUpUseMillisecondPingTolerance);
    ini.WriteInt(_T("USSPingTolerance"), m_iDynUpPingTolerance, _T("eMule"));
	ini.WriteInt(_T("USSPingToleranceMilliseconds"), m_iDynUpPingToleranceMilliseconds); // EastShare - Add by TAHO, USS limit
    ini.WriteInt(_T("USSGoingUpDivider"), m_iDynUpGoingUpDivider, _T("eMule"));
    ini.WriteInt(_T("USSGoingDownDivider"), m_iDynUpGoingDownDivider, _T("eMule"));
    ini.WriteInt(_T("USSNumberOfPings"), m_iDynUpNumberOfPings, _T("eMule"));
	// ZZ:UploadSpeedSense <--

    ini.WriteBool(_T("A4AFSaveCpu"), m_bA4AFSaveCpu, _T("eMule")); // ZZ:DownloadManager
	ini.WriteInt(_T("WebMirrorAlertLevel"), m_nWebMirrorAlertLevel);
	ini.WriteBool(_T("RunAsUnprivilegedUser"), m_bRunAsUser);
	ini.WriteBool(_T("OpenPortsOnStartUp"), m_bOpenPortsOnStartUp);
	ini.WriteInt(_T("DebugLogLevel"), m_byLogLevel);
	ini.WriteInt(_T("WinXPSP2"), IsRunningXPSP2());


	///////////////////////////////////////////////////////////////////////////
	// Section: "Proxy"
	//
	ini.WriteBool(_T("ProxyEnablePassword"),proxy.EnablePassword,_T("Proxy"));
	ini.WriteBool(_T("ProxyEnableProxy"),proxy.UseProxy,_T("Proxy"));
	ini.WriteString(_T("ProxyName"),proxy.name,_T("Proxy"));
	ini.WriteString(_T("ProxyPassword"),A2CT(proxy.password),_T("Proxy"));
	ini.WriteString(_T("ProxyUser"),A2CT(proxy.user),_T("Proxy"));
	ini.WriteInt(_T("ProxyPort"),proxy.port,_T("Proxy"));
	ini.WriteInt(_T("ProxyType"),proxy.type,_T("Proxy"));
	ini.WriteBool(_T("ConnectWithoutProxy"),m_bIsASCWOP,_T("Proxy"));
	ini.WriteBool(_T("ShowErrors"),m_bShowProxyErrors,_T("Proxy"));


	///////////////////////////////////////////////////////////////////////////
	// Section: "Statistics"
	//
	ini.WriteInt(_T("statsConnectionsGraphRatio"), statsConnectionsGraphRatio,_T("Statistics"));
	ini.WriteString(_T("statsExpandedTreeItems"), statsExpandedTreeItems);
	CString buffer2;
	for (int i=0;i<15;i++) {
		buffer.Format(_T("0x%06x"),GetStatsColor(i));
		buffer2.Format(_T("StatColor%i"),i);
		ini.WriteString(buffer2,buffer,_T("Statistics") );
	}


	///////////////////////////////////////////////////////////////////////////
	// Section: "WebServer"
	//
	ini.WriteString(_T("Password"), GetWSPass(), _T("WebServer"));
	ini.WriteString(_T("PasswordLow"), GetWSLowPass());
	ini.WriteInt(_T("Port"), m_nWebPort);
	ini.WriteBool(_T("Enabled"), m_bWebEnabled);
	ini.WriteBool(_T("UseGzip"), m_bWebUseGzip);
	ini.WriteInt(_T("PageRefreshTime"), m_nWebPageRefresh);
	ini.WriteBool(_T("UseLowRightsUser"), m_bWebLowEnabled);


	///////////////////////////////////////////////////////////////////////////
	// Section: "MobileMule"
	//
	ini.WriteString(_T("Password"), GetMMPass(), _T("MobileMule"));
	ini.WriteBool(_T("Enabled"), m_bMMEnabled);
	ini.WriteInt(_T("Port"), m_nMMPort);


	///////////////////////////////////////////////////////////////////////////
	// Section: "PeerCache"
	//
	ini.WriteInt(_T("LastSearch"), m_uPeerCacheLastSearch, _T("PeerCache"));
	ini.WriteBool(_T("Found"), m_bPeerCacheWasFound);
	ini.WriteBool(_T("Enabled"), m_bPeerCacheEnabled);
	ini.WriteInt(_T("PCPort"), m_nPeerCachePort);
}

void CPreferences::ResetStatsColor(int index)
{
	switch(index)
	{
		case  0: m_adwStatsColors[ 0]=RGB(  0,  0, 64);break;
		case  1: m_adwStatsColors[ 1]=RGB(192,192,255);break;
		case  2: m_adwStatsColors[ 2]=RGB(128,255,128);break;
		case  3: m_adwStatsColors[ 3]=RGB(  0,210,  0);break;
		case  4: m_adwStatsColors[ 4]=RGB(  0,128,  0);break;
		case  5: m_adwStatsColors[ 5]=RGB(255,128,128);break;
		case  6: m_adwStatsColors[ 6]=RGB(200,  0,  0);break;
		case  7: m_adwStatsColors[ 7]=RGB(140,  0,  0);break;
		case  8: m_adwStatsColors[ 8]=RGB(150,150,255);break;
		case  9: m_adwStatsColors[ 9]=RGB(192,  0,192);break;
		case 10: m_adwStatsColors[10]=RGB(255,255,128);break;
		case 11: m_adwStatsColors[11]=RGB(255,  0,  0);break;//[TPT] - Meter tray icon color...default RED.
		case 12: m_adwStatsColors[12]=RGB(255,255,255);break;
		case 13: m_adwStatsColors[13]=RGB(255,255,255);break;
		case 14: m_adwStatsColors[14]=RGB(255,190,190);break;
                default:break;
	}
}

void CPreferences::GetAllStatsColors(int iCount, LPDWORD pdwColors)
{
	MEMZERO(pdwColors, sizeof(*pdwColors) * iCount);
	MEMCOPY(pdwColors, m_adwStatsColors, sizeof(*pdwColors) * min(ARRSIZE(m_adwStatsColors), iCount));
}

bool CPreferences::SetAllStatsColors(int iCount, const DWORD* pdwColors)
{
	bool bModified = false;
	int iMin = min(ARRSIZE(m_adwStatsColors), iCount);
	for (int i = 0; i < iMin; i++)
	{
		if (m_adwStatsColors[i] != pdwColors[i])
		{
			m_adwStatsColors[i] = pdwColors[i];
			bModified = true;
		}
	}
	return bModified;
}

// [TPT] - itsonlyme: virtualDirs
void CPreferences::LoadVirtualDirs()
{
	CString fullpath;
	fullpath.Format(_T("%svirtualdirs.ini"), configdir);
	CStdioFile file;
	bool bIsUnicodeFile = IsUnicodeFile(fullpath); // check for BOM
	if (file.Open(fullpath, CFile::modeRead | CFile::shareDenyWrite | (bIsUnicodeFile ? CFile::typeBinary : 0))){
		try {
			if (bIsUnicodeFile)
				file.Seek(sizeof(WORD), SEEK_CUR); // skip BOM

			CString toadd;
			while (file.ReadString(toadd))
			{
				toadd.Trim(_T("\r\n")); // need to trim '\r' in binary mode
				CString type, mapFrom, mapTo;
				int curPos = 0;

				type = toadd.Tokenize(_T(";"), curPos);
				if (curPos == -1)
					continue;

				mapFrom = toadd.Tokenize(_T(";"), curPos);
				if (curPos == -1)
					continue;

				mapTo = toadd.Tokenize(_T(";"), curPos);
				if (curPos != toadd.GetLength()+1)
					continue;

				type.Trim(); 
				mapFrom.Trim(); 
				mapTo.Trim();
				mapFrom.TrimRight(_T("\\")); // we can't trim("\\") since there may be unc shares
				mapTo.TrimRight(_T("\\")); 
				if (type != _T("0") && type != _T("1") && type != _T("2"))
					continue;

				if (type == _T("0"))
					mapFrom.MakeUpper();
				else {
					mapFrom.MakeLower();
					if (!PathFileExists(mapFrom))
						continue;
				}
				mapTo.MakeLower();

				if (type == _T("0"))
					m_fileToVDir_map.SetAt(mapFrom, mapTo);
				else if (type== _T("1")) 
					m_dirToVDir_map.SetAt(mapFrom, mapTo);
				else if (type == _T("2"))
					m_dirToVDirWithSD_map.SetAt(mapFrom, mapTo);
			}
		}
		catch (CFileException* ex) {
			ASSERT(0);
			ex->Delete();
		}
		file.Close();
	}
}

void CPreferences::SaveVirtualDirs()
{
	CString fullpath;
	fullpath.Format(_T("%svirtualdirs.ini"), configdir);
	CStdioFile file;
	CString entry1, entry2, buffer;
	if (file.Open(fullpath,CFile::modeCreate | CFile::modeWrite | CFile::shareDenyWrite | CFile::typeBinary))
	{
		try{
			// write Unicode byte-order mark 0xFEFF
			WORD wBOM = 0xFEFF;
			file.Write(&wBOM, sizeof(wBOM));

			for (POSITION pos = m_fileToVDir_map.GetHeadPosition(); pos != NULL; ) {
				m_fileToVDir_map.GetNextAssoc(pos, entry1, entry2);
				buffer.Format(_T("0;%s;%s"), entry1, entry2);
				file.WriteString(buffer);
				file.Write(_T("\r\n"), sizeof(TCHAR)*2);
			}
			for (POSITION pos = m_dirToVDir_map.GetHeadPosition(); pos != NULL; ){
				m_dirToVDir_map.GetNextAssoc(pos, entry1, entry2);
				buffer.Format(_T("1;%s;%s"), entry1, entry2);
				file.WriteString(buffer);
				file.Write(_T("\r\n"), sizeof(TCHAR)*2);
			}
			for (POSITION pos = m_dirToVDirWithSD_map.GetHeadPosition(); pos != NULL; ) {
				m_dirToVDirWithSD_map.GetNextAssoc(pos, entry1, entry2);
				buffer.Format(_T("2;%s;%s"), entry1, entry2);
				file.WriteString(buffer);
				file.Write(_T("\r\n"), sizeof(TCHAR)*2);
		}
			if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
				file.Flush(); // flush file stream buffers to disk buffers
				if (_commit(_fileno(file.m_pStream)) != 0) // commit disk buffers to disk
					AfxThrowFileException(CFileException::hardIO, GetLastError(), file.GetFileName());
			}
			file.Close();
		}
		catch(CFileException* error){
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer,ARRSIZE(buffer));
			if (thePrefs.GetVerbose())
				AddDebugLogLine(true,_T("Failed to save %s - %s"), fullpath, buffer);
			error->Delete();
		}
	}
}
// [TPT] - itsonlyme: virtualDirs


void CPreferences::LoadPreferences()
{
	USES_CONVERSION;
	TCHAR buffer[256];
	// -khaos--+++> Fix to stats being lost when version changes!
	int loadstatsFromOld = 0;
	// <-----khaos-

	//--- Quick hack to add version tag to preferences.ini-file and solve the issue with the FlatStatusBar tag...
	CString strFileName;
	strFileName.Format(_T("%spreferences.ini"), configdir);
	CIni* pIni = new CIni(strFileName, _T("eMule"));

	CString strCurrVersion, strPrefsVersion;

	strCurrVersion = theApp.m_strCurVersionLong;
	strPrefsVersion = pIni->GetString(_T("AppVersion"));
	delete pIni;
	m_bFirstStart = false;

	CFileFind findFileName;

	if (strCurrVersion != strPrefsVersion){
		m_bFirstStart = true;
		// don't use this; it'll delete all read-only settings from the current pref.ini
//		if(findFileName.FindFile(strFileName)){
//			CFile file;
//			CFileFind findNewName;
//			CString strNewName;
//			strNewName.Format(_T("%spreferences.ini.old"), configdir);
//	
//			if (findNewName.FindFile(strNewName))
//				file.Remove(strNewName);
//	
//			file.Rename(strFileName, strNewName);
//			strFileName = strNewName;
//			// -khaos--+++> Set this to 2 so that LoadStats will load 'em from ini.old
//			loadstatsFromOld = 2;
//			// <-----khaos-
//		}
	}
	CIni ini(strFileName, _T("eMule"));
	//--- end Ozon :)

#ifdef _DEBUG
	m_iDbgHeap = ini.GetInt(_T("DebugHeap"), 1);
#else
	m_iDbgHeap = 0;
#endif

	m_nWebMirrorAlertLevel = ini.GetInt(_T("WebMirrorAlertLevel"),0,_T("eMule"));
	updatenotify=ini.GetBool(_T("UpdateNotifyTestClient"),true, _T("eMule"));

	SetUserNick(ini.GetStringUTF8(_T("Nick"), DEFAULT_NICK));
	if (strNick.IsEmpty() || IsDefaultNick(strNick))
		SetUserNick(DEFAULT_NICK);

	_stprintf(buffer,_T("%sIncoming"),appdir);
	_stprintf(incomingdir,_T("%s"),ini.GetString(_T("IncomingDir"),buffer ));
	MakeFoldername(incomingdir);

	_stprintf(buffer,_T("%sTemp"),appdir);
	_stprintf(tempdir,_T("%s"),ini.GetString(_T("TempDir"),buffer));
	MakeFoldername(tempdir);

	maxGraphDownloadRate=ini.GetFloat(_T("DownloadCapacity"),96);
	if (maxGraphDownloadRate==0) maxGraphDownloadRate=96;
	maxGraphUploadRate=ini.GetFloat(_T("UploadCapacity"),16);
	if (maxGraphUploadRate==0) maxGraphUploadRate=16;	
    minupload=ini.GetFloat(_T("MinUpload"), 1);	
	maxupload=ini.GetFloat(_T("MaxUpload"),12);
	if(maxupload >= UNLIMITED){
		::MessageBox(NULL,
		             _T("Warning, an 'unlimited' datarate for the upload limit is invalid\n\nAttention, une bande passante 'illimit�e' pour l'�mission est invalide\n\nAchtung, unbegrenzte Datarate f�r das Upload ist ung�ltig\n\nAdvirtiendo, el l�mite para el datarate del upload es inv�lido"),
					 _T("Warning/Attention/Achtung/Advirtiendo"), 
					 MB_ICONINFORMATION);
	}

	// Maella end
	if (maxupload>maxGraphUploadRate && maxupload < UNLIMITED) maxupload=maxGraphUploadRate*.8; // [TPT]
	maxdownload=ini.GetFloat(_T("MaxDownload"),76);
	if (maxdownload>maxGraphDownloadRate && maxdownload < UNLIMITED) maxdownload=maxGraphDownloadRate*.8; // [TPT]
	maxconnections=ini.GetInt(_T("MaxConnections"),GetRecommendedMaxConnections());
	maxhalfconnections=ini.GetInt(_T("MaxHalfConnections"),9);

	// reset max halfopen to a default if OS changed to SP2 or away
	int dwSP2 = ini.GetInt(_T("WinXPSP2"), -1);
	int dwCurSP2 = IsRunningXPSP2();
	if (dwSP2 != dwCurSP2){
		if (dwCurSP2 == 0)
			maxhalfconnections = 50;
		else if (dwCurSP2 == 1)
			maxhalfconnections = 9;
	}

	port=ini.GetInt(_T("Port"), DEFAULT_TCP_PORT);
	udpport=ini.GetInt(_T("UDPPort"),port+10);
	nServerUDPPort = ini.GetInt(_T("ServerUDPPort"), -1); // 0 = Don't use UDP port for servers, -1 = use a random port (for backward compatibility)
	// [TPT] - WebCache		
	// yonatan http start 
	// Superlexx - webcache
	/*char tmpWebcacheName[100];
	sprintf(tmpWebcacheName,"%s",ini.GetString(_T("webcacheName"),_T("")));
	webcacheName = tmpWebcacheName; // TODO: something more elegant*/
	webcacheName = ini.GetString(_T("webcacheName"), _T(""));
	webcachePort=ini.GetInt(_T("webcachePort"),0);
	webcacheBlockLimit=ini.GetInt(_T("webcacheBlockLimit"));
	webcacheExtraTimeout=ini.GetBool(_T("webcacheExtraTimeout"));
	PersistentConnectionsForProxyDownloads=ini.GetBool(_T("PersistentConnectionsForProxyDownloads"), false);
	WCAutoupdate=ini.GetBool(_T("WCAutoupdate"), false);
	webcacheCachesLocalTraffic=ini.GetBool(_T("webcacheCachesLocalTraffic"), true);
	webcacheEnabled=ini.GetBool(_T("webcacheEnabled"),false); //webcache disabled on first start so webcache detection on start gets called.
	detectWebcacheOnStart=ini.GetBool(_T("detectWebcacheOnStart"), true); // jp detect webcache on startup
	webcacheLastSearch=(uint32)ini.GetUInt64(_T("webcacheLastSearch"));
	webcacheLastGlobalIP=(uint32)ini.GetUInt64(_T("webcacheLastGlobalIP"));
	webcacheLastResolvedName=ini.GetString(_T("webcacheLastResolvedName"),0);	
	webcacheTrustLevel=(uint8)ini.GetUInt64(_T("webcacheTrustLevel"),30);
	// yonatan http end 
	// [TPT] - WebCache	
	maxsourceperfile=ini.GetInt(_T("MaxSourcesPerFile"),400 );
	m_wLanguageID=ini.GetWORD(_T("Language"),0);
	m_iSeeShares=(EViewSharedFilesAccess)ini.GetInt(_T("SeeShare"),vsfaNobody);
	m_iToolDelayTime=ini.GetInt(_T("ToolTipDelay"),1);
	trafficOMeterInterval=ini.GetInt(_T("StatGraphsInterval"),3);
	statsInterval=ini.GetInt(_T("statsInterval"),5);
	dontcompressavi=ini.GetBool(_T("DontCompressAvi"),false);

	deadserverretries=ini.GetInt(_T("DeadServerRetry"),1);
	if (deadserverretries > MAX_SERVERFAILCOUNT)
		deadserverretries = MAX_SERVERFAILCOUNT;
	m_dwServerKeepAliveTimeout=ini.GetInt(_T("ServerKeepAliveTimeout"),0);
	splitterbarPosition=ini.GetInt(_T("SplitterbarPosition"),75);
	if (splitterbarPosition < 9)
		splitterbarPosition = 9;
	else if (splitterbarPosition > 93)
		splitterbarPosition = 93;
	splitterbarPositionStat=ini.GetInt(_T("SplitterbarPositionStat"),30);
	splitterbarPositionStat_HL=ini.GetInt(_T("SplitterbarPositionStat_HL"),66);
	splitterbarPositionStat_HR=ini.GetInt(_T("SplitterbarPositionStat_HR"),33);
	if (splitterbarPositionStat_HR+1>=splitterbarPositionStat_HL){
		splitterbarPositionStat_HL = 66;
		splitterbarPositionStat_HR = 33;
	}
	splitterbarPositionFriend=ini.GetInt(_T("SplitterbarPositionFriend"),300);
	splitterbarPositionIRC=ini.GetInt(_T("SplitterbarPositionIRC"),200);

	m_uTransferWnd2 = ini.GetInt(_T("TransferWnd2"),DFLT_TRANSFER_WND2);

	statsMax=ini.GetInt(_T("VariousStatisticsMaxValue"),100);
	statsAverageMinutes=ini.GetInt(_T("StatsAverageMinutes"),5);
	MaxConperFive=ini.GetInt(_T("MaxConnectionsPerFiveSeconds"),GetDefaultMaxConperFive());

	reconnect=ini.GetBool(_T("Reconnect"),true);
	scorsystem=ini.GetBool(_T("Scoresystem"),true);
	ICH=ini.GetBool(_T("ICH"),true);
	autoserverlist=ini.GetBool(_T("Serverlist"),false);

	mintotray=ini.GetBool(_T("MinToTray"),false);
	addserversfromserver=ini.GetBool(_T("AddServersFromServer"),true);
	addserversfromclient=ini.GetBool(_T("AddServersFromClient"),true);
	splashscreen=ini.GetBool(_T("Splashscreen"),true);
	bringtoforeground=ini.GetBool(_T("BringToFront"),true);
	transferDoubleclick=ini.GetBool(_T("TransferDoubleClick"),true);
	beepOnError=ini.GetBool(_T("BeepOnError"),true);
	confirmExit=ini.GetBool(_T("ConfirmExit"),false);
	filterLANIPs=ini.GetBool(_T("FilterBadIPs"),true);
	m_bAllocLocalHostIP=ini.GetBool(_T("AllowLocalHostIP"),false);
	autoconnect=ini.GetBool(_T("Autoconnect"),false);
	showRatesInTitle=ini.GetBool(_T("ShowRatesOnTitle"),false);

	onlineSig=ini.GetBool(_T("OnlineSignature"),false);
	startMinimized=ini.GetBool(_T("StartupMinimized"),false);
	m_bAutoStart=ini.GetBool(_T("AutoStart"),false);
	m_bRestoreLastMainWndDlg=ini.GetBool(_T("RestoreLastMainWndDlg"),false);
	m_iLastMainWndDlgID=ini.GetInt(_T("LastMainWndDlgID"),0);
	m_bRestoreLastLogPane=ini.GetBool(_T("RestoreLastLogPane"),false);
	m_iLastLogPaneID=ini.GetInt(_T("LastLogPaneID"),0);
	safeServerConnect =ini.GetBool(_T("SafeServerConnect"),false);

	m_bTransflstRemain =ini.GetBool(_T("TransflstRemainOrder"),false);
	filterserverbyip=ini.GetBool(_T("FilterServersByIP"),false);
	filterlevel=ini.GetInt(_T("FilterLevel"),127);
	checkDiskspace=ini.GetBool(_T("CheckDiskspace"),false);	// SLUGFILLER: checkDiskspace
	m_uMinFreeDiskSpace=ini.GetInt(_T("MinFreeDiskSpace"),20*1024*1024);
	m_bSparsePartFiles=ini.GetBool(_T("SparsePartFiles"),false);
	m_strYourHostname=ini.GetString(_T("YourHostname"), _T(""));
	
	// Maella -Enable/Disable source exchange in preference- (Tarod)
	disablexs = ini.GetBool(_T("DisableXS"), false);
	// Maella end

	// Maella -Reask sources after IP change- (idea Xman)
	reaskSourceAfterIPChange = ini.GetBool(_T("ReaskSourceAfterIPChange"), true); // [TPT]
	// Maella end

	// Maella -Graph: display zoom-
	zoomFactor = ini.GetInt(_T("ZoomFactor"), 1);
	if(zoomFactor < 1) zoomFactor = 1;
	// Maella end

	m_bfadeOut = ini.GetBool(_T("FadeOut"), true);
	// Maella -Filter verbose messages-
	blockPhoenixMsg = ini.GetBool(_T("BlockPhoenixMsg"), true); // [TPT] - Filter own messages
	blockHandshakeMsg = ini.GetBool(_T("BlockHandshakeMsg"), true); // [TPT] - Filter Handshake messages
	blockDeadSources = ini.GetBool(_T("BlockDeadSourcesMsg"), true); // [TPT] - Filter dead sources	
	blockUploadEndMsg = ini.GetBool(_T("BlockUploadEndMsg"), false);	
	blockMaellaSpecificMsg = ini.GetBool(_T("BlockMaellaSpecificMsg"), true);
	blockMaellaSpecificDebugMsg = ini.GetBool(_T("BlockMaellaSpecificDebugMsg"), true);
	// Maella end

	// Maella -MTU Configuration-
	MTU=ini.GetInt(_T("MTU"),1340);
	// Maella end

	// Maella -Minimum Upload Slot-
	minUploadSlot=ini.GetInt(_T("MinUploadSlot"),3);
	if(minUploadSlot < 2) minUploadSlot = 2;
	if(minUploadSlot > 4) minUploadSlot = 4;
	// Maella end

	// Maella -New bandwidth control-
	sendSocketBufferSize=ini.GetInt(_T("SendSocketBufferSize"), 8192);
	if(sendSocketBufferSize < MINIMUM_SEND_BUFFER_SIZE) sendSocketBufferSize = MINIMUM_SEND_BUFFER_SIZE;
	if(sendSocketBufferSize > 8192) sendSocketBufferSize = 8192;
	// Maella end

	// Maella -Network Adapter Feedback Control-
	NAFCEnable=ini.GetInt(_T("NAFCEnable"), false);
	NAFCFullControl=ini.GetInt(_T("NAFCFullControl"), false);
	NAFCNetworkOut=ini.GetFloat(_T("NAFCNetworkOut"), maxGraphUploadRate);
	NAFCSlider=ini.GetInt(_T("NAFCSlider"), 90);
	if (NAFCSlider < 90 || NAFCSlider > 100)
		NAFCSlider = 90;
	m_bDinamicNafc = ini.GetBool(_T("DinamicNafc"), false);
	// Maella end

	// Maella -Accurate measure of bandwidth: eDonkey data + control, network adapter-
	datarateSamples=ini.GetInt(_T("DatarateSamples"), 10);
	if(datarateSamples < 1) datarateSamples = 1;
	if(datarateSamples > 10) datarateSamples = 10;
	// Maella end

	// Maella -One-queue-per-file- (idea bloodymad)
	enableMultiQueue=ini.GetBool(_T("EnableMultiQueue"), false);
	enableReleaseMultiQueue=ini.GetBool(_T("EnableReleaseMultiQueue"), false);
	enableNewUSS=ini.GetBool(_T("EnableNewSlotUpload"), false);
	// Maella end
				
	// [TPT] - Select process priority 
	int mainProcessPriority = ini.GetInt(_T("MainProcessPriority"), NORMAL_PRIORITY_CLASS);
	switch (GetWindowsVersion())
	{
		case _WINVER_98_:
		case _WINVER_95_:	
		case _WINVER_ME_:
//		case _WINVER_NT4_: // same as win95
			if (mainProcessPriority == ABOVE_NORMAL_PRIORITY_CLASS || mainProcessPriority == BELOW_NORMAL_PRIORITY_CLASS)
				mainProcessPriority = NORMAL_PRIORITY_CLASS;
	}

	bool found = false;	
	for (int i = 0; i < PROCESSPRIORITYNUMBER; i++)
	{
		if (PriorityClasses[i] == mainProcessPriority)
		{
			m_MainProcessPriority = mainProcessPriority;			
			found = true;
			break;
		}
	}
	if (!found) m_MainProcessPriority = NORMAL_PRIORITY_CLASS;	
	// [TPT] - Select process priority 

	// added by [TPT]-MoNKi [MoNKi: -invisible mode-]
	SetInvisibleMode( ini.GetBool(_T("InvisibleMode"), false),
					  ini.GetInt(_T("InvisibleModeHKKeyModifier"), MOD_CONTROL | MOD_SHIFT | MOD_ALT),
					  ini.GetString(_T("InvisibleModeHKKey"), _T("E")).GetAt(0));
	// End [TPT]-MoNKi

	m_bHistoryShowShared = ini.GetBool(_T("ShowSharedInHistory"), false);	 // [TPT] - MoNKi: -Downloaded History-

	m_bUseWindowsTextColorOnHighContrast = ini.GetBool(_T("WindowsTextColorOnHighContrast"), true); // [TPT] - MoNKi: -Support for High Contrast Mode-

	m_iCreditSystem=ini.GetInt(_T("CreditSystem"), 2); // [TPT] - Credit System
		
	m_bSpreadBars=ini.GetBool(_T("SpreadBars"), false); // [TPT] - SLUGFILLER: Spreadbars

	// [TPT] - IP Country
	showFlags=ini.GetBool(_T("showFlags"), true);
	showCountryName=ini.GetBool(_T("showCountryName"), true);
	// [TPT] - IP Country
	
	// [TPT] - TBH: minimule
	speedmetermin = ini.GetInt(_T("SpeedMeterMin"),0);
	speedmetermax = ini.GetInt(_T("SpeedMeterMax"),GetMaxGraphDownloadRate());	
	m_bMiniMule = ini.GetBool(_T("ShowMiniMule"), true);
	m_iMiniMuleUpdate = ini.GetInt(_T("MiniMuleUpdate"), 2);
	m_bMiniMuleLives = ini.GetBool(_T("MiniMuleLives"), false);
	m_iMiniMuleTransparency = ini.GetInt(_T("MiniMuleTransparency"), 255);
	// [TPT] - TBH: minimule

	m_bDBStyle = ini.GetBool(_T("DBStyle"), true); // [TPT] - Double buffer style in lists
	m_bShowBitmapInMenus = ini.GetBool(_T("ShowBitmapInMenus"), true);
	m_bShowInMSN7 = ini.GetBool(_T("ShowInMSN7"), false);//[TPT] - Show in MSN7
	m_bShowUpPrioInDownloadList = ini.GetBool(_T("ShowUpPrioInDownloadList"), false);


	// [TPT] - quick start	
	m_QuickStart=ini.GetBool(_T("QuickStart"),false);
	m_QuickStartMaxCon=ini.GetInt(_T("QuickStartMaxCon"), 1001);
	m_QuickStartMaxConPerFive=ini.GetInt(_T("QuickStartMaxConPerFive"), 151);
	if (maxconnections == m_QuickStartMaxCon) 
		maxconnections = 400;
	if (MaxConperFive == m_QuickStartMaxConPerFive) 
		MaxConperFive = GetDefaultMaxConperFive();
	// [TPT] - quick start

	// [TPT] - SLUGFILLER: hideOS
	hideOS=ini.GetInt(_T("HideOvershares"),5);
	selectiveShare=ini.GetBool(_T("SelectiveShare"),false);
	// [TPT] - SLUGFILLER: hideOS

	infiniteQueue=ini.GetBool(_T("InfiniteQueue"),false);	// [TPT] - SLUGFILLER: infiniteQueue
	// [TPT] - itsonlyme: displayOptions START
	showFileSystemIcon=ini.GetBool(_T("ShowFileSystemIcon"),true);
	showLocalRating=ini.GetBool(_T("ShowLocalRating"),true); // [TPT] - SLUGFILLER: showComments
	// [TPT] - itsonlyme: displayOptions END

	m_currentAdapterIndex = ini.GetInt(_T("CurrentAdapterIndex"),0); // [TPT] - NAFC Selection

	// [TPT] - eWombat SNAFU v2
	m_bSnafu=ini.GetBool(_T("Snafu"), false);
	m_bAntiCreditTheft=ini.GetBool(_T("AntiCreditTheft"), false);
	m_bAntiFriendshare=ini.GetBool(_T("AntiFriendShare"), false);
	
	cumulateBandwidth = ini.GetBool(_T("CumulateBandwidth"), false); // [TPT]-Cumulate Bandwidth
	minimizeSlots = ini.GetBool(_T("MinimizeSlots"), false); // [TPT] - Pawcio: MUS
	manageConnection = ini.GetBool(_T("ManageConnection"), false); // [TPT] - Manage Connection
	
	// [TPT] - khaos::categorymod+ Load Preferences
	m_bShowCatNames = ini.GetBool(_T("ShowCatName"),true);	
	m_bActiveCatDefault = ini.GetBool(_T("ActiveCatDefault"), true);
	m_bSelCatOnAdd = ini.GetBool(_T("SelCatOnAdd"), true);
	m_bAutoSetResumeOrder = ini.GetBool(_T("AutoSetResumeOrder"), true);	
	m_iStartDLInEmptyCats = ini.GetInt(_T("StartDLInEmptyCats"), 0);
	m_bUseAutoCat = ini.GetBool(_T("UseAutoCat"), true);
	m_bShowPriorityInTab = ini.GetBool(_T("ShowPriorityInTab"), false);
	// [TPT] - khaos::categorymod-

	// [TPT] - Memory Consuming
	m_bMemoryConsumingGraph = ini.GetBool(_T("MemoryConsuming"),true);
	m_bNAFCGraph = ini.GetBool(_T("NAFCGraph"),true);
	m_bOverheadGraph = ini.GetBool(_T("OverheadGraph"), true);
	// [TPT] - TBH-AutoBackup Begin
	autobackup = ini.GetBool(_T("AutoBackup"),false);
	autobackup2 = ini.GetBool(_T("AutoBackup2"),false);
	// [TPT] - TBH-AutoBackup End
	
	// [TPT] - MoNKi: -UPnPNAT Support-
	m_bUPnPNat = ini.GetBool(_T("UPnPNAT"), false);
	m_bUPnPNatWeb = ini.GetBool(_T("UPnPNAT_Web"), false);	
	m_bUPnPVerboseLog = ini.GetBool(_T("UPnPVerbose"), true, _T("eMule"));
	m_iUPnPPort = ini.GetInt(_T("UPnPPort"), 0, _T("eMule"));
	// [TPT] - MoNKi: -UPnPNAT Support-

	// [TPT] - Powershare
	m_iPowershareMode=ini.GetInt(_T("PowershareMode"),0);//Disabled by default
	PowerShareLimit=ini.GetInt(_T("PowerShareLimit"),0);
	// [TPT] - Powershare END
	
	m_bSaveUploadQueueWaitTime = ini.GetBool(_T("SaveUploadQueueWaitTime"), false/*true changed by sirob*/); // [TPT] - SUQWT
	
	m_bUnlimitedUP = ini.GetBool(_T("UnlimitedUp"), false); //[TPT] - Unlimited upload with no downloads

	// [TPT] - Fakecheck
	m_FakesDatVersion=ini.GetInt(_T("FakesDatVersion"),0);
	UpdateFakeStartup=ini.GetBool(_T("UpdateFakeStartup"),false);
	_stprintf(UpdateURLFakeList,_T("%s"),ini.GetString(_T("UpdateURLFakeList"),_T("")));
	if (StrStrI(UpdateURLFakeList,_T("http://www.emuleitor.com/downloads/Morph/fakes.txt")) ||
		StrStrI(UpdateURLFakeList,_T("http://membres.lycos.fr/ipfilter/fakes.txt")))
		_stprintf(UpdateURLFakeList,_T("%s"),_T(""));	
	// [TPT] - Fakecheck END

	// [TPT] - Manual eMfriend.met download
	_stprintf(m_updateURLFriendList, _T("%s"), ini.GetString(_T("UpdateURLFriendList"),_T("")));	
	// [TPT] - Manual eMfriend.met download
	// Barry - New properties...
	autoconnectstaticonly = ini.GetBool(_T("AutoConnectStaticOnly"),false); 
	autotakeed2klinks = ini.GetBool(_T("AutoTakeED2KLinks"),true); 
	addnewfilespaused = ini.GetBool(_T("AddNewFilesPaused"),false); 
	depth3D = ini.GetInt(_T("3DDepth"), 0);

	// as temporarial converter for previous versions
	if (strPrefsVersion < _T("0.25a")) // before 0.25a
		if (ini.GetBool(_T("FlatStatusBar"),false))
			depth3D = 0;
		else 
			depth3D = 5;

	//START [TPT] - enkeyDEV(kei-kun) -Notifier preferences-
    useNotifierUserTimings=ini.GetBool(_T("UseNotifierUserTimings"), false);
	notifierLessFramerate=ini.GetBool(_T("NotifierLessFramerate"), false);
	notifierUserTimeToShow=ini.GetInt(_T("NotifierUserTimeToShow"), 400);
	notifierUserTimeToStay=ini.GetInt(_T("NotifierUserTimeToStay"), 4000);
	notifierUserTimeToHide=ini.GetInt(_T("NotifierUserTimeToHide"), 300);		    
	notifierAutoClose=ini.GetBool(_T("NotifierAutoClose"),true); 	
	useErrorNotifier=ini.GetBool(_T("NotifyOnError"),false);			// added by InterCeptor (notify on error) 11.11.02
	useNewDownloadNotifier=ini.GetBool(_T("NotifyOnNewDownload"),false);
    useDownloadNotifier=ini.GetBool(_T("NotifyOnDownload"),false);	// Added by enkeyDEV
	useChatNotifier=ini.GetBool(_T("NotifyOnChat"),false);
    useLogNotifier=ini.GetBool(_T("NotifyOnLog"),false);
    useSoundInNotifier=ini.GetBool(_T("NotifierUseSound"),false);
	notifierPopsEveryChatMsg=ini.GetBool(_T("NotifierPopEveryChatMessage"),false);
	notifierImportantError=ini.GetBool(_T("NotifyOnImportantError"),false);
	notifierNewPvtMsg=ini.GetBool(_T("NotifyNewPvtMsg"),false);//Rocks
	_stprintf(notifierSoundFilePath,_T("%s"),ini.GetString(_T("NotifierSoundPath"),_T("")));
	_stprintf(notifierConfiguration,_T("%s"),ini.GetString(_T("NotifierConfiguration"),_T("")));
	notifierNewVersion=ini.GetBool(_T("NotifierPopNewVersion"),false);
	notifierSearchCompleted=ini.GetBool(_T("NotifierPopSearchCompleted"), false);
	//END [TPT] - enkeyDEV(kei-kun) -Notifier preferences-
	_stprintf(datetimeformat,_T("%s"),ini.GetString(_T("DateTimeFormat"),_T("%A, %x, %X")));
	if (_tcslen(datetimeformat)==0) _tcscpy(datetimeformat,_T("%A, %x, %X"));

	_stprintf(datetimeformat4log,_T("%s"),ini.GetString(_T("DateTimeFormat4Log"),_T("%c")));
	if (_tcslen(datetimeformat4log)==0) _tcscpy(datetimeformat4log,_T("%c"));

	_stprintf(m_sircserver,_T("%s"),ini.GetString(_T("DefaultIRCServerNew"),_T("ircchat.emule-project.net")));
	_stprintf(m_sircnick,_T("%s"),ini.GetString(_T("IRCNick"),_T("eMule")));
	m_bircaddtimestamp=ini.GetBool(_T("IRCAddTimestamp"),true);
	_stprintf(m_sircchannamefilter,_T("%s"),ini.GetString(_T("IRCFilterName"), _T("") ));
	m_bircusechanfilter=ini.GetBool(_T("IRCUseFilter"), false);
	m_iircchanneluserfilter=ini.GetInt(_T("IRCFilterUser"), 0);
	_stprintf(m_sircperformstring,_T("%s"),ini.GetString(_T("IRCPerformString"), _T("") ));
	m_bircuseperform=ini.GetBool(_T("IRCUsePerform"), false);
	m_birclistonconnect=ini.GetBool(_T("IRCListOnConnect"), true);
	m_bircacceptlinks=ini.GetBool(_T("IRCAcceptLink"), true);
	m_bircacceptlinksfriends=ini.GetBool(_T("IRCAcceptLinkFriends"), true);
	m_bircsoundevents=ini.GetBool(_T("IRCSoundEvents"), false);
	m_bircignoremiscmessage=ini.GetBool(_T("IRCIgnoreMiscMessages"), false);
	m_bircignorejoinmessage=ini.GetBool(_T("IRCIgnoreJoinMessages"), true);
	m_bircignorepartmessage=ini.GetBool(_T("IRCIgnorePartMessages"), true);
	m_bircignorequitmessage=ini.GetBool(_T("IRCIgnoreQuitMessages"), true);
	m_bircignoreemuleprotoaddfriend=ini.GetBool(_T("IRCIgnoreEmuleProtoAddFriend"), false);
	m_bircallowemuleprotoaddfriend=ini.GetBool(_T("IRCAllowEmuleProtoAddFriend"), true);
	m_bircignoreemuleprotosendlink=ini.GetBool(_T("IRCIgnoreEmuleProtoSendLink"), false);
	m_birchelpchannel=ini.GetBool(_T("IRCHelpChannel"),true);
	smartidcheck=ini.GetBool(_T("SmartIdCheck"),true);

	log2disk=ini.GetBool(_T("SaveLogToDisk"),false);
	uMaxLogFileSize = ini.GetInt(_T("MaxLogFileSize"), 1024*1024);
	iMaxLogBuff = ini.GetInt(_T("MaxLogBuff"),64) * 1024;
	m_bEnableVerboseOptions=ini.GetBool(_T("VerboseOptions"), true);
	if (m_bEnableVerboseOptions)
	{
		m_bVerbose=ini.GetBool(_T("Verbose"),false);
		m_bFullVerbose=ini.GetBool(_T("FullVerbose"),false);
		debug2disk=ini.GetBool(_T("SaveDebugToDisk"),false);
		m_bDebugSourceExchange=ini.GetBool(_T("DebugSourceExchange"),false);
		m_bLogBannedClients=ini.GetBool(_T("LogBannedClients"), true);
		m_bLogRatingDescReceived=ini.GetBool(_T("LogRatingDescReceived"),true);
		m_bLogSecureIdent=ini.GetBool(_T("LogSecureIdent"),true);
		m_bLogFilteredIPs=ini.GetBool(_T("LogFilteredIPs"),true);
		m_bLogFileSaving=ini.GetBool(_T("LogFileSaving"),false);
        m_bLogA4AF=ini.GetBool(_T("LogA4AF"),false); // ZZ:DownloadManager
		m_bLogUlDlEvents=ini.GetBool(_T("LogUlDlEvents"),true);
		// [TPT] - WebCache 
		m_bLogWebCacheEvents=ini.GetBool(_T("LogWebCacheEvents"),true);//JP log webcache events
		m_bLogICHEvents=ini.GetBool(_T("LogICHEvents"),true);//JP log ICH events
	}
	else
	{
		if (m_bRestoreLastLogPane && m_iLastLogPaneID>=2)
			m_iLastLogPaneID = 1;
	}
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	// following options are for debugging or when using an external debug device viewer only.
	m_iDebugServerTCPLevel=ini.GetInt(_T("DebugServerTCP"),0);
	m_iDebugServerUDPLevel=ini.GetInt(_T("DebugServerUDP"),0);
	m_iDebugServerSourcesLevel=ini.GetInt(_T("DebugServerSources"),0);
	m_iDebugServerSearchesLevel=ini.GetInt(_T("DebugServerSearches"),0);
	m_iDebugClientTCPLevel=ini.GetInt(_T("DebugClientTCP"),0);
	m_iDebugClientUDPLevel=ini.GetInt(_T("DebugClientUDP"),0);
	m_iDebugClientKadUDPLevel=ini.GetInt(_T("DebugClientKadUDP"),0);
#else
	// for normal release builds ensure that those options are all turned off
	m_iDebugServerTCPLevel=0;
	m_iDebugServerUDPLevel=0;
	m_iDebugServerSourcesLevel=0;
	m_iDebugServerSearchesLevel=0;
	m_iDebugClientTCPLevel=0;
	m_iDebugClientUDPLevel=0;
	m_iDebugClientKadUDPLevel=0;
#endif

	m_bpreviewprio=ini.GetBool(_T("PreviewPrio"),false);
	m_bupdatequeuelist=ini.GetBool(_T("UpdateQueueListPref"),false);
	m_bmanualhighprio=ini.GetBool(_T("ManualHighPrio"),false);
	m_btransferfullchunks=ini.GetBool(_T("FullChunkTransfers"),true);
	m_istartnextfile=ini.GetInt(_T("StartNextFile"),0);
	m_bshowoverhead=ini.GetBool(_T("ShowOverhead"),false);
	moviePreviewBackup=ini.GetBool(_T("VideoPreviewBackupped"),true);
	m_iPreviewSmallBlocks=ini.GetInt(_T("PreviewSmallBlocks"), 0);
	m_iPreviewCopiedArchives=ini.GetInt(_T("PreviewCopiedArchives"), 1);
	m_iInspectAllFileTypes=ini.GetInt(_T("InspectAllFileTypes"), 0);

	// read file buffer size (with backward compatibility)
	m_iFileBufferSize=ini.GetInt(_T("FileBufferSizePref"),0); // old setting
	if (m_iFileBufferSize == 0)
		m_iFileBufferSize = 256*1024;
	else
		m_iFileBufferSize = ((m_iFileBufferSize*15000 + 512)/1024)*1024;
	m_iFileBufferSize=ini.GetInt(_T("FileBufferSize"),m_iFileBufferSize);

	// read queue size (with backward compatibility)
	m_iQueueSize=ini.GetInt(_T("QueueSizePref"),0); // old setting
	if (m_iQueueSize == 0)
		m_iQueueSize = 50*100;
	else
		m_iQueueSize = m_iQueueSize*100;
	m_iQueueSize=ini.GetInt(_T("QueueSize"),m_iQueueSize);

	m_iCommitFiles=ini.GetInt(_T("CommitFiles"), 1); // 1 = "commit" on application shut down; 2 = "commit" on each file saveing
	versioncheckdays=ini.GetInt(_T("Check4NewVersionDelay"),5);
	m_bDAP=ini.GetBool(_T("DAPPref"),true);
	m_bUAP=ini.GetBool(_T("UAPPref"),true);
	m_bPreviewOnIconDblClk=ini.GetBool(_T("PreviewOnIconDblClk"),false);
	indicateratings=ini.GetBool(_T("IndicateRatings"),true);
	watchclipboard=ini.GetBool(_T("WatchClipboard4ED2kFilelinks"),false);
	m_iSearchMethod=ini.GetInt(_T("SearchMethod"),0);

	showCatTabInfos=ini.GetBool(_T("ShowInfoOnCatTabs"),false);
//	resumeSameCat=ini.GetBool(_T("ResumeNextFromSameCat"),false);
	dontRecreateGraphs =ini.GetBool(_T("DontRecreateStatGraphsOnResize"),false);
	m_bExtControls =ini.GetBool(_T("ShowExtControls"),false);

	versioncheckLastAutomatic=ini.GetInt(_T("VersionCheckLastAutomatic"),0);
	m_bDisableKnownClientList=ini.GetInt(_T("DisableKnownClientList"),false);
	m_bDisableQueueList=ini.GetInt(_T("DisableQueueList"),false);
	m_bCreditSystem=ini.GetInt(_T("UseCreditSystem"),true);
	scheduler=ini.GetBool(_T("EnableScheduler"),false);
	msgonlyfriends=ini.GetBool(_T("MessagesFromFriendsOnly"),false);
	msgsecure=ini.GetBool(_T("MessageFromValidSourcesOnly"),true);
	autofilenamecleanup=ini.GetBool(_T("AutoFilenameCleanup"),false);
	m_bUseAutocompl=ini.GetBool(_T("UseAutocompletion"),true);
	m_bShowDwlPercentage=ini.GetBool(_T("ShowDwlPercentage"),false);
	networkkademlia=ini.GetBool(_T("NetworkKademlia"),false);
	networked2k=ini.GetBool(_T("NetworkED2K"),true);
	m_bRemove2bin=ini.GetBool(_T("RemoveFilesToBin"),true);
	m_bShowCopyEd2kLinkCmd=ini.GetBool(_T("ShowCopyEd2kLinkCmd"),false);

	m_iMaxChatHistory=ini.GetInt(_T("MaxChatHistoryLines"),100);
	if (m_iMaxChatHistory < 1)
		m_iMaxChatHistory = 100;
	maxmsgsessions=ini.GetInt(_T("MaxMessageSessions"),50);
	m_bShowActiveDownloadsBold = ini.GetBool(_T("ShowActiveDownloadsBold"), false);

	_stprintf(TxtEditor,_T("%s"),ini.GetString(_T("TxtEditor"),_T("notepad.exe")));
	_stprintf(VideoPlayer,_T("%s"),ini.GetString(_T("VideoPlayer"),_T("")));
	
	_stprintf(m_sTemplateFile,_T("%s"),ini.GetString(_T("WebTemplateFile"),_T("eMule.tmpl")));

	_stprintf(messageFilter,_T("%s"),ini.GetString(_T("MessageFilter"),_T("Your client has an infinite queue|Your client is connecting too fast|fastest download speed")));
	commentFilter = ini.GetString(_T("CommentFilter"),_T("http://|www."));
	commentFilter.MakeLower();
	_stprintf(filenameCleanups,_T("%s"),ini.GetString(_T("FilenameCleanups"),_T("http|www.|.com|shared|powered|sponsored|sharelive|filedonkey|saugstube|eselfilme|eseldownloads|emulemovies|spanishare|eselpsychos.de|saughilfe.de|goldesel.6x.to|freedivx.org|elitedivx|deviance|-ftv|ftv|-flt|flt")));
	m_iExtractMetaData=ini.GetInt(_T("ExtractMetaData"),2); // 0=disable, 1=mp3+avi, 2=MediaDet
	m_bAdjustNTFSDaylightFileTime=ini.GetBool(_T("AdjustNTFSDaylightFileTime"), true);

	m_bUseSecureIdent=ini.GetBool(_T("SecureIdent"),true);
	m_bAdvancedSpamfilter=ini.GetBool(_T("AdvancedSpamFilter"),true);
	m_bRemoveFinishedDownloads=ini.GetBool(_T("AutoClearCompleted"),false);
	m_bUseOldTimeRemaining= ini.GetBool(_T("UseSimpleTimeRemainingcomputation"),false);

	// Toolbar
	m_sToolbarSettings = ini.GetString(_T("ToolbarSetting"), strDefaultToolbar);
	m_sToolbarBitmap = ini.GetString(_T("ToolbarBitmap"), _T(""));
	m_sToolbarBitmapFolder = ini.GetString(_T("ToolbarBitmapFolder"), appdir + _T("skins"));
	m_nToolbarLabels = (EToolbarLabelType)ini.GetInt(_T("ToolbarLabels"), CMuleToolbarCtrl::GetDefaultLabelType());
	m_bReBarToolbar = ini.GetBool(_T("ReBarToolbar"), 1);
	m_sizToolbarIconSize.cx = m_sizToolbarIconSize.cy = ini.GetInt(_T("ToolbarIconSize"), 32);
	m_iStraightWindowStyles=ini.GetInt(_T("StraightWindowStyles"),0);
	m_strSkinProfile = ini.GetString(_T("SkinProfile"), _T(""));
	m_strSkinProfileDir = ini.GetString(_T("SkinProfileDir"), appdir + _T("skins"));

	ini.SerGet(true, downloadColumnWidths,
		ARRSIZE(downloadColumnWidths), _T("DownloadColumnWidths"));
	ini.SerGet(true, downloadColumnHidden,
		ARRSIZE(downloadColumnHidden), _T("DownloadColumnHidden"));
	ini.SerGet(true, downloadColumnOrder,
		ARRSIZE(downloadColumnOrder), _T("DownloadColumnOrder"));
	// [TPT] - TBH Transfers Window Style
	ini.SerGet(true, downloadClientsColumnWidths,
		ARRSIZE(downloadClientsColumnWidths), _T("DownloadClientsColumnWidths"));
	ini.SerGet(true, downloadClientsColumnHidden,
		ARRSIZE(downloadClientsColumnHidden), _T("DownloadClientsColumnHidden"));
	ini.SerGet(true, downloadClientsColumnOrder,
		ARRSIZE(downloadClientsColumnOrder), _T("DownloadClientsColumnOrder"));
	// [TPT] - TBH Transfers Window Style
	ini.SerGet(true, uploadColumnWidths,
		ARRSIZE(uploadColumnWidths), _T("UploadColumnWidths"));
	ini.SerGet(true, uploadColumnHidden,
		ARRSIZE(uploadColumnHidden), _T("UploadColumnHidden"));
	ini.SerGet(true, uploadColumnOrder,
		ARRSIZE(uploadColumnOrder), _T("UploadColumnOrder"));
	ini.SerGet(true, queueColumnWidths,
		ARRSIZE(queueColumnWidths), _T("QueueColumnWidths"));
	ini.SerGet(true, queueColumnHidden,
		ARRSIZE(queueColumnHidden), _T("QueueColumnHidden"));
	ini.SerGet(true, queueColumnOrder,
		ARRSIZE(queueColumnOrder), _T("QueueColumnOrder"));
	ini.SerGet(true, searchColumnWidths,
		ARRSIZE(searchColumnWidths), _T("SearchColumnWidths"));
	ini.SerGet(true, searchColumnHidden,
		ARRSIZE(searchColumnHidden), _T("SearchColumnHidden"));
	ini.SerGet(true, searchColumnOrder,
		ARRSIZE(searchColumnOrder), _T("SearchColumnOrder"));
	ini.SerGet(true, sharedColumnWidths,
		ARRSIZE(sharedColumnWidths), _T("SharedColumnWidths"));
	ini.SerGet(true, sharedColumnHidden,
		ARRSIZE(sharedColumnHidden), _T("SharedColumnHidden"));
	ini.SerGet(true, sharedColumnOrder,
		ARRSIZE(sharedColumnOrder), _T("SharedColumnOrder"));
	ini.SerGet(true, serverColumnWidths,
		ARRSIZE(serverColumnWidths), _T("ServerColumnWidths"));
	ini.SerGet(true, serverColumnHidden,
		ARRSIZE(serverColumnHidden), _T("ServerColumnHidden"));
	ini.SerGet(true, serverColumnOrder,
		ARRSIZE(serverColumnOrder), _T("ServerColumnOrder"));
	ini.SerGet(true, clientListColumnWidths,
		ARRSIZE(clientListColumnWidths), _T("ClientListColumnWidths"));
	ini.SerGet(true, clientListColumnHidden,
		ARRSIZE(clientListColumnHidden), _T("ClientListColumnHidden"));
	ini.SerGet(true, clientListColumnOrder,
		ARRSIZE(clientListColumnOrder), _T("ClientListColumnOrder"));
	
	ini.SerGet(true, FilenamesListColumnWidths,
		ARRSIZE(FilenamesListColumnWidths), _T("FilenamesListColumnWidths"));
	ini.SerGet(true, FilenamesListColumnHidden,
		ARRSIZE(FilenamesListColumnHidden), _T("FilenamesListColumnHidden"));
	ini.SerGet(true, FilenamesListColumnOrder,
		ARRSIZE(FilenamesListColumnOrder), _T("FilenamesListColumnOrder"));
	// [TPT] - MoNKi: -Downloaded History-
	ini.SerGet(true, m_HistoryColumnWidths,
		ARRSIZE(m_HistoryColumnWidths), _T("m_HistoryColumnWidths"));
	ini.SerGet(true, m_HistoryColumnHidden,
		ARRSIZE(m_HistoryColumnHidden), _T("m_HistoryColumnHidden"));
	ini.SerGet(true, m_HistoryColumnOrder,
		ARRSIZE(m_HistoryColumnOrder), _T("m_HistoryColumnOrder"));
	// [TPT] - MoNKi: -Downloaded History-
	// [TPT] - Friend State Column
	ini.SerGet(true, friendListColumnWidths,
		ARRSIZE(friendListColumnWidths), _T("friendListColumnWidths"));
	ini.SerGet(true, friendListColumnHidden,
		ARRSIZE(friendListColumnHidden), _T("friendListColumnHidden"));
	ini.SerGet(true, friendListColumnOrder,
		ARRSIZE(friendListColumnOrder), _T("friendListColumnOrder"));
	// [TPT] - Friend State Column

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	// [TPT] - SLUGFILLER: multiSort - load multiple params
	ini.SerGet(true, tableSortItemDownload,
		ARRSIZE(tableSortItemDownload), _T("TableSortItemDownload"), NULL, -1);
	ini.SerGet(true, (BYTE*)tableSortAscendingDownload,
		ARRSIZE(tableSortAscendingDownload), _T("TableSortAscendingDownload"));
	// [TPT] - TBH Transfers Window Style
	ini.SerGet(true, tableSortItemDownloadClient,
		ARRSIZE(tableSortItemDownloadClient), _T("TableSortItemDownloadClient"), NULL, -1);
	ini.SerGet(true, (BYTE*)tableSortAscendingDownloadClient,
		ARRSIZE(tableSortAscendingDownloadClient), _T("TableSortAscendingDownloadClient")); 
	// [TPT] - TBH Transfers Window Style
	// [TPT] - MoNKi: -Downloaded History-
	ini.SerGet(true, tableSortItemHistory,
		ARRSIZE(tableSortItemHistory), _T("TableSortItemHistory"), NULL, -1); 
	ini.SerGet(true, (BYTE*)tableSortAscendingHistory,
		ARRSIZE(tableSortAscendingHistory), _T("TableSortAscendingHistory"), NULL, -1); 
	// [TPT] - MoNKi: -Downloaded History-
	// [TPT] - Friend State Column
	ini.SerGet(true, tableSortItemFriendList,
		ARRSIZE(tableSortItemFriendList), _T("TableSortItemFriendList"), NULL, -1); 
	ini.SerGet(true, (BYTE*)tableSortAscendingFriendList,
		ARRSIZE(tableSortAscendingFriendList), _T("TableSortAscendingFriendList"), NULL, -1); 
	// [TPT] - Friend State Column
	ini.SerGet(true, tableSortItemUpload,
		ARRSIZE(tableSortItemUpload), _T("TableSortItemUpload"), NULL, -1);
	ini.SerGet(true, (BYTE*)tableSortAscendingUpload,
		ARRSIZE(tableSortAscendingUpload), _T("TableSortAscendingUpload"));
	ini.SerGet(true, tableSortItemQueue,
		ARRSIZE(tableSortItemQueue), _T("TableSortItemQueue"), NULL, -1);
	ini.SerGet(true, (BYTE*)tableSortAscendingQueue,
		ARRSIZE(tableSortAscendingQueue), _T("TableSortAscendingQueue"));
	ini.SerGet(true, tableSortItemSearch,
		ARRSIZE(tableSortItemSearch), _T("TableSortItemSearch"), NULL, -1);
	ini.SerGet(true, (BYTE*)tableSortAscendingSearch,
		ARRSIZE(tableSortAscendingSearch), _T("TableSortAscendingSearch"));
	ini.SerGet(true, tableSortItemShared,
		ARRSIZE(tableSortItemShared), _T("TableSortItemShared"), NULL, -1);
	ini.SerGet(true, (BYTE*)tableSortAscendingShared,
		ARRSIZE(tableSortAscendingShared), _T("TableSortAscendingShared"));
	ini.SerGet(true, tableSortItemServer,
		ARRSIZE(tableSortItemServer), _T("TableSortItemServer"), NULL, -1);
	ini.SerGet(true, (BYTE*)tableSortAscendingServer,
		ARRSIZE(tableSortAscendingServer), _T("TableSortAscendingServer"));
	ini.SerGet(true, tableSortItemClientList,
		ARRSIZE(tableSortItemClientList), _T("TableSortItemClientList"), NULL, -1);
	ini.SerGet(true, (BYTE*)tableSortAscendingClientList,
		ARRSIZE(tableSortAscendingClientList), _T("TableSortAscendingClientList"));
	ini.SerGet(true, tableSortItemFilenames,
		ARRSIZE(tableSortItemFilenames), _T("TableSortItemFilenames"), NULL, -1);
	ini.SerGet(true, (BYTE*)tableSortAscendingFilenames,
		ARRSIZE(tableSortAscendingFilenames), _T("TableSortAscendingFilenames"));
	// topmost must be valid
	if (tableSortItemDownload[0] == -1) {
		tableSortItemDownload[0] = 0;
		tableSortAscendingDownload[0] = true;
	}
	if (tableSortItemDownloadClient[0] == -1) {
		tableSortItemDownloadClient[0] = 0;
		tableSortAscendingDownloadClient[0] = true;
	} // [TPT] - TBH Transfers Window Style
	if (tableSortItemUpload[0] == -1) {
		tableSortItemUpload[0] = 0;
		tableSortAscendingUpload[0] = true;
	}
	if (tableSortItemQueue[0] == -1) {
		tableSortItemQueue[0] = 0;
		tableSortAscendingQueue[0] = true;
	}
	if (tableSortItemSearch[0] == -1) {
		tableSortItemSearch[0] = 0;
		tableSortAscendingSearch[0] = true;
	}
	if (tableSortItemShared[0] == -1) {
		tableSortItemShared[0] = 0;
		tableSortAscendingShared[0] = true;
	}
	if (tableSortItemServer[0] == -1) {
		tableSortItemServer[0] = 0;
		tableSortAscendingServer[0] = true;
	}
	if (tableSortItemClientList[0] == -1) {
		tableSortItemClientList[0] = 0;
		tableSortAscendingClientList[0] = true;
	}
	if (tableSortItemFilenames[0] == -1) {
		tableSortItemFilenames[0] = 1;
		tableSortAscendingFilenames[0] = false;
	}
	// [TPT] - MoNKi: -Downloaded History-
	if (tableSortItemHistory[0] == -1) {
		tableSortItemHistory[0] = 0;
		tableSortAscendingHistory[0] = true;
	}
	// [TPT] - MoNKi: -Downloaded History-
	// [TPT] - Friend State Column
	if (tableSortItemFriendList[0] == -1) {
		tableSortItemFriendList[0] = 0;
		tableSortAscendingFriendList[0] = true;
	}
	// [TPT] - Friend State Column
	// [TPT] - SLUGFILLER: multiSort

	LPBYTE pData = NULL;
	UINT uSize = sizeof m_lfHyperText;
	if (ini.GetBinary(_T("HyperTextFont"), &pData, &uSize) && uSize == sizeof m_lfHyperText)
		MEMCOPY(&m_lfHyperText, pData, sizeof m_lfHyperText);
	else
		MEMZERO(&m_lfHyperText, sizeof m_lfHyperText);
	delete[] pData;

	pData = NULL;
	uSize = sizeof m_lfLogText;
	if (ini.GetBinary(_T("LogTextFont"), &pData, &uSize) && uSize == sizeof m_lfLogText)
		MEMCOPY(&m_lfLogText, pData, sizeof m_lfLogText);
	else
		MEMZERO(&m_lfLogText, sizeof m_lfLogText);
	delete[] pData;

	m_crLogError = ini.GetColRef(_T("LogErrorColor"), m_crLogError);
	m_crLogWarning = ini.GetColRef(_T("LogWarningColor"), m_crLogWarning);
	m_crLogSuccess = ini.GetColRef(_T("LogSuccessColor"), m_crLogSuccess);

	if (statsAverageMinutes < 1)
		statsAverageMinutes = 5;

	// ZZ:UploadSpeedSense -->
    m_bDynUpEnabled = ini.GetBool(_T("USSEnabled"), false, _T("eMule"));
    m_bDynUpUseMillisecondPingTolerance = ini.GetBool(_T("USSUseMillisecondPingTolerance"), false);
    m_iDynUpPingTolerance = ini.GetInt(_T("USSPingTolerance"), 500, _T("eMule"));
	m_iDynUpPingToleranceMilliseconds = ini.GetInt(_T("USSPingToleranceMilliseconds"), 200);
	if( minupload < 1 )
	{
		minupload = 1;
	}
	m_iDynUpGoingUpDivider = ini.GetInt(_T("USSGoingUpDivider"), 1000, _T("eMule"));
    m_iDynUpGoingDownDivider = ini.GetInt(_T("USSGoingDownDivider"), 1000, _T("eMule"));
    m_iDynUpNumberOfPings = ini.GetInt(_T("USSNumberOfPings"), 1, _T("eMule"));
	// ZZ:UploadSpeedSense <--

    m_bA4AFSaveCpu = ini.GetBool(_T("A4AFSaveCpu"), false, _T("eMule")); // ZZ:DownloadManager

	m_bRunAsUser = ini.GetBool(_T("RunAsUnprivilegedUser"), false);
	m_bOpenPortsOnStartUp = ini.GetBool(_T("OpenPortsOnStartUp"), false);
	m_byLogLevel = ini.GetInt(_T("DebugLogLevel"), DLP_VERYLOW);
	m_bTrustEveryHash = ini.GetBool(_T("AICHTrustEveryHash"), false);


	///////////////////////////////////////////////////////////////////////////
	// Section: "Proxy"
	//
	proxy.EnablePassword = ini.GetBool(_T("ProxyEnablePassword"),false,_T("Proxy"));
	proxy.UseProxy = ini.GetBool(_T("ProxyEnableProxy"),false,_T("Proxy"));
	_sntprintf(proxy.name, ARRSIZE(proxy.name), _T("%s"), ini.GetString(_T("ProxyName"), _T(""), _T("Proxy")));
	_snprintf(proxy.password, ARRSIZE(proxy.password), "%s", T2CA(ini.GetString(_T("ProxyPassword"), _T(""), _T("Proxy"))));
	_snprintf(proxy.user, ARRSIZE(proxy.user), "%s", T2CA(ini.GetString(_T("ProxyUser"), _T(""), _T("Proxy"))));
	proxy.port = ini.GetInt(_T("ProxyPort"),1080,_T("Proxy"));
	proxy.type = ini.GetInt(_T("ProxyType"),PROXYTYPE_NOPROXY,_T("Proxy"));
	m_bIsASCWOP = ini.GetBool(_T("ConnectWithoutProxy"),false,_T("Proxy"));
	m_bShowProxyErrors = ini.GetBool(_T("ShowErrors"),false,_T("Proxy"));


	///////////////////////////////////////////////////////////////////////////
	// Section: "Statistics"
	//
	statsSaveInterval = ini.GetInt(_T("SaveInterval"), 60, _T("Statistics"));
	statsConnectionsGraphRatio = ini.GetInt(_T("statsConnectionsGraphRatio"), 3, _T("Statistics"));
	_stprintf(statsExpandedTreeItems,_T("%s"),ini.GetString(_T("statsExpandedTreeItems"),_T("111000000100000110000010000011110000010010"),_T("Statistics")));
	CString buffer2;
	for (int i=0;i<ARRSIZE(m_adwStatsColors);i++) {
		buffer2.Format(_T("StatColor%i"),i);
		_stprintf(buffer,_T("%s"),ini.GetString(buffer2,_T("0"),_T("Statistics")));
		m_adwStatsColors[i] = 0;
		if (_stscanf(buffer, _T("%i"), &m_adwStatsColors[i]) != 1 || m_adwStatsColors[i] == 0)
			ResetStatsColor(i);
	}
	m_bShowVerticalHourMarkers = ini.GetBool(_T("ShowVerticalHourMarkers"),true,_T("Statistics"));

	// -khaos--+++> Load Stats
	// I changed this to a seperate function because it is now also used
	// to load the stats backup and to load stats from preferences.ini.old.
	LoadStats(loadstatsFromOld);
	// <-----khaos-

	///////////////////////////////////////////////////////////////////////////
	// Section: "WebServer"
	//
	_stprintf(m_sWebPassword,_T("%s"),ini.GetString(_T("Password"), _T(""),_T("WebServer")));
	_stprintf(m_sWebLowPassword,_T("%s"),ini.GetString(_T("PasswordLow"), _T("")));
	m_nWebPort=ini.GetInt(_T("Port"), 4711);
	m_bWebEnabled=ini.GetBool(_T("Enabled"), false);
	m_bWebUseGzip=ini.GetBool(_T("UseGzip"), true);
	m_bWebLowEnabled=ini.GetBool(_T("UseLowRightsUser"), false);
	m_nWebPageRefresh=ini.GetInt(_T("PageRefreshTime"), 120);
	m_iWebTimeoutMins=ini.GetInt(_T("WebTimeoutMins"), 5 );

	///////////////////////////////////////////////////////////////////////////
	// Section: "MobileMule"
	//
	_stprintf(m_sMMPassword,_T("%s"),ini.GetString(_T("Password"), _T(""),_T("MobileMule")));
	m_bMMEnabled = ini.GetBool(_T("Enabled"), false);
	m_nMMPort = ini.GetInt(_T("Port"), 80);

	///////////////////////////////////////////////////////////////////////////
	// Section: "PeerCache"
	//
	m_uPeerCacheLastSearch = ini.GetInt(_T("LastSearch"), 0, _T("PeerCache"));
	m_bPeerCacheWasFound = ini.GetBool(_T("Found"), false);
	m_bPeerCacheEnabled = ini.GetBool(_T("Enabled"), true);
	m_nPeerCachePort = ini.GetInt(_T("PCPort"), 0);
	m_bPeerCacheShow = ini.GetBool(_T("Show"), false);

	LoadCats();
	LoadVirtualDirs();	// [TPT] - itsonlyme: virtualDirs
	SetLanguage();

	if (loadstatsFromOld == 2)
		SavePreferences();
}


WORD CPreferences::GetWindowsVersion(){
	static bool bWinVerAlreadyDetected = false;
	if(!bWinVerAlreadyDetected)
	{	
		bWinVerAlreadyDetected = true;
		m_wWinVer = DetectWinVersion();	
	}	
	return m_wWinVer;
}

uint16 CPreferences::GetDefaultMaxConperFive(){
	switch (GetWindowsVersion()){
		case _WINVER_98_:
			return 5;
		case _WINVER_95_:	
		case _WINVER_ME_:
			return MAXCON5WIN9X;
		case _WINVER_2K_:
		case _WINVER_XP_:
			return MAXCONPER5SEC;
		default:
			return MAXCONPER5SEC;
	}
}

// Barry - Provide a mechanism for all tables to store/retrieve sort order
int CPreferences::GetColumnSortItem(Table t, int column) // [TPT] - SLUGFILLER: multiSort
{
	switch(t) 
	{
		case tableDownload:
			return tableSortItemDownload[column];	// [TPT] - SLUGFILLER: multiSort
		case tableDownloadClients:
			return tableSortItemDownloadClient[column];	// [TPT] - SLUGFILLER: multiSort // [TPT] - TBH Transfers Window Style
		case tableUpload:
			return tableSortItemUpload[column];	// [TPT] - SLUGFILLER: multiSort
		case tableQueue:
			return tableSortItemQueue[column];	// [TPT] - SLUGFILLER: multiSort
		case tableSearch:
			return tableSortItemSearch[column];	// [TPT] - SLUGFILLER: multiSort
		case tableShared:
			return tableSortItemShared[column];	// [TPT] - SLUGFILLER: multiSort
		case tableServer:
			return tableSortItemServer[column];	// [TPT] - SLUGFILLER: multiSort
		case tableClientList:
			return tableSortItemClientList[column];	// [TPT] - SLUGFILLER: multiSort
		case tableFilenames:
			return tableSortItemFilenames[column]; // [TPT] - SLUGFILLER: multiSort
		// [TPT] - MoNKi: -Downloaded History-
		case tableHistory:	
			return tableSortItemHistory[column]; // [TPT] - SLUGFILLER: multiSort
		// [TPT] - MoNKi: -Downloaded History-
		// [TPT] - Friend State Column
		case tableFriendList:	
			return tableSortItemFriendList[column]; // [TPT] - SLUGFILLER: multiSort
		// [TPT] - Friend State Column
	}
	return 0;
}

// Barry - Provide a mechanism for all tables to store/retrieve sort order
bool CPreferences::GetColumnSortAscending(Table t, int column) // [TPT] - SLUGFILLER: multiSort
{
	switch(t) 
	{
		case tableDownload:
			return tableSortAscendingDownload[column];	// [TPT] - SLUGFILLER: multiSort
		case tableDownloadClients:
			return tableSortAscendingDownloadClient[column];	// [TPT] - SLUGFILLER: multiSort
		case tableUpload:
			return tableSortAscendingUpload[column];	// [TPT] - SLUGFILLER: multiSort
		case tableQueue:
			return tableSortAscendingQueue[column];	// [TPT] - SLUGFILLER: multiSort
		case tableSearch:
			return tableSortAscendingSearch[column];	// [TPT] - SLUGFILLER: multiSort
		case tableShared:
			return tableSortAscendingShared[column];	// [TPT] - SLUGFILLER: multiSort
		case tableServer:
			return tableSortAscendingServer[column];	// [TPT] - SLUGFILLER: multiSort
		case tableClientList:
			return tableSortAscendingClientList[column];	// [TPT] - SLUGFILLER: multiSort
		case tableFilenames:
			return tableSortAscendingFilenames[column];	// [TPT] - SLUGFILLER: multiSort
		// [TPT] - MoNKi: -Downloaded History-
		case tableHistory:
			return tableSortAscendingHistory[column]; // [TPT] - SLUGFILLER: multiSort
		// [TPT] - MoNKi: -Downloaded History-
		// [TPT] - Friend State Column
		case tableFriendList:
			return tableSortAscendingFriendList[column]; // [TPT] - SLUGFILLER: multiSort
		// [TPT] - Friend State Column
	}
	return true;
}

// [TPT] - SLUGFILLER: multiSort
int CPreferences::GetColumnSortCount(Table t) 
{
	int arrsz = 0;
	int* tb;
	switch(t)
	{
		case tableDownload:
			tb = tableSortItemDownload;
			arrsz = ARRSIZE(tableSortItemDownload);
			break;
		// [TPT] - TBH Transfers Window Style
		case tableDownloadClients:
			tb = tableSortItemDownloadClient;
			arrsz = ARRSIZE(tableSortItemDownloadClient);			
			break;
		// [TPT] - TBH Transfers Window Style
		case tableUpload:
			tb = tableSortItemUpload;
			arrsz = ARRSIZE(tableSortItemUpload);
			break;
		case tableQueue:
			tb = tableSortItemQueue;
			arrsz = ARRSIZE(tableSortItemQueue);
			break;
		case tableSearch:
			tb = tableSortItemSearch;
			arrsz = ARRSIZE(tableSortItemSearch);
			break;
		case tableShared:
			tb = tableSortItemShared;
			arrsz = ARRSIZE(tableSortItemShared);
			break;
		case tableServer:
			tb = tableSortItemServer;
			arrsz = ARRSIZE(tableSortItemServer);
			break;
		case tableClientList:
			tb = tableSortItemClientList;
			arrsz = ARRSIZE(tableSortItemClientList);
			break;
		case tableFilenames:
			tb = tableSortItemFilenames;
			arrsz = ARRSIZE(tableSortItemFilenames);
			break;			
		// [TPT] - MoNKi: -Downloaded History-
		case tableHistory:
			tb = tableSortItemHistory;
			arrsz = ARRSIZE(tableSortItemHistory);
			break;		
		// [TPT] - MoNKi: -Downloaded History-
		// [TPT] - Friend State Column
		case tableFriendList:
			tb = tableSortItemFriendList;
			arrsz = ARRSIZE(tableSortItemFriendList);
			break;		
		// [TPT] - Friend State Column
	}
	int i;
	for (i = 0; i < arrsz; i++)
		if (tb[i] == -1)
			break;
	return i;
}
// [TPT] - SLUGFILLER: multiSort

// Barry - Provide a mechanism for all tables to store/retrieve sort order
void CPreferences::SetColumnSortItem(Table t, int sortItem)
{
	// [TPT] - SLUGFILLER: multiSort - roll params
	int arrsz = 0;
	int* tb;
	bool* tba;
	switch(t) 
	{
		case tableDownload:
			tb = tableSortItemDownload;
			tba = tableSortAscendingDownload;
			arrsz = ARRSIZE(tableSortItemDownload);
			break;
		// [TPT] - TBH Transfers Window Style
		case tableDownloadClients:
			tb = tableSortItemDownloadClient;
			tba = tableSortAscendingDownloadClient;
			arrsz = ARRSIZE(tableSortItemDownloadClient);			
			break;
		// [TPT] - TBH Transfers Window Style
		case tableUpload:
			tb = tableSortItemUpload;
			tba = tableSortAscendingUpload;
			arrsz = ARRSIZE(tableSortItemUpload);
			break;
		case tableQueue:
			tb = tableSortItemQueue;
			tba = tableSortAscendingQueue;
			arrsz = ARRSIZE(tableSortItemQueue);
			break;
		case tableSearch:
			tb = tableSortItemSearch;
			tba = tableSortAscendingSearch;
			arrsz = ARRSIZE(tableSortItemSearch);
			break;
		case tableShared:
			tb = tableSortItemShared;
			tba = tableSortAscendingShared;
			arrsz = ARRSIZE(tableSortItemShared);
			break;
		case tableServer:
			tb = tableSortItemServer;
			tba = tableSortAscendingServer;
			arrsz = ARRSIZE(tableSortItemServer);
			break;
		case tableClientList:
			tb = tableSortItemClientList;
			tba = tableSortAscendingClientList;
			arrsz = ARRSIZE(tableSortItemClientList);
			break;
		case tableFilenames:
			tb = tableSortItemFilenames;
			tba = tableSortAscendingFilenames;
			arrsz = ARRSIZE(tableSortItemFilenames);
			break;					
		// [TPT] - MoNKi: -Downloaded History-
		case tableHistory:
			tb = tableSortItemHistory;
			tba = tableSortAscendingHistory;
			arrsz = ARRSIZE(tableSortItemHistory);
			break;								
		// [TPT] - MoNKi: -Downloaded History-
		// [TPT] - Friend State Column
		case tableFriendList:
			tb = tableSortItemFriendList;
			tba = tableSortAscendingFriendList;
			arrsz = ARRSIZE(tableSortItemFriendList);
			break;					
		// [TPT] - Friend State Column
	}
	int i;
	for (i = 0; i < arrsz-1; i++)
		if (tb[i] == sortItem || tb[i] == -1)
			break;
	for (; i > 0; i--) {
		tb[i] = tb[i-1];
		tba[i] = tba[i-1];
	}
	tb[0] = sortItem;
	// [TPT] - SLUGFILLER: multiSort
}

// Barry - Provide a mechanism for all tables to store/retrieve sort order
void CPreferences::SetColumnSortAscending(Table t, bool sortAscending)
{
	switch(t) 
	{
		case tableDownload:
			tableSortAscendingDownload[0] = sortAscending;	// [TPT] - SLUGFILLER: multiSort - change for newest param only
			break;
		// [TPT] - TBH Transfers Window Style
		case tableDownloadClients:
			tableSortAscendingDownloadClient[0] = sortAscending;	// [TPT] - SLUGFILLER: multiSort - change for newest param only
			break;
		// [TPT] - TBH Transfers Window Style
		case tableUpload:
			tableSortAscendingUpload[0] = sortAscending;	// [TPT] - SLUGFILLER: multiSort - change for newest param only
			break;
		case tableQueue:
			tableSortAscendingQueue[0] = sortAscending;	// [TPT] - SLUGFILLER: multiSort - change for newest param only
			break;
		case tableSearch:
			tableSortAscendingSearch[0] = sortAscending;	// [TPT] - SLUGFILLER: multiSort - change for newest param only
			break;
		case tableShared:
			tableSortAscendingShared[0] = sortAscending;	// [TPT] - SLUGFILLER: multiSort - change for newest param only
			break;
		case tableServer:
			tableSortAscendingServer[0] = sortAscending;	// [TPT] - SLUGFILLER: multiSort - change for newest param only
			break;
		case tableClientList:
			tableSortAscendingClientList[0] = sortAscending;	// [TPT] - SLUGFILLER: multiSort - change for newest param only
			break;
		case tableFilenames:
			tableSortAscendingFilenames[0]= sortAscending; // [TPT] - SLUGFILLER: multiSort - change for newest param only
		// [TPT] - MoNKi: -Downloaded History-
		case tableHistory:
			tableSortAscendingHistory[0] = sortAscending; // [TPT] - SLUGFILLER: multiSort - change for newest param only
			break;
		// [TPT] - MoNKi: -Downloaded History-
		// [TPT] - Friend State Column
		case tableFriendList:
			tableSortAscendingFriendList[0] = sortAscending; // [TPT] - SLUGFILLER: multiSort - change for newest param only
			break;
		// [TPT] - Friend State Column
	}
}

//////////////////////////////////////////////////////////
// category implementations
//////////////////////////////////////////////////////////

void CPreferences::SaveCats(){

	// Cats
	CString catinif,ixStr,buffer;
	catinif.Format(_T("%sCategory.ini"),configdir);
	_tremove(catinif);

		CIni catini( catinif, _T("Category") );
		catini.WriteInt(_T("Count"),catMap.GetCount()-1, _T("General"));
		catini.WriteInt(_T("CategoryVersion"), 2, _T("General")); // khaos::categorymod+
		for (int ix=0;ix<catMap.GetCount();ix++){
			ixStr.Format(_T("Cat#%i"),ix);
			catini.WriteString(_T("Title"),catMap.GetAt(ix)->title,ixStr);
			catini.WriteString(_T("Incoming"),catMap.GetAt(ix)->incomingpath,ixStr);
			catini.WriteString(_T("Comment"),catMap.GetAt(ix)->comment,ixStr);
			buffer.Format(_T("%lu"),catMap.GetAt(ix)->color,ixStr);
			catini.WriteString(_T("Color"),buffer,ixStr);
			catini.WriteInt(_T("Priority"),catMap.GetAt(ix)->prio,ixStr);
			catini.WriteBool(_T("downloadInLinealPriorityOrder"), catMap.GetAt(ix)->downloadInLinealPriorityOrder, ixStr); // [TPT] - ZZ:DownloadManager
			// [TPT] - khaos::categorymod+ Save View Filters
			catini.WriteInt(_T("vfFromCats"), catMap.GetAt(ix)->viewfilters.nFromCats, ixStr);
			catini.WriteBool(_T("vfVideo"), catMap.GetAt(ix)->viewfilters.bVideo, ixStr);
			catini.WriteBool(_T("vfAudio"), catMap.GetAt(ix)->viewfilters.bAudio, ixStr);
			catini.WriteBool(_T("vfArchives"), catMap.GetAt(ix)->viewfilters.bArchives, ixStr);
			catini.WriteBool(_T("vfImages"), catMap.GetAt(ix)->viewfilters.bImages, ixStr);
			catini.WriteBool(_T("vfWaiting"), catMap.GetAt(ix)->viewfilters.bWaiting, ixStr);
			catini.WriteBool(_T("vfTransferring"), catMap.GetAt(ix)->viewfilters.bTransferring, ixStr);
			catini.WriteBool(_T("vfPaused"), catMap.GetAt(ix)->viewfilters.bPaused, ixStr);
			catini.WriteBool(_T("vfStopped"), catMap.GetAt(ix)->viewfilters.bStopped, ixStr);
			catini.WriteBool(_T("vfComplete"), catMap.GetAt(ix)->viewfilters.bComplete, ixStr);
			catini.WriteBool(_T("vfHashing"), catMap.GetAt(ix)->viewfilters.bHashing, ixStr);
			catini.WriteBool(_T("vfErrorUnknown"), catMap.GetAt(ix)->viewfilters.bErrorUnknown, ixStr);
			catini.WriteBool(_T("vfCompleting"), catMap.GetAt(ix)->viewfilters.bCompleting, ixStr);
			catini.WriteInt(_T("vfFSizeMin"), catMap.GetAt(ix)->viewfilters.nFSizeMin, ixStr);
			catini.WriteInt(_T("vfFSizeMax"), catMap.GetAt(ix)->viewfilters.nFSizeMax, ixStr);
			catini.WriteInt(_T("vfRSizeMin"), catMap.GetAt(ix)->viewfilters.nRSizeMin, ixStr);
			catini.WriteInt(_T("vfRSizeMax"), catMap.GetAt(ix)->viewfilters.nRSizeMax, ixStr);
			catini.WriteInt(_T("vfTimeRemainingMin"), catMap.GetAt(ix)->viewfilters.nTimeRemainingMin, ixStr);
			catini.WriteInt(_T("vfTimeRemainingMax"), catMap.GetAt(ix)->viewfilters.nTimeRemainingMax, ixStr);
			catini.WriteInt(_T("vfSourceCountMin"), catMap.GetAt(ix)->viewfilters.nSourceCountMin, ixStr);
			catini.WriteInt(_T("vfSourceCountMax"), catMap.GetAt(ix)->viewfilters.nSourceCountMax, ixStr);
			catini.WriteInt(_T("vfAvailSourceCountMin"), catMap.GetAt(ix)->viewfilters.nAvailSourceCountMin, ixStr);
			catini.WriteInt(_T("vfAvailSourceCountMax"), catMap.GetAt(ix)->viewfilters.nAvailSourceCountMax, ixStr);
			catini.WriteString(_T("vfAdvancedFilterMask"), catMap.GetAt(ix)->viewfilters.sAdvancedFilterMask, ixStr);
			// Save Selection Criteria
			catini.WriteBool(_T("scFileSize"), catMap.GetAt(ix)->selectioncriteria.bFileSize, ixStr);
			catini.WriteBool(_T("scAdvancedFilterMask"), catMap.GetAt(ix)->selectioncriteria.bAdvancedFilterMask, ixStr);			
			// [TPT] - khaos::categorymod-
		}
}


// [TPT] - khaos::categorymod+ We now save data for the 'All' category.
void CPreferences::LoadCats() {
	CString ixStr,catinif;//,cat_a,cat_b,cat_c;
	TCHAR buffer[100];

	catinif.Format(_T("%sCategory.ini"), configdir);
	CIni catini;

	bool bCreateDefault = false;
	bool bSkipLoad = false;
	if (!PathFileExists(catinif))
	{
		bCreateDefault = true;
		bSkipLoad = true;
	}
	else
	{
		catini.SetFileName(catinif);
		catini.SetSection(_T("General"));
		if (catini.GetInt(_T("CategoryVersion")) == 0)
			bCreateDefault = true;
	}

	if (bCreateDefault)
	{
		Category_Struct* defcat=new Category_Struct;

		_stprintf(defcat->title, _T("Default"));
		defcat->prio=PR_NORMAL; // [TPT] - ZZ:DownloadManager
		_stprintf(defcat->incomingpath, incomingdir);
		_stprintf(defcat->comment, _T("The default category.  It can't be merged or deleted."));
		defcat->color = 0;
		defcat->downloadInLinealPriorityOrder = false; // [TPT] - ZZ:DownloadManager
		defcat->viewfilters.bArchives = true;
		defcat->viewfilters.bAudio = true;
		defcat->viewfilters.bComplete = true;
		defcat->viewfilters.bCompleting = true;
		defcat->viewfilters.bErrorUnknown = true;
		defcat->viewfilters.bHashing = true;
		defcat->viewfilters.bImages = true;
		defcat->viewfilters.bPaused = true;
		defcat->viewfilters.bStopped = true;
		defcat->viewfilters.bSuspendFilters = false;
		defcat->viewfilters.bTransferring = true;
		defcat->viewfilters.bVideo = true;
		defcat->viewfilters.bWaiting = true;
		defcat->viewfilters.nAvailSourceCountMax = 0;
		defcat->viewfilters.nAvailSourceCountMin = 0;
		defcat->viewfilters.nFromCats = 2;
		defcat->viewfilters.nFSizeMax = 0;
		defcat->viewfilters.nFSizeMin = 0;
		defcat->viewfilters.nRSizeMax = 0;
		defcat->viewfilters.nRSizeMin = 0;
		defcat->viewfilters.nSourceCountMax = 0;
		defcat->viewfilters.nSourceCountMin = 0;
		defcat->viewfilters.nTimeRemainingMax = 0;
		defcat->viewfilters.nTimeRemainingMin = 0;
		defcat->viewfilters.sAdvancedFilterMask = _T("");
		defcat->selectioncriteria.bAdvancedFilterMask = true;
		defcat->selectioncriteria.bFileSize = true;
		AddCat(defcat);
		if (bSkipLoad)
		{
			SaveCats();
			return;
		}
	}

	int max = catini.GetInt(_T("Count"), 0, _T("General"));

	for (int ix = bCreateDefault ? 1 : 0; ix <= max; ix++)
	{
		ixStr.Format(_T("Cat#%i"),ix);
        catini.SetSection(ixStr);

		Category_Struct* newcat = new Category_Struct;
		_stprintf(newcat->title,_T("%s"),catini.GetString(_T("Title"),_T(""),ixStr));
		_stprintf(newcat->incomingpath,_T("%s"),catini.GetString(_T("Incoming"),_T(""),ixStr));
		MakeFoldername(newcat->incomingpath);
		if (!IsShareableDirectory(newcat->incomingpath)){
			_sntprintf(newcat->incomingpath, ARRSIZE(newcat->incomingpath), _T("%s"), GetIncomingDir());
			MakeFoldername(newcat->incomingpath);
		}
		_stprintf(newcat->comment,_T("%s"),catini.GetString(_T("Comment"),_T(""),ixStr));
		newcat->prio =catini.GetInt(_T("a4afPriority"),PR_NORMAL,ixStr); // ZZ:DownloadManager
		_stprintf(buffer,_T("%s"),catini.GetString(_T("Color"),_T("0"),ixStr));
		newcat->color=_tstoi64(buffer);
		newcat->prio = catini.GetInt(_T("Priority"),PR_NORMAL,ixStr); // [TPT] - ZZ:DownloadManager
		newcat->downloadInLinealPriorityOrder = catini.GetBool(_T("downloadInLinealPriorityOrder"), FALSE, ixStr); // [TPT] - ZZ:DownloadManager
		// Load View Filters
		newcat->viewfilters.nFromCats = catini.GetInt(_T("vfFromCats"), ix == 0 ? 0 : 2);
		newcat->viewfilters.bSuspendFilters = false;
		newcat->viewfilters.bVideo = catini.GetBool(_T("vfVideo"), true);
		newcat->viewfilters.bAudio = catini.GetBool(_T("vfAudio"), true);
		newcat->viewfilters.bArchives = catini.GetBool(_T("vfArchives"), true);
		newcat->viewfilters.bImages = catini.GetBool(_T("vfImages"), true);
		newcat->viewfilters.bWaiting = catini.GetBool(_T("vfWaiting"), true);
		newcat->viewfilters.bTransferring = catini.GetBool(_T("vfTransferring"), true);
		newcat->viewfilters.bPaused = catini.GetBool(_T("vfPaused"), true);
		newcat->viewfilters.bStopped = catini.GetBool(_T("vfStopped"), true);
		newcat->viewfilters.bComplete = catini.GetBool(_T("vfComplete"), true);
		newcat->viewfilters.bHashing = catini.GetBool(_T("vfHashing"), true);
		newcat->viewfilters.bErrorUnknown = catini.GetBool(_T("vfErrorUnknown"), true);
		newcat->viewfilters.bCompleting = catini.GetBool(_T("vfCompleting"), true);
		newcat->viewfilters.nFSizeMin = catini.GetInt(_T("vfFSizeMin"), 0);
		newcat->viewfilters.nFSizeMax = catini.GetInt(_T("vfFSizeMax"), 0);
		newcat->viewfilters.nRSizeMin = catini.GetInt(_T("vfRSizeMin"), 0);
		newcat->viewfilters.nRSizeMax = catini.GetInt(_T("vfRSizeMax"), 0);
		newcat->viewfilters.nTimeRemainingMin = catini.GetInt(_T("vfTimeRemainingMin"), 0);
		newcat->viewfilters.nTimeRemainingMax = catini.GetInt(_T("vfTimeRemainingMax"), 0);
		newcat->viewfilters.nSourceCountMin = catini.GetInt(_T("vfSourceCountMin"), 0);
		newcat->viewfilters.nSourceCountMax = catini.GetInt(_T("vfSourceCountMax"), 0);
		newcat->viewfilters.nAvailSourceCountMin = catini.GetInt(_T("vfAvailSourceCountMin"), 0);
		newcat->viewfilters.nAvailSourceCountMax = catini.GetInt(_T("vfAvailSourceCountMax"), 0);
		newcat->viewfilters.sAdvancedFilterMask = catini.GetString(_T("vfAdvancedFilterMask"), _T(""));
		// Load Selection Criteria
		newcat->selectioncriteria.bFileSize = catini.GetBool(_T("scFileSize"), true);
		newcat->selectioncriteria.bAdvancedFilterMask = catini.GetBool(_T("scAdvancedFilterMask"), true);

		AddCat(newcat);
		if (!PathFileExists(newcat->incomingpath)) 
			::CreateDirectory(newcat->incomingpath,0);
	}
}
// [TPT] - khaos::categorymod-
void CPreferences::RemoveCat(int index)	{
	if (index>=0 && index<catMap.GetCount()) { 
		Category_Struct* delcat;
		delcat=catMap.GetAt(index); 
		catMap.RemoveAt(index); 
		delete delcat;
	}
}

// [TPT] - khaos::categorymod
/*
bool CPreferences::SetCatFilter(int index, int filter){
	if (index>=0 && index<catMap.GetCount()) { 
		Category_Struct* cat;
		cat=catMap.GetAt(index); 
		cat->filter=filter;
		return true;
	} 
	
	return false;
}

int CPreferences::GetCatFilter(int index){
	if (index>=0 && index<catMap.GetCount()) {
		return catMap.GetAt(index)->filter;
	}
	
    return 0;
}

bool CPreferences::GetCatFilterNeg(int index){
	if (index>=0 && index<catMap.GetCount()) {
		return catMap.GetAt(index)->filterNeg;
	}
	
    return false;
}

void CPreferences::SetCatFilterNeg(int index, bool val) {
	if (index>=0 && index<catMap.GetCount()) {
		catMap.GetAt(index)->filterNeg=val;
	}
}*/
// [TPT] - khaos::categorymod

bool CPreferences::MoveCat(UINT from, UINT to){
	if (from>=(UINT)catMap.GetCount() || to >=(UINT)catMap.GetCount()+1 || from==to) return false;

	Category_Struct* tomove;

	tomove=catMap.GetAt(from);

	if (from < to) {
		catMap.RemoveAt(from);
		catMap.InsertAt(to-1,tomove);
	} else {
		catMap.InsertAt(to,tomove);
		catMap.RemoveAt(from+1);
	}
	
	SaveCats();

	return true;
}


bool CPreferences::IsInstallationDirectory(const CString& rstrDir)
{
	CString strFullPath;
	if (PathCanonicalize(strFullPath.GetBuffer(MAX_PATH), rstrDir))
		strFullPath.ReleaseBuffer();
	else
		strFullPath = rstrDir;
	
	// skip sharing of several special eMule folders
	if (!CompareDirectories(strFullPath, GetAppDir()))			// ".\eMule"
		return true;
	if (!CompareDirectories(strFullPath, GetConfigDir()))		// ".\eMule\config"
		return true;
	if (!CompareDirectories(strFullPath, GetWebServerDir()))	// ".\eMule\webserver"
		return true;
	if (!CompareDirectories(strFullPath, GetLangDir()))			// ".\eMule\lang"
		return true;

	return false;
}

bool CPreferences::IsShareableDirectory(const CString& rstrDir)
{
	if (IsInstallationDirectory(rstrDir))
		return false;

	CString strFullPath;
	if (PathCanonicalize(strFullPath.GetBuffer(MAX_PATH), rstrDir))
		strFullPath.ReleaseBuffer();
	else
		strFullPath = rstrDir;
	
	// skip sharing of several special eMule folders
	if (!CompareDirectories(strFullPath, GetTempDir()))			// ".\eMule\temp"
		return false;

	return true;
}

void CPreferences::UpdateLastVC()
{
	versioncheckLastAutomatic = safe_mktime(CTime::GetCurrentTime().GetLocalTm());
}

void CPreferences::SetWSPass(CString strNewPass)
{
	_stprintf(m_sWebPassword,_T("%s"),MD5Sum(strNewPass).GetHash().GetBuffer(0));
}

void CPreferences::SetWSLowPass(CString strNewPass)
{
	_stprintf(m_sWebLowPassword,_T("%s"),MD5Sum(strNewPass).GetHash().GetBuffer(0));
}

void CPreferences::SetMMPass(CString strNewPass)
{
	_stprintf(m_sMMPassword,_T("%s"),MD5Sum(strNewPass).GetHash().GetBuffer(0));
}

uint16 CPreferences::GetMaxSourcePerFileSoft()
{
	UINT temp = ((UINT)maxsourceperfile * 9L) / 10;
	if (temp > MAX_SOURCES_FILE_SOFT)
		return MAX_SOURCES_FILE_SOFT;
	return temp;
}

uint16 CPreferences::GetMaxSourcePerFileUDP()
{	
	UINT temp = ((UINT)maxsourceperfile * 3L) / 4;
	if (temp > MAX_SOURCES_FILE_UDP)
		return MAX_SOURCES_FILE_UDP;
	return temp;
}

void CPreferences::SetNetworkKademlia(bool val)
{
	networkkademlia = val; 
}

CString CPreferences::GetHomepageBaseURLForLevel(uint8 nLevel){
	CString tmp;
	if (nLevel == 0)
		tmp = _T("http://emule-project.net");
	else if (nLevel == 1)
		tmp = _T("http://www.emule-project.org");
	else if (nLevel == 2)
		tmp = _T("http://www.emule-project.com");
	else if (nLevel < 100)
		tmp.Format(_T("http://www%i.emule-project.net"),nLevel-2);
	else if (nLevel < 150)
		tmp.Format(_T("http://www%i.emule-project.org"),nLevel);
	else if (nLevel < 200)
		tmp.Format(_T("http://www%i.emule-project.com"),nLevel);
	else if (nLevel == 200)
		tmp = _T("http://emule.sf.net");
	else if (nLevel == 201)
		tmp = _T("http://www.emuleproject.net");
	else if (nLevel == 202)
		tmp = _T("http://sourceforge.net/projects/emule/");
	else
		tmp = _T("http://www.emule-project.net");
	return tmp;
}

CString CPreferences::GetVersionCheckBaseURL(){
	CString tmp;
	uint8 nWebMirrorAlertLevel = GetWebMirrorAlertLevel();
	if (nWebMirrorAlertLevel < 100)
		tmp = _T("http://vcheck.emule-project.net");
	else if (nWebMirrorAlertLevel < 150)
		tmp.Format(_T("http://vcheck%i.emule-project.org"),nWebMirrorAlertLevel);
	else if (nWebMirrorAlertLevel < 200)
		tmp.Format(_T("http://vcheck%i.emule-project.com"),nWebMirrorAlertLevel);
	else if (nWebMirrorAlertLevel == 200)
		tmp = _T("http://emule.sf.net");
	else if (nWebMirrorAlertLevel == 201)
		tmp = _T("http://www.emuleproject.net");
	else
		tmp = _T("http://vcheck.emule-project.net");
	return tmp;
}

bool CPreferences::IsDefaultNick(const CString strCheck){
	// not fast, but this function is called often
	for (int i = 0; i != 255; i++){
		if (GetHomepageBaseURLForLevel(i) == strCheck)
			return true;
	}
	return ( strCheck == _T("http://emule-project.net") );
}

// [TPT] - itsonlyme: viewSharedFiles
void CPreferences::SetUserNick(LPCTSTR pszNick)
{
	strNick = pszNick;
	if (theApp.sharedfiles) 
		theApp.sharedfiles->ShowLocalFilesDialog(true);
}
// [TPT] - itsonlyme: viewSharedFiles

uint8 CPreferences::GetWebMirrorAlertLevel(){
	// Known upcoming DDoS Attacks
	if (m_nWebMirrorAlertLevel == 0){
		// no threats known at this time
	}
	// end
	if (UpdateNotify())
		return m_nWebMirrorAlertLevel;
	else
		return 0;
}

bool CPreferences::IsRunAsUserEnabled(){
	return (GetWindowsVersion() == _WINVER_XP_ || GetWindowsVersion() == _WINVER_2K_) && m_bRunAsUser;
}

bool CPreferences::GetUseReBarToolbar()
{
	return GetReBarToolbar() && theApp.m_ullComCtrlVer >= MAKEDLLVERULL(5,8,0,0);
}


// emulEspa�a: added by [TPT]-MoNKi [MoNKi: -invisible mode-]
void CPreferences::SetInvisibleMode(bool on, UINT keymodifier, TCHAR key) 
{
	m_bInvisibleMode = on;
	m_iInvisibleModeHotKeyModifier = keymodifier;
	m_cInvisibleModeHotKey = key;
	if(theApp.emuledlg!=NULL){
		//Always unregister, the keys could be different.
		theApp.emuledlg->UnRegisterInvisibleHotKey();
		if(m_bInvisibleMode)	theApp.emuledlg->RegisterInvisibleHotKey();
	}
}	
// End emulEspa�a

// [TPT] - MoNKi: -UPnPNAT Support-
uint16	CPreferences::GetPort(bool newPort)
{
	static portNumber = 0;

	if(m_iUPnPTCPExternal != 0)
		return m_iUPnPTCPExternal;

	if (portNumber == 0 || newPort)
		portNumber = port;

	return portNumber;
}

uint16	CPreferences::GetUDPPort(bool newPort)
{
	static portNumber = 0;
	
	if(udpport == 0)
		return 0;
	
	if(m_iUPnPUDPExternal != 0)
		return m_iUPnPUDPExternal;

	if (portNumber == 0 || newPort)
		portNumber = udpport;

	return portNumber;
}
// [TPT] - MoNKi: -UPnPNAT Support-
