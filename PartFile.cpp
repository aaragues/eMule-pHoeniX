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
#include <sys/stat.h>
#include <io.h>
#include <winioctl.h>
#ifdef _DEBUG
#include "DebugHelpers.h"
#endif
#include "emule.h"
#include "PartFile.h"
#include "UpDownClient.h"
#include "UrlClient.h"
#include "ED2KLink.h"
#include "Preview.h"
#include "ArchiveRecovery.h"
#include "SearchList.h"
#include "BandWidthControl.h" // [TPT]
#include "Kademlia/Kademlia/Kademlia.h"
#include "kademlia/kademlia/search.h"
#include "kademlia/kademlia/SearchManager.h"
#include "kademlia/utils/MiscUtils.h"
#include "kademlia/kademlia/prefs.h"
#include "kademlia/kademlia/Entry.h"
#include "DownloadQueue.h"
#include "IPFilter.h"
#include "MMServer.h"
#include "OtherFunctions.h"
#include "Packets.h"
#include "Preferences.h"
#include "SafeFile.h"
#include "SharedFileList.h"
#include "ListenSocket.h"
#include "Sockets.h"
#include "Server.h"
#include "ServerList.h"	// [TPT] - itsonlyme: cacheUDPsearchResults
#include "KnownFileList.h"
#include "emuledlg.h"
#include "TransferWnd.h"
#include "TaskbarNotifier.h"
#include "FileDetailDialog.h"	// [TPT] - SLUGFILLER: modelessDialogs
#include "ClientList.h"
#include "Statistics.h"
#include "shahashset.h"
#include "PeerCacheSocket.h"
#include "Log.h"
// [TPT]
#include "ServerList.h"
#include "SharedFilesWnd.h"
// [TPT] - WebCache	
#include "WebCacheSocket.h" // yonatan http
#include "WebCachedBlockList.h" //JP remove all blocks if download stopped
#include "ThrottledChunkList.h" // jp Don't request chunks for which we are currently receiving proxy sources
#include "WebCacheProxyClient.h" // jp stalled proxy download fix attempt
// [TPT] - WebCache	

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

#ifndef FSCTL_SET_SPARSE
#define FSCTL_SET_SPARSE                CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 49, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#endif

#define MAX_PREF_SERVERS	10	// [TPT] - itsonlyme: cacheUDPsearchResults

// Barry - use this constant for both places
#define PROGRESS_HEIGHT 3

// [TPT]
//CBarShader CPartFile::s_LoadBar(PROGRESS_HEIGHT); // Barry - was 5
//CBarShader CPartFile::s_ChunkBar(16); 
IMPLEMENT_DYNAMIC(CPartFile, CKnownFile)

CPartFile::CPartFile()
// Maella -New Save/load Sources- enkeyDEV(Ottavio84)
:	m_sourcesaver(this) 
// Maella end
{
	Init();
}

CPartFile::CPartFile(CSearchFile* searchresult)
// Maella -New Save/load Sources- enkeyDEV(Ottavio84)
:	m_sourcesaver(this) 
// Maella end
{
	Init();
	md4cpy(m_abyFileHash, searchresult->GetFileHash());
	for (int i = 0; i < searchresult->taglist.GetCount();i++){
		const CTag* pTag = searchresult->taglist[i];
		switch (pTag->GetNameID()){
			case FT_FILENAME:{
				ASSERT( pTag->IsStr() );
				if (pTag->IsStr()){
					if (GetFileName().IsEmpty())
						SetFileName(pTag->GetStr(), true);
				}
				break;
			}
			case FT_FILESIZE:{
				ASSERT( pTag->IsInt() );
				if (pTag->IsInt())
					SetFileSize(pTag->GetInt());
				break;
			}
			default:{
				bool bTagAdded = false;
				if (pTag->GetNameID() != 0 && pTag->GetName() == NULL && (pTag->IsStr() || pTag->IsInt()))
				{
					static const struct
					{
						uint8	nName;
						uint8	nType;
					} _aMetaTags[] = 
					{
						{ FT_MEDIA_ARTIST,  2 },
						{ FT_MEDIA_ALBUM,   2 },
						{ FT_MEDIA_TITLE,   2 },
						{ FT_MEDIA_LENGTH,  3 },
						{ FT_MEDIA_BITRATE, 3 },
						{ FT_MEDIA_CODEC,   2 },
						{ FT_FILETYPE,		2 },
						{ FT_FILEFORMAT,	2 }
					};
					for (int t = 0; t < ARRSIZE(_aMetaTags); t++)
					{
						if (pTag->GetType() == _aMetaTags[t].nType && pTag->GetNameID() == _aMetaTags[t].nName)
						{
							// skip string tags with empty string values
							if (pTag->IsStr() && pTag->GetStr().IsEmpty())
								break;

							// skip integer tags with '0' values
							if (pTag->IsInt() && pTag->GetInt() == 0)
								break;

							TRACE(_T("CPartFile::CPartFile(CSearchFile*): added tag %s\n"), pTag->GetFullInfo());
							CTag* newtag = new CTag(*pTag);
							taglist.Add(newtag);
							bTagAdded = true;
							break;
						}
					}
				}

				if (!bTagAdded)
					TRACE(_T("CPartFile::CPartFile(CSearchFile*): ignored tag %s\n"), pTag->GetFullInfo());
			}
		}
	}
	CreatePartFile();
}

CPartFile::CPartFile(CString edonkeylink)
// [TPT]
// Maella -New Save/load Sources- enkeyDEV(Ottavio84)
:	m_sourcesaver(this) 
// Maella end
{
	CED2KLink* pLink = 0;
	try {
		pLink = CED2KLink::CreateLinkFromUrl(edonkeylink);
		_ASSERT( pLink != 0 );
		CED2KFileLink* pFileLink = pLink->GetFileLink();
		if (pFileLink==0) 
			throw GetResString(IDS_ERR_NOTAFILELINK);
		InitializeFromLink(pFileLink);
	} catch (CString error) {
		TCHAR buffer[200];
		_stprintf(buffer,GetResString(IDS_ERR_INVALIDLINK),error.GetBuffer());
		LogError(LOG_STATUSBAR, GetResString(IDS_ERR_LINKERROR), buffer);
		SetStatus(PS_ERROR);
	}
	delete pLink;
}

void CPartFile::InitializeFromLink(CED2KFileLink* fileLink)
{
	Init();
	try{
		SetFileName(fileLink->GetName(), true);
		SetFileSize(fileLink->GetSize());
		md4cpy(m_abyFileHash, fileLink->GetHashKey());
		if (!theApp.downloadqueue->IsFileExisting(m_abyFileHash))
		{
			if (fileLink->m_hashset && fileLink->m_hashset->GetLength() > 0)
			{
				try
				{
					if (!LoadHashsetFromFile(fileLink->m_hashset, true))
					{
						ASSERT( hashlist.GetCount() == 0 );
						AddDebugLogLine(false, _T("eD2K link \"%s\" specified with invalid hashset"), fileLink->GetName());
					}
					else
						hashsetneeded = false;
				}
				catch (CFileException* e)
				{
					TCHAR szError[MAX_CFEXP_ERRORMSG];
					e->GetErrorMessage(szError, ARRSIZE(szError));
					AddDebugLogLine(false, _T("Error: Failed to process hashset for eD2K link \"%s\" - %s"), fileLink->GetName(), szError);
					e->Delete();
				}
			}
			CreatePartFile();
		}
		else
			SetStatus(PS_ERROR);
	}
	catch(CString error){
		TCHAR buffer[200];
		_stprintf(buffer, GetResString(IDS_ERR_INVALIDLINK), error.GetBuffer());
		LogError(LOG_STATUSBAR, GetResString(IDS_ERR_LINKERROR), buffer);
		SetStatus(PS_ERROR);
	}
}

CPartFile::CPartFile(CED2KFileLink* fileLink)
// [TPT]
// Maella -New Save/load Sources- enkeyDEV(Ottavio84)
:	m_sourcesaver(this) 
// Maella end
{
	InitializeFromLink(fileLink);
}

void CPartFile::Init(){
	// [TPT] - WebCache			
	LastWebcacheSourceCountTime = ::GetTickCount(); //JP speed up sorting webcache column
	WebcacheSources = 0; //JP speed up sorting webcache column
	WebcacheSourcesOurProxy = 0; //JP added from Gnaddelwarz
	WebcacheSourcesNotOurProxy = 0;//JP added from Gnaddelwarz
	Webcacherequests = 0; //JP WC-Filedetails
	SuccessfulWebcacherequests = 0; //JP WC-Filedetails
	WebCacheDownDataThisFile = 0; //JP WC-Filedetails //[TPT] - Webcache 1.9 beta3
	// [TPT] - WebCache
	newdate = true;
	m_LastSearchTime = 0;
	m_LastSearchTimeKad = 0;
	m_TotalSearchesKad = 0;
	lastpurgetime = ::GetTickCount();
	paused = false;
	stopped= false;
	status = PS_EMPTY;
	insufficient = false;
	m_bCompletionError = false;
	m_uTransferred = 0;
	m_iLastPausePurge = time(NULL);
	m_AllocateThread=NULL;
	m_iAllocinfo = 0;
	if(thePrefs.GetNewAutoDown()){
		m_iDownPriority = PR_HIGH;
		m_bAutoDownPriority = true;
	}
	else{
		m_iDownPriority = PR_NORMAL;
		m_bAutoDownPriority = false;
	}
	srcarevisible = false;
	MEMZERO(m_anStates, sizeof(m_anStates));
	m_nDownDatarate = 0; // [TPT]
	hashsetneeded = true;
	count = 0;
	percentcompleted = 0;
	completedsize=0;
	m_bPreviewing = false;
	lastseencomplete = NULL;
	availablePartsCount=0;
	m_ClientSrcAnswered = 0;
	m_LastNoNeededCheck = 0;
	m_MaxSourcesPerFile = thePrefs.GetMaxSourcePerFile();// [TPT] - Sivka AutoHL
	m_uRating = 0;
	(void)m_strComment;
	m_nTotalBufferData = 0;
	m_nLastBufferFlushTime = 0;
	m_bRecoveringArchive = false;
	m_uCompressionGain = 0;
	m_uCorruptionLoss = 0;
	m_uPartsSavedDueICH = 0;
	// [TPT] - SLUGFILLER: showComments remove - moved comments stuff to CKnownFile
	m_category=0;	
	// Maella -Downloading source list-
	m_sourceListChange = false;
	// Maella end
	m_lastRefreshedDLDisplay = 0;
	m_is_A4AF_auto=false;
	m_bLocalSrcReqQueued = false;
	MEMZERO(src_stats, sizeof(src_stats));
	MEMZERO(net_stats, sizeof(net_stats));
	m_nCompleteSourcesTime = time(NULL);
	m_nCompleteSourcesCount = 0;
	m_nCompleteSourcesCountLo = 0;
	m_nCompleteSourcesCountHi = 0;
	m_dwFileAttributes = 0;
	m_bDeleteAfterAlloc=false;
	m_tActivated = 0;
	m_nDlActiveTime = 0;
	m_tLastModified = 0;
	m_tCreated = 0;
	m_eFileOp = PFOP_NONE;
	m_uFileOpProgress = 0;
    m_bpreviewprio = false;
    m_random_update_wait = (uint32)(rand()/(RAND_MAX/1000));
    lastSwapForSourceExchangeTick = ::GetTickCount();
	m_DeadSourceList.Init(false);
	// [TPT] - khaos::categorymod+
	m_catResumeOrder=0;
	// [TPT] - khaos::categorymod-	
	
	m_nVirtualCompleteSourcesCount = 0;// [TPT] - Powershare
	m_ics_filemode = 0;	// [TPT] - enkeyDev: ICS

	m_detailDialogInterface = new CFileDetailDialogInterface(this);	// [TPT] - SLUGFILLER: modelessDialogs
}

CPartFile::~CPartFile()
{
	// Barry - Ensure all buffered data is written
	try{
		if (m_AllocateThread != NULL){
			HANDLE hThread = m_AllocateThread->m_hThread;
			// 2 minutes to let the thread finish
			if (WaitForSingleObject(hThread, 120000) == WAIT_TIMEOUT)
				TerminateThread(hThread, 100);
		}

		if (m_hpartfile.m_hFile != INVALID_HANDLE_VALUE)
			FlushBuffer(true);
	}
	catch(CFileException* e){
		e->Delete();
	}
	
	if (m_hpartfile.m_hFile != INVALID_HANDLE_VALUE){
		// commit file and directory entry
		m_hpartfile.Close();
		// Update met file (with current directory entry)
		SavePartFile();
	}

	for (POSITION pos = gaplist.GetHeadPosition();pos != 0;)
		delete gaplist.GetNext(pos);

	pos = m_BufferedData_list.GetHeadPosition();
	while (pos){
		PartFileBufferedData *item = m_BufferedData_list.GetNext(pos);
		delete[] item->data;
		delete item;
	}

	// [TPT]
	// Maella -Extended clean-up II-
	theApp.clientlist->CleanUp(this);
	// Maella end
	delete m_detailDialogInterface; // [TPT] - SLUGFILLER: modelessDialogs
}

#ifdef _DEBUG
void CPartFile::AssertValid() const
{
	CKnownFile::AssertValid();

	(void)m_LastSearchTime;
	(void)m_LastSearchTimeKad;
	(void)m_TotalSearchesKad;
	srclist.AssertValid();
	A4AFsrclist.AssertValid();
	(void)lastseencomplete;
	m_hpartfile.AssertValid();
	m_FileCompleteMutex.AssertValid();
	(void)src_stats;
	(void)net_stats;
	CHECK_BOOL(m_bPreviewing);
	CHECK_BOOL(m_bRecoveringArchive);
	CHECK_BOOL(m_bLocalSrcReqQueued);
	CHECK_BOOL(srcarevisible);
	CHECK_BOOL(hashsetneeded);
	(void)m_iLastPausePurge;
	(void)count;
	(void)m_anStates;
	ASSERT( completedsize <= GetFileSize() );
	(void)m_uCorruptionLoss;
	(void)m_uCompressionGain;
	(void)m_uPartsSavedDueICH; 
	(void)m_nDownDatarate; // [TPT]
	(void)m_sourceListChange; //[TPT]
	(void)m_fullname;
	(void)m_partmetfilename;
	(void)m_uTransferred;
	CHECK_BOOL(paused);
	CHECK_BOOL(stopped);
	CHECK_BOOL(insufficient);
	CHECK_BOOL(m_bCompletionError);
	ASSERT( m_iDownPriority == PR_LOW || m_iDownPriority == PR_NORMAL || m_iDownPriority == PR_HIGH );
	CHECK_BOOL(m_bAutoDownPriority);
	ASSERT( status == PS_READY || status == PS_EMPTY || status == PS_WAITINGFORHASH || status == PS_ERROR || status == PS_COMPLETING || status == PS_COMPLETE );
	CHECK_BOOL(newdate);
	(void)lastpurgetime;
	(void)m_LastNoNeededCheck;
	gaplist.AssertValid();
	requestedblocks_list.AssertValid();
	m_SrcpartFrequency.AssertValid();
	ASSERT( percentcompleted >= 0.0F && percentcompleted <= 100.0F );
	corrupted_list.AssertValid();
	(void)availablePartsCount;
	(void)m_ClientSrcAnswered;
	//(void)s_LoadBar;
	//(void)s_ChunkBar;
	//CHECK_BOOL(hasRating);
	//CHECK_BOOL(hasBadRating);
	//CHECK_BOOL(hasComment);
	(void)m_lastRefreshedDLDisplay;
	m_downloadingSourceList.AssertValid();
	m_BufferedData_list.AssertValid();
	(void)m_nTotalBufferData;
	(void)m_nLastBufferFlushTime;
	(void)m_category;
	CHECK_BOOL(m_is_A4AF_auto);
	(void)m_dwFileAttributes;
}

void CPartFile::Dump(CDumpContext& dc) const
{
	CKnownFile::Dump(dc);
}
#endif

void CPartFile::CreatePartFile()
{
	if (GetFileSize() > MAX_EMULE_FILE_SIZE){
		LogError(LOG_STATUSBAR, GetResString(IDS_ERR_CREATEPARTFILE));
		SetStatus(PS_ERROR);
		return;
	}

	// use lowest free partfilenumber for free file (InterCeptor)
	int i = 0; 
	CString filename; 
	do{
		i++; 
		filename.Format(_T("%s\\%03i.part"), thePrefs.GetTempDir(), i); 
	}
	while (PathFileExists(filename));
	m_partmetfilename.Format(_T("%03i.part.met"), i); 
	SetPath(thePrefs.GetTempDir());
	m_fullname.Format(_T("%s\\%s"), thePrefs.GetTempDir(), m_partmetfilename);

	CTag* partnametag = new CTag(FT_PARTFILENAME,RemoveFileExtension(m_partmetfilename));
	taglist.Add(partnametag);
	
	Gap_Struct* gap = new Gap_Struct;
	gap->start = 0;
	gap->end = GetFileSize()-1; // [TPT]
	gaplist.AddTail(gap);

	CString partfull(RemoveFileExtension(m_fullname));
	SetFilePath(partfull);
	if (!m_hpartfile.Open(partfull,CFile::modeCreate|CFile::modeReadWrite|CFile::shareDenyWrite|CFile::osSequentialScan)){
		LogError(LOG_STATUSBAR, GetResString(IDS_ERR_CREATEPARTFILE));
		SetStatus(PS_ERROR);
	}
	else{
		if (thePrefs.GetSparsePartFiles()){
			DWORD dwReturnedBytes = 0;
			if (!DeviceIoControl(m_hpartfile.m_hFile, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &dwReturnedBytes, NULL))
			{
				// Errors:
				// ERROR_INVALID_FUNCTION	returned by WinXP when attempting to create a sparse file on a FAT32 partition
				DWORD dwError = GetLastError();
				if (dwError != ERROR_INVALID_FUNCTION && thePrefs.GetVerboseLogPriority() <= DLP_VERYLOW)
					DebugLogError(_T("Failed to apply NTFS sparse file attribute to file \"%s\" - %s"), partfull, GetErrorMessage(dwError, 1));
			}
		}

		struct _stat fileinfo;
		if (_tstat(partfull, &fileinfo) == 0){
			m_tLastModified = fileinfo.st_mtime;
			m_tCreated = fileinfo.st_ctime;
		}
		else
			AddDebugLogLine(false, _T("Failed to get file date for \"%s\" - %hs"), partfull, _tcserror(errno));
	}
	m_dwFileAttributes = GetFileAttributes(partfull);
	if (m_dwFileAttributes == INVALID_FILE_ATTRIBUTES)
		m_dwFileAttributes = 0;

	if (GetED2KPartHashCount() == 0)
		hashsetneeded = false;
	//[TPT] - SiRoB, SLUGFILLER: SafeHash - setting at the hotspot
	// the important part
	m_PartsShareable.SetSize(GetPartCount());
	for (uint32 i = 0; i < GetPartCount();i++)
		m_PartsShareable[i] = false;
	//[TPT] - SiRoB, SLUGFILLER: SafeHash

	m_SrcpartFrequency.SetSize(GetPartCount());
	m_SrcIncPartFrequency.SetSize(GetPartCount()); // [TPT] - enkeyDev: ICS
	for (uint32 i = 0; i < GetPartCount();i++){
		m_SrcpartFrequency[i] = 0;
		m_SrcIncPartFrequency[i] = 0;	// [TPT] - enkeyDev: ICS
	}
	paused = false;

	if (thePrefs.AutoFilenameCleanup())
		SetFileName(CleanupFilename(GetFileName()));

	SavePartFile();
	SetActive(theApp.IsConnected());
}

/*
 * Due to the synchronized outflushing of the data in Shareaza, I am trying here a good-luck-file-reading approach.
 * update: only worked with 2.0 and not on updates (as to be expected...) , removed for release
 */
//uint8 CPartFile::ImportShareazaTempfile(LPCTSTR in_directory,LPCTSTR in_filename , bool getsizeonly) {...


uint8 CPartFile::LoadPartFile(LPCTSTR in_directory,LPCTSTR in_filename, bool getsizeonly)
{
	bool isnewstyle;
	uint8 version,partmettype=PMT_UNKNOWN;

	CMap<uint16, uint16, Gap_Struct*, Gap_Struct*> gap_map; // Slugfiller
	m_uTransferred = 0;
	m_partmetfilename = in_filename;
	SetPath(in_directory);
	m_fullname.Format(_T("%s\\%s"), GetPath(), m_partmetfilename);
		
	m_SettingsSaver.LoadSettings(this); // [TPT] - Sivka AutoHL
	
	// readfile data form part.met file
	CSafeBufferedFile metFile;
	CFileException fexpMet;
	if (!metFile.Open(m_fullname, CFile::modeRead|CFile::osSequentialScan|CFile::typeBinary|CFile::shareDenyWrite, &fexpMet)){
		CString strError;
		strError.Format(GetResString(IDS_ERR_OPENMET), m_partmetfilename, _T(""));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexpMet.GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		LogError(LOG_STATUSBAR, _T("%s"), strError);
		return false;
	}
	setvbuf(metFile.m_pStream, NULL, _IOFBF, 16384);

	try{
		version = metFile.ReadUInt8();
		
		if (version != PARTFILE_VERSION && version!= PARTFILE_SPLITTEDVERSION ){
			metFile.Close();
			//if (version==83) {				return ImportShareazaTempfile(in_directory, in_filename,getsizeonly);}
			LogError(LOG_STATUSBAR, GetResString(IDS_ERR_BADMETVERSION), m_partmetfilename, GetFileName());
			return false;
		}
		
		isnewstyle=(version== PARTFILE_SPLITTEDVERSION);
		partmettype= isnewstyle?PMT_SPLITTED:PMT_DEFAULTOLD;
		if (!isnewstyle) {
			uint8 test[4];
			metFile.Seek(24, CFile::begin);
			metFile.Read(&test[0],1);
			metFile.Read(&test[1],1);
			metFile.Read(&test[2],1);
			metFile.Read(&test[3],1);

			metFile.Seek(1, CFile::begin);

			if (test[0]==0 && test[1]==0 && test[2]==2 && test[3]==1) {
				isnewstyle=true;	// edonkeys so called "old part style"
				partmettype=PMT_NEWOLD;
			}
		}

		if (isnewstyle) {
			uint32 temp;
			metFile.Read(&temp,4);

			if (temp==0) {	// 0.48 partmets - different again
				LoadHashsetFromFile(&metFile, false);
			} else {
				uchar gethash[16];
				metFile.Seek(2, CFile::begin);
				LoadDateFromFile(&metFile);
				metFile.Read(&gethash, 16);
				md4cpy(m_abyFileHash, gethash);
			}

		} else {
			LoadDateFromFile(&metFile);
			LoadHashsetFromFile(&metFile, false);
		}
		// [TPT] - Powershare
		CMap<uint16,uint16,uint32,uint32> spread_start_map;
		CMap<uint16,uint16,uint32,uint32> spread_end_map;
		CMap<uint16,uint16,uint32,uint32> spread_count_map;
		// [TPT] - Powershare end

		UINT tagcount = metFile.ReadUInt32();
		for (UINT j = 0; j < tagcount; j++){
			CTag* newtag = new CTag(&metFile, false);
			if (!getsizeonly || (getsizeonly && (newtag->GetNameID()==FT_FILESIZE || newtag->GetNameID()==FT_FILENAME))){
			    switch (newtag->GetNameID()){
				    case FT_FILENAME:{
					    if (!newtag->IsStr()) {
						    LogError(LOG_STATUSBAR, GetResString(IDS_ERR_METCORRUPT), m_partmetfilename, GetFileName());
						    delete newtag;
						    return false;
					    }
						if (GetFileName().IsEmpty())
							SetFileName(newtag->GetStr());
					    delete newtag;
					    break;
				    }
				    case FT_LASTSEENCOMPLETE:{
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt())
						    lastseencomplete = newtag->GetInt();
					    delete newtag;
					    break;
				    }
				    case FT_FILESIZE:{
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt())
						    SetFileSize(newtag->GetInt());
					    delete newtag;
					    break;
				    }
				    case FT_TRANSFERRED:{
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt())
						    m_uTransferred = newtag->GetInt();
					    delete newtag;
					    break;
				    }
				    case FT_COMPRESSION:{
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt())
							m_uCompressionGain = newtag->GetInt();
					    delete newtag;
					    break;
				    }
				    case FT_CORRUPTED:{
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt())
							m_uCorruptionLoss = newtag->GetInt();
					    delete newtag;
					    break;
				    }
				    case FT_FILETYPE:{
						ASSERT( newtag->IsStr() );
						if (newtag->IsStr())
						    SetFileType(newtag->GetStr());
					    delete newtag;
					    break;
				    }
				    case FT_CATEGORY:{
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt())
							m_category = newtag->GetInt();
					    delete newtag;
					    break;
				    }
				    case FT_DLPRIORITY:{
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt()){
							if (!isnewstyle){
								m_iDownPriority = newtag->GetInt();
								if( m_iDownPriority == PR_AUTO ){
									m_iDownPriority = PR_HIGH;
									SetAutoDownPriority(true);
								}
								else{
									if (m_iDownPriority != PR_LOW && m_iDownPriority != PR_NORMAL && m_iDownPriority != PR_HIGH)
										m_iDownPriority = PR_NORMAL;
									SetAutoDownPriority(false);
								}
							}
						}
						delete newtag;
						break;
				    }
				    case FT_STATUS:{
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt()){
						    paused = newtag->GetInt();
						    stopped = paused;
						}
					    delete newtag;
					    break;
				    }
				    case FT_ULPRIORITY:{
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt()){
							if (!isnewstyle){
								int iUpPriority = newtag->GetInt();
								if( iUpPriority == PR_AUTO ){
									SetUpPriority(PR_HIGH, false);
									SetAutoUpPriority(true);
								}
								else{
									if (iUpPriority != PR_VERYLOW && iUpPriority != PR_LOW && iUpPriority != PR_NORMAL && iUpPriority != PR_HIGH && iUpPriority != PR_VERYHIGH)
										iUpPriority = PR_NORMAL;
									SetUpPriority(iUpPriority, false);
									SetAutoUpPriority(false);
								}
							}
						}
						delete newtag;
					    break;
				    }
				    case FT_KADLASTPUBLISHSRC:{
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt())
						{
						    SetLastPublishTimeKadSrc(newtag->GetInt(), 0);
							if(GetLastPublishTimeKadSrc() > (uint32)time(NULL)+KADEMLIAREPUBLISHTIMES)
							{
								//There may be a posibility of an older client that saved a random number here.. This will check for that..
								SetLastPublishTimeKadSrc(0,0);
							}
						}
					    delete newtag;
					    break;
				    }
				    case FT_KADLASTPUBLISHNOTES:{
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt())
						{
						    SetLastPublishTimeKadNotes(newtag->GetInt());
						}
					    delete newtag;
					    break;
				    }
                    case FT_DL_PREVIEW:{
                        ASSERT( newtag->IsInt() );
                        if(newtag->GetInt() == 1) {
                            SetPreviewPrio(true);
                        } else {
                            SetPreviewPrio(false);
                        }
					    delete newtag;
					    break;
				    }

				   // statistics
					case FT_ATTRANSFERRED:{
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt())
							statistic.alltimetransferred = newtag->GetInt();
						delete newtag;
						break;
					}
					case FT_ATTRANSFERREDHI:{
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt())
						{
							uint32 hi,low;
							low=statistic.alltimetransferred;
							hi = newtag->GetInt();
							uint64 hi2;
							hi2=hi;
							hi2=hi2<<32;
							statistic.alltimetransferred=low+hi2;
						}
						delete newtag;
						break;
					}
					case FT_ATREQUESTED:{
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt())
							statistic.alltimerequested = newtag->GetInt();
						delete newtag;
						break;
					}
 					case FT_ATACCEPTED:{
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt())
							statistic.alltimeaccepted = newtag->GetInt();
						delete newtag;
						break;
					}
					// [TPT] - khaos::categorymod+
					case FT_CATRESUMEORDER:{
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt())
							m_catResumeOrder = newtag->GetInt();
						delete newtag;
						break;
					}					
					// [TPT] - khaos::categorymod-
					// [TPT] - xMule_MOD: showSharePermissions - load permissions
					case FT_PERMISSIONS: {
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt())
							SetPermissions(newtag->GetInt());
						delete newtag;
						break;
					}
					// [TPT] - xMule_MOD: showSharePermissions
					// old tags: as long as they are not needed, take the chance to purge them					
					case FT_KADLASTPUBLISHKEY:
						ASSERT( newtag->IsInt() );
						delete newtag;
						break;
					case FT_DL_ACTIVE_TIME:
						ASSERT( newtag->IsInt() );
						if (newtag->IsInt())
							m_nDlActiveTime = newtag->GetInt();
						delete newtag;
						break;
					case FT_CORRUPTEDPARTS:
						ASSERT( newtag->IsStr() );
						if (newtag->IsStr())
						{
							ASSERT( corrupted_list.GetHeadPosition() == NULL );
							CString strCorruptedParts(newtag->GetStr());
							int iPos = 0;
							CString strPart = strCorruptedParts.Tokenize(_T(","), iPos);
							while (!strPart.IsEmpty())
							{
								UINT uPart;
								if (_stscanf(strPart, _T("%u"), &uPart) == 1)
								{
									if (uPart < GetPartCount() && !IsCorruptedPart(uPart))
										corrupted_list.AddTail(uPart);
								}
								strPart = strCorruptedParts.Tokenize(_T(","), iPos);
							}
						}
						delete newtag;
						break;
					case FT_AICH_HASH:{
						ASSERT( newtag->IsStr() );
						CAICHHash hash;
						if (DecodeBase32(newtag->GetStr(),hash) == CAICHHash::GetHashSize())
							m_pAICHHashSet->SetMasterHash(hash, AICH_VERIFIED);
						else
							ASSERT( false );
						delete newtag;
						break;
					}
				    default:{
					    if (newtag->GetNameID()==0 && (newtag->GetName()[0]==FT_GAPSTART || newtag->GetName()[0]==FT_GAPEND))
						{
							ASSERT( newtag->IsInt() );
							if (newtag->IsInt())
							{
								Gap_Struct* gap;
								uint16 gapkey = atoi(&newtag->GetName()[1]);
								if (!gap_map.Lookup(gapkey, gap))
								{
									gap = new Gap_Struct;
									gap_map.SetAt(gapkey, gap);
									gap->start = (uint32)-1;
									gap->end = (uint32)-1;
								}
								if (newtag->GetName()[0] == FT_GAPSTART)
									gap->start = newtag->GetInt();
								if (newtag->GetName()[0] == FT_GAPEND)
									gap->end = newtag->GetInt() - 1;
							}
						    delete newtag;
						}
						// [TPT] - SLUGFILLER: Spreadbars
						else if(!newtag->GetNameID() && newtag->IsInt() && (newtag->GetName() != NULL))
						{
							LPCSTR tagname = newtag->GetName();
							if (tagname[0] == FT_SPREADSTART ||
								tagname[0] == FT_SPREADEND ||
								tagname[0] == FT_SPREADCOUNT)
							{
							uint16 spreadkey = atoi(&tagname[1]);
								if (tagname[0] == FT_SPREADSTART)
								spread_start_map.SetAt(spreadkey, newtag->GetInt());
								else if (tagname[0] == FT_SPREADEND)
								spread_end_map.SetAt(spreadkey, newtag->GetInt());
								else if (tagname[0] == FT_SPREADCOUNT)
								spread_count_map.SetAt(spreadkey, newtag->GetInt());
							}
							// [TPT] - SLUGFILLER: Spreadbars
							// [TPT] - Powershare
							else if(CmpED2KTagName(tagname, FT_POWERSHARE) == 0)
								SetPowerShared((newtag->GetInt() <= 3) ? newtag->GetInt() : -1);
							else if(CmpED2KTagName(tagname, FT_POWERSHARE_LIMIT) == 0)
								SetPowerShareLimit((newtag->GetInt() <= 200) ? newtag->GetInt() : -1);
							else if(CmpED2KTagName(tagname, FT_HIDEOS) == 0)
								SetHideOS((newtag->GetInt() <= 10) ? newtag->GetInt() : -1);
							else if(CmpED2KTagName(tagname, FT_SELECTIVE_CHUNK) == 0)
								SetSelectiveChunk(newtag->GetInt()<=1?newtag->GetInt():-1);
							// [TPT] - Powershare
							delete newtag;
						}
					    else
						    taglist.Add(newtag);
				    }
				}
			}
			else
				delete newtag;
		}

	// [TPT] - Powershare
	for (POSITION pos = spread_start_map.GetStartPosition(); pos != NULL; ){
		uint16 spreadkey;
		uint32 spread_start;
		uint32 spread_end;
		uint32 spread_count;
		spread_start_map.GetNextAssoc(pos, spreadkey, spread_start);
		if (!spread_end_map.Lookup(spreadkey, spread_end))
			continue;
		if (!spread_count_map.Lookup(spreadkey, spread_count))
			continue;
		if (!spread_count || spread_start >= spread_end)
			continue;
		statistic.AddBlockTransferred(spread_start, spread_end, spread_count);	// All tags accounted for
	}
	// [TPT] - Powershare END


		// load the hashsets from the hybridstylepartmet
		if (isnewstyle && !getsizeonly && (metFile.GetPosition()<metFile.GetLength()) ) {
			uint8 temp;
			metFile.Read(&temp,1);
			
			uint16 parts=GetPartCount();	// assuming we will get all hashsets
			
			for (uint16 i = 0; i < parts && (metFile.GetPosition()+16<metFile.GetLength()); i++){
				uchar* cur_hash = new uchar[16];
				metFile.Read(cur_hash, 16);
				hashlist.Add(cur_hash);
			}

			uchar* checkhash= new uchar[16];
			if (!hashlist.IsEmpty()){
				uchar* buffer = new uchar[hashlist.GetCount()*16];
				for (int i = 0; i < hashlist.GetCount(); i++)
					md4cpy(buffer+(i*16), hashlist[i]);
				CreateHash(buffer, hashlist.GetCount()*16, checkhash);
				delete[] buffer;
			}
			bool flag=false;
			if (!md4cmp(m_abyFileHash, checkhash))
				flag=true;
			else{
				for (int i = 0; i < hashlist.GetSize(); i++)
					delete[] hashlist[i];
				hashlist.RemoveAll();
				flag=false;
			}
			delete[] checkhash;
		}

		metFile.Close();
	}
	catch(CFileException* error){
		if (error->m_cause == CFileException::endOfFile)
			LogError(LOG_STATUSBAR, GetResString(IDS_ERR_METCORRUPT), m_partmetfilename, GetFileName());
		else{
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer,ARRSIZE(buffer));
			LogError(LOG_STATUSBAR, GetResString(IDS_ERR_FILEERROR), m_partmetfilename, GetFileName(), buffer);
		}
		error->Delete();
		return false;
	}
	catch(...){
		LogError(LOG_STATUSBAR, GetResString(IDS_ERR_METCORRUPT), m_partmetfilename, GetFileName());
		ASSERT(0);
		return false;
	}

	if (GetFileSize() > MAX_EMULE_FILE_SIZE) {
		LogError(LOG_STATUSBAR, GetResString(IDS_ERR_FILEERROR), m_partmetfilename, GetFileName(), _T("File size exceeds supported limit"));
		return false;
	}

	if (getsizeonly) {
		return partmettype;
	}

	// Now to flush the map into the list (Slugfiller)
	for (POSITION pos = gap_map.GetStartPosition(); pos != NULL; ){
		Gap_Struct* gap;
		uint16 gapkey;
		gap_map.GetNextAssoc(pos, gapkey, gap);
		// SLUGFILLER: SafeHash - revised code, and extra safety
		if (gap->start != -1 && gap->end != -1 && gap->start <= gap->end && gap->start < GetFileSize()){
			if (gap->end >= GetFileSize())
				gap->end = GetFileSize()-1; // Clipping
			AddGap(gap->start, gap->end); // All tags accounted for, use safe adding
		}
		delete gap;
		// SLUGFILLER: SafeHash
	}

	// verify corrupted parts list
	POSITION posCorruptedPart = corrupted_list.GetHeadPosition();
	while (posCorruptedPart)
	{
		POSITION posLast = posCorruptedPart;
		UINT uCorruptedPart = corrupted_list.GetNext(posCorruptedPart);
		if (IsComplete(uCorruptedPart*PARTSIZE, (uCorruptedPart+1)*PARTSIZE-1))
			corrupted_list.RemoveAt(posLast);
	}

	//check if this is a backup
	if(_tcsicmp(_tcsrchr(m_fullname, _T('.')), PARTMET_TMP_EXT) == 0)
		m_fullname = RemoveFileExtension(m_fullname);

	// open permanent handle
	CString searchpath(RemoveFileExtension(m_fullname));
	CFileException fexpPart;
	if (!m_hpartfile.Open(searchpath, CFile::modeReadWrite|CFile::shareDenyWrite|CFile::osSequentialScan, &fexpPart)){
		CString strError;
		strError.Format(GetResString(IDS_ERR_FILEOPEN), searchpath, GetFileName());
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexpPart.GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		LogError(LOG_STATUSBAR, _T("%s"), strError);
		return false;
	}

	// read part file creation time
	struct _stat fileinfo;
	if (_tstat(searchpath, &fileinfo) == 0){
		m_tLastModified = fileinfo.st_mtime;
		m_tCreated = fileinfo.st_ctime;
	}
	else
		AddDebugLogLine(false, _T("Failed to get file date for \"%s\" - %hs"), searchpath, _tcserror(errno));

	try{
		SetFilePath(searchpath);
		m_dwFileAttributes = GetFileAttributes(GetFilePath());
		if (m_dwFileAttributes == INVALID_FILE_ATTRIBUTES)
			m_dwFileAttributes = 0;

		// SLUGFILLER: SafeHash - final safety, make sure any missing part of the file is gap
	// [TPT]
	if (m_hpartfile.GetLength() < GetFileSize())
		AddGap(m_hpartfile.GetLength(), GetFileSize()-1);
		// Goes both ways - Partfile should never be too large
		if (m_hpartfile.GetLength() > GetFileSize()){
			TRACE(_T("Partfile \"%s\" is too large! Truncating %I64u bytes.\n"), GetFileName(), m_hpartfile.GetLength() - GetFileSize());
			m_hpartfile.SetLength(GetFileSize());
		}
		// SLUGFILLER: SafeHash

		//[TPT] SiRoB, SLUGFILLER: SafeHash
		// the important part
		m_PartsShareable.SetSize(GetPartCount());
		for (uint32 i = 0; i < GetPartCount();i++)
			m_PartsShareable[i] = false;
		//[TPT] eNd SiRoB, SLUGFILLER: SafeHash

		m_SrcpartFrequency.SetSize(GetPartCount());
		m_SrcIncPartFrequency.SetSize(GetPartCount()); // [TPT] - enkeyDev: ICS
		for (uint32 i = 0; i != GetPartCount();i++){
			m_SrcpartFrequency[i] = 0;
			m_SrcIncPartFrequency[i] = 0;	// [TPT] - enkeyDev: ICS
		}
		SetStatus(PS_EMPTY);
		// check hashcount, filesatus etc
		if (GetHashCount() != GetED2KPartHashCount()){
			ASSERT( hashlist.GetSize() == 0 );
			hashsetneeded = true;
			return true;
		}
		else {
			hashsetneeded = false;
			for (int i = 0; i < hashlist.GetSize(); i++){
				if (i < GetPartCount() && IsComplete(i*PARTSIZE, ((i + 1)*PARTSIZE) - 1)){
					SetStatus(PS_READY);
					break;
				}
			}

			// [TPT] - WebCache		
			// Superlexx - IFP
			if (GetStatus() == PS_EMPTY		// no complete chunk, but file ready for downloading
				&& thePrefs.IsWebCacheDownloadEnabled()	// webcached downloading on
				&& GetPartCount() > 1)		// file size > CHUNKSIZE
				SetStatus(PS_READY);	
			// [TPT] - WebCache	
		}		
		
		if (gaplist.IsEmpty()){	// is this file complete already?
			CompleteFile(false);
			return true;
		}

		if (!isnewstyle) // not for importing
		{
			// check date of .part file - if its wrong, rehash file
			CFileStatus filestatus;
			m_hpartfile.GetStatus(filestatus); // this; "...returns m_attribute without high-order flags" indicates a known MFC bug, wonder how many unknown there are... :)
			uint32 fdate = filestatus.m_mtime.GetTime();
			if (fdate == -1){
				if (thePrefs.GetVerbose())
					AddDebugLogLine(false, _T("Failed to convert file date of %s (%s)"), filestatus.m_szFullName, GetFileName());
			}
			else
				AdjustNTFSDaylightFileTime(fdate, filestatus.m_szFullName);
			if (m_tUtcLastModified != fdate){
				CString strFileInfo;
				strFileInfo.Format(_T("%s (%s)"), GetFilePath(), GetFileName());
				LogError(LOG_STATUSBAR, GetResString(IDS_ERR_REHASH), strFileInfo);
				// rehash
				SetStatus(PS_WAITINGFORHASH);
				CAddFileThread* addfilethread = (CAddFileThread*) AfxBeginThread(RUNTIME_CLASS(CAddFileThread), THREAD_PRIORITY_NORMAL,0, CREATE_SUSPENDED);
				if (addfilethread){
					SetFileOp(PFOP_HASHING);
					SetFileOpProgress(0);
					addfilethread->SetValues(0, GetPath(), m_hpartfile.GetFileName(), this);
					addfilethread->ResumeThread();
				}
				else
					SetStatus(PS_ERROR);
		    }
			//[TPT] - Added by SiRoB, SLUGFILLER: SafeHash - update completed, even though unchecked
			else {
				for (int i = 0; i < GetPartCount(); i++)
					if (IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1))
						m_PartsShareable[i] = true;
			}
			//[TPT] END   - Added by SiRoB, SLUGFILLER: SafeHash
		}
	}
	catch(CFileException* error){
		CString strError;
		strError.Format(_T("Failed to initialize part file \"%s\" (%s)"), m_hpartfile.GetFilePath(), GetFileName());
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (error->GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		LogError(LOG_STATUSBAR, _T("%s"), strError);
		error->Delete();
		return false;
	}

	UpdateCompletedInfos();
	return true;
}

bool CPartFile::SavePartFile()
{
	switch (status){
		case PS_WAITINGFORHASH:
		case PS_HASHING:
			return false;
	}

	m_SettingsSaver.SaveSettings(this); // [TPT] - Sivka AutoHL

	// search part file
	CFileFind ff;
	CString searchpath(RemoveFileExtension(m_fullname));
	bool end = !ff.FindFile(searchpath,0);
	if (!end)
		ff.FindNextFile();
	if (end || ff.IsDirectory()){
		LogError(GetResString(IDS_ERR_SAVEMET) + _T(" - %s"), m_partmetfilename, GetFileName(), GetResString(IDS_ERR_PART_FNF));
		return false;
	}

	//get filedate
	CTime lwtime;
	if (!ff.GetLastWriteTime(lwtime)){
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, _T("Failed to get file date of %s (%s) - %s"), m_partmetfilename, GetFileName(), GetErrorMessage(GetLastError()));
	}
	m_tLastModified = lwtime.GetTime();
	m_tUtcLastModified = m_tLastModified;
	if (m_tUtcLastModified == -1){
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, _T("Failed to convert file date of %s (%s)"), m_partmetfilename, GetFileName());
	}
	else
		AdjustNTFSDaylightFileTime(m_tUtcLastModified, ff.GetFilePath());
	ff.Close();

	CString strTmpFile(m_fullname);
	strTmpFile += PARTMET_TMP_EXT;

	// save file data to part.met file
	CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(strTmpFile, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary|CFile::shareDenyWrite, &fexp)){
		CString strError;
		strError.Format(GetResString(IDS_ERR_SAVEMET), m_partmetfilename, GetFileName());
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		LogError(_T("%s"), strError);
		return false;
	}
	setvbuf(file.m_pStream, NULL, _IOFBF, 16384);

	try{
		//version
		file.WriteUInt8(PARTFILE_VERSION);

		//date
		file.WriteUInt32(m_tUtcLastModified);

		//hash
		file.WriteHash16(m_abyFileHash);
		UINT parts = hashlist.GetCount();
		file.WriteUInt16(parts);
		for (UINT x = 0; x < parts; x++)
			file.WriteHash16(hashlist[x]);

		UINT uTagCount = 0;
		ULONG uTagCountFilePos = (ULONG)file.GetPosition();
		file.WriteUInt32(uTagCount);

		if (WriteOptED2KUTF8Tag(&file, GetFileName(), FT_FILENAME))
			uTagCount++;
		CTag nametag(FT_FILENAME, GetFileName());
		nametag.WriteTagToFile(&file);
		uTagCount++;

		CTag sizetag(FT_FILESIZE, GetFileSize()); // [TPT]
		sizetag.WriteTagToFile(&file);
		uTagCount++;

		if (m_uTransferred){
			CTag transtag(FT_TRANSFERRED, m_uTransferred);
			transtag.WriteTagToFile(&file);
			uTagCount++;
		}
		if (m_uCompressionGain){
			CTag transtag(FT_COMPRESSION, m_uCompressionGain);
			transtag.WriteTagToFile(&file);
			uTagCount++;
		}
		if (m_uCorruptionLoss){
			CTag transtag(FT_CORRUPTED, m_uCorruptionLoss);
			transtag.WriteTagToFile(&file);
			uTagCount++;
		}

		if (paused){
			CTag statustag(FT_STATUS, 1);
			statustag.WriteTagToFile(&file);
			uTagCount++;
		}

		CTag prioritytag(FT_DLPRIORITY, IsAutoDownPriority() ? PR_AUTO : m_iDownPriority);
		prioritytag.WriteTagToFile(&file);
		uTagCount++;

		CTag ulprioritytag(FT_ULPRIORITY, IsAutoUpPriority() ? PR_AUTO : GetUpPriority());
		ulprioritytag.WriteTagToFile(&file);
		uTagCount++;

		if (lastseencomplete.GetTime()){
			CTag lsctag(FT_LASTSEENCOMPLETE,lastseencomplete.GetTime());
			lsctag.WriteTagToFile(&file);
			uTagCount++;
		}

		if (m_category){
			//[TPT] - From Morph
			/*
			CTag categorytag(FT_CATEGORY, m_category);
			*/
			CTag categorytag(FT_CATEGORY, (m_category > thePrefs.GetCatCount() - 1)?0:m_category);
			categorytag.WriteTagToFile(&file);
			uTagCount++;
		}

		if (GetLastPublishTimeKadSrc()){
			CTag kadLastPubSrc(FT_KADLASTPUBLISHSRC, GetLastPublishTimeKadSrc());
			kadLastPubSrc.WriteTagToFile(&file);
			uTagCount++;
		}

		if (GetLastPublishTimeKadNotes()){
			CTag kadLastPubNotes(FT_KADLASTPUBLISHNOTES, GetLastPublishTimeKadNotes());
			kadLastPubNotes.WriteTagToFile(&file);
			uTagCount++;
		}

		if (GetDlActiveTime()){
			CTag tagDlActiveTime(FT_DL_ACTIVE_TIME, GetDlActiveTime());
			tagDlActiveTime.WriteTagToFile(&file);
			uTagCount++;
		}

        if (GetPreviewPrio()){
            CTag tagDlPreview(FT_DL_PREVIEW, GetPreviewPrio() ? 1 : 0);
			tagDlPreview.WriteTagToFile(&file);
			uTagCount++;
		}

		// statistics
		if (statistic.GetAllTimeTransferred()){
			CTag attag1(FT_ATTRANSFERRED, (uint32)statistic.GetAllTimeTransferred());
			attag1.WriteTagToFile(&file);
			uTagCount++;
			
			CTag attag4(FT_ATTRANSFERREDHI, (uint32)(statistic.GetAllTimeTransferred() >> 32));
			attag4.WriteTagToFile(&file);
			uTagCount++;
		}

		if (statistic.GetAllTimeRequests()){
			CTag attag2(FT_ATREQUESTED, statistic.GetAllTimeRequests());
			attag2.WriteTagToFile(&file);
			uTagCount++;
		}
		
		if (statistic.GetAllTimeAccepts()){
			CTag attag3(FT_ATACCEPTED, statistic.GetAllTimeAccepts());
			attag3.WriteTagToFile(&file);
			uTagCount++;
		}
	
		// [TPT] - xMule_MOD: showSharePermissions - save permissions
		CTag permtag(FT_PERMISSIONS, GetPermissions());
		permtag.WriteTagToFile(&file);
		uTagCount++;
		// [TPT] - xMule_MOD: showSharePermissions
		
		// [TPT] - khaos::categorymod+
		CTag catresumetag(FT_CATRESUMEORDER, m_catResumeOrder);
		catresumetag.WriteTagToFile(&file);
		uTagCount++;
		// [TPT] - khaos::categorymod-

		// currupt part infos
		POSITION posCorruptedPart = corrupted_list.GetHeadPosition();
		if (posCorruptedPart)
		{
			CString strCorruptedParts;
			while (posCorruptedPart)
			{
				uint16 uCorruptedPart = corrupted_list.GetNext(posCorruptedPart);
				if (!strCorruptedParts.IsEmpty())
					strCorruptedParts += _T(",");
				strCorruptedParts.AppendFormat(_T("%u"), (UINT)uCorruptedPart);
			}
			ASSERT( !strCorruptedParts.IsEmpty() );
			CTag tagCorruptedParts(FT_CORRUPTEDPARTS, strCorruptedParts);
			tagCorruptedParts.WriteTagToFile(&file);
			uTagCount++;
		}
	
		//AICH Filehash
		if (m_pAICHHashSet->HasValidMasterHash() && (m_pAICHHashSet->GetStatus() == AICH_VERIFIED)){
			CTag aichtag(FT_AICH_HASH, m_pAICHHashSet->GetMasterHash().GetString() );
			aichtag.WriteTagToFile(&file);
			uTagCount++;
		}


		// [TPT] - Powershare
		if (GetHideOS()>=0){
			CTag hideostag(FT_HIDEOS, GetHideOS());
			hideostag.WriteTagToFile(&file);
			uTagCount++;
		}
		if (GetSelectiveChunk()>=0){
			CTag selectivechunktag(FT_SELECTIVE_CHUNK, GetSelectiveChunk());
			selectivechunktag.WriteTagToFile(&file);
			uTagCount++;
		}
		if (GetPowerSharedMode()>=0){
			CTag powersharetag(FT_POWERSHARE, GetPowerSharedMode());
			powersharetag.WriteTagToFile(&file);
			uTagCount++;
		}
		if (GetPowerShareLimit()>=0){
			CTag powersharelimittag(FT_POWERSHARE_LIMIT, GetPowerShareLimit());
			powersharelimittag.WriteTagToFile(&file);
			uTagCount++;
		}


		for (int j = 0; j < taglist.GetCount(); j++){
			if (taglist[j]->IsStr() || taglist[j]->IsInt()){
				taglist[j]->WriteTagToFile(&file);
				uTagCount++;
			}
		}

		//gaps
		char namebuffer[10];
		char* number = &namebuffer[1];
		uint16 i_pos = 0;
		for (POSITION pos = gaplist.GetHeadPosition();pos != 0;)
		{
			Gap_Struct* gap = gaplist.GetNext(pos);
			itoa(i_pos,number,10);
			namebuffer[0] = FT_GAPSTART;
			CTag gapstarttag(namebuffer,gap->start);
			gapstarttag.WriteTagToFile(&file);
			uTagCount++;

			// gap start = first missing byte but gap ends = first non-missing byte in edonkey
			// but I think its easier to user the real limits
			namebuffer[0] = FT_GAPEND;
			CTag gapendtag(namebuffer,gap->end+1);
			gapendtag.WriteTagToFile(&file);
			uTagCount++;
			
			i_pos++;
		}

		// [TPT] - Powershare
		i_pos = 0;
		for (POSITION pos = statistic.spreadlist.GetHeadPosition(); pos; ){
			uint32 count = statistic.spreadlist.GetValueAt(pos);
			if (!count) {
				statistic.spreadlist.GetNext(pos);
				continue;
			}
			uint32 start = statistic.spreadlist.GetKeyAt(pos);
			statistic.spreadlist.GetNext(pos);
			ASSERT(pos != NULL);	// Last value should always be 0
			uint32 end = statistic.spreadlist.GetKeyAt(pos);
			itoa(i_pos,number,10);
			namebuffer[0] = FT_SPREADSTART;
			CTag(namebuffer,start).WriteTagToFile(&file);
			namebuffer[0] = FT_SPREADEND;
			CTag(namebuffer,end).WriteTagToFile(&file);
			namebuffer[0] = FT_SPREADCOUNT;
			CTag(namebuffer,count).WriteTagToFile(&file);
			uTagCount+=3;
			i_pos++;
		}
		// [TPT] - Powershare END


		file.Seek(uTagCountFilePos, CFile::begin);
		file.WriteUInt32(uTagCount);
		file.SeekToEnd();

		if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
			file.Flush(); // flush file stream buffers to disk buffers
			if (_commit(_fileno(file.m_pStream)) != 0) // commit disk buffers to disk
				AfxThrowFileException(CFileException::hardIO, GetLastError(), file.GetFileName());
		}
		file.Close();
	}
	catch(CFileException* error){
		CString strError;
		strError.Format(GetResString(IDS_ERR_SAVEMET), m_partmetfilename, GetFileName());
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (error->GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		LogError(_T("%s"), strError);
		error->Delete();

		// remove the partially written or otherwise damaged temporary file
		file.Abort(); // need to close the file before removing it. call 'Abort' instead of 'Close', just to avoid an ASSERT.
		(void)_tremove(strTmpFile);
		return false;
	}

	// after successfully writing the temporary part.met file...
	if (_tremove(m_fullname) != 0 && errno != ENOENT){
		if (thePrefs.GetVerbose())
			DebugLogError(_T("Failed to remove \"%s\" - %s"), m_fullname, _tcserror(errno));
	}

	if (_trename(strTmpFile, m_fullname) != 0){
		int iErrno = errno;
		if (thePrefs.GetVerbose())
			DebugLogError(_T("Failed to move temporary part.met file \"%s\" to \"%s\" - %s"), strTmpFile, m_fullname, _tcserror(iErrno));

		CString strError;
		strError.Format(GetResString(IDS_ERR_SAVEMET), m_partmetfilename, GetFileName());
		strError += _T(" - ");
		strError += _tcserror(iErrno);
		LogError(_T("%s"), strError);
		return false;
	}

	// create a backup of the successfully written part.met file
	CString BAKName(m_fullname);
	BAKName.Append(PARTMET_BAK_EXT);
	if (!::CopyFile(m_fullname, BAKName, FALSE)){
		if (thePrefs.GetVerbose())
			DebugLogError(_T("Failed to create backup of %s (%s) - %s"), m_fullname, GetFileName(), GetErrorMessage(GetLastError()));
	}

	return true;
}

void CPartFile::PartFileHashFinished(CKnownFile* result){
	newdate = true;
	bool errorfound = false;
	if (GetED2KPartHashCount()==0 || GetHashCount()==0){
		if (IsComplete(0, GetFileSize()-1)){
			if (md4cmp(result->GetFileHash(), GetFileHash())){
				LogWarning(GetResString(IDS_ERR_FOUNDCORRUPTION), 1, GetFileName());
				AddGap(0, GetFileSize()-1);
				errorfound = true;
			}
			else{
				if (GetED2KPartHashCount() != GetHashCount()){
					ASSERT( result->GetED2KPartHashCount() == GetED2KPartHashCount() );
					if (SetHashset(result->GetHashset()))
						hashsetneeded = false;
				}
			}
		}
	}
	else{
		for (uint32 i = 0; i < (uint32)hashlist.GetSize(); i++){
			if (i < GetPartCount() && IsComplete(i*PARTSIZE, (i + 1)*PARTSIZE - 1)){
				if (!(result->GetPartHash(i) && !md4cmp(result->GetPartHash(i), GetPartHash(i)))){
					LogWarning(GetResString(IDS_ERR_FOUNDCORRUPTION), i+1, GetFileName());
					AddGap(i*PARTSIZE, (((i + 1)*PARTSIZE - 1) >= GetFileSize()) ? GetFileSize()-1 : (i + 1)*PARTSIZE - 1);
					errorfound = true;
				}
			}
		}
	}
	if (!errorfound && result->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE && status == PS_COMPLETING){
		delete m_pAICHHashSet;
		m_pAICHHashSet = result->GetAICHHashset();
		result->SetAICHHashset(NULL);
		m_pAICHHashSet->SetOwner(this);
	}
	else if (status == PS_COMPLETING){
		// [TPT] - WebCache
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
			AddDebugLogLine(false, _T("Failed to store new AICH Hashset for completed file %s"), GetFileName());
	}

	delete result;
	if (!errorfound){
		if (status == PS_COMPLETING){
			if (thePrefs.GetVerbose())
				AddDebugLogLine(true, _T("Completed file-hashing for \"%s\""), GetFileName());
			if (theApp.sharedfiles->GetFileByID(GetFileHash()) == NULL)
				theApp.sharedfiles->SafeAddKFile(this);
			CompleteFile(true);
			return;
		}
		else
			AddLogLine(false, GetResString(IDS_HASHINGDONE), GetFileName());
	}
	else{
		SetStatus(PS_READY);
		if (thePrefs.GetVerbose())
			DebugLogError(LOG_STATUSBAR, _T("File-hashing failed for \"%s\""), GetFileName());
		SavePartFile();
		return;
	}
	if (thePrefs.GetVerbose())
		AddDebugLogLine(true, _T("Completed file-hashing for \"%s\""), GetFileName());
	SetStatus(PS_READY);
	SavePartFile();
	theApp.sharedfiles->SafeAddKFile(this);
}

// [TPT]
// Maella -Code Improvement-
void CPartFile::AddGap(uint32 start, uint32 end)
{
	ASSERT( start <= end );

	for (POSITION pos = gaplist.GetHeadPosition(); pos != NULL;){
		POSITION cur_pos = pos;
		Gap_Struct* cur_gap = gaplist.GetNext(pos);
		if (cur_gap->start >= start && cur_gap->end <= end){ // this gap is inside the new gap - delete
			gaplist.RemoveAt(cur_pos);
			delete cur_gap;
		}
		else if (cur_gap->start >= start && cur_gap->start <= end){// a part of this gap is in the new gap - extend limit and delete
			end = cur_gap->end;
			gaplist.RemoveAt(cur_pos);
			delete cur_gap;
		}
		else if (cur_gap->end <= end && cur_gap->end >= start){// a part of this gap is in the new gap - extend limit and delete
			start = cur_gap->start;
			gaplist.RemoveAt(cur_pos);
			delete cur_gap;
		}
		else if (start >= cur_gap->start && end <= cur_gap->end){// new gap is already inside this gap - return
			return;
		}
	}
	Gap_Struct* new_gap = new Gap_Struct;
	new_gap->start = start;
	new_gap->end = end;
	gaplist.AddTail(new_gap);
	UpdateDisplayedInfo();
	newdate = true;
}
// Maella end

bool CPartFile::IsComplete(uint32 start, uint32 end) const
{
	ASSERT( start <= end );

	// [TPT]
	if (end >= GetFileSize())
		end = GetFileSize()-1;
	for (POSITION pos = gaplist.GetHeadPosition();pos != 0;)
	{
		const Gap_Struct* cur_gap = gaplist.GetNext(pos);
		if (   (cur_gap->start >= start          && cur_gap->end   <= end)
			|| (cur_gap->start >= start          && cur_gap->start <= end)
			|| (cur_gap->end   <= end            && cur_gap->end   >= start)
			|| (start          >= cur_gap->start && end            <= cur_gap->end)
		   )
		{
			return false;	
		}
	}
	return true;
}

bool CPartFile::IsPureGap(uint32 start, uint32 end) const
{
	ASSERT( start <= end );

	// [TPT]
	if (end >= GetFileSize())
		end = GetFileSize()-1;
	for (POSITION pos = gaplist.GetHeadPosition();pos != 0;){
		const Gap_Struct* cur_gap = gaplist.GetNext(pos);
		if (start >= cur_gap->start  && end <= cur_gap->end ){
			return true;
		}
	}
	return false;
}

bool CPartFile::IsAlreadyRequested(uint32 start, uint32 end) const
{
	ASSERT( start <= end );

	for (POSITION pos =  requestedblocks_list.GetHeadPosition();pos != 0; ){
		const Requested_Block_Struct* cur_block = requestedblocks_list.GetNext(pos);
		if ((start <= cur_block->EndOffset) && (end >= cur_block->StartOffset))
			return true;
	}
	return false;
}

uint32 CPartFile::GetTotalGapSizeInRange(uint32 uRangeStart, uint32 uRangeEnd) const
{
	ASSERT( uRangeStart <= uRangeEnd );

	uint32 uTotalGapSize = 0;

	// [TPT]
	if (uRangeEnd >= GetFileSize())
		uRangeEnd = GetFileSize() - 1;

	POSITION pos = gaplist.GetHeadPosition();
	while (pos)
	{
		const Gap_Struct* pGap = gaplist.GetNext(pos);

		if (pGap->start < uRangeStart && pGap->end > uRangeEnd)
		{
			uTotalGapSize += uRangeEnd - uRangeStart + 1;
			break;
		}

		if (pGap->start >= uRangeStart && pGap->start <= uRangeEnd)
		{
			uint32 uEnd = (pGap->end > uRangeEnd) ? uRangeEnd : pGap->end;
			uTotalGapSize += uEnd - pGap->start + 1;
		}
		else if (pGap->end >= uRangeStart && pGap->end <= uRangeEnd)
		{
			uTotalGapSize += pGap->end - uRangeStart + 1;
		}
	}

	ASSERT( uTotalGapSize <= uRangeEnd - uRangeStart + 1 );

	return uTotalGapSize;
}

uint32 CPartFile::GetTotalGapSizeInPart(UINT uPart) const
{
	uint32 uRangeStart = uPart * PARTSIZE;
	uint32 uRangeEnd = uRangeStart + PARTSIZE - 1;
	// [TPT]
	if (uRangeEnd >= GetFileSize())
		uRangeEnd = GetFileSize();
	return GetTotalGapSizeInRange(uRangeStart, uRangeEnd);
}

bool CPartFile::GetNextEmptyBlockInPart(uint16 partNumber, Requested_Block_Struct *result) const
{
	Gap_Struct *firstGap;
	Gap_Struct *currentGap;
	uint32 end;
	uint32 blockLimit;

	// Find start of this part
	uint32 partStart = (PARTSIZE * partNumber);
	uint32 start = partStart;

	// What is the end limit of this block, i.e. can't go outside part (or filesize)
	uint32 partEnd = (PARTSIZE * (partNumber + 1)) - 1;
	if (partEnd >= GetFileSize())
		partEnd = GetFileSize() - 1;
	ASSERT( partStart <= partEnd );

	// Loop until find a suitable gap and return true, or no more gaps and return false
	for (;;)
	{
		firstGap = NULL;

		// Find the first gap from the start position
		for (POSITION pos = gaplist.GetHeadPosition(); pos != 0; )
		{
			currentGap = gaplist.GetNext(pos);
			// Want gaps that overlap start<->partEnd
			if ((currentGap->start <= partEnd) && (currentGap->end >= start))
			{
				// Is this the first gap?
				if ((firstGap == NULL) || (currentGap->start < firstGap->start))
					firstGap = currentGap;
			}
		}

		// If no gaps after start, exit
		if (firstGap == NULL)
			return false;

		// Update start position if gap starts after current pos
		if (start < firstGap->start)
			start = firstGap->start;

		// If this is not within part, exit
		if (start > partEnd)
			return false;

		// Find end, keeping within the max block size and the part limit
		end = firstGap->end;
		blockLimit = partStart + (EMBLOCKSIZE * (((start - partStart) / EMBLOCKSIZE) + 1)) - 1;
		if (end > blockLimit)
			end = blockLimit;
		if (end > partEnd)
			end = partEnd;
    
		// If this gap has not already been requested, we have found a valid entry
		if (!IsAlreadyRequested(start, end))
		{
			// Was this block to be returned
			if (result != NULL)
			{
				result->StartOffset = start;
				result->EndOffset = end;
				md4cpy(result->FileID, GetFileHash());
				result->transferred = 0;
			}
			return true;
		}
		else
		{
			// Reposition to end of that gap
			start = end + 1;
		}

		// If tried all gaps then break out of the loop
		if (end == partEnd)
			break;
	}

	// No suitable gap found
	return false;
}

// [TPT] - Maella -Code Improvement-
void CPartFile::FillGap(uint32 start, uint32 end)
{
	ASSERT( start <= end );

	for (POSITION pos = gaplist.GetHeadPosition(); pos != NULL;){
		POSITION cur_pos = pos;
		Gap_Struct* cur_gap = gaplist.GetNext(pos);
		if (cur_gap->start >= start && cur_gap->end <= end){ // our part fills this gap completly
			gaplist.RemoveAt(cur_pos);
			delete cur_gap;
		}
		else if (cur_gap->start >= start && cur_gap->start <= end){// a part of this gap is in the part - set limit
			cur_gap->start = end+1;
		}
		else if (cur_gap->end <= end && cur_gap->end >= start){// a part of this gap is in the part - set limit
			cur_gap->end = start-1;
		}
		else if (start >= cur_gap->start && end <= cur_gap->end){
			uint32 buffer = cur_gap->end;
			cur_gap->end = start-1;
			cur_gap = new Gap_Struct;
			cur_gap->start = end+1;
			cur_gap->end = buffer;
			gaplist.InsertAfter(pos, cur_gap);
			break; // [Lord KiRon]
		}
	}

	UpdateCompletedInfos();
	UpdateDisplayedInfo();
	newdate = true;
}
// Maella end

void CPartFile::UpdateCompletedInfos()
{
   	uint32 allgaps = 0; 

	for (POSITION pos = gaplist.GetHeadPosition();pos !=  0;){ 
		const Gap_Struct* cur_gap = gaplist.GetNext(pos);
		allgaps += cur_gap->end - cur_gap->start + 1;
	}

	UpdateCompletedInfos(allgaps);
}

void CPartFile::UpdateCompletedInfos(uint32 uTotalGaps)
{
	// [TPT]
	if (uTotalGaps > GetFileSize()){
		ASSERT(0);
		uTotalGaps = GetFileSize();
	}

	if (gaplist.GetCount() || requestedblocks_list.GetCount()){ 
		// 'percentcompleted' is only used in GUI, round down to avoid showing "100%" in case 
		// we actually have only "99.9%"
		percentcompleted = floor((1.0 - (double)uTotalGaps/GetFileSize()) * 1000.0) / 10.0;
		completedsize = GetFileSize() - uTotalGaps;
	} 
	else{
		percentcompleted = 100.0F;
		completedsize = GetFileSize();
	}
}

// [TPT] - Maella -Code Improvement-
void CPartFile::DrawShareStatusBar(CDC* dc, LPCRECT rect, bool onlygreyrect, bool bFlat) const
{
	if( !IsPartFile() )
	{
		CKnownFile::DrawShareStatusBar( dc, rect, onlygreyrect, bFlat );
		return;
	}

	const COLORREF crMissing = RGB(255, 0, 0);
	CBarShader chunkBar(rect->bottom - rect->top, rect->right - rect->left);
	chunkBar.SetFileSize(this->GetFileSize());	
	chunkBar.Fill(crMissing); 

	if (!onlygreyrect && !m_SrcpartFrequency.IsEmpty()){
		const COLORREF crProgress = (bFlat) ? RGB(0, 150, 0) : RGB(0, 224, 0);
		const COLORREF crHave     = (bFlat) ? RGB(0, 0, 0) : RGB(104, 104, 104);
		const COLORREF crPending  = (bFlat) ? RGB(255,208,0) : RGB(255, 208, 0);
		const uint16 partCount = GetPartCount();
		for (uint16 i=0; i != partCount; i++)
		{
			if(m_SrcpartFrequency[i] > 0 )
			{
				COLORREF color = RGB(0, (210-(22*(m_SrcpartFrequency[i]-1)) < 0) ? 0 : 210-(22*(m_SrcpartFrequency[i]-1)), 255);
				chunkBar.FillRange(PARTSIZE*(i),PARTSIZE*(i+1),color);
			}
		}
	}
   	chunkBar.Draw(dc, rect->left, rect->top, bFlat); 
} 
// Maella end

// Maella -Code Improvement-
void CPartFile::DrawStatusBar(CDC* dc, LPCRECT rect, bool bFlat) /*const*/
{
	EPartFileStatus eVirtualState = GetStatus();
	bool notgray = eVirtualState == PS_EMPTY || eVirtualState == PS_READY; // SLUGFILLER: grayPause - only test once

	// PreSet colors
	// SLUGFILLER: grayPause - Colors by status
	const COLORREF crProgress = (bFlat) ? RGB(0, 150, 0) : RGB(0, 224, 0);
	const COLORREF crHave     = (notgray) ? ((bFlat) ? RGB(0, 0, 0) : RGB(104, 104, 104)) : ((bFlat) ? RGB(64, 64, 64) : RGB(116, 116, 116));
	const COLORREF crPending  = (notgray) ? RGB(255,208,0) : RGB(191, 168, 64);
	const COLORREF crMissing  = (notgray) ? RGB(255, 0, 0) : RGB(191, 64, 64);	
	// SLUGFILLER: grayPause

	// Set Size
	CBarShader chunkBar(rect->bottom - rect->top, rect->right - rect->left);
	chunkBar.SetFileSize(GetFileSize());	
	chunkBar.Fill(crHave);

	if (status == PS_COMPLETE || status == PS_COMPLETING)
	{
		chunkBar.FillRange(0, GetFileSize(), crProgress);
		chunkBar.Draw(dc, rect->left, rect->top, bFlat);
		percentcompleted = 100.0F;
		completedsize = GetFileSize();
	}
	else if (theApp.m_brushBackwardDiagonal.m_hObject && eVirtualState == PS_INSUFFICIENT || status == PS_ERROR)
	{
		int iOldBkColor = dc->SetBkColor(RGB(255, 255, 0));
		dc->FillRect(rect, &theApp.m_brushBackwardDiagonal);
		dc->SetBkColor(iOldBkColor);

		UpdateCompletedInfos();
	}
	else
	{
	    // red gaps
	    uint32 allgaps = 0;
	    for (POSITION pos = gaplist.GetHeadPosition();pos !=  0;){
		    const Gap_Struct* cur_gap = gaplist.GetNext(pos);
		    allgaps += cur_gap->end - cur_gap->start + 1;
		    bool gapdone = false;
			const uint32 partCount = (uint32)GetPartCount();
		    uint32 gapstart = cur_gap->start;
		    uint32 gapend = cur_gap->end;
			for(uint32 i = 0; i < partCount && gapdone == false; i++){
				if (gapstart >= i*PARTSIZE && gapstart <= ((i+1)*PARTSIZE-1)){ // is in this part?
					if (gapend <= ((i+1)*PARTSIZE)-1)
						gapdone = true;
				    else
						gapend = ((i+1)*PARTSIZE-1); // and next part
    
				    // paint
				    COLORREF color;
					if (m_SrcpartFrequency.GetCount() >= (INT_PTR)i && m_SrcpartFrequency[(uint16)i])  // frequency?
				    // SLUGFILLER: grayPause
				    {
					    if(notgray)
					        color = RGB(0,
								        (210-(22*(m_SrcpartFrequency[(uint16)i]-1)) <  0)? 0:210-(22*(m_SrcpartFrequency[(uint16)i]-1))
								        ,255);
				        else
						    color = RGB(64,
									    (169-(11*(m_SrcpartFrequency[(uint16)i]-1)) <  64)? 64:169-(11*(m_SrcpartFrequency[(uint16)i]-1))
									    ,191);
				    }
				    // SLUGFILLER: grayPause
				    else
					    color = crMissing;
    
					chunkBar.FillRange(gapstart, gapend,  color);
    
					// Next Gap
					gapstart = gapend+1;
					gapend = cur_gap->end;
				}
			}
		}
    
	    // yellow pending parts
	    for (POSITION pos = requestedblocks_list.GetHeadPosition();pos !=  0;){
		    const Requested_Block_Struct* block = requestedblocks_list.GetNext(pos);
		    chunkBar.FillRange(block->StartOffset + block->transferred, block->EndOffset + 1, crPending);
	    }
    
	    chunkBar.Draw(dc, rect->left, rect->top, bFlat);
    
		// Draw green process (in percent)
		if(gaplist.GetCount() || requestedblocks_list.GetCount()){
			completedsize = GetFileSize() - allgaps;
			percentcompleted = 100.0f * ((float)completedsize / (float)GetFileSize());
		}
		else{
			completedsize = GetFileSize();
			percentcompleted = 100.0f;		
		}

	    RECT gaprect;
	    gaprect.top = rect->top;
	    gaprect.bottom = gaprect.top + PROGRESS_HEIGHT; // Barry - was 4
	    gaprect.left = rect->left;
    
	    if(!bFlat) {
			CBarShader percentBar(PROGRESS_HEIGHT);
			percentBar.SetWidth((uint32)(percentcompleted * (float)(rect->right - rect->left) / 100.0f));
			percentBar.SetFileSize(completedsize); // [TPT] - Patch
			percentBar.Fill(crProgress);
			percentBar.Draw(dc, gaprect.left, gaprect.top, false);
		} 
		else {
		    //draw gray progress only if flat
			gaprect.right = rect->left + (uint32)(percentcompleted * (float)(rect->right - rect->left) / 100.0f);
			dc->FillRect(&gaprect, &CBrush(crProgress));
		    gaprect.left = gaprect.right;
		    gaprect.right = rect->right;
		    dc->FillRect(&gaprect, &CBrush(RGB(224,224,224)));
	    }
    
	    UpdateCompletedInfos(allgaps);
    }

	// additionally show any file op progress (needed for PS_COMPLETING and PS_WAITINGFORHASH)
	if (GetFileOp() != PFOP_NONE)
	{
		float blockpixel = (float)(rect->right - rect->left)/100.0F;
		CRect rcFileOpProgress;
		rcFileOpProgress.top = rect->top;
		rcFileOpProgress.bottom = rcFileOpProgress.top + PROGRESS_HEIGHT;
		rcFileOpProgress.left = rect->left;
		if (!bFlat)
		{
			CBarShader percentBar(PROGRESS_HEIGHT);
			percentBar.SetWidth(GetFileOpProgress()*blockpixel + 0.5F);
			percentBar.Fill(RGB(255,208,0));
			percentBar.Draw(dc, rcFileOpProgress.left, rcFileOpProgress.top, false);
		}
		else
		{
			rcFileOpProgress.right = rcFileOpProgress.left + (UINT)(GetFileOpProgress()*blockpixel + 0.5F);
			dc->FillRect(&rcFileOpProgress, &CBrush(RGB(255,208,0)));
			rcFileOpProgress.left = rcFileOpProgress.right;
			rcFileOpProgress.right = rect->right;
			dc->FillRect(&rcFileOpProgress, &CBrush(RGB(224,224,224)));
		}
	}
}
// Maella end

void CPartFile::WritePartStatus(CSafeMemFile* file, CUpDownClient* client) {	// [TPT] - SLUGFILLER: hideOS
	// [TPT] - SLUGFILLER: hideOS
	CArray<uint32> partspread;
	UINT uED2KPartCount;
	uint8 hideOS = GetHideOS() >= 0 ? GetHideOS() : thePrefs.GetHideOvershares();// [TPT] - Powershare
	
	if (client && client->SupportsWebCache() && client->IsBehindOurWebCache()) // [TPT] - WebCache. 
		hideOS = 0;
	if (hideOS && client) {
		uED2KPartCount = CalcPartSpread(partspread, client);
	} else {	// simpler to set as 0 than to create another loop...
		uED2KPartCount = GetED2KPartCount();
		partspread.SetSize(uED2KPartCount);
		for (UINT i = 0; i < uED2KPartCount; i++)
			partspread[i] = 0;
		hideOS = 1;
	}
	// [TPT] - Powershare - Added by SiRoB, See chunk that we hide by HideOS feature
	if (hideOS && client){
		if (client->m_abyUpPartStatusHidden){
			delete[] client->m_abyUpPartStatusHidden;
			client->m_abyUpPartStatusHidden = NULL;
		}
		client->m_bUpPartStatusHiddenBySOTN = false;
		client->m_abyUpPartStatusHidden = new uint8[uED2KPartCount];
		MEMZERO(client->m_abyUpPartStatusHidden, uED2KPartCount);
	}
	// [TPT] - Powershare - Added by SiRoB, See chunk that we hide by HideOS feature
	file->WriteUInt16(uED2KPartCount);
	// [TPT] - SLUGFILLER: hideOS
	UINT uPart = 0;
	while (uPart != uED2KPartCount){
		uint8 towrite = 0;
		for (UINT i = 0; i < 8; i++){
			// [TPT] - Powershare - Added by SiRoB, See chunk that we hide
			if (partspread[uPart] < hideOS)	// [TPT] - SLUGFILLER: hideOS
			{
				if (uPart < GetPartCount() && IsPartShareable(uPart))
					towrite |= (1<<i);
			} else
				if (hideOS && client)
					client->m_abyUpPartStatusHidden[uPart] = 1;
			// [TPT] - Powershare - Added by SiRoB, See chunk that we hide
			++uPart;
			if (uPart == uED2KPartCount)
				break;
		}
		file->WriteUInt8(towrite);
	}
}

// [TPT] - enkeyDev: ICS
void CPartFile::WriteIncPartStatus(CSafeMemFile* file){
	UINT uED2KPartCount = GetED2KPartCount();
	file->WriteUInt16(uED2KPartCount);
	UINT uPart = 0;
	while (uPart != uED2KPartCount){
		uint8 towrite = 0;
		for (UINT i = 0;i < 8;i++){
			if (uPart < GetPartCount() && !IsPureGap(uPart*PARTSIZE, (uPart + 1)*PARTSIZE - 1))
				towrite |= (1<<i);
			++uPart;
			if (uPart == uED2KPartCount)
				break;
		}
		file->WriteUInt8(towrite);
	}
}

// [TPT] - enkeyDev: ICS

void CPartFile::WriteCompleteSourcesCount(CSafeMemFile* file) const
{
	file->WriteUInt16(m_nCompleteSourcesCount);
}

int CPartFile::GetValidSourcesCount() const
{
	int counter = 0;
	for (POSITION pos = srclist.GetHeadPosition(); pos != NULL;){
		EDownloadState nDLState = srclist.GetNext(pos)->GetDownloadState();
		if (nDLState==DS_ONQUEUE || nDLState==DS_DOWNLOADING || nDLState==DS_CONNECTED || nDLState==DS_REMOTEQUEUEFULL)
			++counter;
	}
	return counter;
}

uint16 CPartFile::GetNotCurrentSourcesCount() const
{
	UINT counter = 0;
	for (POSITION pos = srclist.GetHeadPosition(); pos != NULL;){
		EDownloadState nDLState = srclist.GetNext(pos)->GetDownloadState();
		if (nDLState!=DS_ONQUEUE && nDLState!=DS_DOWNLOADING)
			counter++;
	}
	return counter;
}

// SLUGFILLER: checkDiskspace
uint32 CPartFile::GetNeededSpace() const
{
	if (m_hpartfile.GetLength() > GetFileSize())
		return 0;	// Shouldn't happen, but just in case
	return GetFileSize()-m_hpartfile.GetLength();
}
// SLUGFILLER: checkDiskspace

EPartFileStatus CPartFile::GetStatus(bool ignorepause) const
{
	// SLUGFILLER: checkDiskspace
	if ((!paused && !insufficient) || status == PS_ERROR || status == PS_COMPLETING || status == PS_COMPLETE || ignorepause)
		return status;
	else if (paused)
		return PS_PAUSED;
	else
		return PS_INSUFFICIENT;
	// SLUGFILLER: checkDiskspace
}
// Maella -New bandwidth control-
void CPartFile::AddDownloadingSource(CUpDownClient* client){
	// We need to detect a change in the source list
	// It's necessary for the download 'balancing' (see ::Process)
	m_sourceListChange = true;

	POSITION pos = m_downloadingSourceList.Find(client); // to be sure
	if(pos == NULL){
		m_downloadingSourceList.AddTail(client);
	}
}
// Maella end

// Maella -New bandwidth control-
void CPartFile::RemoveDownloadingSource(CUpDownClient* client){
	// We need to detect a change in the source list
	// It's necessary for the download 'balancing' (see ::Process)
	m_sourceListChange = true;

	POSITION pos = m_downloadingSourceList.Find(client); // to be sure
	if(pos != NULL){
		m_downloadingSourceList.RemoveAt(pos);
	}
}
// Maella end

// Maella -New bandwidth control-
uint32 CPartFile::Process(uint32 maxammount, bool isLimited, bool fullProcess)
{
	if (thePrefs.m_iDbgHeap >= 2)
		ASSERT_VALID(this);

	const uint16	nOldTransSourceCount  = GetSrcStatisticsValue(DS_DOWNLOADING) ; 
	DWORD dwCurTick = ::GetTickCount();

	// If buffer size exceeds limit, or if not written within time limit, flush data
	if ((m_nTotalBufferData > thePrefs.GetFileBufferSize()) || (dwCurTick > (m_nLastBufferFlushTime + BUFFER_TIME_LIMIT)))
	{
		// Avoid flushing while copying preview file
		if (!m_bPreviewing)
			FlushBuffer();
	}

	if(fullProcess == false)
	{
		uint32 receivedBlockTotal = 0;
		// Remark: A client could be removed from the list during the 
		//         processing or the entries swapped.
		// [TPT] - Xman set sourcelistchange here:
		// Check if the list has been modified during the processing
		//m_sourceListChange = false;

		POSITION pos = m_downloadingSourceList.GetHeadPosition();
		for(int i = 0; i < m_downloadingSourceList.GetCount(); i++)
		{
			POSITION cur_pos = pos;
			CUpDownClient* cur_src = m_downloadingSourceList.GetNext(pos);
			if (thePrefs.m_iDbgHeap >= 2)
				ASSERT_VALID( cur_src );
			// Check if the list has been modified during the processing
			m_sourceListChange = false; // [TPT] - Xman moved up

			if(cur_src && cur_src->GetDownloadState() != DS_DOWNLOADING)
			{
				ASSERT(FALSE); // Should never happend
				RemoveDownloadingSource(cur_src); // => security
			}
			else
			{
				cur_src->CheckDownloadTimeout();
				if(isLimited == false)
				{
					// Always call this method to avoid a flag (enabled/disable)                  
					if (cur_src->socket) 
					{
						cur_src->socket->DisableDownloadLimit();
						if (cur_src->IsDownloadingFromPeerCache() && cur_src->m_pPCDownSocket && cur_src->m_pPCDownSocket->IsConnected())
							cur_src->m_pPCDownSocket->DisableDownloadLimit();
					}
					// [TPT] - WebCache
					if (cur_src->IsProxy() && cur_src->IsDownloadingFromWebCache() &&
						cur_src->m_pWCDownSocket && cur_src->m_pWCDownSocket->IsConnected())
						cur_src->m_pWCDownSocket->DisableDownloadLimit();					
					// [TPT] - WebCache
				}
                else 
				{
					if(maxammount > 6) // let room for header size
					{
						// Maella -Overhead compensation (pseudo full upload rate control)-
						uint64 receivedBlock;
						//Xman avoid the silly window syndrome
						/*
						The fast sender will quickly fill the receiver's TCP window.
						The receiver then reads N bytes, N being a relatively small number
						compared to the network frame size. A na�ve stack will immediately send
						an ACK to the sender to tell it that there are now N bytes available in
						its TCP window. This will cause the sender to send N bytes of data; 
						since N is smaller than the frame size, there's relatively more protocol
						overhead in the packet compared to a full frame. Because the receiver is
						slow, the TCP window stays very small, and thus hurts throughput because
						the ratio of protocol overhead to application data goes up.
						*/
						uint32 tempmaxamount;
						if(maxammount % MAXFRAGSIZE == 0) {
							tempmaxamount=maxammount;
						} else {
							tempmaxamount= MAXFRAGSIZE*(maxammount/MAXFRAGSIZE+1);
						}

						// Use the global statistic to measure the ammount of data (cleaner than a call back)
						receivedBlock = theApp.pBandWidthControl->GeteMuleIn();													
						
						if (cur_src->socket)
						{
							cur_src->socket->SetDownloadLimit(tempmaxamount); // Trig OnReceive() (go-n-stop mode)
							if (cur_src->IsDownloadingFromPeerCache() && cur_src->m_pPCDownSocket && cur_src->m_pPCDownSocket->IsConnected())
								cur_src->m_pPCDownSocket->SetDownloadLimit(tempmaxamount);
						}
						// [TPT] - WebCache
						if (cur_src->IsProxy() && cur_src->IsDownloadingFromWebCache() &&
							cur_src->m_pWCDownSocket && cur_src->m_pWCDownSocket->IsConnected())
							cur_src->m_pWCDownSocket->SetDownloadLimit(tempmaxamount);
						// [TPT] - WebCache

						receivedBlock = theApp.pBandWidthControl->GeteMuleIn() - receivedBlock;		
						//Xman end: avoid the silly window syndrome
						// Maella end
					        if(receivedBlock > 0){
						        if(maxammount > (uint32)receivedBlock){
								receivedBlockTotal += (uint32)receivedBlock;
								maxammount -= (uint32)receivedBlock;
							} else {
								// Necessary because IP + TCP overhead
								receivedBlockTotal += (uint32)receivedBlock;
								maxammount = 0;
							}

							// Try to 'balance' the download between clients.
							// Move the 'downloader' at the end of the list.
							if(m_sourceListChange == false && cur_src->GetDownloadState() == DS_DOWNLOADING){
								m_downloadingSourceList.RemoveAt(cur_pos);
								m_downloadingSourceList.AddTail(cur_src);
							}
						}
					}
					// In case of an exception, the instance of the client might have been deleted
					if(m_sourceListChange == false &&
						(cur_src->socket != NULL || cur_src->IsProxy()) && // [TPT] - WebCache							
						cur_src->GetDownloadState() == DS_DOWNLOADING)
					{													
						// Block OnReceive() (go-n-stop mode)	
						if (cur_src->socket)
						{
							cur_src->socket->SetDownloadLimit(0);
							if (cur_src->IsDownloadingFromPeerCache() && cur_src->m_pPCDownSocket && cur_src->m_pPCDownSocket->IsConnected())
									cur_src->m_pPCDownSocket->SetDownloadLimit(0);
						}
						// [TPT] - WebCache
						if (cur_src->IsProxy() && cur_src->IsDownloadingFromWebCache() &&
							cur_src->m_pWCDownSocket && cur_src->m_pWCDownSocket->IsConnected())
								cur_src->m_pWCDownSocket->SetDownloadLimit(0);							
						// [TPT] - WebCache						
					}
				}
			}
		}

		return receivedBlockTotal; // exit here
	}

	// Maella -New Save/load Sources- enkeyDEV(Ottavio84)
	m_sourcesaver.Process();
	// Maella end
	bool downloadingbefore=m_anStates[DS_DOWNLOADING]>0;
	// -khaos--+++> Moved this here, otherwise we were setting our permanent variables to 0 every tenth of a second...
	MEMZERO(m_anStates, sizeof(m_anStates));
	MEMZERO(src_stats, sizeof(src_stats));
	MEMZERO(net_stats, sizeof(net_stats));
	uint16 nCountForState;

	uint32 receivedBlockTotal = 0;
		{
		// [TPT] - Xman set sourcelistchange here:
		// Check if the list has been modified during the processing
		//m_sourceListChange = false;
		
		// Don't use loop with iterator => element swap 
		POSITION next_pos = srclist.GetHeadPosition();
		for (int i=0; i<srclist.GetCount(); i++)
		{	
			POSITION cur_pos = next_pos;
			CUpDownClient* cur_src = srclist.GetNext(next_pos);
			if (thePrefs.m_iDbgHeap >= 2)
				ASSERT_VALID( cur_src );

			// BEGIN -rewritten- refreshing statistics (no need for temp vars since it is not multithreaded)
			nCountForState = cur_src->GetDownloadState();
			//special case which is not yet set as downloadstate
			if (nCountForState == DS_ONQUEUE)
			{
				if( cur_src->IsRemoteQueueFull() )
					nCountForState = DS_REMOTEQUEUEFULL;
			}
			if (cur_src->IsBanned())
				nCountForState = DS_BANNED;
			if (cur_src->GetSourceFrom() >= SF_SERVER && cur_src->GetSourceFrom() <= SF_PASSIVE)
				++src_stats[cur_src->GetSourceFrom()];

			if (cur_src->GetServerIP() && cur_src->GetServerPort())
			{
				net_stats[0]++;
				if(cur_src->GetKadPort())
					net_stats[2]++;
			}
			if (cur_src->GetKadPort())
				net_stats[1]++;

			ASSERT( nCountForState < sizeof(m_anStates)/sizeof(m_anStates[0]) );
			m_anStates[nCountForState]++;
			
			// Check if the list has been modified during the processing
			m_sourceListChange = false; // [TPT] - Xman done above

			switch (cur_src->GetDownloadState())
			{
				case DS_DOWNLOADING:
					{
						// [TPT] - WebCache	
						ASSERT( cur_src->socket || cur_src->IsProxy() ); // yonatan http - change following code?
						if (cur_src->socket || (cur_src->IsProxy() && cur_src->m_pWCDownSocket) ) // yonatan http
						// [TPT] - WebCache	
						{
							cur_src->CheckDownloadTimeout();
							if(isLimited == false)
							{
								// Always call this method to avoid a flag (enabled/disable)
								// if(cur_src->socket != NULL){
								//    cur_src->socket->DisableDownloadLimit();
								// }
							}
							else 
							{
								if(maxammount > 6) // let room for header size
								{ 
									// Maella -Overhead compensation (pseudo full upload rate control)-
									uint64 receivedBlock;

									//Xman avoid the silly window syndrome
									/*
									The fast sender will quickly fill the receiver's TCP window.
									The receiver then reads N bytes, N being a relatively small number
									compared to the network frame size. A na�ve stack will immediately send
									an ACK to the sender to tell it that there are now N bytes available in
									its TCP window. This will cause the sender to send N bytes of data; 
									since N is smaller than the frame size, there's relatively more protocol
									overhead in the packet compared to a full frame. Because the receiver is
									slow, the TCP window stays very small, and thus hurts throughput because
									the ratio of protocol overhead to application data goes up.
									*/
									uint32 tempmaxamount;
									if(maxammount % MAXFRAGSIZE == 0) {
										tempmaxamount=maxammount;
									} else {
										tempmaxamount= MAXFRAGSIZE*(maxammount/MAXFRAGSIZE+1);
									}

									// Use the global statistic to measure the ammount of data (cleaner than a call back)
									receivedBlock = theApp.pBandWidthControl->GeteMuleIn();								

									// Trig OnReceive() (go-n-stop mode)
									if (cur_src->socket)
									{
										cur_src->socket->SetDownloadLimit(tempmaxamount); 
										if (cur_src->IsDownloadingFromPeerCache() && cur_src->m_pPCDownSocket && cur_src->m_pPCDownSocket->IsConnected())
											cur_src->m_pPCDownSocket->SetDownloadLimit(tempmaxamount);
								}
									// [TPT] - WebCache
									if (cur_src->IsProxy() && cur_src->IsDownloadingFromWebCache() &&
										cur_src->m_pWCDownSocket && cur_src->m_pWCDownSocket->IsConnected())
										cur_src->m_pWCDownSocket->SetDownloadLimit( tempmaxamount );
									// [TPT] - WebCache

									receivedBlock = theApp.pBandWidthControl->GeteMuleIn() - receivedBlock; //Xman Bugfix
									//Xman end: avoid the silly window syndrome
									// Maella end
									if(receivedBlock > 0)
									{
										if(maxammount > (uint32)receivedBlock)
										{
											receivedBlockTotal += (uint32)receivedBlock;
											maxammount -= (uint32)receivedBlock;
										} 
										else 
										{
											// Necessary because IP + TCP overhead
											receivedBlockTotal += maxammount;
											maxammount = 0;
										}
										// Try to 'balance' the download between clients.
										// Move the 'downloader' at the end of the list.
										if(m_sourceListChange == false && cur_src->GetDownloadState() == DS_DOWNLOADING)
										{
											srclist.RemoveAt(cur_pos);
											srclist.AddTail(cur_src);										
										}
									}
								}

								// In case of an exception, the instance of the client might have been deleted
								if(m_sourceListChange == false &&
									(cur_src->socket || cur_src->IsProxy()) && // [TPT] - WebCache
										cur_src->GetDownloadState() == DS_DOWNLOADING)
									{
										// Block OnReceive() (go-n-stop mode)
										if (cur_src->socket)
										{
											cur_src->socket->SetDownloadLimit(0);
											if (cur_src->IsDownloadingFromPeerCache() && cur_src->m_pPCDownSocket && cur_src->m_pPCDownSocket->IsConnected())
												cur_src->m_pPCDownSocket->SetDownloadLimit(0);
										}
										// [TPT] - WebCache
										if (cur_src->IsProxy() && cur_src->IsDownloadingFromWebCache() &&
											cur_src->m_pWCDownSocket && cur_src->m_pWCDownSocket->IsConnected())
											cur_src->m_pWCDownSocket->SetDownloadLimit(0);
										// [TPT] - WebCache									
						}										
					}
				}
					}
					break;
				// Do nothing with this client..
				case DS_BANNED:
					break;
				// Check if something has changed with our or their ID state..
				case DS_LOWTOLOWIP:
				{
					// To Mods, please stop instantly removing these sources..
					// This causes sources to pop in and out creating extra overhead!
					//Make sure this source is still a LowID Client..
					if( cur_src->HasLowID() )
					{
						//Make sure we still cannot callback to this Client..
						if( !theApp.DoCallback( cur_src ) )
						{
							//If we are almost maxed on sources, slowly remove these client to see if we can find a better source.
							// [TPT] - Sivka AutoHL
							if( ((dwCurTick - lastpurgetime) > SEC2MS(30)) && (GetSourceCount() >= (this->m_MaxSourcesPerFile*.8 )) )	
							{
								theApp.downloadqueue->RemoveSource( cur_src );
								lastpurgetime = dwCurTick;
							}
							break;
						}
					}
					// This should no longer be a LOWTOLOWIP..
					cur_src->SetDownloadState(DS_ONQUEUE);
					break;
				}
				case DS_NONEEDEDPARTS:
				{ 
					// To Mods, please stop instantly removing these sources..
					// This causes sources to pop in and out creating extra overhead!					
					if( (dwCurTick - lastpurgetime) > SEC2MS(40) ){
						lastpurgetime = dwCurTick;
						// we only delete them if reaching the limit				
						// [TPT] - Sivka AutoHL
						if (GetSourceCount() >= (this->m_MaxSourcesPerFile *.8 ))
						{	
							theApp.downloadqueue->RemoveSource( cur_src );
							break;
						}			
					}
					// doubled reasktime for no needed parts - save connections and traffic
                    if (cur_src->GetTimeUntilReask() > 0)
						break;
						
					cur_src->SwapToAnotherFile(_T("A4AF for NNP file. CPartFile::Process()"), true, false, false, NULL, true, true); // ZZ:DownloadManager
					// Recheck this client to see if still NNP.. Set to DS_NONE so that we force a TCP reask next time..
					cur_src->SetDownloadState(DS_NONE);
					break;
				}
				case DS_ONQUEUE:
				{
					// To Mods, please stop instantly removing these sources..
					// This causes sources to pop in and out creating extra overhead!
					if( cur_src->IsRemoteQueueFull() ) 
					{
						// [TPT] - Sivka AutoHL
						if( ((dwCurTick - lastpurgetime) > MIN2MS(1)) && (GetSourceCount() >= (this->m_MaxSourcesPerFile *.8 )) )
						{
							theApp.downloadqueue->RemoveSource( cur_src );
							lastpurgetime = dwCurTick;
							break;
						}
					}

					// Maella -Unnecessary Protocol Overload-
					// Maella -Spread Request- (idea SlugFiller)					
					if(theApp.IsConnected() == true)
					{
						// Check if a refresh is required for the download session with a cheap UDP
						if((cur_src->GetLastAskedTime() != 0) &&
							(cur_src->GetNextTCPAskedTime() > dwCurTick + MIN2MS(10)) &&
							// 55 seconds for two attempts to refresh the download session with UDP
							cur_src->GetTimeUntilReask() < SEC2MS(55) && cur_src->GetTimeUntilReask() > 0 && ::GetTickCount()-cur_src->getLastTriedToConnectTime() > MIN2MS(20)) // ZZ:DownloadManager (one resk timestamp for each file)
						{
							// Send a OP_REASKFILEPING (UDP) to refresh the download session
							// The refresh of the download session is necessary to stay in the remote queue
							// => see CUpDownClient::SetLastUpRequest() and MAX_PURGEQUEUETIME
							cur_src->UDPReaskForDownload();
						}
					}
					// Maella end
				}					
				case DS_CONNECTING:
				case DS_TOOMANYCONNS:
				case DS_TOOMANYCONNSKAD:
				case DS_NONE:
				case DS_WAITCALLBACK:
				case DS_WAITCALLBACKKAD:
					// Maella -Spread Request- (idea SlugFiller)
					// Maella -Unnecessary Protocol Overload-
					if(theApp.IsConnected())	
						{
						// Check if a refresh is required for the download session with TCP
						if(//(cur_src->GetLastAskedTime() == 0) || // Never asked before
							(cur_src->GetNextTCPAskedTime() <= dwCurTick) || // Full refresh with TCP is required
							(cur_src->socket != NULL && // Take advantage of the current connection
							cur_src->socket->IsConnected() == true && 
							cur_src->GetNextTCPAskedTime() - MIN2MS(10) < dwCurTick && 
							cur_src->GetLastTCPAskedTime(this) + MIN_REQUESTTIME + MIN2MS(1) < dwCurTick) ||
							(cur_src->GetTimeUntilReask() == 0 && ::GetTickCount()-cur_src->getLastTriedToConnectTime() > MIN2MS(20))) // ZZ:DownloadManager (one resk timestamp for each file)
						{
							// Initialize or refresh the download session with an expensive TCP session
							// The refresh of the download session is necessary to stay in the remote queue
							// => see CUpDownClient::SetLastUpRequest() and MAX_PURGEQUEUETIME
							if(!cur_src->AskForDownload()) // NOTE: This may *delete* the client!!
								break; //I left this break here just as a reminder just in case re rearange things..
						}
					}
					// Maella end
					break;
				}
			}
		if (downloadingbefore!=(m_anStates[DS_DOWNLOADING]>0))
			NotifyStatusChange();

		if( GetMaxSourcesUDPLimit() > GetSourceCount()){ // [TPT] - Sivka AutoHL
			if (theApp.downloadqueue->DoKademliaFileRequest() && (Kademlia::CKademlia::getTotalFile() < KADEMLIATOTALFILE) && (dwCurTick > m_LastSearchTimeKad) &&  Kademlia::CKademlia::isConnected() && theApp.IsConnected() && !stopped){ //Once we can handle lowID users in Kad, we remove the second IsConnected
				//Kademlia
				theApp.downloadqueue->SetLastKademliaFileRequest();
				if (!GetKadFileSearchID())
				{
					Kademlia::CUInt128 kadFileID;
					kadFileID.setValue(GetFileHash());
					Kademlia::CSearch* pSearch = Kademlia::CSearchManager::prepareLookup(Kademlia::CSearch::FILE, true, kadFileID);
					if (pSearch)
					{
						if(m_TotalSearchesKad < 7)
							m_TotalSearchesKad++;
						m_LastSearchTimeKad = dwCurTick + (KADEMLIAREASKTIME*m_TotalSearchesKad);
						SetKadFileSearchID(pSearch->getSearchID());
					}
					else
						SetKadFileSearchID(0);
				}
			}
		}
		else{
			if(GetKadFileSearchID())
			{
				Kademlia::CSearchManager::stopSearch(GetKadFileSearchID(), true);
			}
		}

	
	// check if we want new sources from server
	const uint16 iSourceCount = GetSourceCount(); // Maella -Code Improvement- (Ichi)
	if ( !m_bLocalSrcReqQueued && ((!m_LastSearchTime) || (dwCurTick - m_LastSearchTime) > SERVERREASKTIME) && theApp.serverconnect->IsConnected()
		&& GetMaxSourcesLimitSoft() > iSourceCount && !stopped ) // [TPT] - Sivka AutoHL
	{
		m_bLocalSrcReqQueued = true;
		theApp.downloadqueue->SendLocalSrcRequest(this);
	}
	// Called every second
	//UpdateAutoDownPriority(); // [TPT] - Patch
	if ( GetSrcStatisticsValue(DS_DOWNLOADING) != nOldTransSourceCount )
	{
		// [TPT] - khaos::categorymod+
		//if (theApp.emuledlg->transferwnd->downloadlistctrl.curTab == 0)
		theApp.emuledlg->transferwnd->downloadlistctrl.ChangeCategory(theApp.emuledlg->transferwnd->GetCategoryTab()); 
		//else
		//	UpdateDisplayedInfo(true);
		// [TPT] - khaos::categorymod-
		if (thePrefs.ShowCatTabInfos())
			theApp.emuledlg->transferwnd->UpdateCatTabTitles();
	}
	}
	return receivedBlockTotal;
}
// Maella end

bool CPartFile::CanAddSource(uint32 userid, uint16 port, uint32 serverip, uint16 serverport, UINT* pdebug_lowiddropped, bool Ed2kID)
{
	//The incoming ID could have the userid in the Hybrid format.. 
	uint32 hybridID = 0;
	if( Ed2kID )
	{
		if(IsLowID(userid))
			hybridID = userid;
		else
		hybridID = ntohl(userid);
	}
	else
	{
		hybridID = userid;
		if(!IsLowID(userid))
		userid = ntohl(userid);
	}

	// MOD Note: Do not change this part - Merkur
	if (theApp.serverconnect->IsConnected())
	{
		if(theApp.serverconnect->IsLowID())
		{
			if(theApp.serverconnect->GetClientID() == userid && theApp.serverconnect->GetCurrentServer()->GetIP() == serverip && theApp.serverconnect->GetCurrentServer()->GetPort() == serverport )
				return false;
			if(theApp.serverconnect->GetLocalIP() == userid)
				return false;
		}
		else
		{
			if(theApp.serverconnect->GetClientID() == userid && thePrefs.GetPort() == port)
				return false;
		}
	}
	if (Kademlia::CKademlia::isConnected())
	{
		if(!Kademlia::CKademlia::isFirewalled())
			if(Kademlia::CKademlia::getIPAddress() == hybridID && thePrefs.GetPort() == port)
				return false;
		}

	//This allows *.*.*.0 clients to not be removed if Ed2kID == false
	if ( IsLowID(hybridID) && theApp.IsFirewalled())
	{
		if (pdebug_lowiddropped)
			(*pdebug_lowiddropped)++;
		return false;
	}
	// MOD Note - end
	return true;
}

// [TPT]
// Maella -Accurate measure of bandwidth: eDonkey data + control, network adapter-
void CPartFile::CompDownloadRate()
{	
	// Remark: This method is called once every second

	// Process and retrieve m_nDownDatarate from clients
	m_nDownDatarate = 0;
	for(POSITION pos = m_downloadingSourceList.GetHeadPosition(); pos != NULL; )
	{
		CUpDownClient* cur_client = m_downloadingSourceList.GetNext(pos);
		if(cur_client->GetDownloadState() == DS_DOWNLOADING)
		{ 
			cur_client->CompDownloadRate();
			m_nDownDatarate += cur_client->GetDownloadDatarate();  // [bytes/s]
		}
	}

	// Check and then refresh GUI
	count++;
	if (count >= (100/TIMER_PERIOD)*DISPLAY_REFRESH)
	{
		count = 0;
		UpdateDisplayedInfo();
		UpdateCompletedInfos();		
	}
}
// Maella end


void CPartFile::AddSources(CSafeMemFile* sources, uint32 serverip, uint16 serverport)
{
	UINT count = sources->ReadUInt8();

	if (stopped)
	{
		// since we may received multiple search source UDP results we have to "consume" all data of that packet
		sources->Seek(count*(4+2), SEEK_CUR);
		return;
	}

	UINT debug_lowiddropped = 0;
	UINT debug_possiblesources = 0;
	for (UINT i = 0; i < count; i++)	
	{
		uint32 userid = sources->ReadUInt32();
		uint16 port = sources->ReadUInt16();

		// check the HighID(IP) - "Filter LAN IPs" and "IPfilter" the received sources IP addresses
		if (!IsLowID(userid))
		{
			if (!IsGoodIP(userid))
			{ 
				// check for 0-IP, localhost and optionally for LAN addresses
				if (thePrefs.GetLogFilteredIPs())
					AddDebugLogLine(false, _T("Ignored source (IP=%s) received from server - bad IP"), ipstr(userid));
				continue;
			}
			if (theApp.ipfilter->IsFiltered(userid))
			{
				if (thePrefs.GetLogFilteredIPs())
					AddDebugLogLine(false, _T("Ignored source (IP=%s) received from server - IP filter (%s)"), ipstr(userid), theApp.ipfilter->GetLastHit());
				continue;
			}
			if (theApp.clientlist->IsSnafuClient(userid)){ // [TPT] - eWombat
#ifdef _DEBUG
				if (thePrefs.GetLogBannedClients()){
					CUpDownClient* pClient = theApp.clientlist->FindClientByIP(userid);
					AddDebugLogLine(false, _T("Ignored source (IP=%s) received from server - banned client %s"), ipstr(userid), pClient->DbgGetClientInfo());
				}
#endif
				continue;
			}
		}

		// additionally check for LowID and own IP
		if (!CanAddSource(userid, port, serverip, serverport, &debug_lowiddropped))
		{
			if (thePrefs.GetLogFilteredIPs())
				AddDebugLogLine(false, _T("Ignored source (IP=%s) received from server"), ipstr(userid));
			continue;
		}

		// [TPT] - Sivka AutoHL
		if( this->m_MaxSourcesPerFile > GetSourceCount() )
		{
			debug_possiblesources++;
			CUpDownClient* newsource = new CUpDownClient(this,port,userid,serverip,serverport,true);
			theApp.downloadqueue->CheckAndAddSource(this,newsource);
		}
		else
		{
			// since we may received multiple search source UDP results we have to "consume" all data of that packet
			sources->Seek((count-i-1)*(4+2), SEEK_CUR); //Xman (ttwin) //sources->Seek((count-i)*(4+2), SEEK_CUR);
			if(GetKadFileSearchID())
				Kademlia::CSearchManager::stopSearch(GetKadFileSearchID(), false);
			break;
		}
	}			
	if (thePrefs.GetDebugSourceExchange())
		AddDebugLogLine(false, _T("SXRecv: Server source response; Count=%u, Dropped=%u, PossibleSources=%u, File=\"%s\""), count, debug_lowiddropped, debug_possiblesources, GetFileName());
}

// [TPT] - itsonlyme: cacheUDPsearchResults
CServer *CPartFile::GetNextAvailServer()
{
	if (m_preferredServers.IsEmpty()) 
		return NULL;

	POSITION pos = m_preferredServers.GetHeadPosition();
	SServer aServer = m_preferredServers.GetValueAt(pos);
	m_preferredServers.RemoveAt(pos);

	CServer *nextServer = theApp.serverlist->GetServerByIP(aServer.m_nIP, aServer.m_nPort);
	if (!nextServer) 
		return NULL;

	CString tracemsg;
	tracemsg.Format(_T("GetNextAvailServer returned %s:%i server with %i sources for %s"), nextServer->GetAddress(), nextServer->GetPort(), aServer.m_uAvail, this->GetFileName());
	TRACE(tracemsg);
	if (thePrefs.GetVerbose())
		AddDebugLogLine(false, tracemsg);

	return nextServer;
}

void CPartFile::AddAvailServer(SServer server)
{
	m_preferredServers.Insert(server.m_uAvail, server);
	while (m_preferredServers.GetCount() > MAX_PREF_SERVERS)
		m_preferredServers.RemoveAt(m_preferredServers.GetHeadPosition());
}
// [TPT] - itsonlyme: cacheUDPsearchResults

void CPartFile::AddSource(LPCTSTR pszURL, uint32 nIP)
{
	if (stopped)
		return;

	if (!IsGoodIP(nIP))
	{ 
		// check for 0-IP, localhost and optionally for LAN addresses
		if (thePrefs.GetLogFilteredIPs())
			AddDebugLogLine(false, _T("Ignored URL source (IP=%s) \"%s\" - bad IP"), ipstr(nIP), pszURL);
		return;
	}
	if (theApp.ipfilter->IsFiltered(nIP))
	{
		if (thePrefs.GetLogFilteredIPs())
			AddDebugLogLine(false, _T("Ignored URL source (IP=%s) \"%s\" - IP filter (%s)"), ipstr(nIP), pszURL, theApp.ipfilter->GetLastHit());
		return;
	}

	CUrlClient* client = new CUrlClient;
	if (!client->SetUrl(pszURL, nIP))
	{
		LogError(LOG_STATUSBAR, _T("Failed to process URL source \"%s\""), pszURL);
		delete client;
		return;
	}
	client->SetRequestFile(this);
	client->SetSourceFrom(SF_LINK);
	if (theApp.downloadqueue->CheckAndAddSource(this, client))
		UpdatePartsInfo();
}

// SLUGFILLER: heapsortCompletesrc
static void HeapSort(CArray<uint16,uint16> &count, uint32 first, uint32 last){
	uint32 r;
	for ( r = first; !(r & 0x80000000) && (r<<1) < last; ){
		uint32 r2 = (r<<1)+1;
		if (r2 != last)
			if (count[r2] < count[r2+1])
				r2++;
		if (count[r] < count[r2]){
			uint16 t = count[r2];
			count[r2] = count[r];
			count[r] = t;
			r = r2;
		}
		else
			break;
	}
}
// SLUGFILLER: heapsortCompletesrc

void CPartFile::UpdatePartsInfo()
{
	if( !IsPartFile() )
	{
		CKnownFile::UpdatePartsInfo();
		return;
	}

	// Cache part count
	uint16 partcount = GetPartCount();
	bool flag = (time(NULL) - m_nCompleteSourcesTime > 0); 

	// Reset part counters
	if(m_SrcpartFrequency.GetSize() < partcount)
		m_SrcpartFrequency.SetSize(partcount);
	for(int i = 0; i < partcount; i++)
		m_SrcpartFrequency[i] = 0;
	
	CArray<uint16,uint16> count;
	if (flag)
		count.SetSize(0, srclist.GetSize());
	for (POSITION pos = srclist.GetHeadPosition(); pos != 0; )
	{
		CUpDownClient* cur_src = srclist.GetNext(pos);
		uint8* m_abyPartStatus = cur_src->GetPartStatus();  // [TPT] - netfinity: Anti HideOS
		if( m_abyPartStatus  )
		{		
			// [TPT] - netfinity: Anti HideOS
			if (cur_src->m_abySeenPartStatus == NULL)
			{
				cur_src->m_abySeenPartStatus = new uint8[partcount];
				for (int i = 0; i < partcount; i++)
					cur_src->m_abySeenPartStatus[i] = 0;
			}
			// [TPT] - netfinity: Anti HideOS
			for (int i = 0; i < partcount; i++)
			{
				if (cur_src->IsPartAvailable(i) || cur_src->m_abySeenPartStatus[i]) // [TPT] - netfinity: Anti HideOS
				{
					m_SrcpartFrequency[i] += 1;
					cur_src->m_abySeenPartStatus[i] = 1; // [TPT] - netfinity: Anti HideOS					
				}
			}
			if ( flag )
			{
				count.Add(cur_src->GetUpCompleteSourcesCount());
			}

			// [TPT] - netfinity: Anti HideOS
			// If is a DS_NONEEDEDPARTS source and m_abySeenPartStatus != m_abyPartStatus
			// change state to DS_ONQUEUE
			if (cur_src->GetDownloadState() == DS_NONEEDEDPARTS)
			{
				for (int i = 0; i < partcount; i++)
				{
					if (m_abyPartStatus[i] != cur_src->m_abySeenPartStatus[i])
					{
						cur_src->SetDownloadState(DS_ONQUEUE);
						break;
					}
				}
			}
			// [TPT] - netfinity: Anti HideOS
		}
	}

	if (flag)
	{
		m_nCompleteSourcesCount = m_nCompleteSourcesCountLo = m_nCompleteSourcesCountHi = 0;
	
		for (uint16 i = 0; i < partcount; i++)
		{
			if( !i )
				m_nCompleteSourcesCount = m_SrcpartFrequency[i];
			else if( m_nCompleteSourcesCount > m_SrcpartFrequency[i])
				m_nCompleteSourcesCount = m_SrcpartFrequency[i];
		}
	
		count.Add(m_nCompleteSourcesCount);
	
		int n = count.GetSize();
		if (n > 0)
		{
			// SLUGFILLER: heapsortCompletesrc
			int r;
			for (r = n/2; r--; )
				HeapSort(count, r, n-1);
			for (r = n; --r; ){
				uint16 t = count[r];
				count[r] = count[0];
				count[0] = t;
				HeapSort(count, 0, r-1);
			}
			// SLUGFILLER: heapsortCompletesrc

			// calculate range
			int i = n >> 1;			// (n / 2)
			int j = (n * 3) >> 2;	// (n * 3) / 4
			int k = (n * 7) >> 3;	// (n * 7) / 8

			//When still a part file, adjust your guesses by 20% to what you see..

			//Not many sources, so just use what you see..
			if (n < 5)
			{
//				m_nCompleteSourcesCount;
				m_nCompleteSourcesCountLo= m_nCompleteSourcesCount;
				m_nCompleteSourcesCountHi= m_nCompleteSourcesCount;
			}
			//For low guess and normal guess count
			//	If we see more sources then the guessed low and normal, use what we see.
			//	If we see less sources then the guessed low, adjust network accounts for 80%, we account for 20% with what we see and make sure we are still above the normal.
			//For high guess
			//  Adjust 80% network and 20% what we see.
			else if (n < 20)
			{
				if ( count.GetAt(i) < m_nCompleteSourcesCount )
					m_nCompleteSourcesCountLo = m_nCompleteSourcesCount;
				else
					m_nCompleteSourcesCountLo = (uint16)((float)(count.GetAt(i)*.8)+(float)(m_nCompleteSourcesCount*.2));
				m_nCompleteSourcesCount= m_nCompleteSourcesCountLo;
				m_nCompleteSourcesCountHi= (uint16)((float)(count.GetAt(j)*.8)+(float)(m_nCompleteSourcesCount*.2));
				if( m_nCompleteSourcesCountHi < m_nCompleteSourcesCount )
					m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;
			}
			else
			//Many sources..
			//For low guess
			//	Use what we see.
			//For normal guess
			//	Adjust network accounts for 80%, we account for 20% with what we see and make sure we are still above the low.
			//For high guess
			//  Adjust network accounts for 80%, we account for 20% with what we see and make sure we are still above the normal.
			{
				m_nCompleteSourcesCountLo= m_nCompleteSourcesCount;
				m_nCompleteSourcesCount= (uint16)((float)(count.GetAt(j)*.8)+(float)(m_nCompleteSourcesCount*.2));
				if( m_nCompleteSourcesCount < m_nCompleteSourcesCountLo )
					m_nCompleteSourcesCount = m_nCompleteSourcesCountLo;
				m_nCompleteSourcesCountHi= (uint16)((float)(count.GetAt(k)*.8)+(float)(m_nCompleteSourcesCount*.2));
				if( m_nCompleteSourcesCountHi < m_nCompleteSourcesCount )
					m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;
			}
		}
		m_nCompleteSourcesTime = time(NULL) + (60);
	}
    // [TPT] - Powershare
	m_nVirtualCompleteSourcesCount = (uint16)-1;
	for (uint16 i = 0; i < partcount; i++){
		if(m_nVirtualCompleteSourcesCount > m_SrcpartFrequency[i])
			m_nVirtualCompleteSourcesCount = m_SrcpartFrequency[i];
	}

	UpdatePowerShareLimit(m_nCompleteSourcesCountHi<200, (lastseencomplete!=NULL && m_nCompleteSourcesCountHi==1) || m_nVirtualCompleteSourcesCount==1 || (m_nCompleteSourcesCountHi==0 && m_nVirtualCompleteSourcesCount>0),m_nCompleteSourcesCountHi>((GetPowerShareLimit()>=0)?GetPowerShareLimit():thePrefs.GetPowerShareLimit()));
    // [TPT] - Powershare END
	UpdateDisplayedInfo();
}	

// [TPT] - enkeyDev: ICS - Intelligent Chunk Selection
void CPartFile::NewSrcIncPartsInfo()
{
	uint16 partcount = GetPartCount();

	if (m_SrcIncPartFrequency.GetSize() < partcount)
		m_SrcIncPartFrequency.SetSize(partcount);

	for(int i = 0; i < partcount; i++)
		m_SrcIncPartFrequency[i] = 0;

	CUpDownClient* cur_src;

	for (POSITION pos = srclist.GetHeadPosition(); pos != NULL;){
		cur_src = srclist.GetNext(pos);
		for (int i = 0; i < partcount; i++){
			if (cur_src->IsIncPartAvailable(i))
				m_SrcIncPartFrequency[i] +=1;
		}
	}
	//UpdateDisplayedInfo(); // Not displayed
}

uint32 CPartFile::GetPartSizeToDownload(uint16 partNumber)
{
	Gap_Struct *currentGap;
	uint32 total, gap_start, gap_end, partStart, partEnd;

	total = 0;
	partStart = (PARTSIZE * partNumber);
	partEnd = min(partStart + PARTSIZE, GetFileSize());

	for (POSITION pos = gaplist.GetHeadPosition(); pos != 0; gaplist.GetNext(pos))
	{
		currentGap = gaplist.GetAt(pos);

		if ((currentGap->start < partEnd) && (currentGap->end >= partStart))
		{
			gap_start = max(currentGap->start, partStart);
			gap_end = min(currentGap->end + 1, partEnd);
			total += (gap_end - gap_start); // I'm not sure this works: do gaps overlap?
		}
	}

	return min(total, partEnd - partStart); // This to limit errors: see note above
}

#define	CM_RELEASE_MODE			1
#define	CM_SPREAD_MODE			2
#define	CM_SHARE_MODE			3

#define	CM_SPREAD_MINSRC		10
#define	CM_SHARE_MINSRC			25
#define CM_MAX_SRC_CHUNK		3

bool CPartFile::GetNextRequestedBlock(CUpDownClient* sender, Requested_Block_Struct** newblocks, uint16* count)
{
	if (!(*count)) return false;					// added as in 29a
	if (!(sender->GetPartStatus())) return false;	// added as in 29a

	// Select mode: RELEASE, SPREAD or SHARE

	uint16	part_idx;
	uint16	min_src = 0xFFFF;

	if (m_SrcpartFrequency.GetCount() < GetPartCount())
		min_src = 0;
	else
		for (part_idx = 0; part_idx < GetPartCount(); ++part_idx)
			if (m_SrcpartFrequency[part_idx] < min_src)
				min_src = m_SrcpartFrequency[part_idx];

	if (min_src <= CM_SPREAD_MINSRC)		m_ics_filemode = CM_RELEASE_MODE;
	else if (min_src <= CM_SHARE_MINSRC)	m_ics_filemode = CM_SPREAD_MODE;
	else									m_ics_filemode = CM_SHARE_MODE;

	// Chunk list ordered by preference

	CList<uint16,uint16> chunk_list;
	CList<uint32,uint32> chunk_pref;
	uint32 c_pref;
	uint32 complete_src;
	uint32 incomplete_src;
	uint32 first_last_mod;
	uint32 size2transfer;
	uint16* partsDownloading = CalcDownloadingParts(sender); // Pawcio for enkeyDEV -ICS-

	// Add up modifiers
	// [TPT] - WebCache	: for ICS
	// WebCache: jp Don't request chunks for which we are currently receiving proxy sources START
	ThrottledChunk cur_ThrottledChunk;
	md4cpy(cur_ThrottledChunk.FileID, this->GetFileHash());
	bool isthrottled = false;
	// jp Don't request chunks for which we are currently receiving proxy sources START
	// [TPT] - WebCache	: for ICS

    const bool isPreviewEnable = (thePrefs.GetPreviewPrio() || thePrefs.IsExtControlsEnabled() && GetPreviewPrio()) && IsPreviewableFileType();
	if (isPreviewEnable) m_ics_filemode = CM_SHARE_MODE;

	for (part_idx = 0; part_idx < GetPartCount(); ++part_idx)
	{
		// [TPT] - netfinity: Anti HideOS
		if ((sender->IsPartAvailable(part_idx) || (sender->IsPartialSource() && sender->m_abySeenPartStatus && sender->m_abySeenPartStatus[part_idx])) && GetNextEmptyBlockInPart(part_idx, 0))
		{
			complete_src = 0;
			incomplete_src = 0;
			first_last_mod = 0;

			// Chunk priority modifiers
			if (m_ics_filemode != CM_SHARE_MODE)
			{
				complete_src = m_SrcpartFrequency.GetCount() > part_idx ? m_SrcpartFrequency[part_idx] : 0;
				incomplete_src = m_SrcIncPartFrequency.GetCount() > part_idx ? m_SrcIncPartFrequency[part_idx] : 0;
			}
			if (m_ics_filemode != CM_RELEASE_MODE)
			{
				if (part_idx == 0 || part_idx == (GetPartCount() - 1))		first_last_mod = 2;
				else if (part_idx == 1 || part_idx == (GetPartCount() - 2))	first_last_mod = 1;
				else														first_last_mod = 0;
				if (!isPreviewEnable)										first_last_mod = 0;
			}
			size2transfer = GetPartSizeToDownload(part_idx);

			// [TPT] - WebCache: for ICS
			cur_ThrottledChunk.ChunkNr=part_idx;
			cur_ThrottledChunk.timestamp=GetTickCount();
			isthrottled = ThrottledChunkList.CheckList(cur_ThrottledChunk, false); //compare this chunk to chunks in list and throttle it if it is found

			if (isthrottled)
				size2transfer = min(((size2transfer + 2 * PARTSIZE / CM_MAX_SRC_CHUNK + 0xff) >> 8), 0xFFFF);
			else
			// [TPT] - WebCache: for ICS
				size2transfer = min(((size2transfer + (partsDownloading ? PARTSIZE * partsDownloading[part_idx] / CM_MAX_SRC_CHUNK : 0) + 0xff) >> 8), 0xFFFF);

			switch (m_ics_filemode)
			{
			case CM_RELEASE_MODE:
				complete_src = min(complete_src, 0xFF);
				incomplete_src = min(incomplete_src, 0xFF);
				c_pref = size2transfer | (incomplete_src << 16) | (complete_src << 24);
				break;
			case CM_SPREAD_MODE:
				complete_src = min(complete_src, 0xFF);
				incomplete_src = min(incomplete_src, 0x3F);
				c_pref = first_last_mod | (incomplete_src << 2) | (complete_src << 8) | (size2transfer << 16);
				break;
			case CM_SHARE_MODE:
				c_pref = first_last_mod | (size2transfer << 16);
				break;
			}

			if (partsDownloading && partsDownloading[part_idx] >= ceil((float)size2transfer * CM_MAX_SRC_CHUNK / (float)PARTSIZE)) //Pawcio for enkeyDEV -ICS-
				c_pref |= 0xFF000000;
			// Ordered insertion

			POSITION c_ins_point = chunk_list.GetHeadPosition();
			POSITION p_ins_point = chunk_pref.GetHeadPosition();

			while (c_ins_point && p_ins_point && chunk_pref.GetAt(p_ins_point) < c_pref)
			{
				chunk_list.GetNext(c_ins_point);
				chunk_pref.GetNext(p_ins_point);
			}

			if (c_ins_point)
			{
				int eq_count = 0;
				POSITION p_eq_point = p_ins_point;
				while (p_eq_point != 0 && chunk_pref.GetAt(p_eq_point) == c_pref)
				{
					++eq_count;
					chunk_pref.GetNext(p_eq_point);
				}
				if (eq_count) // insert in random position
				{
					uint16 randomness = (uint16)floor(((float)rand()/RAND_MAX)*eq_count);
					while (randomness)
					{
						chunk_list.GetNext(c_ins_point);
						chunk_pref.GetNext(p_ins_point);
						--randomness;
					}
				}

			} // END if c_ins_point

			if (c_ins_point) // null ptr would add to head, I need to add to tail
			{
				chunk_list.InsertBefore(c_ins_point, part_idx);
				chunk_pref.InsertBefore(p_ins_point, c_pref);
			}
			else
			{
				chunk_list.AddTail(part_idx);
				chunk_pref.AddTail(c_pref);
			}

		} // END if part downloadable
	} // END for every chunk

	if (partsDownloading)
		delete partsDownloading; //Pawcio for enkeyDEV -ICS-

	//Pawcio for enkeyDEV -ICS-
	if(sender->m_lastPartAsked != 0xffff && 
		(sender->IsPartAvailable(sender->m_lastPartAsked) || 
		(sender->IsPartialSource() && sender->m_abySeenPartStatus && sender->m_abySeenPartStatus[sender->m_lastPartAsked])) // [TPT] - netfinity: Anti HideOS
		&& GetNextEmptyBlockInPart(sender->m_lastPartAsked, 0)){
		chunk_list.AddHead(sender->m_lastPartAsked);
		chunk_pref.AddHead((uint32) 0);
	} 
	else {
		sender->m_lastPartAsked = 0xffff;
	}

	uint16 requestedCount = *count;
	uint16 newblockcount = 0;
	*count = 0;

	if (chunk_list.IsEmpty()) return false;

	Requested_Block_Struct* block = new Requested_Block_Struct;
	for (POSITION scan_chunks = chunk_list.GetHeadPosition(); scan_chunks; chunk_list.GetNext(scan_chunks))
	{
		sender->m_lastPartAsked = chunk_list.GetAt(scan_chunks);
		while(GetNextEmptyBlockInPart(chunk_list.GetAt(scan_chunks),block))
		{
			requestedblocks_list.AddTail(block);
			newblocks[newblockcount] = block;
			newblockcount++;
			*count = newblockcount;
			if (newblockcount == requestedCount)
				return true;
			block = new Requested_Block_Struct;
		}
	}
	delete block;

	if (!(*count)) return false; // useless, just to be sure
	return true;
}
// [TPT] - enkeyDev: ICS - Intelligent Chunk Selection

bool CPartFile::RemoveBlockFromList(uint32 start, uint32 end)
{
	ASSERT( start <= end );

	bool bResult = false;
	for (POSITION pos = requestedblocks_list.GetHeadPosition(); pos != NULL; ){
		POSITION posLast = pos;
		Requested_Block_Struct* block = requestedblocks_list.GetNext(pos);
		if (block->StartOffset <= start && block->EndOffset >= end){
			requestedblocks_list.RemoveAt(posLast);
			bResult = true;
		}
	}
	return bResult;
}

bool CPartFile::IsInRequestedBlockList(const Requested_Block_Struct* block) const
{
	return requestedblocks_list.Find(const_cast<Requested_Block_Struct*>(block)) != NULL;
}

void CPartFile::RemoveAllRequestedBlocks(void)
{
	requestedblocks_list.RemoveAll();
}

void CPartFile::CompleteFile(bool bIsHashingDone)
{
	theApp.downloadqueue->RemoveLocalServerRequest(this);
	if(GetKadFileSearchID())
		Kademlia::CSearchManager::stopSearch(GetKadFileSearchID(), false);

	if (srcarevisible)
		theApp.emuledlg->transferwnd->downloadlistctrl.HideSources(this);
	
	if (!bIsHashingDone){
		SetStatus(PS_COMPLETING);
		m_nDownDatarate = 0; // [TPT]
		CAddFileThread* addfilethread = (CAddFileThread*) AfxBeginThread(RUNTIME_CLASS(CAddFileThread), THREAD_PRIORITY_BELOW_NORMAL,0, CREATE_SUSPENDED);
		if (addfilethread){
			SetFileOp(PFOP_HASHING);
			SetFileOpProgress(0);
			addfilethread->SetValues(0,thePrefs.GetTempDir(),RemoveFileExtension(m_partmetfilename),this);
			addfilethread->ResumeThread();				
		}
		else{
			LogError(LOG_STATUSBAR, GetResString(IDS_ERR_FILECOMPLETIONTHREAD));
			SetStatus(PS_ERROR);
		}
		return;
	}
	else{
		StopFile();
		SetStatus(PS_COMPLETING);
		m_is_A4AF_auto=false;
		CWinThread *pThread = AfxBeginThread(CompleteThreadProc, this, THREAD_PRIORITY_BELOW_NORMAL, 0, CREATE_SUSPENDED); // Lord KiRon - using threads for file completion
		if (pThread){
			SetFileOp(PFOP_COPYING);
			SetFileOpProgress(0);
			pThread->ResumeThread();
		}
		else{
			LogError(LOG_STATUSBAR, GetResString(IDS_ERR_FILECOMPLETIONTHREAD));
			SetStatus(PS_ERROR);
			return;
		}
	}
	theApp.emuledlg->transferwnd->downloadlistctrl.ShowFilesCount();
	if (thePrefs.ShowCatTabInfos())
		theApp.emuledlg->transferwnd->UpdateCatTabTitles();
	UpdateDisplayedInfo(true);
}

UINT CPartFile::CompleteThreadProc(LPVOID pvParams) 
{ 
	DbgSetThreadName("PartFileComplete");
	InitThreadLocale();
	CPartFile* pFile = (CPartFile*)pvParams;
	if (!pFile)
		return (UINT)-1; 
   	pFile->PerformFileComplete(); 
   	return 0; 
}

void UncompressFile(LPCTSTR pszFilePath, CPartFile* pPartFile)
{
	// check, if it's a compressed file
	DWORD dwAttr = GetFileAttributes(pszFilePath);
	if (dwAttr == INVALID_FILE_ATTRIBUTES || (dwAttr & FILE_ATTRIBUTE_COMPRESSED) == 0)
		return;

	CString strDir = pszFilePath;
	PathRemoveFileSpec(strDir.GetBuffer());
	strDir.ReleaseBuffer();

	// If the directory of the file has the 'Compress' attribute, do not uncomress the file
	dwAttr = GetFileAttributes(strDir);
	if (dwAttr == INVALID_FILE_ATTRIBUTES || (dwAttr & FILE_ATTRIBUTE_COMPRESSED) != 0)
		return;

	HANDLE hFile = CreateFile(pszFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE){
		if (thePrefs.GetVerbose())
			theApp.QueueDebugLogLine(true, _T("Failed to open file \"%s\" for decompressing - %s"), pszFilePath, GetErrorMessage(GetLastError(), 1));
		return;
	}
	
	if (pPartFile)
		pPartFile->SetFileOp(PFOP_UNCOMPRESSING);

	USHORT usInData = COMPRESSION_FORMAT_NONE;
	DWORD dwReturned = 0;
	if (!DeviceIoControl(hFile, FSCTL_SET_COMPRESSION, &usInData, sizeof usInData, NULL, 0, &dwReturned, NULL)){
		if (thePrefs.GetVerbose())
			theApp.QueueDebugLogLine(true, _T("Failed to decompress file \"%s\" - %s"), pszFilePath, GetErrorMessage(GetLastError(), 1));
	}
	CloseHandle(hFile);
}

DWORD CALLBACK CopyProgressRoutine(LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred,
								   LARGE_INTEGER StreamSize, LARGE_INTEGER StreamBytesTransferred, DWORD dwStreamNumber,
								   DWORD dwCallbackReason, HANDLE hSourceFile, HANDLE hDestinationFile, LPVOID lpData)
{
	CPartFile* pPartFile = (CPartFile*)lpData;
	if (TotalFileSize.QuadPart && pPartFile && pPartFile->IsKindOf(RUNTIME_CLASS(CPartFile)))
	{
		UINT uProgress = TotalBytesTransferred.QuadPart * 100 / TotalFileSize.QuadPart;
		if (uProgress != pPartFile->GetFileOpProgress())
		{
			ASSERT( uProgress <= 100 );
			VERIFY( PostMessage(theApp.emuledlg->GetSafeHwnd(), TM_FILEOPPROGRESS, uProgress, (LPARAM)pPartFile) );
		}
	}
	else
		ASSERT(0);

	return PROGRESS_CONTINUE;
}

DWORD MoveCompletedPartFile(LPCTSTR pszPartFilePath, LPCTSTR pszNewPartFilePath, CPartFile* pPartFile)
{
	DWORD dwMoveResult = ERROR_INVALID_FUNCTION;

	bool bUseDefaultMove = true;
	HMODULE hLib = LoadLibrary(_T("KERNEL32.DLL"));
	if (hLib)
	{
		BOOL (WINAPI *pfnMoveFileWithProgress)(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, DWORD dwFlags);
		(FARPROC&)pfnMoveFileWithProgress = GetProcAddress(hLib, _TWINAPI("MoveFileWithProgress"));
		if (pfnMoveFileWithProgress)
		{
			bUseDefaultMove = false;
			if ((*pfnMoveFileWithProgress)(pszPartFilePath, pszNewPartFilePath, CopyProgressRoutine, pPartFile, MOVEFILE_COPY_ALLOWED))
				dwMoveResult = ERROR_SUCCESS;
			else
				dwMoveResult = GetLastError();
		}
		FreeLibrary(hLib);
	}

	if (bUseDefaultMove)
	{
		if (MoveFile(pszPartFilePath, pszNewPartFilePath))
			dwMoveResult = ERROR_SUCCESS;
		else
			dwMoveResult = GetLastError();
	}

	return dwMoveResult;
}

// Lord KiRon - using threads for file completion
// NOTE: This function is executed within a seperate thread, do *NOT* use any lists/queues of the main thread without
// synchronization. Even the access to couple of members of the CPartFile (e.g. filename) would need to be properly
// synchronization to achive full multi threading compliance.
BOOL CPartFile::PerformFileComplete() 
{
	// If that function is invoked from within the file completion thread, it's ok if we wait (and block) the thread.
	CSingleLock sLock(&m_FileCompleteMutex, TRUE);

	CString strPartfilename(RemoveFileExtension(m_fullname));
	TCHAR* newfilename = _tcsdup(GetFileName());
	_tcscpy(newfilename, (LPCTSTR)StripInvalidFilenameChars(newfilename));

	CString strNewname;
	CString indir;

	if (PathFileExists(thePrefs.GetCategory(GetCategory())->incomingpath)){
		indir = thePrefs.GetCategory(GetCategory())->incomingpath;
		strNewname.Format(_T("%s\\%s"), indir, newfilename);
	}
	else{
		indir = thePrefs.GetIncomingDir();
		strNewname.Format(_T("%s\\%s"), indir, newfilename);
	}

	// close permanent handle
	try{
		if (m_hpartfile.m_hFile != INVALID_HANDLE_VALUE)
			m_hpartfile.Close();
	}
	catch(CFileException* error){
		TCHAR buffer[MAX_CFEXP_ERRORMSG];
		error->GetErrorMessage(buffer, ARRSIZE(buffer));
		AddLogLine(true, GetResString(IDS_ERR_FILEERROR), m_partmetfilename, GetFileName(), buffer);	// I18n: ThreadSafeLog
		error->Delete();
		//return false;
	}

	bool renamed = false;
	if(PathFileExists(strNewname))
	{
		renamed = true;
		int namecount = 0;

		size_t length = _tcslen(newfilename);
		ASSERT(length != 0); //name should never be 0

		//the file extension
		TCHAR *ext = _tcsrchr(newfilename, _T('.'));
		if(ext == NULL)
			ext = newfilename + length;

		TCHAR *last = ext;  //new end is the file name before extension
		last[0] = 0;  //truncate file name

		//search for matching ()s and check if it contains a number
		if((ext != newfilename) && (_tcsrchr(newfilename, _T(')')) + 1 == last)) {
			TCHAR *first = _tcsrchr(newfilename, _T('('));
			if(first != NULL) {
				first++;
				bool found = true;
				for(TCHAR *step = first; step < last - 1; step++)
					if(*step < _T('0') || *step > _T('9')) {
						found = false;
						break;
					}
				if(found) {
					namecount = _tstoi(first);
					last = first - 1;
					last[0] = 0;  //truncate again
				}
			}
		}

		CString strTestName;
		do {
			namecount++;
			strTestName.Format(_T("%s\\%s(%d).%s"), indir, newfilename, namecount, min(ext + 1, newfilename + length));
		}
		while (PathFileExists(strTestName));
		strNewname = strTestName;
	}
	free(newfilename);
	
	// [TPT]
	// Maella -New Save/load Sources- enkeyDEV(Ottavio84)
	m_sourcesaver.DeleteFile();
	// Maella end

	m_SettingsSaver.DeleteFile(this); // [TPT] - Sivka AutoHL 

	DWORD dwMoveResult;
	if ((dwMoveResult = MoveCompletedPartFile(strPartfilename, strNewname, this)) != ERROR_SUCCESS)
	{
		AddLogLine(true,GetResString(IDS_ERR_COMPLETIONFAILED) + _T(" - \"%s\": ") + GetErrorMessage(dwMoveResult), GetFileName(), strNewname);	// I18n: ThreadSafeLog
		// If the destination file path is too long, the default system error message may not be helpful for user to know what failed.
		if (strNewname.GetLength() >= MAX_PATH)
				AddLogLine(true,GetResString(IDS_ERR_COMPLETIONFAILED) + _T(" - \"%s\": Path too long"),GetFileName(), strNewname);	// I18n: ThreadSafeLog

		paused = true;
		stopped = true;
		SetStatus(PS_ERROR);
		m_bCompletionError = true;
		SetFileOp(PFOP_NONE);
		if (theApp.emuledlg && theApp.emuledlg->IsRunning())
			VERIFY( PostMessage(theApp.emuledlg->m_hWnd, TM_FILECOMPLETED, FILE_COMPLETION_THREAD_FAILED, (LPARAM)this) );
		return FALSE;
	}

	UncompressFile(strNewname, this);

	// to have the accurate date stored in known.met we have to update the 'date' of a just completed file.
	// if we don't update the file date here (after commiting the file and before adding the record to known.met), 
	// that file will be rehashed at next startup and there would also be a duplicate entry (hash+size) in known.met
	// because of different file date!
	ASSERT( m_hpartfile.m_hFile == INVALID_HANDLE_VALUE ); // the file must be closed/commited!
	struct _stat st;
	if (_tstat(strNewname, &st) == 0)
	{
		m_tLastModified = st.st_mtime;
		m_tUtcLastModified = m_tLastModified;
		AdjustNTFSDaylightFileTime(m_tUtcLastModified, strNewname);
	}

	// remove part.met file
	if (_tremove(m_fullname))
		theApp.QueueLogLine(true,GetResString(IDS_ERR_DELETEFAILED) + _T(" - ") + CString(_tcserror(errno)),m_fullname);

	// remove backup files
	CString BAKName(m_fullname);
	BAKName.Append(PARTMET_BAK_EXT);
	if (_taccess(BAKName, 0) == 0 && !::DeleteFile(BAKName))
		theApp.QueueLogLine(true,GetResString(IDS_ERR_DELETE) + _T(" - ") + GetErrorMessage(GetLastError()), BAKName);

	BAKName = m_fullname;
	BAKName.Append(PARTMET_TMP_EXT);
	if (_taccess(BAKName, 0) == 0 && !::DeleteFile(BAKName))
		theApp.QueueLogLine(true,GetResString(IDS_ERR_DELETE) + _T(" - ") + GetErrorMessage(GetLastError()), BAKName);

	// initialize 'this' part file for being a 'complete' file, this is to be done *before* releasing the file mutex.
	m_fullname = strNewname;
	SetPath(indir);
	SetFilePath(m_fullname);
	SetStatus(PS_COMPLETE);
	paused = false;
	SetFileOp(PFOP_NONE);

	// clear the blackbox to free up memory
	//m_CorruptionBlackBox.Free(); // [TPT] - netfinity: Moved into main thread!

	//[TPT] - [MoNKi: -Downloaded History-]
	if(theApp.emuledlg && theApp.emuledlg->IsRunning())
		theApp.emuledlg->sharedfileswnd->historylistctrl.AddFile(this);
	//[TPT] - [MoNKi: -Downloaded History-]

	// explicitly unlock the file before posting something to the main thread.
	sLock.Unlock();

	if (theApp.emuledlg && theApp.emuledlg->IsRunning())
		VERIFY( PostMessage(theApp.emuledlg->m_hWnd, TM_FILECOMPLETED, FILE_COMPLETION_THREAD_SUCCESS | (renamed ? FILE_COMPLETION_THREAD_RENAMED : 0), (LPARAM)this) );
	return TRUE;
}

// 'End' of file completion, to avoid multi threading synchronization problems, this is to be invoked from within the
// main thread!
void CPartFile::PerformFileCompleteEnd(DWORD dwResult)
{
	bool isShared = theApp.sharedfiles && theApp.sharedfiles->GetFileByID(GetFileHash()) == this;	// [TPT] - SLUGFILLER: mergeKnown
	if (dwResult & FILE_COMPLETION_THREAD_SUCCESS)
	{
		// clear the blackbox to free up memory
		m_CorruptionBlackBox.Free(); // [TPT] - netfinity: Moved into main thread!

		if (isShared)	// [TPT] - SLUGFILLER: mergeKnown
			theApp.knownfiles->SafeAddKFile(this);
		theApp.downloadqueue->RemoveFile(this);
		theApp.mmserver->AddFinishedFile(this);
		if (thePrefs.GetRemoveFinishedDownloads())
			theApp.emuledlg->transferwnd->downloadlistctrl.RemoveFile(this);
		else
			UpdateDisplayedInfo(true);
		theApp.emuledlg->transferwnd->downloadlistctrl.ShowFilesCount();

		thePrefs.Add2DownCompletedFiles();
		thePrefs.Add2DownSessionCompletedFiles();
		thePrefs.SaveCompletedDownloadsStat();

		// 05-J�n-2004 [bc]: ed2k and Kad are already full of totally wrong and/or not properly attached meta data. Take
		// the chance to clean any available meta data tags and provide only tags which were determined by us.
		UpdateMetaDataTags();

		// republish that file to the ed2k-server to update the 'FT_COMPLETE_SOURCES' counter on the server.
		if (isShared)	// [TPT] - SLUGFILLER: mergeKnown
			theApp.sharedfiles->RepublishFile(this);
	
	// give visual response
		Log(LOG_SUCCESS | LOG_STATUSBAR, GetResString(IDS_DOWNLOADDONE), GetFileName());
		theApp.emuledlg->ShowNotifier(GetResString(IDS_TBN_DOWNLOADDONE) + _T('\n') + GetFileName(), TBN_DLOAD, GetFilePath());
		if (dwResult & FILE_COMPLETION_THREAD_RENAMED)
		{
			CString strFilePath(GetFullName());
			PathStripPath(strFilePath.GetBuffer());
			strFilePath.ReleaseBuffer();
			Log(LOG_STATUSBAR, GetResString(IDS_DOWNLOADRENAMED), strFilePath);
		}
	}

		theApp.downloadqueue->StartNextFileIfPrefs(GetCategory());
	
	// [TPT] - SLUGFILLER: mergeKnown
	if (!isShared)
		delete this;
	// [TPT] - SLUGFILLER: mergeKnown
}

// [TPT] - Code improvement
void  CPartFile::RemoveAllSources(bool bTryToSwap)
{	
	for( POSITION pos = srclist.GetHeadPosition(); pos != NULL; )
	{
		CUpDownClient * cur_src = srclist.GetNext(pos);
		if (bTryToSwap)
		{
			if (!cur_src->SwapToAnotherFile(_T("Removing source. CPartFile::RemoveAllSources()"), true, true, true, NULL, false, false) ) // ZZ:DownloadManager
				theApp.downloadqueue->RemoveSource(cur_src, false);
		}
		else
			theApp.downloadqueue->RemoveSource(cur_src, false);
	}
	UpdatePartsInfo(); 
	NewSrcIncPartsInfo(); // [TPT] - enkeyDev: ICS
	UpdateAvailablePartsCount();

	//[enkeyDEV(Ottavio84) -A4AF-]
	// remove all links A4AF in sources to this file
	if(!A4AFsrclist.IsEmpty())
	{
		// [TPT] - Code improvement
		for(POSITION pos = A4AFsrclist.GetHeadPosition(); pos !=NULL;)
		{
			CUpDownClient* cur_src = A4AFsrclist.GetNext(pos);
			
			POSITION pos3 = cur_src->m_OtherRequests_list.Find(this); 
			if(pos3)
			{ 
				cur_src->m_OtherRequests_list.RemoveAt(pos3);
				theApp.emuledlg->transferwnd->downloadlistctrl.RemoveSource(cur_src,this);
			}
			else{
				pos3 = cur_src->m_OtherNoNeeded_list.Find(this); 
				if(pos3)
				{ 
					cur_src->m_OtherNoNeeded_list.RemoveAt(pos3);
					theApp.emuledlg->transferwnd->downloadlistctrl.RemoveSource(cur_src,this);
				}
			}
		}
		A4AFsrclist.RemoveAll();
	}
	
	// [TPT] - SLUGFILLER: showComments remove - no per-client comments
}

void CPartFile::DeleteFile(){
	ASSERT ( !m_bPreviewing );

	// Barry - Need to tell any connected clients to stop sending the file
	StopFile(true);

	// feel free to implement a runtime handling mechanism!
	if (m_AllocateThread!=NULL) {
		LogWarning(LOG_STATUSBAR, GetResString(IDS_DELETEAFTERALLOC), GetFileName());
		m_bDeleteAfterAlloc=true;
		return;
	}

	theApp.sharedfiles->RemoveFile(this);
	theApp.downloadqueue->RemoveFile(this);
	theApp.emuledlg->transferwnd->downloadlistctrl.RemoveFile(this);

	if (m_hpartfile.m_hFile != INVALID_HANDLE_VALUE)
		m_hpartfile.Close();

	if (_tremove(m_fullname))
		LogError(LOG_STATUSBAR, GetResString(IDS_ERR_DELETE) + _T(" - ") + CString(_tcserror(errno)),m_fullname);	
	CString partfilename(RemoveFileExtension(m_fullname));
	if (_tremove(partfilename))
		LogError(LOG_STATUSBAR, GetResString(IDS_ERR_DELETE) + _T(" - ") + CString(_tcserror(errno)),partfilename);

	CString BAKName(m_fullname);
	BAKName.Append(PARTMET_BAK_EXT);
	if (_taccess(BAKName, 0) == 0 && !::DeleteFile(BAKName))
		LogError(LOG_STATUSBAR, GetResString(IDS_ERR_DELETE) + _T(" - ") + GetErrorMessage(GetLastError()), BAKName);

	// [TPT]
	// Maella -New Save/load Sources- enkeyDEV(Ottavio84)
	m_sourcesaver.DeleteFile();
	// Maella end


	BAKName = m_fullname;
	BAKName.Append(PARTMET_TMP_EXT);
	if (_taccess(BAKName, 0) == 0 && !::DeleteFile(BAKName))
		LogError(LOG_STATUSBAR, GetResString(IDS_ERR_DELETE) + _T(" - ") + GetErrorMessage(GetLastError()), BAKName);

	m_SettingsSaver.DeleteFile(this); // [TPT] - Sivka AutoHL
	
	delete this;
}

bool CPartFile::HashSinglePart(uint16 partnumber)
{
	if ((GetHashCount() <= partnumber) && (GetPartCount() > 1)){
		LogError(LOG_STATUSBAR, GetResString(IDS_ERR_HASHERRORWARNING), GetFileName());
		hashsetneeded = true;
		return true;
	}
	else if (!GetPartHash(partnumber) && GetPartCount() != 1){
		LogError(LOG_STATUSBAR, GetResString(IDS_ERR_INCOMPLETEHASH), GetFileName());
		hashsetneeded = true;
		return true;		
	}
	else{
		uchar hashresult[16];
		m_hpartfile.Seek((LONGLONG)PARTSIZE*partnumber,0);
		uint32 length = PARTSIZE;
		if ((ULONGLONG)PARTSIZE*(partnumber+1) > m_hpartfile.GetLength()){
			length = (m_hpartfile.GetLength() - ((ULONGLONG)PARTSIZE*partnumber));
			ASSERT( length <= PARTSIZE );
		}
		CreateHash(&m_hpartfile, length, hashresult, NULL);

		if (GetPartCount()>1 || GetFileSize()==PARTSIZE){
			if (md4cmp(hashresult,GetPartHash(partnumber)))
				return false;
			else
				return true;
		}
		else{
			if (md4cmp(hashresult,m_abyFileHash))
				return false;
			else
				return true;
		}
	}
}

bool CPartFile::IsCorruptedPart(uint16 partnumber) const
{
	return (corrupted_list.Find(partnumber) != NULL);
}

// [TPT] - Removed

bool CPartFile::IsPreviewableFileType() const {
    return IsArchive() || IsMovie();
}

void CPartFile::SetDownPriority(uint8 np, bool resort)
{
	//Changed the default resort to true. As it is was, we almost never sorted the download list when a priority changed.
	//If we don't keep the download list sorted, priority means nothing in downloadqueue.cpp->process().
	//Also, if we call this method with the same priotiry, don't do anything to help use less CPU cycles.
	if ( m_iDownPriority != np )
	{
		//We have a new priotiry
	if (np != PR_LOW && np != PR_NORMAL && np != PR_HIGH){
			//This should never happen.. Default to Normal.
		ASSERT(0);
		np = PR_NORMAL;
	}

	m_iDownPriority = np;
		//Some methods will change a batch of priorites then call these methods. 
	    if(resort) {
			//Sort the downloadqueue so contacting sources work correctly.
	theApp.downloadqueue->SortByPriority();
	theApp.downloadqueue->CheckDiskspaceTimed(); // SLUGFILLER: checkDiskspace
	    }
		//Update our display to show the new info based on our new priority.
	UpdateDisplayedInfo(true);
		//Save the partfile. We do this so that if we restart eMule before this files does
		//any transfers, it will remember the new priority.
	SavePartFile();
	}
}

bool CPartFile::CanOpenFile() const
{
	return (GetStatus()==PS_COMPLETE);
}

void CPartFile::OpenFile() const
{
	ShellOpenFile(GetFullName(), NULL);
}

bool CPartFile::CanStopFile() const
{
	bool bFileDone = (GetStatus()==PS_COMPLETE || GetStatus()==PS_COMPLETING);
	return (!IsStopped() && GetStatus()!=PS_ERROR && !bFileDone);
}

void CPartFile::StopFile(bool bCancel, bool resort)
{
	// Barry - Need to tell any connected clients to stop sending the file
	PauseFile(false, resort);
	m_LastSearchTimeKad = 0;
	m_TotalSearchesKad = 0;
	RemoveAllSources(true);

	m_MaxSourcesPerFile = thePrefs.GetMaxSourcePerFile(); // [TPT] - Sivka AutoHL

	paused = true;
	stopped = true;
	insufficient = false;
	m_nDownDatarate = 0; // [TPT]
	MEMZERO(m_anStates, sizeof(m_anStates));
	if (!bCancel)
		FlushBuffer(true);
	// [TPT] - WebCache			
	//JP Cancel Proxy Downloads
	CancelProxyDownloads();
	thePrefs.UpdateWebcacheReleaseAllowed(); //JP webcache release
	// [TPT] - WebCache	

    if(resort) {
	theApp.downloadqueue->SortByPriority();
	theApp.downloadqueue->CheckDiskspace();	// SLUGFILLER: checkDiskspace
    }
	UpdateDisplayedInfo(true);
}

void CPartFile::StopPausedFile()
{
	//Once an hour, remove any sources for files which are no longer active downloads
	EPartFileStatus uState = GetStatus();
	if( (uState==PS_PAUSED || uState==PS_INSUFFICIENT || uState==PS_ERROR) && !stopped && time(NULL) - m_iLastPausePurge > (60*60) )
	{
		StopFile();
	}
	else
	{
		if (m_bDeleteAfterAlloc && m_AllocateThread==NULL)
		{
			DeleteFile();
			return;
		}
	}
}

bool CPartFile::CanPauseFile() const
{
	bool bFileDone = (GetStatus()==PS_COMPLETE || GetStatus()==PS_COMPLETING);
	return (GetStatus()!=PS_PAUSED && GetStatus()!=PS_ERROR && !bFileDone);
}

void CPartFile::PauseFile(bool bInsufficient, bool resort)
{
	// if file is already in 'insufficient' state, don't set it again to insufficient. this may happen if a disk full
	// condition is thrown before the automatically and periodically check free diskspace was done.
	if (bInsufficient && insufficient)
		return;

	// if file is already in 'paused' or 'insufficient' state, do not refresh the purge time
	if (!paused && !insufficient)
		m_iLastPausePurge = time(NULL);
	theApp.downloadqueue->RemoveLocalServerRequest(this);

	if(GetKadFileSearchID())
	{
		Kademlia::CSearchManager::stopSearch(GetKadFileSearchID(), true);
		m_LastSearchTimeKad = 0; //If we were in the middle of searching, reset timer so they can resume searching.
	}

	SetActive(false);

	if (status==PS_COMPLETE || status==PS_COMPLETING)
		return;

	// [TPT] - WebCache			
	PauseProxyDownloads(); //JP Pause Proxy Downloads
	// [TPT] - WebCache	

	Packet* packet = new Packet(OP_CANCELTRANSFER,0);
	for( POSITION pos = srclist.GetHeadPosition(); pos != NULL; )
	{
		CUpDownClient* cur_src = srclist.GetNext(pos);
		if (cur_src->GetDownloadState() == DS_DOWNLOADING)
		{
			// [TPT] - WebCache	
			if( cur_src->IsProxy() ) { // yonatan http - quick fix - WC-TODO: !!!
                SINGLEProxyClient->SetDownloadState(DS_NONE);
				SINGLEProxyClient->SetWebCacheDownState( WCDS_NONE );
				//jp stalled proxy-download on paused file fix attempt
				if (SINGLEProxyClient->ProxyClientIsBusy())
				{
					SINGLEProxyClient->DeleteBlock(); // so SingleProxyClient is not busy any more
				}
// JP taken care of in WCProxyClient::UpdateClient or WCProxyClient::WCProxyClient
//				else
//				{
//					if( SINGLEProxyClient->m_pWCDownSocket ) // if we get a 504 is the socket already deleted?
//						SINGLEProxyClient->m_pWCDownSocket->Safe_Delete(); // should this even be here?
//				}
				WebCachedBlockList.TryToDL(); //JP if we reach this point SingleProxyClient can't be busy
			} else {
			//webcache end
				cur_src->SendCancelTransfer(packet);
				cur_src->SetDownloadState(DS_ONQUEUE, _T("You cancelled the download. Sending OP_CANCELTRANSFER"), CUpDownClient::DSR_PAUSED); // [TPT] - Maella -Download Stop Reason-
			}
			// [TPT] - WebCache	
		}
	}
	delete packet;

	if (bInsufficient)
	{
		LogError(LOG_STATUSBAR, _T("Insufficient diskspace - pausing download of \"%s\""), GetFileName());
		insufficient = true;
	}
	else
	{
		paused = true;
		insufficient = false;
	}
	NotifyStatusChange();
	m_nDownDatarate = 0; // [TPT]
	m_anStates[DS_DOWNLOADING] = 0; // -khaos--+++> Renamed var.
	if (!bInsufficient)
	{
        if(resort) {
		theApp.downloadqueue->SortByPriority();
		theApp.downloadqueue->CheckDiskspace(); // SLUGFILLER: checkDiskspace
        }
		SavePartFile();
	}
	UpdateDisplayedInfo(true);
}

bool CPartFile::CanResumeFile() const
{
	return (GetStatus()==PS_PAUSED || GetStatus()==PS_INSUFFICIENT || (GetStatus()==PS_ERROR && GetCompletionError()));
}

void CPartFile::ResumeFile(bool resort)
{
	if (status==PS_COMPLETE || status==PS_COMPLETING)
		return;
	if (status==PS_ERROR && m_bCompletionError){
		ASSERT( gaplist.IsEmpty() );
		if (gaplist.IsEmpty()){
			// rehashing the file could probably be avoided, but better be in the safe side..
			m_bCompletionError = false;
			CompleteFile(false);
		}
		return;
	}
	paused = false;
	stopped = false;
	SetActive(theApp.IsConnected());
	m_LastSearchTime = 0;

	// [TPT] - WebCache		
	ResumeProxyDownloads(); //JP Resume Proxy Downloads
	// [TPT] - WebCache	

	theApp.downloadqueue->timeAllowUnlimitedUP = ::GetTickCount();//Reset the time
    if(resort) {
	theApp.downloadqueue->SortByPriority();
	theApp.downloadqueue->CheckDiskspace(); // SLUGFILLER: checkDiskspace
    }
	SavePartFile();
	NotifyStatusChange();

	UpdateDisplayedInfo(true);
// [TPT] - WebCache
	thePrefs.UpdateWebcacheReleaseAllowed(); //jp webcache release
}

// SLUGFILLER: checkDiskspace
void CPartFile::ResumeFileInsufficient()
{
	if (status==PS_COMPLETE || status==PS_COMPLETING)
		return;
	if (!insufficient)
		return;
	AddLogLine(false, _T("Resuming download of \"%s\""), GetFileName());
	insufficient = false;
	SetActive(theApp.IsConnected());
	// [TPT] - WebCache		
	ResumeProxyDownloads(); //JP Resume Proxy Downloads
	// [TPT] - WebCache	
	theApp.downloadqueue->timeAllowUnlimitedUP = ::GetTickCount();//Reset the time
	m_LastSearchTime = 0;
	UpdateDisplayedInfo(true);
	// [TPT] - WebCache	
	thePrefs.UpdateWebcacheReleaseAllowed(); //jp webcache release
	// [TPT] - WebCache	
}
// SLUGFILLER: checkDiskspace

CString CPartFile::getPartfileStatus() const
{
	switch(GetStatus()){
		case PS_HASHING:
		case PS_WAITINGFORHASH:
			return GetResString(IDS_HASHING);

		case PS_COMPLETING:{
			CString strState = GetResString(IDS_COMPLETING);
			if (GetFileOp() == PFOP_HASHING)
				strState += _T(" (") + GetResString(IDS_HASHING) + _T(")");
			else if (GetFileOp() == PFOP_COPYING)
				strState += _T(" (Copying)");
			else if (GetFileOp() == PFOP_UNCOMPRESSING)
				strState += _T(" (Uncompressing)");
			return strState;
		}

		case PS_COMPLETE:
			return GetResString(IDS_COMPLETE);

		case PS_PAUSED:
			if (stopped)
				return GetResString(IDS_STOPPED);
			return GetResString(IDS_PAUSED);

		case PS_INSUFFICIENT:
			return GetResString(IDS_INSUFFICIENT);

		case PS_ERROR:
			if (m_bCompletionError)
				return GetResString(IDS_INSUFFICIENT);
			return GetResString(IDS_ERRORLIKE);
	}

	if (GetSrcStatisticsValue(DS_DOWNLOADING) > 0)
		return GetResString(IDS_DOWNLOADING);
	else
		return GetResString(IDS_WAITING);
} 

int CPartFile::getPartfileStatusRang() const
{
	switch (GetStatus()) {
		case PS_HASHING: 
		case PS_WAITINGFORHASH:
			return 7;

		case PS_COMPLETING:
			return 1;

		case PS_COMPLETE:
			return 0;

		case PS_PAUSED:
			if (IsStopped())
				return 6;
			else
			return 5;
		case PS_INSUFFICIENT:
			return 4;

		case PS_ERROR:
			return 8;
	}
	if (GetSrcStatisticsValue(DS_DOWNLOADING) == 0)
		return 3; // waiting?
	return 2; // downloading?
} 

time_t CPartFile::getTimeRemainingSimple() const
{
	if (GetDownloadFileDatarate()==0) return -1; // [TPT]
	
	return( (GetFileSize()-GetCompletedSize())/ GetDownloadFileDatarate()); // [TPT]
}

time_t CPartFile::getTimeRemaining() const
{
	uint32 completesize = GetCompletedSize();
	time_t simple = -1;
	time_t estimate = -1;
	if( GetDownloadFileDatarate() > 0 ) // [TPT] - Maella
	{
		simple = (GetFileSize() - completesize) / GetDownloadFileDatarate(); // [TPT] - Maella
	}
	if( GetDlActiveTime() && completesize >= 512000 )
		estimate = ((GetFileSize()-completesize)/((float)completesize/(float)GetDlActiveTime()));
		
	if( simple == -1 )
		{
		//We are not transferring at the moment.
		if( estimate == -1 )
			//We also don't have enough data to guess
			return -1;
		else if( estimate > HR2S(24*15) )
			//The estimate is too high
			return -1;
	else
			return estimate;
		}
	else if( estimate == -1 )
	{
		//We are transferring but estimate doesn't have enough data to guess
			return simple;
	}
	if( simple < estimate )
		return simple;
	if( estimate > HR2S(24*15) )
		//The estimate is too high..
	return -1;
	return estimate;
}

void CPartFile::PreviewFile()
{
	if (IsArchive(true)){
		if (!m_bRecoveringArchive && !m_bPreviewing)
			CArchiveRecovery::recover(this, true, thePrefs.GetPreviewCopiedArchives());
		return;
	}

	if (!IsReadyForPreview()){
		ASSERT( false );
		return;
	}

	if (thePrefs.IsMoviePreviewBackup()){
		m_bPreviewing = true;
		CPreviewThread* pThread = (CPreviewThread*) AfxBeginThread(RUNTIME_CLASS(CPreviewThread), THREAD_PRIORITY_NORMAL,0, CREATE_SUSPENDED);
		pThread->SetValues(this,thePrefs.GetVideoPlayer());
		pThread->ResumeThread();
	}
	else{
		CString strLine = GetFullName();
		
		// strip available ".met" extension to get the part file name.
		if (strLine.GetLength()>4 && strLine.Right(4)==_T(".met"))
			strLine.Delete(strLine.GetLength()-4,4);

		// if the path contains spaces, quote the entire path
		if (strLine.Find(_T(' ')) != -1)
			strLine = _T('\"') + strLine + _T('\"');

		if (!thePrefs.GetVideoPlayer().IsEmpty())
		{
			// get directory of video player application
			CString path = thePrefs.GetVideoPlayer();
			int pos = path.ReverseFind(_T('\\'));
			if (pos == -1)
				path.Empty();
			else
				path = path.Left(pos + 1);

			if (thePrefs.GetPreviewSmallBlocks())
				FlushBuffer(true);

			ShellExecute(NULL, _T("open"), thePrefs.GetVideoPlayer(), strLine, path, SW_SHOWNORMAL);
		}
		else
			ShellExecute(NULL, _T("open"), strLine, NULL, NULL, SW_SHOWNORMAL);
	}
}

bool CPartFile::IsReadyForPreview() const
{
	// Barry - Allow preview of archives of any length > 1k
	if (IsArchive(true))
	{
		//if (GetStatus() != PS_COMPLETE && GetStatus() != PS_COMPLETING 
		//	&& GetFileSize()>1024 && GetCompletedSize()>1024 
		//	&& !m_bRecoveringArchive 
		//	&& GetFreeDiskSpaceX(thePrefs.GetTempDir())+100000000 > 2*GetFileSize())
		//	return true;

		// check part file state
	    EPartFileStatus uState = GetStatus();
		if (uState == PS_COMPLETE || uState == PS_COMPLETING)
			return false;

		// check part file size(s)
		if (GetFileSize() < 1024 || GetCompletedSize() < 1024)
			return false;

		// check if we already trying to recover an archive file from this part file
		if (m_bRecoveringArchive)
			return false;

		// check free disk space
		UINT uMinFreeDiskSpace = (thePrefs.IsCheckDiskspaceEnabled() && thePrefs.GetMinFreeDiskSpace() > 0)
									? thePrefs.GetMinFreeDiskSpace()
									: 20*1024*1024;
		if (thePrefs.GetPreviewCopiedArchives())
			uMinFreeDiskSpace += GetFileSize()*2;
		else 
			uMinFreeDiskSpace += GetCompletedSize() + 16*1024;
		if (GetFreeDiskSpaceX(thePrefs.GetTempDir()) < uMinFreeDiskSpace)
			return false;
		return true; 
	}

	if (thePrefs.IsMoviePreviewBackup())
	{
		return !( (GetStatus() != PS_READY && GetStatus() != PS_PAUSED) 
				|| m_bPreviewing || GetPartCount() < 5 || !IsMovie() || (GetFreeDiskSpaceX(thePrefs.GetTempDir()) + 100000000) < GetFileSize()
				|| ( !IsPartShareable(0) || !IsPartShareable(GetPartCount()-1)));	// [TPT] - SLUGFILLER: SafeHash - only play hashed parts
	}
	else
	{
		TCHAR szVideoPlayerFileName[_MAX_FNAME];
		_tsplitpath(thePrefs.GetVideoPlayer(), NULL, NULL, szVideoPlayerFileName, NULL);

		// enable the preview command if the according option is specified 'PreviewSmallBlocks' 
		// or if VideoLAN client is specified
		if (thePrefs.GetPreviewSmallBlocks() || !_tcsicmp(szVideoPlayerFileName, _T("vlc")))
		{
		    if (m_bPreviewing)
			    return false;

		    EPartFileStatus uState = GetStatus();
			if (!(uState == PS_READY || uState == PS_EMPTY || uState == PS_PAUSED || uState == PS_INSUFFICIENT))
			    return false;

			// default: check the ED2K file format to be of type audio, video or CD image. 
			// but because this could disable the preview command for some file types which eMule does not know,
			// this test can be avoided by specifying 'PreviewSmallBlocks=2'
			if (thePrefs.GetPreviewSmallBlocks() <= 1)
			{
				// check the file extension
				EED2KFileType eFileType = GetED2KFileTypeID(GetFileName());
				if (!(eFileType == ED2KFT_VIDEO || eFileType == ED2KFT_AUDIO || eFileType == ED2KFT_CDIMAGE))
				{
					// check the ED2K file type
					const CString& rstrED2KFileType = GetStrTagValue(FT_FILETYPE);
					if (rstrED2KFileType.IsEmpty() || !(!_tcscmp(rstrED2KFileType, _T(ED2KFTSTR_AUDIO)) || !_tcscmp(rstrED2KFileType, _T(ED2KFTSTR_VIDEO))))
						return false;
				}
			}

		    // If it's an MPEG file, VLC is even capable of showing parts of the file if the beginning of the file is missing!
		    bool bMPEG = false;
		    LPCTSTR pszExt = _tcsrchr(GetFileName(), _T('.'));
		    if (pszExt != NULL){
			    CString strExt(pszExt);
			    strExt.MakeLower();
			    bMPEG = (strExt==_T(".mpg") || strExt==_T(".mpeg") || strExt==_T(".mpe") || strExt==_T(".mp3") || strExt==_T(".mp2") || strExt==_T(".mpa"));
		    }

		    if (bMPEG){
			    // TODO: search a block which is at least 16K (Audio) or 256K (Video)
			    if (GetCompletedSize() < 16*1024)
				    return false;
		    }
		    else{
			    // For AVI files it depends on the used codec..
				if (thePrefs.GetPreviewSmallBlocks() >= 2){
					if (GetCompletedSize() < 256*1024)
						return false;
				}
				else{
			    if (!IsComplete(0, 256*1024))
				    return false;
		    }
			}
    
		    return true;
		}
		else{
		    return !((GetStatus() != PS_READY && GetStatus() != PS_PAUSED) 
				    || m_bPreviewing || GetPartCount() < 2 || !IsMovie() || !IsPartShareable(0)); //[TPT] - SLUGFILLER - Safehash
		}
	}
}

void CPartFile::UpdateAvailablePartsCount(){
	UINT availablecounter = 0;
	UINT iPartCount = GetPartCount();
	for (UINT ixPart = 0; ixPart < iPartCount; ixPart++){
		// [TPT] - netfinity: Anti HideOS
		for(POSITION pos = srclist.GetHeadPosition(); pos; )
		{
			CUpDownClient* cur_client = srclist.GetNext(pos);
			if (cur_client->IsPartAvailable(ixPart) || 
				(cur_client->IsPartialSource() && cur_client->m_abySeenPartStatus && cur_client->m_abySeenPartStatus[ixPart])){
				availablecounter++; 
				break;
			}
		}
		// [TPT] - netfinity: Anti HideOS
	}
	if (iPartCount == availablecounter && availablePartsCount < iPartCount)
		lastseencomplete = CTime::GetCurrentTime();
	availablePartsCount = availablecounter;
}

Packet* CPartFile::CreateSrcInfoPacket(CUpDownClient* forClient) const
{
	//We need to find where the dangling pointers are before uncommenting this..
	if(!IsPartFile())
		return CKnownFile::CreateSrcInfoPacket(forClient);

	if (forClient->GetRequestFile() != this)
		return NULL;

	// [TPT] - Maella -CPartFile::CreateSrcInfoPacket-
	if(GetSourceCount() == 0) return NULL;
	// Maella end

	if ( !(GetStatus() == PS_READY || GetStatus() == PS_EMPTY))
		return NULL;

	if (srclist.IsEmpty())
		return NULL;

	CSafeMemFile data(1024);
	uint16 nCount = 0;

	data.WriteHash16(m_abyFileHash);
	data.WriteUInt16(nCount);
	bool bNeeded;
	for (POSITION pos = srclist.GetHeadPosition();pos != 0;){
		bNeeded = false;
		CUpDownClient* cur_src = srclist.GetNext(pos);
		// [TPT] - WebCache	
		if (cur_src->HasLowID() || !cur_src->IsValidSource() || cur_src->IsProxy()) // Superlexx - webcache
			continue;
		if (cur_src->SupportsWebCache()) // Superlexx - webcache
			bNeeded = true;
		// [TPT] - Maella
		if (!bNeeded) // Superlexx - webcache
		{
		// [TPT] - WebCache	
			try {
				// only send source which have needed parts for this client if possible
				if (forClient->GetRequestFile() == this && forClient->GetPartStatus() != NULL && cur_src->GetPartStatus() != NULL)
				{
					// Maella -Code Improvement-
					const uint8* reqStatus = forClient->GetPartStatus();
					const uint8* srcStatus = cur_src->GetPartStatus();			
					const uint16 partCount = GetPartCount();
					
					// One of these parameters can change....But which one?
					if(cur_src->GetRequestFile() != this)
					{
						ASSERT(false);
						continue;
					}
					
					if(forClient->GetRequestFile() != this)
					{
						ASSERT(false);
						continue;
					}
					
					if(cur_src->GetPartCount() != partCount)
					{
						ASSERT(false);
						continue;
					}
					
					if(forClient->GetPartCount() != partCount)
					{
						ASSERT(false);
						continue;
					}
					
					if(reqStatus == NULL)
					{
						ASSERT(false);
						continue;
					}
	
					if(srcStatus == NULL)
					{
						ASSERT(false);
						continue;
					}
	
					for (uint16 x = 0; x != partCount; x++)
					{
						if (srcStatus[x] && !reqStatus[x])
						{
							bNeeded = true;
							break;
						}
					}				
				}
			}
			catch(...)
			{
				CString error; 
				error.Format(_T("Exception in %s() at line %u"), __FUNCTION__, __LINE__);
				AddDebugLogLine(false, error);
				bNeeded = false;
			}
		} // [TPT] - WebCache	
		// [TPT] - Maella end

		if( bNeeded ){
			nCount++;
			uint32 dwID;
			if(forClient->GetSourceExchangeVersion() > 2)
				dwID = cur_src->GetUserIDHybrid();
			else
				dwID = ntohl(cur_src->GetUserIDHybrid());
			data.WriteUInt32(dwID);
			data.WriteUInt16(cur_src->GetUserPort());
			data.WriteUInt32(cur_src->GetServerIP());
			data.WriteUInt16(cur_src->GetServerPort());
			if (forClient->GetSourceExchangeVersion() > 1)
				data.WriteHash16(cur_src->GetUserHash());
			if (nCount > 500)
				break;
		}
	}
	if (!nCount)
		return 0;
	data.Seek(16, 0);
	data.WriteUInt16(nCount);

	Packet* result = new Packet(&data, OP_EMULEPROT);
	result->opcode = OP_ANSWERSOURCES;
	// 16+2+501*(4+2+4+2+16) = 14046 bytes max.
	if ( result->size > 354 )
		result->PackPacket();
	if (thePrefs.GetDebugSourceExchange())
		AddDebugLogLine(false, _T("SXSend: Client source response; Count=%u, %s, File=\"%s\""), nCount, forClient->DbgGetClientInfo(), GetFileName());
	return result;
}

void CPartFile::AddClientSources(CSafeMemFile* sources, uint8 sourceexchangeversion, CUpDownClient* pClient)
{
	if (stopped)
		return;

	UINT nCount = sources->ReadUInt16();

	if (thePrefs.GetDebugSourceExchange()){
		CString strSrc;
		if (pClient)
			strSrc.Format(_T(", %s"), pClient->DbgGetClientInfo());
		AddDebugLogLine(false, _T("SXRecv: Client source response; Count=%u, %sFile=\"%s\""), nCount, strSrc, GetFileName());
	}

	// Check if the data size matches the 'nCount' for v1 or v2 and eventually correct the source
	// exchange version while reading the packet data. Otherwise we could experience a higher
	// chance in dealing with wrong source data, userhashs and finally duplicate sources.
	UINT uDataSize = sources->GetLength() - sources->GetPosition();
	//Checks if version 1 packet is correct size
	if (nCount*(4+2+4+2) == uDataSize)
	{
		if( sourceexchangeversion != 1 )
			return;
	}
	//Checks if version 2&3 packet is correct size
	else if (nCount*(4+2+4+2+16) == uDataSize)
	{
		if( sourceexchangeversion == 1 )
			return;
	}
	else
	{
		// If v4 inserts additional data (like v2), the above code will correctly filter those packets.
		// If v4 appends additional data after <count>(<Sources>)[count], we are in trouble with the 
		// above code. Though a client which does not understand v4+ should never receive such a packet.
		if (thePrefs.GetVerbose())
			DebugLogWarning(_T("Received invalid source exchange packet (v%u) of data size %u for %s"), sourceexchangeversion, uDataSize, GetFileName());
		return;
	}

	for (UINT i = 0; i < nCount; i++)
	{
		uint32 dwID = sources->ReadUInt32();
		uint16 nPort = sources->ReadUInt16();
		uint32 dwServerIP = sources->ReadUInt32();
		uint16 nServerPort = sources->ReadUInt16();

		uchar achUserHash[16];
		if (sourceexchangeversion > 1)
			sources->ReadHash16(achUserHash);

		//Clients send ID's the the Hyrbid format so highID clients with *.*.*.0 won't be falsely switched to a lowID..
		if (sourceexchangeversion == 3)
		{
			uint32 dwIDED2K = ntohl(dwID);

			// check the HighID(IP) - "Filter LAN IPs" and "IPfilter" the received sources IP addresses
			if (!IsLowID(dwID))
			{
				if (!IsGoodIP(dwIDED2K))
				{
					// check for 0-IP, localhost and optionally for LAN addresses
					if (thePrefs.GetLogFilteredIPs())
						AddDebugLogLine(false, _T("Ignored source (IP=%s) received via source exchange - bad IP"), ipstr(dwIDED2K));
					continue;
				}
				if (theApp.ipfilter->IsFiltered(dwIDED2K))
				{
					if (thePrefs.GetLogFilteredIPs())
						AddDebugLogLine(false, _T("Ignored source (IP=%s) received via source exchange - IP filter (%s)"), ipstr(dwIDED2K), theApp.ipfilter->GetLastHit());
					continue;
				}
				if (theApp.clientlist->IsSnafuClient(dwIDED2K)){ // [TPT] - eWombat
#ifdef _DEBUG
					if (thePrefs.GetLogBannedClients()){
						CUpDownClient* pClient = theApp.clientlist->FindClientByIP(dwIDED2K);
						AddDebugLogLine(false, _T("Ignored source (IP=%s) received via source exchange - banned client %s"), ipstr(dwIDED2K), pClient->DbgGetClientInfo());
					}
#endif
					continue;
				}
			}

			// additionally check for LowID and own IP
			if (!CanAddSource(dwID, nPort, dwServerIP, nServerPort, NULL, false))
			{
				if (thePrefs.GetLogFilteredIPs())
					AddDebugLogLine(false, _T("Ignored source (IP=%s) received via source exchange"), ipstr(dwIDED2K));
				continue;
			}
		}
		else
		{
			// check the HighID(IP) - "Filter LAN IPs" and "IPfilter" the received sources IP addresses
			if (!IsLowID(dwID))
			{
				if (!IsGoodIP(dwID))
				{ 
					// check for 0-IP, localhost and optionally for LAN addresses
					if (thePrefs.GetLogFilteredIPs())
						AddDebugLogLine(false, _T("Ignored source (IP=%s) received via source exchange - bad IP"), ipstr(dwID));
					continue;
				}
				if (theApp.ipfilter->IsFiltered(dwID))
				{
					if (thePrefs.GetLogFilteredIPs())
						AddDebugLogLine(false, _T("Ignored source (IP=%s) received via source exchange - IP filter (%s)"), ipstr(dwID), theApp.ipfilter->GetLastHit());
					continue;
				}
				if (theApp.clientlist->IsSnafuClient(dwID)){ // [TPT] - eWombat
#ifdef _DEBUG
					if (thePrefs.GetLogBannedClients()){
						CUpDownClient* pClient = theApp.clientlist->FindClientByIP(dwID);
						AddDebugLogLine(false, _T("Ignored source (IP=%s) received via source exchange - banned client %s"), ipstr(dwID), pClient->DbgGetClientInfo());
					}
#endif
					continue;
				}
			}

			// additionally check for LowID and own IP
			if (!CanAddSource(dwID, nPort, dwServerIP, nServerPort))
			{
				if (thePrefs.GetLogFilteredIPs())
					AddDebugLogLine(false, _T("Ignored source (IP=%s) received via source exchange"), ipstr(dwID));
				continue;
			}
		}

		// [TPT] - Sivka AutoHL
		if( this->m_MaxSourcesPerFile  > GetSourceCount() )
		{
			CUpDownClient* newsource;
			if (sourceexchangeversion == 3)
				newsource = new CUpDownClient(this,nPort,dwID,dwServerIP,nServerPort,false);
			else
				newsource = new CUpDownClient(this,nPort,dwID,dwServerIP,nServerPort,true);
			if (sourceexchangeversion > 1)
				newsource->SetUserHash(achUserHash);
			newsource->SetSourceFrom(SF_SOURCE_EXCHANGE);
			theApp.downloadqueue->CheckAndAddSource(this,newsource);
		} 
		else
			break;
	}
}

// making this function return a higher when more sources have the extended
// protocol will force you to ask a larger variety of people for sources
/*int CPartFile::GetCommonFilePenalty() const
{
	//TODO: implement, but never return less than MINCOMMONPENALTY!
	return MINCOMMONPENALTY;
}
*/
/* Barry - Replaces BlockReceived() 

           Originally this only wrote to disk when a full 180k block 
           had been received from a client, and only asked for data in 
		   180k blocks.

		   This meant that on average 90k was lost for every connection
		   to a client data source. That is a lot of wasted data.

		   To reduce the lost data, packets are now written to a buffer
		   and flushed to disk regularly regardless of size downloaded.
		   This includes compressed packets.

		   Data is also requested only where gaps are, not in 180k blocks.
		   The requests will still not exceed 180k, but may be smaller to
		   fill a gap.
*/
uint32 CPartFile::WriteToBuffer(uint32 transize, const BYTE *data, uint32 start, uint32 end, Requested_Block_Struct *block, 
								const CUpDownClient* client)
{
	ASSERT( (int)transize > 0 );
	ASSERT( start <= end );

	// Increment transferred bytes counter for this file
	m_uTransferred += transize;

	// This is needed a few times
	uint32 lenData = end - start + 1;
	ASSERT( (int)lenData > 0 );

	if( lenData > transize ) {
		m_uCompressionGain += lenData - transize;
		thePrefs.Add2SavedFromCompression(lenData-transize);
	}

	// Occasionally packets are duplicated, no point writing it twice
	if (IsComplete(start, end))
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, _T("PrcBlkPkt: Already written block %s; File=%s; %s"), DbgGetBlockInfo(start, end), GetFileName(), client->DbgGetClientInfo());
		return 0;
	}

	// log transferinformation in our "blackbox"
	m_CorruptionBlackBox.TransferredData(start, end, client);

	// Create copy of data as new buffer
	BYTE *buffer = new BYTE[lenData];
	MEMCOPY(buffer, data, lenData);

	// Create a new buffered queue entry
	PartFileBufferedData *item = new PartFileBufferedData;
	item->data = buffer;
	item->start = start;
	item->end = end;
	item->block = block;

	// Add to the queue in the correct position (most likely the end)
	PartFileBufferedData *queueItem;
	bool added = false;
	POSITION pos = m_BufferedData_list.GetTailPosition();
	while (pos != NULL)
	{
		POSITION posLast = pos;
		queueItem = m_BufferedData_list.GetPrev(pos);
		if (item->end > queueItem->end)
		{
			added = true;
			m_BufferedData_list.InsertAfter(posLast, item);
			break;
		}
	}
	if (!added)
		m_BufferedData_list.AddHead(item);

	// Increment buffer size marker
	m_nTotalBufferData += lenData;

	// Mark this small section of the file as filled
	FillGap(item->start, item->end);

	// Update the flushed mark on the requested block 
	// The loop here is unfortunate but necessary to detect deleted blocks.
	pos = requestedblocks_list.GetHeadPosition();
	while (pos != NULL)
	{	
		if (requestedblocks_list.GetNext(pos) == item->block)
			item->block->transferred += lenData;
	}

	if (gaplist.IsEmpty())
		FlushBuffer(true);

	// Return the length of data written to the buffer
	return lenData;
}

void CPartFile::FlushBuffer(bool forcewait, bool bForceICH, bool bNoAICH)
{
	bool bIncreasedFile=false;

	m_nLastBufferFlushTime = GetTickCount();
	if (m_BufferedData_list.IsEmpty())
		return;

	if (m_AllocateThread!=NULL) {
		// diskspace is being allocated right now.
		// so dont write and keep the data in the buffer for later.
		return;
	}else if (m_iAllocinfo>0) {
		bIncreasedFile=true;
		m_iAllocinfo=0;
	}

	//if (thePrefs.GetVerbose())
	//	AddDebugLogLine(false, _T("Flushing file %s - buffer size = %ld bytes (%ld queued items) transferred = %ld [time = %ld]\n"), GetFileName(), m_nTotalBufferData, m_BufferedData_list.GetCount(), m_uTransferred, m_nLastBufferFlushTime);

	uint32 partCount = GetPartCount();
	bool *changedPart = new bool[partCount];
	// Remember which parts need to be checked at the end of the flush
	for (int partNumber=0; (uint32)partNumber<partCount; partNumber++)
		changedPart[partNumber] = false;

	try
	{
		bool bCheckDiskspace = thePrefs.IsCheckDiskspaceEnabled() && thePrefs.GetMinFreeDiskSpace() > 0;
		ULONGLONG uFreeDiskSpace = bCheckDiskspace ? GetFreeDiskSpaceX(thePrefs.GetTempDir()) : 0;

		// Check free diskspace for compressed/sparse files before possibly increasing the file size
		if (bCheckDiskspace && !IsNormalFile())
		{
			// Compressed/sparse files; regardless whether the file is increased in size, 
			// check the amount of data which will be written
			// would need to use disk cluster sizes for more accuracy
			if (m_nTotalBufferData + thePrefs.GetMinFreeDiskSpace() >= uFreeDiskSpace)
				AfxThrowFileException(CFileException::diskFull, 0, m_hpartfile.GetFileName());
		}

		// Ensure file is big enough to write data to (the last item will be the furthest from the start)
		PartFileBufferedData *item = m_BufferedData_list.GetTail();
		if (m_hpartfile.GetLength() <= item->end)
		{
			// Check free diskspace for normal files before increasing the file size
			if (bCheckDiskspace && IsNormalFile())
			{
				// Normal files; check if increasing the file would reduce the amount of min. free space beyond the limit
				// would need to use disk cluster sizes for more accuracy
				ULONGLONG uIncrease = item->end + 1 - m_hpartfile.GetLength();
				if (uIncrease + thePrefs.GetMinFreeDiskSpace() >= uFreeDiskSpace)
					AfxThrowFileException(CFileException::diskFull, 0, m_hpartfile.GetFileName());
			}

			if (!IsNormalFile() || item->end-m_hpartfile.GetLength() < 2097152) 
				forcewait=true;	// <2MB -> alloc it at once

			// Allocate filesize
			if (!forcewait) {
				m_AllocateThread= AfxBeginThread(AllocateSpaceThread, this, THREAD_PRIORITY_LOWEST, 0, CREATE_SUSPENDED);
				if (m_AllocateThread == NULL)
				{
					TRACE(_T("Failed to create alloc thread! -> allocate blocking\n"));
					forcewait=true;
				} else {
					m_iAllocinfo=item->end+1;
					m_AllocateThread->ResumeThread();
					delete[] changedPart;
					return;
				}
			}
			
			if (forcewait) {
				bIncreasedFile=true;
				// If this is a NTFS compressed file and the current block is the 1st one to be written and there is not 
				// enough free disk space to hold the entire *uncompressed* file, windows throws a 'diskFull'!?
				if (IsNormalFile())
					m_hpartfile.SetLength(item->end+1); // allocate disk space (may throw 'diskFull')
			}
		}

		// Loop through queue
		for (int i = m_BufferedData_list.GetCount(); i>0; i--)
		{
			// Get top item
			item = m_BufferedData_list.GetHead();

			// This is needed a few times
			uint32 lenData = item->end - item->start + 1;

			// SLUGFILLER: SafeHash - could be more than one part
			for (uint32 curpart = item->start/PARTSIZE; curpart <= item->end/PARTSIZE; curpart++)
				changedPart[curpart] = true;
			// SLUGFILLER: SafeHash

			// Go to the correct position in file and write block of data
			m_hpartfile.Seek(item->start, CFile::begin);
			m_hpartfile.Write(item->data, lenData);

			// Remove item from queue
			m_BufferedData_list.RemoveHead();

			// Decrease buffer size
			m_nTotalBufferData -= lenData;

			// Release memory used by this item
			delete [] item->data;
			delete item;
		}

		// Partfile should never be too large
		// [TPT]
		if (m_hpartfile.GetLength() > GetFileSize()){
			// it's "last chance" correction. the real bugfix has to be applied 'somewhere' else
			TRACE(_T("Partfile \"%s\" is too large! Truncating %I64u bytes.\n"), GetFileName(), m_hpartfile.GetLength() - GetFileSize());
			m_hpartfile.SetLength(GetFileSize());
		}

		// Flush to disk
		m_hpartfile.Flush();

		// Check each part of the file
		uint32 partRange = (m_hpartfile.GetLength() % PARTSIZE > 0)? (m_hpartfile.GetLength() % PARTSIZE) - 1 : (PARTSIZE - 1);
		for (int partNumber = partCount-1; partNumber >= 0; partNumber--)
		{
			if (changedPart[partNumber] == false)
			{
				// Any parts other than last must be full size
				partRange = PARTSIZE - 1;
				continue;
			}

			// Is this 9MB part complete
			if (IsComplete(PARTSIZE * partNumber, (PARTSIZE * (partNumber + 1)) - 1))
			{
				// Is part corrupt
				if (!HashSinglePart(partNumber))
				{
					LogWarning(LOG_STATUSBAR, GetResString(IDS_ERR_PARTCORRUPT), partNumber, GetFileName());
					AddGap(PARTSIZE*partNumber, PARTSIZE*partNumber + partRange);
					//[TPT] START - Added by SiRoB, SafeHash
                    			m_PartsShareable[partNumber] = false;
					//[TPT] END   - Added by SiRoB, SafeHash
					// add part to corrupted list, if not already there
					if (!IsCorruptedPart(partNumber))
						corrupted_list.AddTail(partNumber);

					// request AICH recovery data
					if (!bNoAICH)
						RequestAICHRecovery((uint16)partNumber);

					// update stats
					m_uCorruptionLoss += (partRange + 1);
					thePrefs.Add2LostFromCorruption(partRange + 1);
				}
				else
				{
					if (!hashsetneeded){
						if (thePrefs.GetVerbose())
							AddDebugLogLine(DLP_VERYLOW, false, _T("Finished part %u of \"%s\""), partNumber, GetFileName());
					}

					// tell the blackbox about the verified data
					m_CorruptionBlackBox.VerifiedData(PARTSIZE * partNumber, PARTSIZE*partNumber + partRange);

					// if this part was successfully completed (although ICH is active), remove from corrupted list
					POSITION posCorrupted = corrupted_list.Find(partNumber);
					if (posCorrupted)
						corrupted_list.RemoveAt(posCorrupted);
					//[TPT] START - Added by SiRoB, SafeHash
                    			m_PartsShareable[partNumber] = true;
					//[TPT] END   - Added by SiRoB, SafeHash

					if (status == PS_EMPTY)
					{
						if (theApp.emuledlg->IsRunning()) // may be called during shutdown!
						{
							if (GetHashCount() == GetED2KPartHashCount() && !hashsetneeded)
							{
								// Successfully completed part, make it available for sharing
								SetStatus(PS_READY);
								theApp.sharedfiles->SafeAddKFile(this);
							}
						}
					}
				}
			}
			else if (IsCorruptedPart(partNumber) && (thePrefs.IsICHEnabled() || bForceICH))
			{
				// Try to recover with minimal loss
				if (HashSinglePart(partNumber))
				{
					m_uPartsSavedDueICH++;
					thePrefs.Add2SessionPartsSavedByICH(1);

					uint32 uRecovered = GetTotalGapSizeInPart(partNumber);
					FillGap(PARTSIZE*partNumber, PARTSIZE*partNumber + partRange);
					RemoveBlockFromList(PARTSIZE*partNumber, PARTSIZE*partNumber + partRange);

					// tell the blackbox about the verified data
					m_CorruptionBlackBox.VerifiedData(PARTSIZE * partNumber, PARTSIZE*partNumber + partRange);

					// remove from corrupted list
					POSITION posCorrupted = corrupted_list.Find(partNumber);
					if (posCorrupted)
						corrupted_list.RemoveAt(posCorrupted);

					AddLogLine(true, GetResString(IDS_ICHWORKED), partNumber, GetFileName(), CastItoXBytes(uRecovered, false, false));

					// correct file stats
					if (m_uCorruptionLoss >= uRecovered) // check, in case the tag was not present in part.met
						m_uCorruptionLoss -= uRecovered;
					// here we can't know if we have to subtract the amount of recovered data from the session stats
					// or the cumulative stats, so we subtract from where we can which leads eventuall to correct 
					// total stats
					if (thePrefs.sesLostFromCorruption >= uRecovered)
						thePrefs.sesLostFromCorruption -= uRecovered;
					else if (thePrefs.cumLostFromCorruption >= uRecovered)
						thePrefs.cumLostFromCorruption -= uRecovered;

					//[TPT] START - Added by SiRoB, SafeHash
                    			m_PartsShareable[partNumber] = true;
					//[TPT] END   - Added by SiRoB, SafeHash
					if (status == PS_EMPTY)
					{
						if (theApp.emuledlg->IsRunning()) // may be called during shutdown!
						{
							if (GetHashCount() == GetED2KPartHashCount() && !hashsetneeded)
							{
								// Successfully recovered part, make it available for sharing
								SetStatus(PS_READY);
								theApp.sharedfiles->SafeAddKFile(this);
							}
						}
					}
				}
			}

			// Any parts other than last must be full size
			partRange = PARTSIZE - 1;
		}

		// Update met file
		SavePartFile();

		if (theApp.emuledlg->IsRunning()) // may be called during shutdown!
		{
			// Is this file finished?
			if (gaplist.IsEmpty())
				CompleteFile(false);

			// Check free diskspace
			//
			// Checking the free disk space again after the file was written could most likely be avoided, but because
			// we do not use real physical disk allocation units for the free disk computations, it should be more safe
			// and accurate to check the free disk space again, after file was written and buffers were flushed to disk.
			//
			// If useing a normal file, we could avoid the check disk space if the file was not increased.
			// If useing a compressed or sparse file, we always have to check the space 
			// regardless whether the file was increased in size or not.
			if (bCheckDiskspace && ((IsNormalFile() && bIncreasedFile) || !IsNormalFile()))
			{
				switch(GetStatus())
				{
				case PS_PAUSED:
				case PS_ERROR:
				case PS_COMPLETING:
				case PS_COMPLETE:
					break;
				default:
					if (GetFreeDiskSpaceX(thePrefs.GetTempDir()) < thePrefs.GetMinFreeDiskSpace())
					{
						if (IsNormalFile())
						{
							// Normal files: pause the file only if it would still grow
							uint32 nSpaceToGrow = GetNeededSpace();
							if (nSpaceToGrow)
								PauseFile(true/*bInsufficient*/);
						}
						else
						{
							// Compressed/sparse files: always pause the file
							PauseFile(true/*bInsufficient*/);
						}
					}
				}
			}
		}
	}
	catch (CFileException* error)
	{
		FlushBuffersExceptionHandler(error);	
	}
	catch(...)
	{
		FlushBuffersExceptionHandler();
	}
	delete[] changedPart;
}

void CPartFile::FlushBuffersExceptionHandler(CFileException* error)
{
	if (thePrefs.IsCheckDiskspaceEnabled() && error->m_cause == CFileException::diskFull)
	{
		LogError(LOG_STATUSBAR, GetResString(IDS_ERR_OUTOFSPACE), GetFileName());
		if (theApp.emuledlg->IsRunning() && thePrefs.GetNotifierPopOnImportantError()){
			CString msg;
			msg.Format(GetResString(IDS_ERR_OUTOFSPACE), GetFileName());
			theApp.emuledlg->ShowNotifier(msg, TBN_IMPORTANTEVENT);
		}

		// 'CFileException::diskFull' is also used for 'not enough min. free space'
		if (theApp.emuledlg->IsRunning())
		{
			if (thePrefs.IsCheckDiskspaceEnabled() && thePrefs.GetMinFreeDiskSpace()==0)
				theApp.downloadqueue->CheckDiskspace(true);
			else
				PauseFile(true/*bInsufficient*/);
		}
	}
	else
	{
		if (thePrefs.IsErrorBeepEnabled())
			Beep(800,200);

		if (error->m_cause == CFileException::diskFull) 
		{
			LogError(LOG_STATUSBAR, GetResString(IDS_ERR_OUTOFSPACE), GetFileName());
			// may be called during shutdown!
			if (theApp.emuledlg->IsRunning() && thePrefs.GetNotifierPopOnImportantError()){
				CString msg;
				msg.Format(GetResString(IDS_ERR_OUTOFSPACE), GetFileName());
				theApp.emuledlg->ShowNotifier(msg, TBN_IMPORTANTEVENT);
			}
		}
		else
		{
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer,ARRSIZE(buffer));
			LogError(LOG_STATUSBAR, GetResString(IDS_ERR_WRITEERROR), GetFileName(), buffer);
			SetStatus(PS_ERROR);
		}
		paused = true;
		m_iLastPausePurge = time(NULL);
		theApp.downloadqueue->RemoveLocalServerRequest(this);
		m_nDownDatarate = 0; // [TPT]
		m_anStates[DS_DOWNLOADING] = 0;
	}

	if (theApp.emuledlg->IsRunning()) // may be called during shutdown!
		UpdateDisplayedInfo();

	error->Delete();
}

void CPartFile::FlushBuffersExceptionHandler()
{
	ASSERT(0);
	LogError(LOG_STATUSBAR, GetResString(IDS_ERR_WRITEERROR), GetFileName(), GetResString(IDS_UNKNOWN));
	SetStatus(PS_ERROR);
	paused = true;
	m_iLastPausePurge = time(NULL);
	theApp.downloadqueue->RemoveLocalServerRequest(this);
	m_nDownDatarate = 0; // [TPT]
	m_anStates[DS_DOWNLOADING] = 0;
	if (theApp.emuledlg->IsRunning()) // may be called during shutdown!
		UpdateDisplayedInfo();
}

UINT AFX_CDECL CPartFile::AllocateSpaceThread(LPVOID lpParam)
{
	DbgSetThreadName("Partfile-Allocate Space");
	InitThreadLocale();

	CPartFile* myfile=(CPartFile*)lpParam;
	theApp.QueueDebugLogLine(false,_T("ALLOC:Start (%s) (%s)"),myfile->GetFileName(), CastItoXBytes(myfile->m_iAllocinfo, false, false) );

	try{
		// If this is a NTFS compressed file and the current block is the 1st one to be written and there is not 
		// enough free disk space to hold the entire *uncompressed* file, windows throws a 'diskFull'!?
		myfile->m_hpartfile.SetLength(myfile->m_iAllocinfo); // allocate disk space (may throw 'diskFull')

		// force the alloc, by temporary writing a non zero to the fileend
		byte x=255;
		myfile->m_hpartfile.Seek(-1,CFile::end);
		myfile->m_hpartfile.Write(&x,1);
		myfile->m_hpartfile.Flush();
		x=0;
		myfile->m_hpartfile.Seek(-1,CFile::end);
		myfile->m_hpartfile.Write(&x,0);
		myfile->m_hpartfile.Flush();
	}
	catch (CFileException* error)
	{
		VERIFY( PostMessage(theApp.emuledlg->m_hWnd,TM_FILEALLOCEXC,(WPARAM)myfile,(LPARAM)error) );
		myfile->m_AllocateThread=NULL;

		return 1;
	}
	catch(...)
	{

		VERIFY( PostMessage(theApp.emuledlg->m_hWnd,TM_FILEALLOCEXC,(WPARAM)myfile,0) );
		myfile->m_AllocateThread=NULL;
		return 2;
	}

	myfile->m_AllocateThread=NULL;
	theApp.QueueDebugLogLine(false,_T("ALLOC:End (%s)"),myfile->GetFileName());
	return 0;
}

// Barry - This will invert the gap list, up to caller to delete gaps when done
// 'Gaps' returned are really the filled areas, and guaranteed to be in order
void CPartFile::GetFilledList(CTypedPtrList<CPtrList, Gap_Struct*> *filled) const
{
	if (gaplist.GetHeadPosition() == NULL)
		return;

	Gap_Struct *gap;
	Gap_Struct *best=NULL;
	POSITION pos;
	uint32 start = 0;
	uint32 bestEnd = 0;

	// Loop until done
	bool finished = false;
	while (!finished)
	{
		finished = true;
		// Find first gap after current start pos
		bestEnd = GetFileSize(); // [TPT]
		pos = gaplist.GetHeadPosition();
		while (pos != NULL)
		{
			gap = gaplist.GetNext(pos);
			if ((gap->start > start) && (gap->end < bestEnd))
			{
				best = gap;
				bestEnd = best->end;
				finished = false;
			}
		}

		// TODO: here we have a problem - it occured that eMule crashed because of "best==NULL" while
		// recovering an archive which was currently in "completing" state...
		if (best==NULL){
			ASSERT(0);
			return;
		}

		if (!finished)
		{
			// Invert this gap
			gap = new Gap_Struct;
			gap->start = start;
			gap->end = best->start - 1;
			start = best->end + 1;
			filled->AddTail(gap);
		}
	// [TPT]
		else if (best->end < GetFileSize())
		{
			gap = new Gap_Struct;
			gap->start = best->end + 1;
			gap->end = GetFileSize();
			filled->AddTail(gap);
		}
	}
}

// [TPT] - SLUGFILLER: showComments remove - no per-client comments


void CPartFile::UpdateDisplayedInfo(bool force)
{
	if (theApp.emuledlg->IsRunning()){
		DWORD curTick = ::GetTickCount();

        if(force || curTick-m_lastRefreshedDLDisplay > MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE+m_random_update_wait) {
			theApp.emuledlg->transferwnd->downloadlistctrl.UpdateItem(this);
			m_lastRefreshedDLDisplay = curTick;
		}
	}
}

// [TPT] - Autopriority khaos::kmod+ Make these settings for auto-priority customizable in preferences.
//				For now, made them dynamic; because ADP should be relative given
//				what it is currently used for.

void CPartFile::UpdateAutoDownPriority()
{
	// Small optimization: Do not bother with all this if file is paused or stopped.
	if( !IsAutoDownPriority() || IsPaused() || IsStopped() )
		return;
	uint16 nHighestSC = theApp.downloadqueue->GetHighestAvailableSourceCount();
	if ( GetAvailableSrcCount() > ((int)(nHighestSC * .40)) ) {		
			SetDownPriority( PR_LOW );
		return;
	}
	if ( GetAvailableSrcCount() > ((int)(nHighestSC * .20)) ) {		
			SetDownPriority( PR_NORMAL );
		return;
	}	
	SetDownPriority( PR_HIGH );
}

// [TPT] - khaos:kmod-

uint8 CPartFile::GetCategory() /*const*/
{
	return m_category > thePrefs.GetCatCount() - 1?0:m_category;
}


// Ornis: Creating progressive presentation of the partfilestatuses - for webdisplay
CString CPartFile::GetProgressString(uint16 size) const
{
	char crProgress = '0';//green
	char crHave = '1';	// black
	char crPending='2';	// yellow
	char crMissing='3';  // red
	
	char crWaiting[6];
	crWaiting[0]='4'; // blue few source
	crWaiting[1]='5';
	crWaiting[2]='6';
	crWaiting[3]='7';
	crWaiting[4]='8';
	crWaiting[5]='9'; // full sources

	CString my_ChunkBar;
	for (uint16 i=0;i<=size+1;i++) my_ChunkBar.AppendChar(crHave);	// one more for safety

	float unit= (float)size/(float)GetFileSize(); // [TPT]

	if(GetStatus() == PS_COMPLETE || GetStatus() == PS_COMPLETING) {
		CharFillRange(&my_ChunkBar,0,(uint32)(GetFileSize()*unit), crProgress);
	} else

	// red gaps
	for (POSITION pos = gaplist.GetHeadPosition();pos !=  0;){
		Gap_Struct* cur_gap = gaplist.GetNext(pos);
		bool gapdone = false;
		uint32 gapstart = cur_gap->start;
		uint32 gapend = cur_gap->end;
		for (uint32 i = 0; i < GetPartCount(); i++){
			if (gapstart >= i*PARTSIZE && gapstart <=  (i+1)*PARTSIZE){ // is in this part?
				if (gapend <= (i+1)*PARTSIZE)
					gapdone = true;
				else{
					gapend = (i+1)*PARTSIZE; // and next part
				}
				// paint
				uint8 color;
				if (m_SrcpartFrequency.GetCount() >= (INT_PTR)i && m_SrcpartFrequency[(uint16)i])  // frequency?
					//color = crWaiting;
					color = m_SrcpartFrequency[(uint16)i] <  10 ? crWaiting[m_SrcpartFrequency[(uint16)i]/2]:crWaiting[5];
				else
					color = crMissing;

				CharFillRange(&my_ChunkBar,(uint32)(gapstart*unit), (uint32)(gapend*unit + 1),  color);

				if (gapdone) // finished?
					break;
				else{
					gapstart = gapend;
					gapend = cur_gap->end;
				}
			}
		}
	}

	// yellow pending parts
	for (POSITION pos = requestedblocks_list.GetHeadPosition();pos !=  0;){
		Requested_Block_Struct* block =  requestedblocks_list.GetNext(pos);
		CharFillRange(&my_ChunkBar, (uint32)((block->StartOffset + block->transferred)*unit), (uint32)(block->EndOffset*unit),  crPending);
	}

	return my_ChunkBar;
}

void CPartFile::CharFillRange(CString* buffer,uint32 start, uint32 end, char color) const
{
	for (uint32 i=start;i<=end;i++)
		buffer->SetAt(i,color);
}

void CPartFile::SetCategory(uint8 cat, bool setprio)
{
	m_category=cat;
	
// ZZ:DownloadManager -->
	// set new prio
	if (IsPartFile()){
		SavePartFile();
	}
// <-- ZZ:DownloadManager
}

void CPartFile::SetStatus(EPartFileStatus in)
{
	ASSERT( in != PS_PAUSED && in != PS_INSUFFICIENT );

	status=in;
	if (theApp.emuledlg->IsRunning()){
		NotifyStatusChange();
		UpdateDisplayedInfo(true);
		if (thePrefs.ShowCatTabInfos())
			theApp.emuledlg->transferwnd->UpdateCatTabTitles();
	}
}

void CPartFile::NotifyStatusChange() {
	if (theApp.emuledlg->IsRunning())
		theApp.emuledlg->transferwnd->downloadlistctrl.UpdateCurrentCategoryView(this);
}

uint64 CPartFile::GetRealFileSize() const
{
	return ::GetDiskFileSize(GetFilePath());
}

uint8* CPartFile::MMCreatePartStatus(){
	// create partstatus + info in mobilemule protocol specs
	// result needs to be deleted[] | slow, but not timecritical
	uint8* result = new uint8[GetPartCount()+1];
	for (int i = 0; i != GetPartCount(); i++){
		result[i] = 0;
		if (IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)){
			result[i] = 1;
			continue;
		}
		else{
			if (IsComplete(i*PARTSIZE + (0*(PARTSIZE/3)), ((i*PARTSIZE)+(1*(PARTSIZE/3)))-1))
				result[i] += 2;
			if (IsComplete(i*PARTSIZE+ (1*(PARTSIZE/3)), ((i*PARTSIZE)+(2*(PARTSIZE/3)))-1))
				result[i] += 4;
			if (IsComplete(i*PARTSIZE+ (2*(PARTSIZE/3)), ((i*PARTSIZE)+(3*(PARTSIZE/3)))-1))
				result[i] += 8;
			uint8 freq = (uint8)m_SrcpartFrequency[i];
			if (freq > 44)
				freq = 44;
			freq = ceilf((float)freq/3);
			freq <<= 4;
			result[i] += freq;
		}

	}
	return result;
};

// [TPT]
/*uint16 CPartFile::GetSrcStatisticsValue(EDownloadState nDLState) const
{
	ASSERT( nDLState < ARRSIZE(m_anStates) );
	return m_anStates[nDLState];
}*/

/*uint16 CPartFile::GetTransferringSrcCount() const
{
	return GetSrcStatisticsValue(DS_DOWNLOADING);
}*/

/* // [TPT] - enkeyDev: ICS
// [Maella -Enhanced Chunk Selection- (based on jicxicmic)]

#pragma pack(1)
struct Chunk {
	uint16 part;			// Index of the chunk
	union {
		uint16 frequency;	// Availability of the chunk
		uint16 rank;		// Download priority factor (highest = 0, lowest = 0xffff)
	};
};
#pragma pack()

bool CPartFile::GetNextRequestedBlock(CUpDownClient* sender, 
                                      Requested_Block_Struct** newblocks, 
									  uint16* count)
{
	// The purpose of this function is to return a list of blocks (~180KB) to
	// download. To avoid a prematurely stop of the downloading, all blocks that 
	// are requested from the same source must be located within the same 
	// chunk (=> part ~9MB).
	//  
	// The selection of the chunk to download is one of the CRITICAL parts of the 
	// edonkey network. The selection algorithm must insure the best spreading
	// of files.
	//  
	// The selection is based on 4 criteria:
	//  1.  Frequency of the chunk (availability), very rare chunks must be downloaded 
	//      as quickly as possible to become a new available source.
	//  2.  Parts used for preview (first + last chunk), preview or check a 
	//      file (e.g. movie, mp3)
	//  3.  Request state (downloading in process), try to ask each source for another 
	//      chunk. Spread the requests between all sources.
	//  4.  Completion (shortest-to-complete), partially retrieved chunks should be 
	//      completed before starting to download other one.
	//  
	// The frequency criterion defines three zones: very rare (<10%), rare (<50%)
	// and common (>30%). Inside each zone, the criteria have a specific �weight�, used 
	// to calculate the priority of chunks. The chunk(s) with the highest 
	// priority (highest=0, lowest=0xffff) is/are selected first.
	//  
	//          very rare   (preview)       rare                      common
	//    0% <---- +0 pt ----> 10% <----- +10000 pt -----> 50% <---- +20000 pt ----> 100%
	// 1.  <------- frequency: +25*frequency pt ----------->
	// 2.  <- preview: +1 pt --><-------------- preview: set to 10000 pt ------------->
	// 3.                       <------ request: download in progress +20000 pt ------>
	// 4a. <- completion: 0% +100, 25% +75 .. 100% +0 pt --><-- !req => completion --->
	// 4b.                                                  <--- req => !completion -->
	//  
	// Unrolled, the priority scale is:
	//  
	// 0..xxxx       unrequested and requested very rare chunks
	// 10000..1xxxx  unrequested rare chunks + unrequested preview chunks
	// 20000..2xxxx  unrequested common chunks (priority to the most complete)
	// 30000..3xxxx  requested rare chunks + requested preview chunks
	// 40000..4xxxx  requested common chunks (priority to the least complete)
	//
	// This algorithm usually selects first the rarest chunk(s). However, partially
	// complete chunk(s) that is/are close to completion may overtake the priority 
	// (priority inversion).
	// For the common chuncks, the algorithm tries to spread the dowload between
	// the sources
	//

	// Check input parameters
	if(count == 0)
		return false;
	if(sender->GetPartStatus() == NULL)
		return false;

	// Define and create the list of the chunks to download
	const uint16 partCount = GetPartCount();
	CList<Chunk> chunksList(partCount);

	// Main loop
	uint16 newBlockCount = 0;
	while(newBlockCount != *count){
		// Create a request block stucture if a chunk has been previously selected
		if(sender->m_lastPartAsked != 0xffff){
			Requested_Block_Struct* pBlock = new Requested_Block_Struct;
			if(GetNextEmptyBlockInPart(sender->m_lastPartAsked, pBlock) == true){
				// Keep a track of all pending requested blocks
				requestedblocks_list.AddTail(pBlock);
				// Update list of blocks to return
				newblocks[newBlockCount++] = pBlock;
				// Skip end of loop (=> CPU load)
				continue;
			} 
			else {
				// All blocks for this chunk have been already requested
				delete pBlock;
				// => Try to select another chunk
				sender->m_lastPartAsked = 0xffff;
			}
		}

		// Check if a new chunk must be selected (e.g. download starting, previous chunk complete)
		if(sender->m_lastPartAsked == 0xffff){

			// Quantify all chunks (create list of chunks to download) 
			// This is done only one time and only if it is necessary (=> CPU load)
			if(chunksList.IsEmpty() == TRUE){
				// Indentify the locally missing part(s) that this source has
				for(uint16 i=0; i < partCount; i++){
					if(sender->IsPartAvailable(i) == true && GetNextEmptyBlockInPart(i, NULL) == true){
						// Create a new entry for this chunk and add it to the list
						Chunk newEntry;
						newEntry.part = i;
						newEntry.frequency = m_SrcpartFrequency[i];
						chunksList.AddTail(newEntry);
					}
				}

				// Check if any block(s) could be downloaded
				if(chunksList.IsEmpty() == TRUE){
					break; // Exit main loop while()
				}

				// [TPT]
				// Define the bounds of the three zones (very rare, rare)
				// more depending on available sources
				uint8 modif=10;
				const uint16 sourceCount = GetSourceCount();
				if (sourceCount>200) modif=5; else if (sourceCount>800) modif=2;
				uint16 limit= modif*sourceCount/ 100;
				if (limit==0) limit=1;

				const uint16 veryRareBound = limit;
				const uint16 rareBound = 2*limit;

				// Cache Preview state (Criterion 2)
                const bool isPreviewEnable = (thePrefs.GetPreviewPrio() || thePrefs.IsExtControlsEnabled() && GetPreviewPrio()) && IsPreviewableFileType();

				// Collect and calculate criteria for all chunks
				for(POSITION pos = chunksList.GetHeadPosition(); pos != NULL; ){
					Chunk& cur_chunk = chunksList.GetNext(pos);
					// [TPT] - WebCache	
					// jp Don't request chunks for which we are currently receiving proxy sources START
					ThrottledChunk cur_ThrottledChunk;
					md4cpy(cur_ThrottledChunk.FileID, GetFileHash());
					cur_ThrottledChunk.ChunkNr=cur_chunk.part;
					cur_ThrottledChunk.timestamp=GetTickCount();
					bool isthrottled = ThrottledChunkList.CheckList(cur_ThrottledChunk, false); //compare this chunk to chunks in list and throttle it if it is found
					// jp Don't request chunks for which we are currently receiving proxy sources END					
					// [TPT] - WebCache	


					
					// Offsets of chunk
					const uint32 uStart = cur_chunk.part * PARTSIZE;
					const uint32 uEnd  = ((GetFileSize() - 1) < (uStart + PARTSIZE - 1)) ? 
										  (GetFileSize() - 1) : (uStart + PARTSIZE - 1);
					ASSERT( uStart <= uEnd );

					// Criterion 2. Parts used for preview
					// Remark: - We need to download the first part and the last part(s).
					//        - When the last part is very small, it's necessary to 
					//          download the two last parts.
					bool critPreview = false;
					if(isPreviewEnable == true){
						if(cur_chunk.part == 0){
							critPreview = true; // First chunk
						}
						else if(cur_chunk.part == partCount-1){
							critPreview = true; // Last chunk 
						}
						else if(cur_chunk.part == partCount-2){
							// Last chunk - 1 (only if last chunk is too small)
							const uint32 sizeOfLastChunk = GetFileSize() - uEnd;
							if(sizeOfLastChunk < PARTSIZE/3){
								critPreview = true; // Last chunk - 1
							}
						}
					}

					// Criterion 3. Request state (downloading in process from other source(s))
					const bool critRequested = cur_chunk.frequency > veryRareBound &&  // => CPU load
											IsAlreadyRequested(uStart, uEnd);

					// Criterion 4. Completion
					uint32 partSize = PARTSIZE;
					for(POSITION pos = gaplist.GetHeadPosition(); pos != NULL; ) {
						const Gap_Struct* cur_gap = gaplist.GetNext(pos);
						// Check if Gap is into the limit
						if(cur_gap->start < uStart) {
							if(cur_gap->end > uStart && cur_gap->end < uEnd) {
								partSize -= cur_gap->end - uStart + 1;
							}
						else if(cur_gap->end >= uEnd) {
								partSize = 0;
								break; // exit loop for()
							}
						}
						else if(cur_gap->start <= uEnd) {
							if(cur_gap->end < uEnd) {
								partSize -= cur_gap->end - cur_gap->start + 1;
							}
							else {
								partSize -= uEnd - cur_gap->start + 1;
							}
						}
					}
					const uint16 critCompletion = (uint16)(partSize/(PARTSIZE/100)); // in [%]

					// Calculate priority with all criteria
					if(cur_chunk.frequency <= veryRareBound){
						// 0..xxxx unrequested + requested very rare chunks
						cur_chunk.rank = (25 * cur_chunk.frequency) +      // Criterion 1
							             ((critPreview == true) ? 0 : 1) + // Criterion 2
										 (100 - critCompletion);           // Criterion 4
						// [TPT] - WebCache											 
						if (isthrottled) cur_chunk.rank += (critCompletion+1);  // jp Don't request chunks for which we are currently receiving proxy sources
						//if (isthrottled && (critCompletion<=80)) cur_chunk.rank += (critCompletion+1); //jp not needed any more because we only add valid chunks to throttled chunks list now
					
					}
					else if(critPreview == true){
						// 10000..10100  unrequested preview chunks
						// 30000..30100  requested preview chunks
						cur_chunk.rank = ((critRequested == false) ? 10000 : 30000) + // Criterion 3
										 (100 - critCompletion);                      // Criterion 4
						// [TPT] - WebCache	
						if (isthrottled) cur_chunk.rank += (critCompletion+1);  // jp Don't request chunks for which we are currently receiving proxy sources
						//if (isthrottled && (critCompletion<=80)) cur_chunk.rank += (critCompletion+1); //jp not needed any more because we only add valid chunks to throttled chunks list now
					}
					else if(cur_chunk.frequency <= rareBound){
						// 10101..1xxxx  unrequested rare chunks
						// 30101..3xxxx  requested rare chunks
						cur_chunk.rank = (25 * cur_chunk.frequency) +                 // Criterion 1 
										 ((critRequested == false) ? 10101 : 30101) + // Criterion 3
										 (100 - critCompletion);                      // Criterion 4
						// [TPT] - WebCache	
						if (isthrottled) cur_chunk.rank += (critCompletion+1);  // jp Don't request chunks for which we are currently receiving proxy sources
						//if (isthrottled && (critCompletion<=80)) cur_chunk.rank += (critCompletion+1); //jp not needed any more because we only add valid chunks to throttled chunks list now
					
					}
					else { // common chunk
						if(critRequested == false){ // Criterion 3
							// 20000..2xxxx  unrequested common chunks
							cur_chunk.rank = 20000 +                // Criterion 3
											(100 - critCompletion); // Criterion 4
						// [TPT] - WebCache	
						if (isthrottled) cur_chunk.rank += (critCompletion+1);  // jp Don't request chunks for which we are currently receiving proxy sources
						//if (isthrottled && (critCompletion<=80)) cur_chunk.rank += (critCompletion+1); //jp not needed any more because we only add valid chunks to throttled chunks list now
						
						}
						else{
							// 40000..4xxxx  requested common chunks
							// Remark: The weight of the completion criterion is inversed
							//         to spead the requests over the completing chunks. 
							//         Without this, the chunk closest to completion will  
							//         received every new sources.
							cur_chunk.rank = 40000 +                // Criterion 3
											(critCompletion);       // Criterion 4				
						}
					}
				}
			}

			// Select the next chunk to download
			if(chunksList.IsEmpty() == FALSE){
				// Find and count the chunck(s) with the highest priority
				uint16 count = 0; // Number of found chunks with same priority
				uint16 rank = 0xffff; // Highest priority found
				for(POSITION pos = chunksList.GetHeadPosition(); pos != NULL; ){
					const Chunk& cur_chunk = chunksList.GetNext(pos);
					if(cur_chunk.rank < rank){
						count = 1;
						rank = cur_chunk.rank;
					}
					else if(cur_chunk.rank == rank){
						count++;
					}
				}

				// Use a random access to avoid that everybody tries to download the 
				// same chunks at the same time (=> spread the selected chunk among clients)
				uint16 randomness = 1 + (uint16)((((uint32)rand()*(count-1))+(RAND_MAX/2))/RAND_MAX);
				for(POSITION pos = chunksList.GetHeadPosition(); ; ){
					POSITION cur_pos = pos;
					const Chunk& cur_chunk = chunksList.GetNext(pos);
					if(cur_chunk.rank == rank){
						randomness--; 
						if(randomness == 0){
							// Selection process is over 
							sender->m_lastPartAsked = cur_chunk.part;
							// Remark: this list might be reused up to �*count� times
							chunksList.RemoveAt(cur_pos);
							break; // exit loop for()
						}  
					}
				}
			}
			else {
				// There is no remaining chunk to download
				break; // Exit main loop while()
			}
		}
	}
	// Return the number of the blocks 
	*count = newBlockCount;
	
	// Return
	return (newBlockCount > 0);
}
// Maella end
*/ // [TPT] - enkeyDev: ICS

CString CPartFile::GetInfoSummary(CPartFile* partfile) const
{
	CString strHash=EncodeBase16(partfile->GetFileHash(),16);

	CString Sbuffer,info,lsc, compl,buffer,lastdwl;

	if (partfile->IsPartFile()) {
		lsc.Format(_T("%s"), CastItoXBytes(partfile->GetCompletedSize(), false, false));
		compl.Format(_T("%s"), CastItoXBytes(partfile->GetFileSize(), false, false));
		buffer.Format(_T("%s/%s"),lsc,compl);
		compl.Format(_T("%s: %s (%.1f%%)\n"),GetResString(IDS_DL_TRANSFCOMPL),buffer,partfile->GetPercentCompleted());
	} else
		compl=_T("\n");

	if (partfile->lastseencomplete==NULL) lsc.Format(_T("%s"),GetResString(IDS_NEVER).MakeLower() ); else
		lsc.Format(_T("%s"), partfile->lastseencomplete.Format( thePrefs.GetDateTimeFormat()));

	float availability = 0;
	if(partfile->GetPartCount() != 0) {
		// Elandal: type to float before division
		// avoids implicit cast warning
		availability = partfile->GetAvailablePartCount() * 100.0 / partfile->GetPartCount();
	}

	CString avail;
	if (partfile->IsPartFile()) avail.Format(GetResString(IDS_AVAIL),partfile->GetPartCount(),partfile->GetAvailablePartCount(),availability);

	if (partfile->GetCFileDate()!=NULL) lastdwl.Format(_T("%s"),partfile->GetCFileDate().Format( thePrefs.GetDateTimeFormat()));
		else lastdwl=GetResString(IDS_NEVER);
	
	CString sourcesinfo;
	if (partfile->IsPartFile() ) sourcesinfo.Format( GetResString(IDS_DL_SOURCES)+_T(": ")+ GetResString(IDS_SOURCESINFO),partfile->GetSourceCount(),partfile->GetValidSourcesCount(),partfile->GetSrcStatisticsValue(DS_NONEEDEDPARTS),partfile->GetSrcA4AFCount());
		
	// always show space on disk
	CString sod=_T("  (")+GetResString(IDS_ONDISK) +CastItoXBytes(partfile->GetRealFileSize(), false, false)+_T(")");

	CString status;
	if (partfile->GetTransferringSrcCount()>0)
		status.Format(GetResString(IDS_PARTINFOS2)+_T("\n"),partfile->GetTransferringSrcCount());
	else 
		status.Format(_T("%s\n"),partfile->getPartfileStatus());

	//TODO: don't show the part.met filename for completed files..
	info.Format(GetResString(IDS_SW_NAME)+_T(": %s\n")
		+ GetResString(IDS_FD_HASH) + _T(" %s\n")
		+ GetResString(IDS_FD_SIZE) + _T(" %s  %s\n")
		+ GetResString(IDS_FD_MET)+ _T(" %s\n\n")
		+ GetResString(IDS_STATUS) + _T(": ") + status
		+ _T("%s")
		+ sourcesinfo
		+ (sourcesinfo.IsEmpty() ? _T("") : _T("\n\n"))
		+ _T("%s")
		+ GetResString(IDS_LASTSEENCOMPL) + _T(" ") + lsc + _T("\n")
		+ GetResString(IDS_FD_LASTCHANGE) + _T(" ") + lastdwl,
		partfile->GetFileName(),
		strHash,
		CastItoXBytes(partfile->GetFileSize(), false, false),	sod,
		partfile->GetPartMetFileName(),
		compl,
		avail
	);

	return info;
}

bool CPartFile::GrabImage(uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth,void* pSender)
{
	if (!IsPartFile()){
		return CKnownFile::GrabImage(GetPath() + CString(_T("\\")) + GetFileName(),nFramesToGrab, dStartTime, bReduceColor, nMaxWidth, pSender);
	}
	else{
		if ( ((GetStatus() != PS_READY && GetStatus() != PS_PAUSED) || m_bPreviewing || GetPartCount() < 2 || !IsComplete(0,PARTSIZE-1))  )
			return false;
		CString strFileName = RemoveFileExtension(GetFullName());
		if (m_FileCompleteMutex.Lock(100)){
			m_bPreviewing = true; 
			try{
				if (m_hpartfile.m_hFile != INVALID_HANDLE_VALUE){
					m_hpartfile.Close();
				}
			}
			catch(CFileException* exception){
				exception->Delete();
				m_FileCompleteMutex.Unlock();
				m_bPreviewing = false; 
				return false;
			}
		}
		else
			return false;

		return CKnownFile::GrabImage(strFileName,nFramesToGrab, dStartTime, bReduceColor, nMaxWidth, pSender);
	}
}

void CPartFile::GrabbingFinished(CxImage** imgResults, uint8 nFramesGrabbed, void* pSender)
{
	// unlock and reopen the file
	if (IsPartFile()){
		CString strFileName = RemoveFileExtension(GetFullName());
		if (!m_hpartfile.Open(strFileName, CFile::modeReadWrite|CFile::shareDenyWrite|CFile::osSequentialScan)){
			// uhuh, that's really bad
			LogError(LOG_STATUSBAR, GetResString(IDS_FAILEDREOPEN), RemoveFileExtension(GetPartMetFileName()), GetFileName());
			SetStatus(PS_ERROR);
			StopFile();
		}
		m_bPreviewing = false;
		m_FileCompleteMutex.Unlock();
		// continue processing
	}
	CKnownFile::GrabbingFinished(imgResults, nFramesGrabbed, pSender);
}

void CPartFile::GetSizeToTransferAndNeededSpace(uint32& rui32SizeToTransfer, uint32& rui32NeededSpace) const
{
	bool bNormalFile = IsNormalFile();
	for (POSITION pos = gaplist.GetHeadPosition();pos != 0;)
	{
		const Gap_Struct* cur_gap = gaplist.GetNext(pos);
		uint32 uGapSize = cur_gap->end - cur_gap->start;
		rui32SizeToTransfer += uGapSize;
		if (bNormalFile && cur_gap->end == GetFileSize()-1)
			rui32NeededSpace = uGapSize;
	}
	if (!bNormalFile)
		rui32NeededSpace = rui32SizeToTransfer;
}

void CPartFile::SetLastAnsweredTimeTimeout()
{
	m_ClientSrcAnswered = 2 * CONNECTION_LATENCY + ::GetTickCount() - SOURCECLIENTREASKS;
}

/*Checks, if a given item should be shown in a given category
AllcatTypes:
	0	all
	1	all not assigned
	2	not completed
	3	completed
	4	waiting
	5	transferring
	6	errorous
	7	paused
	8	stopped
	10	Video
	11	Audio
	12	Archive
	13	CDImage
	14  Doc
	15  Pic
	16  Program
*/
// [TPT] - khaos::categorymod+
bool CPartFile::CheckShowItemInGivenCat(int inCategory)
{

	Category_Struct* curCat = thePrefs.GetCategory(inCategory);

	if (curCat == NULL) 
		return false;

	if (curCat->viewfilters.bSuspendFilters && (GetCategory() == inCategory || curCat->viewfilters.nFromCats == 0))
		return true;

	if (curCat->viewfilters.nFromCats == 2 && GetCategory() != inCategory)
		return false;

	EPartFileStatus status = GetStatus();
	if (!curCat->viewfilters.bVideo && IsMovie())
		return false;
	if (!curCat->viewfilters.bAudio && ED2KFT_AUDIO == GetED2KFileTypeID(GetFileName()))
		return false;
	if (!curCat->viewfilters.bArchives && IsArchive())
		return false;
	if (!curCat->viewfilters.bImages && ED2KFT_CDIMAGE == GetED2KFileTypeID(GetFileName()))
		return false;
	if (!curCat->viewfilters.bWaiting && status != PS_PAUSED && !IsStopped() && 
		((status == PS_READY || status == PS_EMPTY) && GetTransferringSrcCount() == 0))
		return false;
	if (!curCat->viewfilters.bTransferring && 
		((status == PS_READY || status == PS_EMPTY) && GetTransferringSrcCount() > 0))
		return false;
	if (!curCat->viewfilters.bPaused && status == PS_PAUSED)
		return false;
	if (!curCat->viewfilters.bStopped && IsStopped() && 
		!FileIsComplete())
		return false;
	if (!curCat->viewfilters.bComplete && status == PS_COMPLETE)
		return false;
	if (!curCat->viewfilters.bCompleting && status == PS_COMPLETING)
		return false;
	if (!curCat->viewfilters.bHashing && status == PS_HASHING)
		return false;
	if (!curCat->viewfilters.bErrorUnknown && (status == PS_ERROR || status == PS_UNKNOWN))
		return false;
	if (GetFileSize() < curCat->viewfilters.nFSizeMin || 
		(curCat->viewfilters.nFSizeMax != 0 && GetFileSize() > curCat->viewfilters.nFSizeMax))
		return false;
	uint32 nTemp = GetFileSize() - GetCompletedSize();
	if (nTemp < curCat->viewfilters.nRSizeMin || 
		(curCat->viewfilters.nRSizeMax != 0 && nTemp > curCat->viewfilters.nRSizeMax))
		return false;
	if (curCat->viewfilters.nTimeRemainingMin > 0 || curCat->viewfilters.nTimeRemainingMax > 0)
		{
		sint32 nTemp2 = getTimeRemaining();
		if (nTemp2 < (sint32)curCat->viewfilters.nTimeRemainingMin || 
			(curCat->viewfilters.nTimeRemainingMax != 0 && nTemp2 > (sint32)curCat->viewfilters.nTimeRemainingMax))
			return false;
	}
	nTemp = GetSourceCount();
	if (nTemp < curCat->viewfilters.nSourceCountMin || 
		(curCat->viewfilters.nSourceCountMax != 0 && nTemp > curCat->viewfilters.nSourceCountMax))
		return false;
	nTemp = GetAvailableSrcCount();
	if (nTemp < curCat->viewfilters.nAvailSourceCountMin || 
		(curCat->viewfilters.nAvailSourceCountMax != 0 && nTemp > curCat->viewfilters.nAvailSourceCountMax))
	return false;
	if (!curCat->viewfilters.sAdvancedFilterMask.IsEmpty() && 
		!theApp.downloadqueue->ApplyFilterMask(GetFileName(), inCategory))
	return false;

	return true;
}
// [TPT] - khaos::categorymod-

void CPartFile::GetTooltipFileInfo(CString &info)
{
	CString sTransfSrcCount;
	if(GetTransferringSrcCount() > 0)
		sTransfSrcCount.Format(GetResString(IDS_TT_PARTINFOS3), GetTransferringSrcCount());

	CString lsc;

	if (lastseencomplete == NULL)
		lsc.LoadString(IDS_NEVER);
	else
		lsc = lastseencomplete.Format(thePrefs.GetDateTimeFormat());
	//lsc.AppendFormat(" (%s)", GetCompleteSourcesString());
	//lsc.Replace("<", "<<");
	//lsc.Replace(">", ">>");

	CString lastdwl;
	if (GetCFileDate() == NULL)
		lastdwl = GetResString(IDS_NEVER);
	else
		lastdwl.Format(_T("%s"), GetCFileDate().Format( thePrefs.GetDateTimeFormat()));

	CString dwlstart;
	if (GetCrFileDate()!=NULL) {
  		uint32 fromtime=(GetStatus()!=PS_COMPLETE)?time(NULL):GetFileDate();
  		dwlstart.Format(_T("%s"),GetCrCFileDate().Format( thePrefs.GetDateTimeFormat()));
   }
		else dwlstart=GetResString(IDS_UNKNOWN);

	CString sourcesinfo;
	float availability;
	if (IsPartFile())
	{
		sourcesinfo.Format(GetResString(IDS_SOURCESINFO), GetSourceCount(), GetValidSourcesCount(), GetSrcStatisticsValue(DS_NONEEDEDPARTS), GetSrcA4AFCount());
		if(GetPartCount() == 0)
			availability = 0;
		else
			availability = GetAvailablePartCount() * 100 / GetPartCount();
  }

	CString filename(GetFileName());
	if(filename.GetLength() > 100)
		{
		int m = (int)ceil((float)filename.GetLength() / 100);
		for(int i = 0; i < m; i++)
		{
			int n = (filename.GetLength() / m) * (i + 1);
			while (filename[n] != _T(' ') && n < filename.GetLength()) n++;
			if(n < filename.GetLength())
				filename.Insert(n, _T("\n<t=3>"));
			else
				break;
		}
	}

	info.Format(GetResString(IDS_TT_FILENAME), filename);
	info.AppendFormat(_T("<t=2>%s (%s bytes)<br>"), CastItoXBytes(GetFileSize()), GetFormatedUInt(GetFileSize()));
	switch(GetStatus())
	{
		case PS_COMPLETING:
		case PS_COMPLETE:
			info.AppendFormat(_T("<t=2>%s<br><hr=100%%><br>"), getPartfileStatus());
			break;
		default:
			info.AppendFormat(GetResString(IDS_TT_PARTINFOS2), (int)GetPercentCompleted(), CastItoXBytes(GetCompletedSize()), CastItoXBytes(GetFileSize()), sTransfSrcCount);
	}
	info.AppendFormat(GetResString(IDS_TT_HASH), md4str(GetFileHash()));
	info.AppendFormat(GetResString(IDS_TT_PARTINFOS), GetPartMetFileName());

	if (IsPartFile())
	{
		info.AppendFormat(GetResString(IDS_TT_STATUS), getPartfileStatus());
		info.AppendFormat(GetResString(IDS_TT_PARTINFOS4), GetPartCount(), GetAvailablePartCount(), availability);
		info.AppendFormat(GetResString(IDS_TT_SOURCES), sourcesinfo);
	}
	info.AppendFormat(GetResString(IDS_TT_LASTSEENCOMPL), lsc);
	info.AppendFormat(GetResString(IDS_TT_LASTCHANGE), lastdwl);
	info.AppendFormat(GetResString(IDS_TT_DL_ADD), dwlstart);
}
// [TPT] - MFCK [addon] - New Tooltips [Rayita]

void CPartFile::SetFileName(LPCTSTR pszFileName, bool bReplaceInvalidFileSystemChars)
{
	CKnownFile* pFile = theApp.sharedfiles->GetFileByID(GetFileHash());
	if (pFile && pFile == this)
	{
		theApp.sharedfiles->RemoveKeywords(this);
		CKnownFile::SetFileName(pszFileName, bReplaceInvalidFileSystemChars);
		theApp.sharedfiles->AddKeywords(this);
	}
	else
		CKnownFile::SetFileName(pszFileName, bReplaceInvalidFileSystemChars);

	theApp.emuledlg->transferwnd->downloadlistctrl.UpdateCurrentCategoryView(this);
}

void CPartFile::SetActive(bool bActive)
{
	time_t tNow = time(NULL);
	if (bActive)
	{
		if (theApp.IsConnected())
		{
			if (m_tActivated == 0)
				m_tActivated = tNow;
		}
	}
	else
	{
		if (m_tActivated != 0)
		{
			m_nDlActiveTime += tNow - m_tActivated;
			m_tActivated = 0;
		}
	}
}

uint32 CPartFile::GetDlActiveTime() const
{
	uint32 nDlActiveTime = m_nDlActiveTime;
	if (m_tActivated != 0)
		nDlActiveTime += time(NULL) - m_tActivated;
	return nDlActiveTime;
}

void CPartFile::SetFileOp(EPartFileOp eFileOp)
{
	m_eFileOp = eFileOp;
}

void CPartFile::SetFileOpProgress(UINT uProgress)
{
	ASSERT( uProgress <= 100 );
	m_uFileOpProgress = uProgress;
}

int CPartFile::RightFileHasHigherPrio(CPartFile* left, CPartFile* right) {
    if(!right) {
        return false;
    }

    if(!left ||
       !left->IsA4AFAuto() &&
      (
          right->IsA4AFAuto() ||
          thePrefs.GetCategory(right->GetCategory())->prio > thePrefs.GetCategory(left->GetCategory())->prio ||
          thePrefs.GetCategory(right->GetCategory())->prio == thePrefs.GetCategory(left->GetCategory())->prio &&
          (
              right->GetDownPriority() > left->GetDownPriority() ||
              right->GetDownPriority() == left->GetDownPriority() &&
              (
                  right->GetCategory() == left->GetCategory() && right->GetCategory() != 0 &&
				  // [TPT] - adapted to LP
                  thePrefs.GetCategory(right->GetCategory())->downloadInLinealPriorityOrder && 
				  ((right->GetCatResumeOrder() < left->GetCatResumeOrder())			// cur_file has lower LP than pfile
					||	 
					((right->GetCatResumeOrder() == left->GetCatResumeOrder()) &&	// or has the same LP
					((right->GetDownPriority() > left->GetDownPriority())  ||		// and cur_file priority is great than pfile
					((right->GetDownPriority() == left->GetDownPriority()) &&		// or has the same
					(right->GetCrFileDate() < left->GetCrFileDate())))))			// and cur_file is older than pfile                  
              )
          )
		  ||
		  (right->m_nCompleteSourcesCount < left->m_nCompleteSourcesCount) // [TPT] - If right file has less complete sources than left file
      )
    ) {
        return true;
    } else {
        return false;
    }
}

void CPartFile::RequestAICHRecovery(uint16 nPart){
	if (!m_pAICHHashSet->HasValidMasterHash() || (m_pAICHHashSet->GetStatus() != AICH_TRUSTED && m_pAICHHashSet->GetStatus() != AICH_VERIFIED)){
		AddDebugLogLine(DLP_DEFAULT, false, _T("Unable to request AICH Recoverydata because we have no trusted Masterhash"));
		return;
	}
	if (GetFileSize() <= EMBLOCKSIZE || GetFileSize() - PARTSIZE*nPart <= EMBLOCKSIZE)
		return;
	if (CAICHHashSet::IsClientRequestPending(this, nPart)){
		// [TPT] - WebCache
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
			AddDebugLogLine(DLP_DEFAULT, false, _T("RequestAICHRecovery: Already a request for this part pending"));
		return;
	}

	// first check if we have already the recoverydata, no need to rerequest it then
	if (m_pAICHHashSet->IsPartDataAvailable(nPart*PARTSIZE)){
		// [TPT] - WebCache
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
			AddDebugLogLine(DLP_DEFAULT, false, _T("Found PartRecoveryData in memory"));
		AICHRecoveryDataAvailable(nPart);
		return;
	}

	ASSERT( nPart < GetPartCount() );
	// find some random client which support AICH to ask for the blocks
	// first lets see how many we have at all, we prefer high id very much
	uint32 cAICHClients = 0;
	uint32 cAICHLowIDClients = 0;
	for (POSITION pos = srclist.GetHeadPosition(); pos != NULL;){
		CUpDownClient* pCurClient = srclist.GetNext(pos);
		if (pCurClient->IsSupportingAICH() && pCurClient->GetReqFileAICHHash() != NULL && !pCurClient->IsAICHReqPending()
			&& (*pCurClient->GetReqFileAICHHash()) == m_pAICHHashSet->GetMasterHash())
		{
			if (pCurClient->HasLowID())
				cAICHLowIDClients++;
			else
				cAICHClients++;
		}
	}
	if ((cAICHClients | cAICHLowIDClients) == 0){
		// [TPT] - WebCache
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
			AddDebugLogLine(DLP_DEFAULT, false, _T("Unable to request AICH Recoverydata because found no client who supports it and has the same hash as the trusted one"));
		return;
	}
	uint32 nSeclectedClient;
	if (cAICHClients > 0)
		nSeclectedClient = (rand() % cAICHClients) + 1;
	else
		nSeclectedClient = (rand() % cAICHLowIDClients) + 1;
	
	CUpDownClient* pClient = NULL;
	for (POSITION pos = srclist.GetHeadPosition(); pos != NULL;){
		CUpDownClient* pCurClient = srclist.GetNext(pos);
		if (pCurClient->IsSupportingAICH() && pCurClient->GetReqFileAICHHash() != NULL && !pCurClient->IsAICHReqPending()
			&& (*pCurClient->GetReqFileAICHHash()) == m_pAICHHashSet->GetMasterHash())
		{
			if (cAICHClients > 0){
				if (!pCurClient->HasLowID())
					nSeclectedClient--;
			}
			else{
				ASSERT( pCurClient->HasLowID());
				nSeclectedClient--;
			}
			if (nSeclectedClient == 0){
				pClient = pCurClient;
				break;
			}
		}
	}
	if (pClient == NULL){
		ASSERT( false );
		return;
	}
	// [TPT] - WebCache
	if(thePrefs.GetLogICHEvents()) //JP log ICH events
		AddDebugLogLine(DLP_DEFAULT, false, _T("Requesting AICH Hash (%s) form client %s"),cAICHClients? _T("HighId"):_T("LowID"), pClient->DbgGetClientInfo());

	pClient->SendAICHRequest(this, nPart);
}

void CPartFile::AICHRecoveryDataAvailable(uint16 nPart){
	if (GetPartCount() < nPart){
		ASSERT( false );
		return;
	}
	FlushBuffer(true, true, true);
	uint32 length = PARTSIZE;
	if ((ULONGLONG)PARTSIZE*(nPart+1) > m_hpartfile.GetLength()){
		length = (m_hpartfile.GetLength() - ((ULONGLONG)PARTSIZE*nPart));
		ASSERT( length <= PARTSIZE );
	}	
	// if the part was already ok, it would now be complete
	if (IsComplete(nPart*PARTSIZE, ((nPart*PARTSIZE)+length)-1)){
		// [TPT] - WebCache
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
			AddDebugLogLine(DLP_DEFAULT, false, _T("Processing AICH Recovery data: The part (%u) is already complete, canceling"));
		return;
	}
	


	CAICHHashTree* pVerifiedHash = m_pAICHHashSet->m_pHashTree.FindHash(nPart*PARTSIZE, length);
	if (pVerifiedHash == NULL || !pVerifiedHash->m_bHashValid){
		// [TPT] - WebCache
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
			AddDebugLogLine(DLP_DEFAULT, false, _T("Processing AICH Recovery data: Unable to get verified hash from hashset (should never happen)"));
		ASSERT( false );
		return;
	}
	CAICHHashTree htOurHash(pVerifiedHash->m_nDataSize, pVerifiedHash->m_bIsLeftBranch, pVerifiedHash->m_nBaseSize);
	try{
		m_hpartfile.Seek((LONGLONG)PARTSIZE*nPart,0);
		CreateHash(&m_hpartfile,length, NULL, &htOurHash);
	}
	catch(...){
		ASSERT( false );
		return;
	}
	if (!htOurHash.m_bHashValid){
		// [TPT] - WebCache
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
			AddDebugLogLine(DLP_DEFAULT, false, _T("Processing AICH Recovery data: Failed to retrieve AICH Hashset of corrupt part"));
		ASSERT( false );
		return;
	}

	// now compare the hash we just did, to the verified hash and readd all blocks which are ok
	uint32 nRecovered = 0;
	for (uint32 pos = 0; pos < length; pos += EMBLOCKSIZE){
		const uint32 nBlockSize = min(EMBLOCKSIZE, length - pos);
		CAICHHashTree* pVerifiedBlock = pVerifiedHash->FindHash(pos, nBlockSize);
		CAICHHashTree* pOurBlock = htOurHash.FindHash(pos, nBlockSize);
		if ( pVerifiedBlock == NULL || pOurBlock == NULL || !pVerifiedBlock->m_bHashValid || !pOurBlock->m_bHashValid){
			ASSERT( false );
			continue;
		}
		if (pOurBlock->m_Hash == pVerifiedBlock->m_Hash){
			FillGap(PARTSIZE*nPart+pos, PARTSIZE*nPart + pos + (nBlockSize-1));
			RemoveBlockFromList(PARTSIZE*nPart+pos, PARTSIZE*nPart + pos + (nBlockSize-1));
			nRecovered += nBlockSize;
			// tell the blackbox about the verified data
			m_CorruptionBlackBox.VerifiedData(PARTSIZE*nPart+pos, PARTSIZE*nPart + pos + (nBlockSize-1));
		}
		else{
			// inform our "blackbox" about the corrupted block which may ban clients who sent it
			m_CorruptionBlackBox.CorruptedData(PARTSIZE*nPart+pos, PARTSIZE*nPart + pos + (nBlockSize-1));
		}
	}

	// [TPT] - Fix: Correct stats(netfinity)
	// correct file stats
	if (m_uCorruptionLoss >= nRecovered)
		m_uCorruptionLoss -= nRecovered;
	// here we can't know if we have to subtract the amount of recovered data from the session stats
	// or the cumulative stats, so we subtract from where we can which leads eventuall to correct 
	// total stats
	if (thePrefs.sesLostFromCorruption >= nRecovered)
		thePrefs.sesLostFromCorruption -= nRecovered;
	else if (thePrefs.cumLostFromCorruption >= nRecovered)
		thePrefs.cumLostFromCorruption -= nRecovered;
	// [TPT] - Fix: Correct stats(netfinity)

	// ok now some sanity checks
	if (IsComplete(nPart*PARTSIZE, ((nPart*PARTSIZE)+length)-1)){
		// this is a bad, but it could probably happen under some rare circumstances
		// make sure that MD4 agrres to this fact too
		if (!HashSinglePart(nPart)){
			// [TPT] - WebCache
			if(thePrefs.GetLogICHEvents()) //JP log ICH events
				AddDebugLogLine(DLP_DEFAULT, false, _T("Processing AICH Recovery data: The part (%u) got completed while recovering - but MD4 says it corrupt! Setting hashset to error state, deleting part"));
			// now we are fu... unhappy
			m_pAICHHashSet->SetStatus(AICH_ERROR);
			AddGap(PARTSIZE*nPart, ((nPart*PARTSIZE)+length)-1);
			//[TPT] START - Added by SiRoB, SafeHash
            		m_PartsShareable[nPart] = false;
			//[TPT] END   - Added by SiRoB, SafeHash
			ASSERT( false );
			return;
		}
		else{
			// [TPT] - WebCache
			if(thePrefs.GetLogICHEvents()) //JP log ICH events
				AddDebugLogLine(DLP_DEFAULT, false, _T("Processing AICH Recovery data: The part (%u) got completed while recovering and MD4 agrees"));
			// alrighty not so bad
			POSITION posCorrupted = corrupted_list.Find(nPart);
			if (posCorrupted)
				corrupted_list.RemoveAt(posCorrupted);
			//[TPT] START - Added by SiRoB, SafeHash
            		m_PartsShareable[nPart] = true;
			//[TPT] END   - Added by SiRoB, SafeHash
			if (status == PS_EMPTY && theApp.emuledlg->IsRunning()){
				if (GetHashCount() == GetED2KPartHashCount() && !hashsetneeded){
					// Successfully recovered part, make it available for sharing
					SetStatus(PS_READY);
					theApp.sharedfiles->SafeAddKFile(this);
				}
			}

			if (theApp.emuledlg->IsRunning()){
				// Is this file finished?
				if (gaplist.IsEmpty())
					CompleteFile(false);
			}
		}
	} // end sanity check
	// Update met file
	SavePartFile();
	// make sure the user appreciates our great recovering work :P
	AddLogLine(true, GetResString(IDS_AICH_WORKED), CastItoXBytes(nRecovered), CastItoXBytes(length), nPart, GetFileName());
	//AICH successfully recovered %s of %s from part %u for %s

}

//[TPT] - Added by SiRoB, SLUGFILLER: SafeHash
bool CPartFile::IsPartShareable(uint16 partnumber) const
{
	if (partnumber < GetPartCount())
		return m_PartsShareable[partnumber];
	else
		return false;
}

bool CPartFile::IsRangeShareable(uint32 start, uint32 end) const
{
	uint16 first = start/PARTSIZE;
	uint16 last = end/PARTSIZE+1;
	if (last > GetPartCount() || first >= last)
		return false;
	for (uint16 i = first; i < last; i++)
		if (!m_PartsShareable[i])
			return false;
	return true;
}
//[TPT] END   - Added by SiRoB, SLUGFILLER: SafeHash

// [TPT] - Sivka AutoHL Begin
uint16 CPartFile::GetMaxSourcesLimitSoft()
{
	UINT temp = ((UINT)m_MaxSourcesPerFile * 9L) / 10;
	if (temp > MAX_SOURCES_FILE_SOFT)
		return MAX_SOURCES_FILE_SOFT;
	return temp;
}

uint16 CPartFile::GetMaxSourcesUDPLimit()
{
	UINT temp = ((UINT)m_MaxSourcesPerFile * 3L) / 4;
	if (temp > MAX_SOURCES_FILE_UDP)
		return MAX_SOURCES_FILE_UDP;
	return temp;
}
// [TPT] - Sivka AutoHL End

// [TPT] - enkeyDev: ICS Pawcio
uint16* CPartFile::CalcDownloadingParts(CUpDownClient* client){
	if (!client)
		return NULL;

	uint16  partsCount = GetPartCount();
	if (!partsCount)
		return NULL;

	uint16* partsDownloading = new uint16[partsCount];
	MEMZERO(partsDownloading, partsCount * sizeof(uint16));

	CUpDownClient* cur_client;
	POSITION pos = m_downloadingSourceList.GetHeadPosition();
	while (pos){
		cur_client = m_downloadingSourceList.GetNext(pos);
		uint16 clientPart = cur_client->m_lastPartAsked;
		if (cur_client != client && cur_client->GetRequestFile() && 
			!md4cmp(cur_client->GetRequestFile()->GetFileHash(), GetFileHash()) && 
			clientPart < partsCount && cur_client->GetDownloadDatarate() > 150)
			partsDownloading[clientPart]++;
	}
	return partsDownloading;
}
// [TPT] - enkeyDev: ICS


// [TPT] - WebCache

// JP added handling of proxy-sources on pause/cancel/resume START

// JP cancel proxy downloads
//remove all sources for this file from WCBlockList, StoppedWCBlockList and ThrottledChunkList
void CPartFile::CancelProxyDownloads()
{
uchar currenthash[16];
md4cpy(currenthash, GetFileHash());

//remove all proxy-sources from WebCachedBlockList
POSITION pos = WebCachedBlockList.GetHeadPosition();
int i = 0;
while (i < WebCachedBlockList.GetCount())
{
	CWebCachedBlock* cur_block = WebCachedBlockList.GetAt(WebCachedBlockList.FindIndex(i));
	if (!(bool)md4cmp(cur_block->m_FileID, currenthash))
	{
		WebCachedBlockList.RemoveAt(WebCachedBlockList.FindIndex(i));
		delete cur_block;
	}
	else
		i++;
}

//remove all proxy-sources from StoppedWebCachedBlockList
pos = StoppedWebCachedBlockList.GetHeadPosition();
i = 0;
while (i < StoppedWebCachedBlockList.GetCount())
{
	CWebCachedBlock* cur_block = StoppedWebCachedBlockList.GetAt(StoppedWebCachedBlockList.FindIndex(i));
	if (!(bool)md4cmp(cur_block->m_FileID, currenthash))
	{
		StoppedWebCachedBlockList.RemoveAt(StoppedWebCachedBlockList.FindIndex(i));
		delete cur_block;
	}
	else
		i++;
}

//remove all throttled chunks for this file from ThrottledChunkList
pos = ThrottledChunkList.GetHeadPosition();
i = 0;
while (i < ThrottledChunkList.GetCount())
	{
	ThrottledChunk cur_chunk = ThrottledChunkList.GetAt(ThrottledChunkList.FindIndex(i));
	if (!(bool)md4cmp(cur_chunk.FileID, currenthash))
		ThrottledChunkList.RemoveAt(ThrottledChunkList.FindIndex(i));
	else
		i++;
	}
}


//JP pause proxy downloads
//jp swap blocks from WCBlockList to StoppedWCBlockList
void CPartFile::PauseProxyDownloads()
{
uchar currenthash[16];
md4cpy(currenthash, GetFileHash());

POSITION pos = WebCachedBlockList.GetHeadPosition();
int i = 0;
while (i < WebCachedBlockList.GetCount())
{
	CWebCachedBlock* cur_block = WebCachedBlockList.GetAt(WebCachedBlockList.FindIndex(i));
	if (!(bool)md4cmp(cur_block->m_FileID, currenthash))
	{
		WebCachedBlockList.RemoveAt(WebCachedBlockList.FindIndex(i));
		StoppedWebCachedBlockList.AddTail(cur_block);
	}
	else
		i++;
}
}

//JP resume proxy downloads
//jp swap blocks from StoppedWCBlockList to WCBlockList
void CPartFile::ResumeProxyDownloads()
{
	uchar currenthash[16];
	md4cpy(currenthash, GetFileHash());

	POSITION pos = StoppedWebCachedBlockList.GetHeadPosition();
	int i = 0;
	while (i < StoppedWebCachedBlockList.GetCount())
	{
	CWebCachedBlock* cur_block = StoppedWebCachedBlockList.GetAt(StoppedWebCachedBlockList.FindIndex(i));
	if (!(bool)md4cmp(cur_block->m_FileID, currenthash))
	{
		StoppedWebCachedBlockList.RemoveAt(StoppedWebCachedBlockList.FindIndex(i));
		WebCachedBlockList.AddTail(cur_block);
	}
	else
		i++;
	}
	if (!SINGLEProxyClient || !SINGLEProxyClient->ProxyClientIsBusy()) WebCachedBlockList.TryToDL();
}

//JP WC-Source count START
uint16 CPartFile::GetWebcacheSourceCount() const
{
	if (::GetTickCount() - LastWebcacheSourceCountTime > SEC2MS(1))
	CountWebcacheSources();
	return WebcacheSources;
}

uint16 CPartFile::GetWebcacheSourceOurProxyCount() const
{
if (::GetTickCount() - LastWebcacheSourceCountTime > SEC2MS(1))
	CountWebcacheSources();
return WebcacheSourcesOurProxy;
}

uint16 CPartFile::GetWebcacheSourceNotOurProxyCount() const
{
if (::GetTickCount() - LastWebcacheSourceCountTime > SEC2MS(1))
	CountWebcacheSources();
return WebcacheSourcesNotOurProxy;
}

void CPartFile::CountWebcacheSources() const
{
const_cast< CPartFile * >( this )->LastWebcacheSourceCountTime = ::GetTickCount();
UINT counter = 0;
UINT counterOur = 0;
UINT counterNotOur = 0;

for (POSITION pos = srclist.GetHeadPosition(); pos != NULL;)
{
		CUpDownClient* cur_client = srclist.GetNext(pos);
		if (cur_client->SupportsWebCache() || cur_client->IsProxy() )
			counter++;
	if (cur_client->SupportsWebCache())
	{
		if (cur_client->IsBehindOurWebCache())
			++counterOur;
		else if (cur_client->GetWebCacheName() != "")
			++counterNotOur;
	}
}
CPartFile* self = const_cast< CPartFile * >( this );
self->WebcacheSources = counter;
self->WebcacheSourcesOurProxy = counterOur;
self->WebcacheSourcesNotOurProxy = counterNotOur;
}
//JP WC-Source count END

//JP Throttle OHCB-production START
uint16 CPartFile::GetMaxNumberOfWebcacheConnectionsForThisFile()
{
	uint32 blocks = GetNumberOfBlocksForThisFile();
	if (blocks > 500) return 0;
	else if (blocks > 400) return 1;
	else if (blocks > 300) return 2;
	else if (blocks > 200) return 3;
	else if (blocks > 100) return 4;
	else return 5;
}

uint32 CPartFile::GetNumberOfBlocksForThisFile()
{
	uchar currenthash[16];
	md4cpy(currenthash, GetFileHash());

	POSITION pos = WebCachedBlockList.GetHeadPosition();
	int counter = 0;
	int i = 0;
	while (i < WebCachedBlockList.GetCount())
	{
		CWebCachedBlock* cur_block = WebCachedBlockList.GetAt(WebCachedBlockList.FindIndex(i));
		if (!(bool)md4cmp(cur_block->m_FileID, currenthash))
			counter++;
		i++;
	}
	return counter;
}

uint16 CPartFile::GetNumberOfCurrentWebcacheConnectionsForThisFile()
{
	int counter = 0;
	for (POSITION pos = srclist.GetHeadPosition(); pos != NULL;)
	{
		if (srclist.GetNext(pos)->IsDownloadingFromWebCache())
			counter++;
	}
	return counter;
}

//JP Throttle OHCB-production END
//[TPT] - Webcache 1.9 beta3
void CPartFile::AddWebCachedBlockToStats( bool IsGood, uint32 bytes )
{
	Webcacherequests++;
	if (IsGood){
		WebCacheDownDataThisFile += bytes;
		SuccessfulWebcacherequests++;
	}
}

// [TPT] - WebCache