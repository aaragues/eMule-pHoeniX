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
#pragma once
#include "DeadSourceList.h"
#include "UpDownClient.h"//[TPT] - Webcache 1.9 beta3

#include <atlcoll.h>	// [TPT]- modID Slugfiller: modid

class CClientReqSocket;
class CUpDownClient;
class CPartFile; // [TPT]
namespace Kademlia{
	class CContact;
	class CUInt128;
};
typedef CTypedPtrList<CPtrList, CUpDownClient*> CUpDownClientPtrList;

#define	NUM_CLIENTLIST_STATS	19
#define BAN_CLEANUP_TIME	1200000 // 20 min

//------------CDeletedClient Class----------------------
// this class / list is a bit overkill, but currently needed to avoid any exploit possibtility
// it will keep track of certain clients attributes for 2 hours, while the CUpDownClient object might be deleted already
// currently: IP, Port, UserHash
struct PORTANDHASH{
	uint16 nPort;
	void* pHash;
};

class CDeletedClient{
public:
	CDeletedClient(const CUpDownClient* pClient);
	CArray<PORTANDHASH> m_ItemsList;
	uint32							m_dwInserted;
	uint32							m_cBadRequest;
};

// ----------------------CClientList Class---------------
class CClientList
{
	friend class CClientListCtrl;

	enum buddyState
	{
		Disconnected,
		Connecting,
		Connected
	};

public:
	CClientList();
	~CClientList();
	void	AddClient(CUpDownClient* toadd,bool bSkipDupTest = false);
	void	RemoveClient(CUpDownClient* toremove, LPCTSTR pszReason = NULL);
	void	GetStatistics(uint32& totalclient, int stats[NUM_CLIENTLIST_STATS],
						  CMap<uint32, uint32, uint32, uint32>& clientVersionEDonkey,
						  CMap<uint32, uint32, uint32, uint32>& clientVersionEDonkeyHybrid,
						  CMap<uint32, uint32, uint32, uint32>& clientVersionEMule,
						  CMap<uint32, uint32, uint32, uint32>& clientVersionAMule);
	uint32	GetClientCount()	{ return list.GetCount();}
	// [TPT]- modID Slugfiller: modid
	void	GetModStatistics(CRBMap<uint32, CRBMap<CString, uint32>* > *clientMods);
	void	ReleaseModStatistics(CRBMap<uint32, CRBMap<CString, uint32>* > *clientMods);
	// [TPT]- modID Slugfiller: modid
	void	DeleteAll();
	bool	AttachToAlreadyKnown(CUpDownClient** client, CClientReqSocket* sender);
	CUpDownClient* FindClientByIP(uint32 clientip, UINT port) const;
	CUpDownClient* FindClientByUserHash(const uchar* clienthash) const;
	CUpDownClient* FindClientByIP(uint32 clientip) const;
	CUpDownClient* FindClientByIP_UDP(uint32 clientip, UINT nUDPport) const;
	CUpDownClient* FindClientByServerID(uint32 uServerIP, uint32 uUserID) const;
	CUpDownClient* FindClientByUserID_KadPort(uint32 clientID,uint16 kadPort) const;
	CUpDownClient* FindClientByIP_KadPort(uint32 ip, uint16 port) const;
	CUpDownClient* GetRandomKadClient() const;
//	void	GetClientListByFileID(CUpDownClientPtrList *clientlist, const uchar *fileid);	// #zegzav:updcliuplst

	// [TPT] - eWombat SNAFU v2
	/*
	void	AddBannedClient(uint32 dwIP);
	bool	IsBannedClient(uint32 dwIP) const;
	void	RemoveBannedClient(uint32 dwIP);
	UINT	GetBannedCount() const		{return m_bannedList.GetCount(); }
	*/
	// [TPT] - eWombat SNAFU v2

	// Tracked clients
	void	AddTrackClient(CUpDownClient* toadd);
	bool	ComparePriorUserhash(uint32 dwIP, uint16 nPort, void* pNewHash);
	UINT	GetClientsFromIP(uint32 dwIP) const;
	void	TrackBadRequest(const CUpDownClient* upcClient, sint32 nIncreaseCounter);
	uint32	GetBadRequests(const CUpDownClient* upcClient) const;

	void	Process();
	void	RequestTCP(Kademlia::CContact* contact);
	void	RequestBuddy(Kademlia::CContact* contact);
	void	IncomingBuddy(Kademlia::CContact* contact, Kademlia::CUInt128* buddyID);
	void	RemoveFromKadList(CUpDownClient* torem);
	void	AddToKadList(CUpDownClient* toadd);
	uint8	GetBuddyStatus() {return m_bHaveBuddy;}
	void	DoCallBack( const uchar* hashid );
	CUpDownClient* GetBuddy() {return m_pBuddy;}

	// [TPT] - Maella -Extended clean-up II-	
	void CleanUp(CPartFile* pDeletedFile);
	// Maella end

	// Maella -Inform sources of an ID change-
	void TrigReaskForDownload(bool immediate);
	// Maella end

	bool	IsValidClient(CUpDownClient* tocheck);
	void	Debug_SocketDeleted(CClientReqSocket* deleted);

    // ZZ:UploadSpeedSense -->
    bool GiveClientsForTraceRoute();
	// ZZ:UploadSpeedSense <--

    void ProcessA4AFClients(); // ZZ:DownloadManager
	CDeadSourceList	m_globDeadSourceList;

	// [TPT] - WebCache
	// yonatan - not 2 be confused with the one in CUploadQueue!
	CUpDownClient*	FindClientByWebCacheUploadId(const uint32 id);
	//[TPT] - Webcache 1.9 beta3
	// Superlexx - OHCB manager
	CUpDownClientPtrList* XpressOHCBRecipients(uint16 maxNrOfClients, const Requested_Block_Struct* block);
	void SendOHCBs(); // sends OHCBs to every client every x minutes
	uint32 m_dwLastSendOHCBs;
	// Superlexx - COtN - moved here from the CUpDownClient
	uint16 GetNumberOfClientsBehindOurWebCacheHavingSameFileAndNeedingThisBlock(Pending_Block_Struct* pending, uint16 maxNrOfClients);
	//[TPT] - Webcache 1.9 beta3
	// [TPT] - WebCache

	void	CleanUpClientList();

private:
	CUpDownClientPtrList list;
	// CMap<uint32, uint32, uint32, uint32> m_bannedList; // [TPT] - eWombat SNAFU v2
	CMap<uint32, uint32, CDeletedClient*, CDeletedClient*> m_trackedClientsList;
	// uint32	m_dwLastBannCleanUp; // [TPT] - eWombat SNAFU v2
	uint32	m_dwLastTrackedCleanUp;
	// uint32 m_dwLastClientCleanUp; // [TPT] - Maella -Extended clean-up II-
	uint8 m_bHaveBuddy;
	CUpDownClientPtrList KadList;
	CCriticalSection m_RequestTCPLock;
	CUpDownClient* m_pBuddy;
	
//>>> eWombat [SNAFU][SUI][TPT]
public:
	void ProcessEx(void);

	bool AddSnafuClient(CUpDownClient* toadd);
	bool RemoveSnafuClient(CUpDownClient* toremove,bool bForce=false);
	bool IsSnafuClient(uint32 dwIP) const;	
	int	 GetSnafuCount() {return m_snafulist.GetSize();}
	DWORD GetTrackedCount() {return m_trackedClientsList.GetSize();}

protected:
	CUpDownClientPtrList m_snafulist;	
//>>> eWombat [SNAFU][SUI][TPT]
};
