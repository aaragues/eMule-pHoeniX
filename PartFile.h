//Copyright (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#pragma once
#include "KnownFile.h"
#include "DeadSourceList.h"
#include "CorruptionBlackBox.h"
#include "server.h"	// [TPT] - itsonlyme: cacheUDPsearchResults
// [TPT]
#include "sourcesaver.h" //<<-- enkeyDEV(Ottavio84) -New SLS-
#include "HardLimit.h" // [TPT] - Sivka AutoHL

enum EPartFileStatus{
	PS_READY			= 0,
	PS_EMPTY			= 1,
	PS_WAITINGFORHASH	= 2,
	PS_HASHING			= 3,
	PS_ERROR			= 4,
	PS_INSUFFICIENT		= 5,	// SLUGFILLER: checkDiskspace
	PS_UNKNOWN			= 6,
	PS_PAUSED			= 7,
	PS_COMPLETING		= 8,
	PS_COMPLETE			= 9
};

#define PR_VERYLOW			4 // I Had to change this because it didn't save negative number correctly.. Had to modify the sort function for this change..
#define PR_LOW				0 //*
#define PR_NORMAL			1 // Don't change this - needed for edonkey clients and server!
#define PR_HIGH				2 //*
#define PR_VERYHIGH			3
#define PR_AUTO				5 //UAP Hunter

//#define BUFFER_SIZE_LIMIT 500000 // Max bytes before forcing a flush
#define BUFFER_TIME_LIMIT	60000	// Max milliseconds before forcing a flush

#define	PARTMET_BAK_EXT	_T(".bak")
#define	PARTMET_TMP_EXT	_T(".backup")

#define STATES_COUNT		17

#define MAX_PREF_SERVERS	10	// [TPT] - itsonlyme: cacheUDPsearchResults

enum EPartFileFormat{
	PMT_UNKNOWN			= 0,
	PMT_DEFAULTOLD,
	PMT_SPLITTED,
	PMT_NEWOLD,
	PMT_SHAREAZA,
	PMT_BADFORMAT	
};

#define	FILE_COMPLETION_THREAD_FAILED	0x0000
#define	FILE_COMPLETION_THREAD_SUCCESS	0x0001
#define	FILE_COMPLETION_THREAD_RENAMED	0x0002

enum EPartFileOp{
	PFOP_NONE = 0,
	PFOP_HASHING,
	PFOP_COPYING,
	PFOP_UNCOMPRESSING,
	PFOP_SR13_IMPORTPARTS //[TPT] - SR13: Import Parts
};

class CSearchFile;
class CUpDownClient;
enum EDownloadState;
class CxImage;
class CSafeMemFile;
class CFileDetailDialogInterface;	// [TPT] - SLUGFILLER: modelessDialogs
class CServer;	// [TPT] - itsonlyme: cacheUDPsearchResults

#pragma pack(1)
struct Requested_Block_Struct
{
	uint32	StartOffset;
	uint32	EndOffset;
	uchar	FileID[16];
	uint32  transferred; // Barry - This counts bytes completed
};
#pragma pack()

struct Gap_Struct
{
	uint32 start;
	uint32 end;
};

// [TPT]
#pragma pack(4) // Maella -Code Improvement-
struct PartFileBufferedData
{
	BYTE *data;						// Barry - This is the data to be written
	uint32 start;					// Barry - This is the start offset of the data
	uint32 end;						// Barry - This is the end offset of the data
	Requested_Block_Struct *block;	// Barry - This is the requested block that this data relates to
};
#pragma pack()

typedef CTypedPtrList<CPtrList, CUpDownClient*> CUpDownClientPtrList;

class CPartFile : public CKnownFile
{
	DECLARE_DYNAMIC(CPartFile)

	friend class CPartFileConvert;
public:
	CPartFile();
	CPartFile(CSearchFile* searchresult);  //used when downloading a new file
	CPartFile(CString edonkeylink);
	CPartFile(class CED2KFileLink* fileLink);
	virtual ~CPartFile();

	bool	IsPartFile() const { return !(status == PS_COMPLETE); }

	// eD2K filename
	virtual void SetFileName(LPCTSTR pszFileName, bool bReplaceInvalidFileSystemChars = false); // 'bReplaceInvalidFileSystemChars' is set to 'false' for backward compatibility!

	// part.met filename (without path!)
	const CString& GetPartMetFileName() const { return m_partmetfilename; }

	// full path to part.met file or completed file
	const CString& GetFullName() const { return m_fullname; }
	void	SetFullName(CString name) { m_fullname = name; }

	// local file system related properties
	bool	IsNormalFile() const { return (m_dwFileAttributes & (FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE)) == 0; }
	uint64	GetRealFileSize() const;
	void	GetSizeToTransferAndNeededSpace(uint32& pui32SizeToTransfer, uint32& pui32NeededSpace) const;
	uint32	GetNeededSpace() const; // SLUGFILLER: checkDiskspace

	// last file modification time (NT's version of UTC), to be used for stats only!
	CTime	GetCFileDate() const { return CTime(m_tLastModified); }
	uint32	GetFileDate() const { return m_tLastModified; }

	// file creation time (NT's version of UTC), to be used for stats only!
	CTime	GetCrCFileDate() const { return CTime(m_tCreated); }
	uint32	GetCrFileDate() const { return m_tCreated; }

	void	InitializeFromLink(CED2KFileLink* fileLink);
	bool	CreateFromFile(LPCTSTR directory, LPCTSTR filename, LPVOID pvProgressParam) {return false;}// not supported in this class
	bool	LoadFromFile(FILE* file) { return false; }
	bool	WriteToFile(FILE* file) { return false; }
	uint8	LoadPartFile(LPCTSTR in_directory, LPCTSTR filename, bool getsizeonly = false); //filename = *.part.met
//	uint8	ImportShareazaTempfile(LPCTSTR in_directory,LPCTSTR in_filename , bool getsizeonly);
	//[TPT] START - Added by SiRoB, SLUGFILLER: SafeHash
	bool	IsPartShareable(uint16 partnumber) const;
	bool	IsRangeShareable(uint32 start, uint32 end) const;
	//[TPT] END   - Added by SiRoB, SLUGFILLER: SafeHash

	bool	SavePartFile();
	void	PartFileHashFinished(CKnownFile* result);
	bool	HashSinglePart(uint16 partnumber); // true = ok , false = corrupted
    // [TPT] - Sivka AutoHL Begin
	void	SetMaxSourcesPerFile(uint16 in){m_MaxSourcesPerFile=in;}
	uint16	GetMaxSourcesPerFile(){return m_MaxSourcesPerFile;}
	uint16	GetMaxSourcesLimitSoft();
	uint16	GetMaxSourcesUDPLimit();
    // [TPT] - Sivka AutoHL End
	void	AddGap(uint32 start, uint32 end);
	void	FillGap(uint32 start, uint32 end);
	void	DrawStatusBar(CDC* dc, LPCRECT rect, bool bFlat) /*const*/;
	virtual void	DrawShareStatusBar(CDC* dc, LPCRECT rect, bool onlygreyrect, bool	 bFlat) const;
	bool	IsComplete(uint32 start, uint32 end) const;
	bool	IsPureGap(uint32 start, uint32 end) const;
	bool	IsAlreadyRequested(uint32 start, uint32 end) const;
	bool	IsCorruptedPart(uint16 partnumber) const;
	uint32	GetTotalGapSizeInRange(uint32 uRangeStart, uint32 uRangeEnd) const;
	uint32	GetTotalGapSizeInPart(UINT uPart) const;
	void	UpdateCompletedInfos();
	void	UpdateCompletedInfos(uint32 uTotalGaps);
	virtual void	UpdatePartsInfo();

	bool	GetNextRequestedBlock(CUpDownClient* sender, Requested_Block_Struct** newblocks, uint16* count) /*const*/;
	void	WritePartStatus(CSafeMemFile* file, CUpDownClient* client = NULL);	// [TPT] - SLUGFILLER: hideOS
	void	WriteCompleteSourcesCount(CSafeMemFile* file) const;
	void	AddSources(CSafeMemFile* sources,uint32 serverip, uint16 serverport);
	void	AddSource(LPCTSTR pszURL, uint32 nIP);
	static bool CanAddSource(uint32 userid, uint16 port, uint32 serverip, uint16 serverport, UINT* pdebug_lowiddropped = NULL, bool Ed2kID = true);
	
	EPartFileStatus	GetStatus(bool ignorepause = false) const;
	void	SetStatus(EPartFileStatus in);
	void	NotifyStatusChange();
	bool	IsStopped() const { return stopped; }
	bool	GetCompletionError() const { return m_bCompletionError; }
	uint32  GetCompletedSize() const { return completedsize; }
	CString getPartfileStatus() const;
	int		getPartfileStatusRang() const;
	void	SetActive(bool bActive);

	uint8	GetDownPriority() const { return m_iDownPriority; }
	void	SetDownPriority(uint8 iNewDownPriority, bool resort = true);
	bool	IsAutoDownPriority(void) const { return m_bAutoDownPriority; }
	void	SetAutoDownPriority(bool NewAutoDownPriority) { m_bAutoDownPriority = NewAutoDownPriority; }
	void	UpdateAutoDownPriority();
	// [TPT]	
	uint16	GetAvailableSrcCount() const { return m_anStates[0]+m_anStates[1]; } // [TPT] - khaos::categorymod+
	uint16	GetSourceCount() const { return srclist.GetCount(); }
	uint16	GetSrcA4AFCount() const { return A4AFsrclist.GetCount(); }
	uint16	GetSrcStatisticsValue(EDownloadState nDLState) const {return m_anStates[nDLState];}
	uint16	GetTransferringSrcCount() const {return m_downloadingSourceList.GetCount();}
	// Maella end
	uint32	GetTransferred() const { return m_uTransferred; }
	//uint32	GetDatarate() const { return datarate; }
	float	GetPercentCompleted() const { return percentcompleted; }
	uint16	GetNotCurrentSourcesCount() const;
	int		GetValidSourcesCount() const;
	//bool	IsArchive(bool onlyPreviewable = false) const; // Barry - Also want to preview archives // [TPT]
	bool    IsPreviewableFileType() const;
	time_t	getTimeRemaining() const;
	time_t	getTimeRemainingSimple() const;
	uint32	GetDlActiveTime() const;

	// Barry - Added as replacement for BlockReceived to buffer data before writing to disk
	uint32	WriteToBuffer(uint32 transize, const BYTE *data, uint32 start, uint32 end, Requested_Block_Struct *block, const CUpDownClient* client);
	void	FlushBuffer(bool forcewait=false, bool bForceICH = false, bool bNoAICH = false);
	// Barry - This will invert the gap list, up to caller to delete gaps when done
	// 'Gaps' returned are really the filled areas, and guaranteed to be in order
	void	GetFilledList(CTypedPtrList<CPtrList, Gap_Struct*> *filled) const;

	// Barry - Added to prevent list containing deleted blocks on shutdown
	void	RemoveAllRequestedBlocks(void);
	bool	RemoveBlockFromList(uint32 start, uint32 end);
	bool	IsInRequestedBlockList(const Requested_Block_Struct* block) const;
	void	RemoveAllSources(bool bTryToSwap);

	bool	CanOpenFile() const;
	bool	IsReadyForPreview() const;
	bool	CanStopFile() const;
	bool	CanPauseFile() const;
	bool	CanResumeFile() const;

	void	OpenFile() const;
	void	PreviewFile();
	void	DeleteFile();
	void	StopFile(bool bCancel = false, bool resort = true);
	void	PauseFile(bool bInsufficient = false, bool resort = true);
	void	StopPausedFile();
	void	ResumeFile(bool resort = true);
	void	ResumeFileInsufficient();

	virtual Packet* CreateSrcInfoPacket(CUpDownClient* forClient) const;
	void	AddClientSources(CSafeMemFile* sources, uint8 sourceexchangeversion, CUpDownClient* pClient = NULL);

	// [TPT] - itsonlyme: cacheUDPsearchResults
	struct SServer {
		SServer() {
			m_nIP = m_nPort = 0;
			m_uAvail = 0;
		}
		SServer(uint32 nIP, UINT nPort) {
			m_nIP = nIP;
			m_nPort = nPort;
			m_uAvail = 0;
		}
		uint32 m_nIP;
		uint16 m_nPort;
		UINT   m_uAvail;
	};
	void	AddAvailServer(SServer server);
	CServer*	GetNextAvailServer();
	// [TPT] - itsonlyme: cacheUDPsearchResults
	
	uint16	GetAvailablePartCount() const { return availablePartsCount; }
	void	UpdateAvailablePartsCount();

	uint32	GetLastAnsweredTime() const	{ return m_ClientSrcAnswered; }
	void	SetLastAnsweredTime() { m_ClientSrcAnswered = ::GetTickCount(); }
	void	SetLastAnsweredTimeTimeout();

	uint64	GetCorruptionLoss() const { return m_uCorruptionLoss; }
	uint64	GetCompressionGain() const { return m_uCompressionGain; }
	uint32	GetRecoveredPartsByICH() const { return m_uPartsSavedDueICH; }

	// [TPT] - SLUGFILLER: showComments remove - moved comments stuff to CKnownFile


	void	AddDownloadingSource(CUpDownClient* client);
	void	RemoveDownloadingSource(CUpDownClient* client);

	bool	IsA4AFAuto() const { return m_is_A4AF_auto; }
	void	SetA4AFAuto(bool in) { m_is_A4AF_auto = in; }

	CString GetProgressString(uint16 size) const;
	CString GetInfoSummary(CPartFile* partfile) const;

//	int		GetCommonFilePenalty() const;
	void	UpdateDisplayedInfo(bool force = false);

	uint8	GetCategory() /*const*/;
	void	SetCategory(uint8 cat,bool setprio=true);
	// [TPT] - khaos::categorymod+
	void	SetCatResumeOrder(uint16 order)	{ m_catResumeOrder = order; SavePartFile(); }
	uint16	GetCatResumeOrder()				{ return m_catResumeOrder; }
	// [TPT] - khaos::categorymod-
	bool	CheckShowItemInGivenCat(int inCategory) /*const*/;

	uint8*	MMCreatePartStatus();

	//preview
	virtual bool GrabImage(uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth,void* pSender);
	virtual void GrabbingFinished(CxImage** imgResults, uint8 nFramesGrabbed, void* pSender);

	void	FlushBuffersExceptionHandler(CFileException* error);
	void	FlushBuffersExceptionHandler();

	void	PerformFileCompleteEnd(DWORD dwResult);
	
	void	SetFileOp(EPartFileOp eFileOp);
	EPartFileOp GetFileOp() const { return m_eFileOp; }
	void	SetFileOpProgress(UINT uProgress);
	UINT	GetFileOpProgress() const { return m_uFileOpProgress; }

	void	RequestAICHRecovery(uint16 nPart);
	void	AICHRecoveryDataAvailable(uint16 nPart);

	CFileDetailDialogInterface*	GetDetailDialogInterface() const { return m_detailDialogInterface; }	// SLUGFILLER: modelessDialogs

	uint32	m_LastSearchTime;
	uint32	m_LastSearchTimeKad;
	uint8	m_TotalSearchesKad;
	uint32	m_iAllocinfo;
	CUpDownClientPtrList srclist;
	CUpDownClientPtrList A4AFsrclist; //<<-- enkeyDEV(Ottavio84) -A4AF-
	CTime	lastseencomplete;
	CFile	m_hpartfile;				// permanent opened handle to avoid write conflicts
	CMutex 	m_FileCompleteMutex;		// Lord KiRon - Mutex for file completion
	uint16	src_stats[4];
	uint16  net_stats[3];
	volatile bool m_bPreviewing;
	volatile bool m_bRecoveringArchive; // Is archive recovery in progress
	bool	m_bLocalSrcReqQueued;
	bool	srcarevisible;				// used for downloadlistctrl
	bool	hashsetneeded;
    bool    AllowSwapForSourceExchange() { return ::GetTickCount()-lastSwapForSourceExchangeTick > 30*1000; } // ZZ:DownloadManager
    void    SetSwapForSourceExchangeTick() { lastSwapForSourceExchangeTick = ::GetTickCount(); } // ZZ:DownloadManager
    
    bool    GetPreviewPrio() const { return m_bpreviewprio; }
	void    SetPreviewPrio(bool in) { m_bpreviewprio=in; }

    static int RightFileHasHigherPrio(CPartFile* left, CPartFile* right);

	CDeadSourceList	m_DeadSourceList;

	// [TPT] - enkeyDev: ICS
	uint16* CalcDownloadingParts(CUpDownClient* client); // Pawcio for enkeyDEV: ICS
	void	WriteIncPartStatus(CSafeMemFile* file);
    void    NewSrcIncPartsInfo();
	uint32	GetPartSizeToDownload(uint16 partNumber);
    // [TPT] - enkeyDev: ICS

	// [TPT] - Maella -New bandwidth control-
	uint32	Process(uint32 maxammount, bool isLimited, bool fullProcess); // in byte, not in percent
	// Maella end
	// Maella -Accurate measure of bandwidth: eDonkey data + control, network adapter-
	void	CompDownloadRate();
	uint32	GetDownloadFileDatarate() const {return m_nDownDatarate;}	
	// Maella end

#ifdef _DEBUG
	// Diagnostic Support
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	bool	GetNextEmptyBlockInPart(uint16 partnumber,Requested_Block_Struct* result) const;
	void	CompleteFile(bool hashingdone);
	void	CreatePartFile();
	void	Init();
	CHardLimit m_SettingsSaver; // [TPT] - Sivka AutoHL

private:
	BOOL 		PerformFileComplete(); // Lord KiRon
	static UINT CompleteThreadProc(LPVOID pvParams); // Lord KiRon - Used as separate thread to complete file
	static UINT AFX_CDECL AllocateSpaceThread(LPVOID lpParam);
	void		CharFillRange(CString* buffer,uint32 start, uint32 end, char color) const;

	CCorruptionBlackBox	m_CorruptionBlackBox;
	//static CBarShader s_LoadBar; // [TPT]
	//static CBarShader s_ChunkBar; // [TPT]
	uint32	m_iLastPausePurge;
	uint16	count;
	uint16	m_anStates[STATES_COUNT];
	uint32	completedsize;
	uint64	m_uCorruptionLoss;
	uint64	m_uCompressionGain;
	uint32	m_uPartsSavedDueICH;
	//uint32	datarate; // [TPT]
	CString m_fullname;
	CString m_partmetfilename;
	uint32	m_uTransferred;
	bool	paused;
	bool	stopped;
	bool	insufficient; // SLUGFILLER: checkDiskspace
	bool	m_bCompletionError;
	uint8	m_iDownPriority;
	bool	m_bAutoDownPriority;
	EPartFileStatus	status;
	bool	newdate;	// indicates if there was a writeaccess to the .part file
	uint32	lastpurgetime;
	uint32	m_LastNoNeededCheck;
	CTypedPtrList<CPtrList, Gap_Struct*> gaplist;
	CTypedPtrList<CPtrList, Requested_Block_Struct*> requestedblocks_list;
	CArray<uint16,uint16> m_SrcpartFrequency;
	// SLUGFILLER: SafeHash
	CArray<bool,bool> m_PartsShareable;
	// SLUGFILLER: SafeHash
	float	percentcompleted;
	CList<uint16, uint16> corrupted_list;
	uint32	m_ClientSrcAnswered;
	uint16	availablePartsCount;

	uint16	m_MaxSourcesPerFile; // [TPT] - Sivka AutoHL

	// [TPT] - SLUGFILLER: showComments remove - moved comments stuff to CKnownFile
	bool	m_is_A4AF_auto;
	CWinThread* m_AllocateThread;
	DWORD	m_lastRefreshedDLDisplay;
	CUpDownClientPtrList m_downloadingSourceList;
	bool	m_bDeleteAfterAlloc;
    bool	m_bpreviewprio;
	// Barry - Buffered data to be written
	CTypedPtrList<CPtrList, PartFileBufferedData*> m_BufferedData_list;
	uint32	m_nTotalBufferData;
	uint32	m_nLastBufferFlushTime;
	uint8	m_category;
	// [TPT] - khaos::categorymod+
	uint16	m_catResumeOrder;
	// [TPT] - khaos::categorymod-
	DWORD	m_dwFileAttributes;
	time_t	m_tActivated;
	uint32	m_nDlActiveTime;
	uint32	m_tLastModified;	// last file modification time (NT's version of UTC), to be used for stats only!
	uint32	m_tCreated;			// file creation time (NT's version of UTC), to be used for stats only!
    uint32	m_random_update_wait;	
	volatile EPartFileOp m_eFileOp;
	volatile UINT m_uFileOpProgress;
	CRBMultiMap<UINT, SServer>	m_preferredServers;	// [TPT] - itsonlyme: cacheUDPsearchResults

    DWORD   lastSwapForSourceExchangeTick; // ZZ:DownloadManaager
   
	CFileDetailDialogInterface*	m_detailDialogInterface;	// SLUGFILLER: modelessDialogs

 
    // [TPT] - enkeyDev: ICS
    CArray<uint16,uint16> m_SrcIncPartFrequency;
    int     m_ics_filemode;
    // [TPT] - enkeyDev: ICS
	
	// [TPT] - WebCache    
    // JP added handling of proxy-sources on pause/cancel/resume START
	public:
	void CancelProxyDownloads();
	void PauseProxyDownloads();
	void ResumeProxyDownloads();
	// JP added handling of proxy-sources on pause/cancel/resume END
	
	//JP webcache column START
	//JP added stuff from Gnaddelwarz
	uint16	GetWebcacheSourceCount() const; //JP webcache column
	uint16 GetWebcacheSourceOurProxyCount() const;
	uint16 GetWebcacheSourceNotOurProxyCount() const;
	void	CountWebcacheSources() const;
	uint16	WebcacheSources;
	uint16 WebcacheSourcesOurProxy;
	uint16 WebcacheSourcesNotOurProxy;
	uint32  LastWebcacheSourceCountTime; //JP speed up webcache column
	//JP webcache column END

	//JP webcache file detail dialogue START
	uint32  WebCacheDownDataThisFile;//[TPT] - Webcache 1.9 beta3
	uint32	Webcacherequests;
	uint32	SuccessfulWebcacherequests;
	void	AddWebCachedBlockToStats( bool IsGood, uint32 bytes );//[TPT] - Webcache 1.9 beta3
	//JP webcache file detail dialogue END

	//JP Throttle OHCB-production START
	uint32 GetNumberOfBlocksForThisFile();
	uint16 GetMaxNumberOfWebcacheConnectionsForThisFile();
	uint16 GetNumberOfCurrentWebcacheConnectionsForThisFile();
	//JP Throttle OHCB-production END
    // [TPT] - WebCache	

	// [TPT] - Maella -Code Improvement-
private:	

	// Maella -Downloading source list-
	bool m_sourceListChange;	
	// Maella end

	// Maella -Accurate measure of bandwidth: eDonkey data + control, network adapter-
	uint32 m_nDownDatarate;
	// Maella end

	// Maella -New Save/load Sources- enkeyDEV(Ottavio84)
	CSourceSaver m_sourcesaver; 
	// Maella end
	
	// [TPT] - MFCK [addon] - New Tooltips [Rayita]
public:
	void GetTooltipFileInfo(CString &info);
	//void AddClientFileName(LPCSTR fname, CUpDownClient* client);
	//CFileNameList& GetClientFileNameList() { return m_ClientFileNameList; }
	//CString GetClientFileName(const uchar* userhash);
	//void DeleteFileNames() { m_ClientFileNameList.RemoveAll(); }
	// [TPT] - MFCK [addon] - New Tooltips [Rayita]

	// [TPT] - MFCK [addon] - New Tooltips [Rayita]
	bool FileIsComplete() const	{ return status == PS_COMPLETE || status == PS_COMPLETING; }
	bool IsReady() const		{ return status == PS_READY && !paused && !insufficient; }
	bool IsPaused() const		{ return paused || insufficient; }
	bool IsInsufficient() const { return insufficient; }
	bool IsNoEmpty() const		{ return !m_bIsEmpty && status == PS_READY; }
	bool IsError() const		{ return status == PS_ERROR; }
private:
	bool	m_bIsEmpty;
	//CFileNameList m_ClientFileNameList;
	// [TPT] - MFCK [addon] - New Tooltips [Rayita]
};
