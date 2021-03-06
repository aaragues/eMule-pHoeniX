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
#include "ClientList.h"
#include "otherfunctions.h"
#include "Kademlia/Kademlia/kademlia.h"
#include "Kademlia/Kademlia/prefs.h"
#include "Kademlia/Kademlia/search.h"
#include "Kademlia/Kademlia/searchmanager.h"
#include "Kademlia/routing/contact.h"
#include "Kademlia/net/kademliaudplistener.h"
#include "kademlia/utils/UInt128.h"
#include "LastCommonRouteFinder.h"
#include "UploadQueue.h"
#include "DownloadQueue.h"
#include "UpDownClient.h"
#include "ClientCredits.h"
#include "ListenSocket.h"
#include "Opcodes.h"
#include "Sockets.h"
#include "emuledlg.h"
#include "TransferWnd.h"
#include "serverwnd.h"
#include "ClientDetailDialog.h"	// [TPT] - SLUGFILLER: modelessDialogs
#include "Log.h"
#include "packets.h"

//[TPT] - Webcache 1.9 beta3
#include "PartFile.h"
#include "SharedFileList.h"
#include "WebCache.h"
//[TPT] - Webcache 1.9 beta3


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


CClientList::CClientList(){
	//m_dwLastBannCleanUp = 0; // [TPT] - eWombat SNAFU v2
	m_dwLastTrackedCleanUp = 0;
	//m_dwLastClientCleanUp = 0; // [TPT] - Maella -Extended clean-up II-
	m_bHaveBuddy = Disconnected;
	//m_bannedList.InitHashTable(331); // [TPT] - eWombat SNAFU v2
	m_trackedClientsList.InitHashTable(2011);
	m_globDeadSourceList.Init(true);
	m_pBuddy = NULL;
	m_dwLastSendOHCBs = 0;//[TPT] - Webcache 1.9 beta3
}

CClientList::~CClientList(){
	POSITION pos = m_trackedClientsList.GetStartPosition();
	uint32 nKey;
	CDeletedClient* pResult;
	while (pos != NULL){
		m_trackedClientsList.GetNextAssoc( pos, nKey, pResult );
		m_trackedClientsList.RemoveKey(nKey);
		delete pResult;
	}
	m_snafulist.RemoveAll(); // [TPT] - eWombat SNAFU v2
}

void CClientList::GetStatistics(uint32 &ruTotalClients, int stats[NUM_CLIENTLIST_STATS], 
								CMap<uint32, uint32, uint32, uint32>& clientVersionEDonkey, 
								CMap<uint32, uint32, uint32, uint32>& clientVersionEDonkeyHybrid, 
								CMap<uint32, uint32, uint32, uint32>& clientVersionEMule, 
								CMap<uint32, uint32, uint32, uint32>& clientVersionAMule)
{
	ruTotalClients = list.GetCount();
	MEMZERO(stats, sizeof(stats[0]) * NUM_CLIENTLIST_STATS);

	for (POSITION pos = list.GetHeadPosition(); pos != NULL; )
	{
		const CUpDownClient* cur_client = list.GetNext(pos);

		if (cur_client->HasLowID())
			stats[14]++;
		
		switch (cur_client->GetClientSoft())
		{
			case SO_EMULE:
			case SO_OLDEMULE:{
				stats[2]++;
				//uint8 version = cur_client->GetMuleVersion();
				//if (version == 0xFF || version == 0x66 || version==0x69 || version==0x90 || version==0x33 || version==0x60)
				//	continue;
				clientVersionEMule[cur_client->GetVersion()]++;
				break;
			}

			case SO_EDONKEYHYBRID : 
				stats[4]++;
				clientVersionEDonkeyHybrid[cur_client->GetVersion()]++;
				break;
			
			case SO_AMULE:
				stats[10]++;
				clientVersionAMule[cur_client->GetVersion()]++;
				break;

			case SO_EDONKEY:
				stats[1]++;
				clientVersionEDonkey[cur_client->GetVersion()]++;
				break;

			case SO_MLDONKEY:
				stats[3]++;
				break;
			
			case SO_SHAREAZA:
				stats[11]++;
				break;

			// all remaining 'eMule Compatible' clients
			case SO_CDONKEY:
			case SO_XMULE:
			case SO_LPHANT:
				stats[5]++;
				break;

			default:
				stats[0]++;
				break;
		}

		if (cur_client->Credits() != NULL)
		{
			switch (cur_client->Credits()->GetCurrentIdentState(cur_client->GetIP()))
			{
				case IS_IDENTIFIED:
					stats[12]++;
					break;
				case IS_IDFAILED:
				case IS_IDNEEDED:
				case IS_IDBADGUY:
					stats[13]++;
					break;
			}
		}

		if (cur_client->GetDownloadState()==DS_ERROR || cur_client->GetUploadState()==US_ERROR)
			stats[6]++; // Error

		switch (cur_client->GetUserPort())
		{
			case 4662:
				stats[8]++; // Default Port
				break;
			default:
				stats[9]++; // Other Port
		}
	
		// Network client stats
		if (cur_client->GetServerIP() && cur_client->GetServerPort())
		{
			stats[15]++;		// eD2K
			if(cur_client->GetKadPort())
			{
				stats[17]++;	// eD2K/Kad
				stats[16]++;	// Kad
			}
		}
		else if (cur_client->GetKadPort())
			stats[16]++;		// Kad
		else
			stats[18]++;		// Unknown
	}
}

// [TPT]- modID Slugfiller: modid
void CClientList::GetModStatistics(CRBMap<uint32, CRBMap<CString, uint32>* > *clientMods){
	if (!clientMods)
		return;
	clientMods->RemoveAll();
	
	// [TPT] Code improvement
	for (POSITION pos = list.GetHeadPosition(); pos != NULL;) {		
		CUpDownClient* cur_client =	list.GetNext(pos);

		switch (cur_client->GetClientSoft()) {
		case SO_EMULE   :
		case SO_OLDEMULE:
			break;
		default:
			continue;
		}

		CRBMap<CString, uint32> *versionMods;

		if (!clientMods->Lookup(cur_client->GetVersion(), versionMods)){
			versionMods = new CRBMap<CString, uint32>;
			versionMods->RemoveAll();
			clientMods->SetAt(cur_client->GetVersion(), versionMods);
		}

		uint32 count;

		if (!versionMods->Lookup(cur_client->GetClientModVer(), count))
			count = 1;
		else
			count++;

		versionMods->SetAt(cur_client->GetClientModVer(), count);
	}
	// [TPT] end
}

void CClientList::ReleaseModStatistics(CRBMap<uint32, CRBMap<CString, uint32>* > *clientMods){
	if (!clientMods)
		return;
	POSITION pos = clientMods->GetHeadPosition();
	while(pos != NULL)
	{
		uint32 version;
		CRBMap<CString, uint32> *versionMods;
		clientMods->GetNextAssoc(pos, version, versionMods);
		delete versionMods;
	}
	clientMods->RemoveAll();
}
// [TPT]- modID Slugfiller: modid


void CClientList::AddClient(CUpDownClient* toadd, bool bSkipDupTest)
{
	// skipping the check for duplicate list entries is only to be done for optimization purposes, if the calling
	// function has ensured that this client instance is not already within the list -> there are never duplicate
	// client instances in this list.
	if (!bSkipDupTest){
		if(list.Find(toadd))
			return;
	}
	theApp.emuledlg->transferwnd->clientlistctrl.AddClient(toadd);
	list.AddTail(toadd);
}

// ZZ:UploadSpeedSense -->
bool CClientList::GiveClientsForTraceRoute() {
    // this is a host that lastCommonRouteFinder can use to traceroute
    return theApp.lastCommonRouteFinder->AddHostsToCheck(list);
}
// ZZ:UploadSpeedSense <--

void CClientList::RemoveClient(CUpDownClient* toremove, LPCTSTR pszReason){
	POSITION pos = list.Find(toremove);
	if (pos){
		//just to be sure...
		/*CString strInfo(_T("Client removed from CClientList::RemoveClient()."));
		if (pszReason){
			strInfo += _T(" Reason: ");
			strInfo += pszReason;
		}*/
		theApp.uploadqueue->RemoveFromUploadQueue(toremove, pszReason, CUpDownClient::USR_NONE); // [TPT] - Maella -Upload Stop Reason-
		theApp.uploadqueue->RemoveFromWaitingQueue(toremove);
		// [TPT] - SUQWT
		if ( toremove != NULL && toremove->Credits() != NULL) 		
			toremove->Credits()->ClearWaitStartTime();		
		// [TPT] - SUQWT
		theApp.downloadqueue->RemoveSource(toremove);
		theApp.emuledlg->transferwnd->clientlistctrl.RemoveClient(toremove);
		RemoveFromKadList(toremove);
		RemoveSnafuClient(toremove,true); // [TPT] - eWombat
		list.RemoveAt(pos);
	}
}

void CClientList::DeleteAll(){
	theApp.uploadqueue->DeleteAll();
	theApp.downloadqueue->DeleteAll();
	// [TPT] - Code improvement
	for (POSITION pos = list.GetHeadPosition(); pos != NULL;){
		POSITION cur_pos = pos;
		CUpDownClient* cur_client =	list.GetNext(pos);
		list.RemoveAt(cur_pos);
		delete cur_client; // recursiv: this will call RemoveClient
	}
	list.RemoveAll();
	// [TPT] - Code improvement
}

bool CClientList::AttachToAlreadyKnown(CUpDownClient** client, CClientReqSocket* sender){
	CUpDownClient* tocheck = (*client);
	CUpDownClient* found_client = NULL;
	CUpDownClient* found_client2 = NULL;
	// [TPT] - Code Improvement
	for (POSITION pos = list.GetHeadPosition(); pos != 0;){	//
		POSITION cur_pos = pos;		
		CUpDownClient* cur_client = list.GetNext(pos);
		if (tocheck->Compare(cur_client,false)){ //matching userhash
			found_client2 = cur_client;
		}
		if (tocheck->Compare(cur_client,true)){	 //matching IP
			found_client = cur_client;
			break;
		}
	}
	if (found_client == NULL)
		found_client = found_client2;

	if (found_client != NULL){
		if (tocheck == found_client){
			//we found the same client instance (client may have sent more than one OP_HELLO). do not delete that client!
			return true;
		}
		if (sender){
			if (found_client->socket){
				if (found_client->socket->IsConnected() 
					&& (found_client->GetIP() != tocheck->GetIP() || found_client->GetUserPort() != tocheck->GetUserPort() ) )
				{
					// if found_client is connected and has the IS_IDENTIFIED, it's safe to say that the other one is a bad guy
					if (found_client->Credits() && found_client->Credits()->GetCurrentIdentState(found_client->GetIP()) == IS_IDENTIFIED){
						if (thePrefs.GetLogBannedClients())
							AddDebugLogLine(false, _T("Clients: %s (%s), Banreason: Userhash invalid"), tocheck->GetUserName(), ipstr(tocheck->GetConnectIP()));
						// [TPT] - eWombat SNAFU v2
						if (thePrefs.GetAntiCreditTheft())
							tocheck->DoSnafu(SNAFU_ACT,false);
						// [TPT] - eWombat SNAFU v2
						else
							tocheck->Ban(_T("Bad hash"));
						return false;
					}
	
					//IDS_CLIENTCOL Warning: Found matching client, to a currently connected client: %s (%s) and %s (%s)
					if (thePrefs.GetLogBannedClients())
						AddDebugLogLine(true,GetResString(IDS_CLIENTCOL), tocheck->GetUserName(), ipstr(tocheck->GetConnectIP()), found_client->GetUserName(), ipstr(found_client->GetConnectIP()));
					return false;
				}
				found_client->socket->client = 0;
				found_client->socket->Safe_Delete();
			}
			found_client->socket = sender;
			tocheck->socket = 0;
		}
		*client = 0;
		delete tocheck;
		*client = found_client;
		return true;
	}
	return false;
}

CUpDownClient* CClientList::FindClientByIP(uint32 clientip, UINT port) const
{
	for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
	{
		CUpDownClient* cur_client = list.GetNext(pos);
		if (cur_client->GetIP() == clientip && cur_client->GetUserPort() == port)
			return cur_client;
	}
	return 0;
}

CUpDownClient* CClientList::FindClientByUserHash(const uchar* clienthash) const
{
	for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
	{
		CUpDownClient* cur_client = list.GetNext(pos);
		if (!md4cmp(cur_client->GetUserHash() ,clienthash))
			return cur_client;
	}
	return 0;
}

CUpDownClient* CClientList::FindClientByIP(uint32 clientip) const
{
	for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
	{
		CUpDownClient* cur_client = list.GetNext(pos);
		if (cur_client->GetIP() == clientip)
			return cur_client;
	}
	return 0;
}

CUpDownClient* CClientList::FindClientByIP_UDP(uint32 clientip, UINT nUDPport) const
{
	for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
	{
		CUpDownClient* cur_client = list.GetNext(pos);
		if (cur_client->GetIP() == clientip && cur_client->GetUDPPort() == nUDPport)
			return cur_client;
	}
	return 0;
}

CUpDownClient* CClientList::FindClientByUserID_KadPort(uint32 clientID, uint16 kadPort) const
{
	for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
	{
		CUpDownClient* cur_client = list.GetNext(pos);
		if (cur_client->GetUserIDHybrid() == clientID && cur_client->GetKadPort() == kadPort)
			return cur_client;
	}
	return 0;
}

CUpDownClient* CClientList::FindClientByIP_KadPort(uint32 ip, uint16 port) const
{
	for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
	{
		CUpDownClient* cur_client = list.GetNext(pos);
		if (cur_client->GetIP() == ip && cur_client->GetKadPort() == port)
			return cur_client;
	}
	return 0;
}

//TODO: This needs to change to a random Kad user.
CUpDownClient* CClientList::GetRandomKadClient() const
{
	for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
	{
		CUpDownClient* cur_client =	list.GetNext(pos);
		if (cur_client->GetKadPort())
			return cur_client;
	}
	return 0;
}

CUpDownClient* CClientList::FindClientByServerID(uint32 uServerIP, uint32 uED2KUserID) const
{
	uint32 uHybridUserID = ntohl(uED2KUserID);
	for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
	{
		CUpDownClient* cur_client =	list.GetNext(pos);
		if (cur_client->GetServerIP() == uServerIP && cur_client->GetUserIDHybrid() == uHybridUserID)
			return cur_client;
	}
	return 0;
}

// [TPT] - eWombat SNAFU v2
/*
void CClientList::AddBannedClient(uint32 dwIP){
	m_bannedList.SetAt(dwIP, ::GetTickCount());
}

bool CClientList::IsBannedClient(uint32 dwIP) const
/*{
	uint32 dwBantime;
	if (m_bannedList.Lookup(dwIP, dwBantime)){
		if (dwBantime + CLIENTBANTIME > ::GetTickCount())
			return true;
		//RemoveBannedClient(dwIP);
	}
	return false; 
}

void CClientList::RemoveBannedClient(uint32 dwIP){
	m_bannedList.RemoveKey(dwIP);
}
*/
// [TPT] - eWombat SNAFU v2

////////////////////////////////////////
/// Tracked clients
void CClientList::AddTrackClient(CUpDownClient* toadd){
	CDeletedClient* pResult = 0;
	if (m_trackedClientsList.Lookup(toadd->GetIP(), pResult)){
		pResult->m_dwInserted = ::GetTickCount();
		for (int i = 0; i != pResult->m_ItemsList.GetCount(); i++){
			if (pResult->m_ItemsList[i].nPort == toadd->GetUserPort()){
				// already tracked, update
				pResult->m_ItemsList[i].pHash = toadd->Credits();
				return;
			}
		}
		PORTANDHASH porthash = { toadd->GetUserPort(), toadd->Credits()};
		pResult->m_ItemsList.Add(porthash);
	}
	else{
		m_trackedClientsList.SetAt(toadd->GetIP(), new CDeletedClient(toadd));
	}
}

// true = everything ok, hash didn't changed
// false = hash changed
bool CClientList::ComparePriorUserhash(uint32 dwIP, uint16 nPort, void* pNewHash){
	CDeletedClient* pResult = 0;
	if (m_trackedClientsList.Lookup(dwIP, pResult)){
		for (int i = 0; i != pResult->m_ItemsList.GetCount(); i++){
			if (pResult->m_ItemsList[i].nPort == nPort){
				if (pResult->m_ItemsList[i].pHash != pNewHash)
					return false;
				else
					break;
			}
		}
	}
	return true;
}

UINT CClientList::GetClientsFromIP(uint32 dwIP) const
{
	CDeletedClient* pResult;
	if (m_trackedClientsList.Lookup(dwIP, pResult))
		return pResult->m_ItemsList.GetCount();
	return 0;
}

void CClientList::TrackBadRequest(const CUpDownClient* upcClient, sint32 nIncreaseCounter){
	CDeletedClient* pResult = NULL;
	if (upcClient->GetIP() == 0){
		ASSERT( false );
		return;
	}
	if (m_trackedClientsList.Lookup(upcClient->GetIP(), pResult)){
		pResult->m_dwInserted = ::GetTickCount();
		pResult->m_cBadRequest += nIncreaseCounter;
	}
	else{
		CDeletedClient* ccToAdd = new CDeletedClient(upcClient);
		ccToAdd->m_cBadRequest = nIncreaseCounter;
		m_trackedClientsList.SetAt(upcClient->GetIP(), ccToAdd);
	}
}

uint32 CClientList::GetBadRequests(const CUpDownClient* upcClient) const{
	CDeletedClient* pResult = NULL;
	if (upcClient->GetIP() == 0){
		ASSERT( false );
		return 0;
	}
	if (m_trackedClientsList.Lookup(upcClient->GetIP(), pResult)){
		return pResult->m_cBadRequest;
	}
	else
		return 0;
}

void CClientList::Process(){
	const uint32 cur_tick = ::GetTickCount();
	// [TPT] - eWombat SNAFU v2
	/*if (m_dwLastBannCleanUp + BAN_CLEANUP_TIME < cur_tick){
		m_dwLastBannCleanUp = cur_tick;
		
		POSITION pos = m_bannedList.GetStartPosition();
		uint32 nKey;
		uint32 dwBantime;
		while (pos != NULL){
			m_bannedList.GetNextAssoc( pos, nKey, dwBantime );
			if (dwBantime + CLIENTBANTIME < cur_tick )
				RemoveBannedClient(nKey);
		}
	}*/
	// [TPT] - eWombat SNAFU v2

	
	if (m_dwLastTrackedCleanUp + TRACKED_CLEANUP_TIME < cur_tick ){
		m_dwLastTrackedCleanUp = cur_tick;
		if (thePrefs.GetLogBannedClients())
			AddDebugLogLine(false, _T("Cleaning up TrackedClientList, %i clients on List..."), m_trackedClientsList.GetCount());
		POSITION pos = m_trackedClientsList.GetStartPosition();
		uint32 nKey;
		CDeletedClient* pResult;
		while (pos != NULL){
			m_trackedClientsList.GetNextAssoc( pos, nKey, pResult );
			if (pResult->m_dwInserted + KEEPTRACK_TIME < cur_tick ){
				m_trackedClientsList.RemoveKey(nKey);
				delete pResult;
			}
		}
		if (thePrefs.GetLogBannedClients())
			AddDebugLogLine(false, _T("...done, %i clients left on list"), m_trackedClientsList.GetCount());
	}

	//We need to try to connect to the clients in KadList
	//If connected, remove them from the list and send a message back to Kad so we can send a ACK.
	//If we don't connect, we need to remove the client..
	//The sockets timeout should delete this object.
	// buddy is just a flag that is used to make sure we are still connected or connecting to a buddy.
	buddyState buddy = Disconnected;
	// [TPT] - Code Improvement
	for (POSITION pos = KadList.GetHeadPosition(); pos != 0;)
	{
		POSITION cur_pos = pos;
		KadList.GetNext(pos);
		CUpDownClient* cur_client =	KadList.GetAt(cur_pos);
		if( !Kademlia::CKademlia::isRunning() )
		{
			//Clear out this list if we stop running Kad.
			//Setting the Kad state to KS_NONE causes it to be removed in the switch below.
			cur_client->SetKadState(KS_NONE);
		}
		switch(cur_client->GetKadState())
		{
			case KS_QUEUED_FWCHECK:
				//Another client asked us to try to connect to them to check their firewalled status.
				cur_client->TryToConnect(true);
				break;

			case KS_CONNECTING_FWCHECK:
				//Ignore this state as we are just waiting for results.
				break;

			case KS_CONNECTED_FWCHECK:
				//We successfully connected to the client.
				//We now send a ack to let them know.
				Kademlia::CKademlia::getUDPListener()->sendNullPacket(KADEMLIA_FIREWALLED_ACK, ntohl(cur_client->GetIP()), cur_client->GetKadPort());
				//We are done with this client. Set Kad status to KS_NONE and it will be removed in the next cycle.
				cur_client->SetKadState(KS_NONE);
				break;

			case KS_INCOMING_BUDDY:
				//A firewalled client wants us to be his buddy.
				//If we already have a buddy, we set Kad state to KS_NONE and it's removed in the next cycle.
				//If not, this client will change to KS_CONNECTED_BUDDY when it connects.
				if( m_bHaveBuddy == Connected )
					cur_client->SetKadState(KS_NONE);
				break;
			case KS_QUEUED_BUDDY:
				//We are firewalled and want to request this client to be a buddy.
				//But first we check to make sure we are not already trying another client.
				//If we are not already trying. We try to connect to this client.
				//If we are already connected to a buddy, we set this client to KS_NONE and it's removed next cycle.
				//If we are trying to connect to a buddy, we just ignore as the one we are trying may fail and we can then try this one.
				if( m_bHaveBuddy == Disconnected )
				{
					buddy = Connecting;
					m_bHaveBuddy = Connecting;
					cur_client->SetKadState(KS_CONNECTING_BUDDY);
					cur_client->TryToConnect(true);
					theApp.emuledlg->serverwnd->UpdateMyInfo();
				}
				else if( m_bHaveBuddy == Connected )
					cur_client->SetKadState(KS_NONE);
				break;
			case KS_CONNECTING_BUDDY:
				//We are trying to connect to this client.
				//Although it should NOT happen, we make sure we are not already connected to a buddy.
				//If we are we set to KS_NONE and it's removed next cycle.
				//But if we are not already connected, make sure we set the flag to connecting so we know 
				//things are working correctly.
				if( m_bHaveBuddy == Connected )
					cur_client->SetKadState(KS_NONE);
				else
				{
					ASSERT( m_bHaveBuddy == Connecting );
					buddy = Connecting;
				}
				break;
			case KS_CONNECTED_BUDDY:
				//A potential connected buddy client wanting to me in the Kad network
				//We set our flag to connected to make sure things are still working correctly.
				buddy = Connected;
				//If m_bhaveBuddy is not connected already, we set this client as our buddy!
				if( m_bHaveBuddy != Connected )
				{
					m_pBuddy = cur_client;
					m_bHaveBuddy = Connected;
					theApp.emuledlg->serverwnd->UpdateMyInfo();
				}
				if( m_pBuddy == cur_client && theApp.IsFirewalled() && cur_client->SendBuddyPingPong())
				{
					Packet* buddyPing = new Packet(OP_BUDDYPING, 0, OP_EMULEPROT);
					cur_client->SafeSendPacket(buddyPing);
					cur_client->SetLastBuddyPingPongTime();
				}
				break;
			default:
				RemoveFromKadList(cur_client);
		}
	}
	
	//We either never had a buddy, or lost our buddy..
	if( buddy == Disconnected )
	{
		if( m_bHaveBuddy != Disconnected || m_pBuddy )
		{
			if( Kademlia::CKademlia::isRunning() && theApp.IsFirewalled() )
			{
				//We are a lowID client and we just lost our buddy.
				//Go ahead and instantly try to find a new buddy.
				Kademlia::CKademlia::getPrefs()->setFindBuddy();
			}
			m_pBuddy = NULL;
			m_bHaveBuddy = Disconnected;
			theApp.emuledlg->serverwnd->UpdateMyInfo();
		}
	}

	if ( Kademlia::CKademlia::isConnected() )
	{
		if( Kademlia::CKademlia::isFirewalled() )
	{
			if( m_bHaveBuddy == Disconnected && Kademlia::CKademlia::getPrefs()->getFindBuddy() )
		{
				//We are a firewalled client with no buddy. We have also waited a set time 
				//to try to avoid a false firewalled status.. So lets look for a buddy..
			Kademlia::CSearch *findBuddy = new Kademlia::CSearch;
			findBuddy->setSearchTypes(Kademlia::CSearch::FINDBUDDY);
			Kademlia::CUInt128 ID(true);
			ID.xor(Kademlia::CKademlia::getPrefs()->getKadID());
			findBuddy->setTargetID(ID);
				if( !Kademlia::CSearchManager::startSearch(findBuddy) )
				{
					//This search ID was already going. Most likely reason is that
					//we found and lost our buddy very quickly and the last search hadn't
					//had time to be removed yet. Go ahead and set this to happen again
					//next time around.
					Kademlia::CKademlia::getPrefs()->setFindBuddy();
				}
		}
	}
		else
		{
			if( m_pBuddy )
			{
				//Lets make sure that if we have a buddy, they are firewalled!
				//If they are also not firewalled, then someone must have fixed their firewall or stopped saturating their line.. 
				//We just set the state of this buddy to KS_NONE and things will be cleared up with the next cycle.
				if( !m_pBuddy->HasLowID() )
					m_pBuddy->SetKadState(KS_NONE);
			}
		}
	}
	else
	{
		if( m_pBuddy )
		{
			//We are not connected anymore. Just set this buddy to KS_NONE and things will be cleared out on next cycle.
			m_pBuddy->SetKadState(KS_NONE);
		}
	}
	SendOHCBs();//[TPT] - Webcache 1.9 beta3
	//CleanUpClientList();//moved to maintimer
}

#ifdef _DEBUG
void CClientList::Debug_SocketDeleted(CClientReqSocket* deleted){
	for (POSITION pos = list.GetHeadPosition(); pos != NULL;){
		CUpDownClient* cur_client =	list.GetNext(pos);
		if (!AfxIsValidAddress(cur_client, sizeof(CUpDownClient))) {
			AfxDebugBreak();
		}
		if (thePrefs.m_iDbgHeap >= 2)
			ASSERT_VALID(cur_client);
		if (cur_client->socket == deleted){
			AfxDebugBreak();
		}
	}
}
#endif

bool CClientList::IsValidClient(CUpDownClient* tocheck)
{
	if (thePrefs.m_iDbgHeap >= 2)
		ASSERT_VALID(tocheck);
	return list.Find(tocheck);
}

void CClientList::RequestTCP(Kademlia::CContact* contact)
{
	uint32 nContactIP = ntohl(contact->getIPAddress());
	// don't connect ourself
	if (theApp.serverconnect->GetLocalIP() == nContactIP && thePrefs.GetPort() == contact->getTCPPort())
		return;

	CUpDownClient* pNewClient = FindClientByIP(nContactIP, contact->getTCPPort());

	if (!pNewClient)
		pNewClient = new CUpDownClient(0, contact->getTCPPort(), contact->getIPAddress(), 0, 0, false );

	//Add client to the lists to be processed.
	pNewClient->SetKadPort(contact->getUDPPort());
	pNewClient->SetKadState(KS_QUEUED_FWCHECK);
	KadList.AddTail(pNewClient);
	//This method checks if this is a dup already.
	AddClient(pNewClient);
}

void CClientList::RequestBuddy(Kademlia::CContact* contact)
{
	uint32 nContactIP = ntohl(contact->getIPAddress());
	// don't connect ourself
	if (theApp.serverconnect->GetLocalIP() == nContactIP && thePrefs.GetPort() == contact->getTCPPort())
		return;
	CUpDownClient* pNewClient = FindClientByIP(nContactIP, contact->getTCPPort());
	if (!pNewClient)
		pNewClient = new CUpDownClient(0, contact->getTCPPort(), contact->getIPAddress(), 0, 0, false );

	//Add client to the lists to be processed.
	pNewClient->SetKadPort(contact->getUDPPort());
	pNewClient->SetKadState(KS_QUEUED_BUDDY);
	byte ID[16];
	contact->getClientID().toByteArray(ID);
	pNewClient->SetUserHash(ID);
	AddToKadList(pNewClient);
	//This method checks if this is a dup already.
	AddClient(pNewClient);
}

void CClientList::IncomingBuddy(Kademlia::CContact* contact, Kademlia::CUInt128* buddyID )
{
	uint32 nContactIP = ntohl(contact->getIPAddress());
	//If eMule already knows this client, abort this.. It could cause conflicts.
	//Although the odds of this happening is very small, it could still happen.
	if (FindClientByIP(nContactIP, contact->getTCPPort()))
	{
		return;
	}

	// don't connect ourself
	if (theApp.serverconnect->GetLocalIP() == nContactIP && thePrefs.GetPort() == contact->getTCPPort())
		return;

	//Add client to the lists to be processed.
	CUpDownClient* pNewClient = new CUpDownClient(0, contact->getTCPPort(), contact->getIPAddress(), 0, 0, false );
	pNewClient->SetKadPort(contact->getUDPPort());
	pNewClient->SetKadState(KS_INCOMING_BUDDY);
	byte ID[16];
	contact->getClientID().toByteArray(ID);
	pNewClient->SetUserHash(ID);
	buddyID->toByteArray(ID);
	pNewClient->SetBuddyID(ID);
	AddToKadList(pNewClient);
	AddClient(pNewClient);
}

void CClientList::RemoveFromKadList(CUpDownClient* torem){
	POSITION pos = KadList.Find(torem);
	if(pos)
	{
		if(torem == m_pBuddy)
		{
			m_pBuddy = NULL;
			theApp.emuledlg->serverwnd->UpdateMyInfo();
		}
		KadList.RemoveAt(pos);
	}
}

void CClientList::AddToKadList(CUpDownClient* toadd){
	if(!toadd)
		return;
	POSITION pos = KadList.Find(toadd);
	if(pos)
	{
		return;
	}
	KadList.AddTail(toadd);
}

void CClientList::CleanUpClientList(){
	// we remove clients which are not needed any more by time
	// this check is also done on CUpDownClient::Disconnected, however it will not catch all
	// cases (if a client changes the state without beeing connected
	//
	// Adding this check directly to every point where any state changes would be more effective,
	// is however not compatible with the current code, because there are points where a client has
	// no state for some code lines and the code is also not prepared that a client object gets
	// invalid while working with it (aka setting a new state)
	// so this way is just the easy and safe one to go (as long as emule is basically single threaded)
	// [TPT] - Maella -Extended clean-up II-		
	static uint16 cleanUpCounter; cleanUpCounter++;
	if (cleanUpCounter < 600) { // 10 minutes
		return;		
	}

	cleanUpCounter = 0;

	uint32 cDeleted = 0;

	for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
	{
		POSITION cur_pos = pos;			
		CUpDownClient* pCurClient =	list.GetNext(pos);
		if ((pCurClient->GetUploadState() == US_NONE || pCurClient->GetUploadState() == US_BANNED && !pCurClient->IsBanned())
			&& pCurClient->GetDownloadState() == DS_NONE
			&& pCurClient->GetChatState() == MS_NONE
			&& pCurClient->GetKadState() == KS_NONE
			&& !pCurClient->GetDetailDialogInterface()->IsDialogOpen()	// [TPT] - SLUGFILLER: modelessDialogs - keep client object, otherwise the dialog would disappear
				&& pCurClient->socket == NULL
				// [TPT] - WebCache
				&& !pCurClient->IsProxy()) //JP don't delete SINGLEProxyClient
		{
			cDeleted++;
			// [TPT] - Xman
			if(!pCurClient->m_OtherNoNeeded_list.IsEmpty() || !pCurClient->m_OtherRequests_list.IsEmpty())
			{
				if (thePrefs.GetVerbose()) 
					PhoenixLogWarning(_T("Extended clean-up reports an error in CleanUpClientList with client %s"), pCurClient->GetUserName());
				pCurClient->m_lastCleanUpCheck = GetTickCount();
				return;
			}

			const DWORD delta = GetTickCount() - pCurClient->m_lastCleanUpCheck;
			if(delta > CLIENTLIST_CLEANUP_TIME)
			{
				// Remove instance of client			
				if ((thePrefs.GetBlockMaellaSpecificMsg() == false) && (thePrefs.GetVerbose()))
					PhoenixLogWarning(_T("Extended clean-up, delete client '%s' => upload+download states == none"), pCurClient->GetUserName());
			
				delete pCurClient;
			}
		}
		else
		{
			pCurClient->m_lastCleanUpCheck = GetTickCount();
		}
	}

	DEBUG_ONLY(AddDebugLogLine(false,_T("Cleaned ClientList, removed %i not used known clients"), cDeleted));
	
}


CDeletedClient::CDeletedClient(const CUpDownClient* pClient)
{
	m_cBadRequest = 0;
	m_dwInserted = ::GetTickCount();
	PORTANDHASH porthash = { pClient->GetUserPort(), pClient->Credits()};
	m_ItemsList.Add(porthash);
}

// ZZ:DownloadManager -->
void CClientList::ProcessA4AFClients() {
    //if(thePrefs.GetLogA4AF()) AddDebugLogLine(false, _T(">>> Starting A4AF check"));	
	// [TPT]
	for (POSITION pos = list.GetHeadPosition(); pos != NULL;){
		POSITION cur_pos = pos;
		CUpDownClient* cur_client =	list.GetNext(pos);

        if(cur_client->GetDownloadState() != DS_DOWNLOADING &&
           cur_client->GetDownloadState() != DS_CONNECTED &&
           (!cur_client->m_OtherRequests_list.IsEmpty() || !cur_client->m_OtherNoNeeded_list.IsEmpty())) {
            //AddDebugLogLine(false, _T("+++ ZZ:DownloadManager: Trying for better file for source: %s '%s'"), cur_client->GetUserName(), cur_client->reqfile->GetFileName());
               cur_client->SwapToAnotherFile(_T("Periodic A4AF check CClientList::ProcessA4AFClients()"), false, false, false, NULL, true, false);
        }
	}
    //if(thePrefs.GetLogA4AF()) AddDebugLogLine(false, _T(">>> Done with A4AF check"));
}
// <-- ZZ:DownloadManager

// [TPT] - Maella -Extended clean-up II-
void CClientList::CleanUp(CPartFile* pDeletedFile){
	for(POSITION pos = list.GetHeadPosition(); pos != NULL;){		
		CUpDownClient* cur_client =	list.GetNext(pos);
		cur_client->CleanUp(pDeletedFile);
	}	
}

// Maella -Inform sources of an ID change-
void CClientList::TrigReaskForDownload(bool immediate){
	for(POSITION pos = list.GetHeadPosition(); pos != NULL;){				
		CUpDownClient* cur_client =	list.GetNext(pos);
		if(immediate == true){
			// Compute the next time that the file might be saftly reasked (=> no Ban())
			cur_client->SetNextTCPAskedTime(0);
		}
		else{
			// Compute the next time that the file might be saftly reasked (=> no Ban())
			cur_client->TrigNextSafeAskForDownload(cur_client->GetRequestFile());
		}
	}	
}
// Maella end


//<<< [TPT] - eWombat SNAFU v2
bool CClientList::AddSnafuClient(CUpDownClient* toadd)
{
	if (toadd)
	{
		POSITION pos = m_snafulist.Find(toadd);
		if (!pos)
		{
			m_snafulist.AddTail(toadd);
			return true;
		}
	}
	return false;
}

bool CClientList::RemoveSnafuClient(CUpDownClient* toremove,bool bForce)
{
	if (toremove)
	{
		POSITION pos = m_snafulist.Find(toremove);
		if (pos)
		{
			if (!bForce)
			{
				CUpDownClient* client =	m_snafulist.GetAt(pos);
				if (client && (client->IsSnafu() || client->IsNotSUI() || client->IsBanned()))
					return false;
			}
			m_snafulist.RemoveAt(pos);
			return true;
		}
	}
	return false;
}

bool CClientList::IsSnafuClient(uint32 dwIP) const
{
	//Well, here I got some user dumps
	try
	{
		for (POSITION pos = m_snafulist.GetHeadPosition(); pos != 0;)
		{		
			POSITION cur_pos = pos;
			CUpDownClient* client =	m_snafulist.GetNext(pos);
			if (client == NULL) 
			{			
				continue;
			}
			if (client && client->GetIP() && client->GetIP() == dwIP)
				return true;	
		}
		return false;
	}
	catch (...)
	{
		PhoenixLogWarning(_T("Unkown exception in IsSnafuClient()"));
		return false;
	}
	
}


void CClientList::ProcessEx(void)
{
	const uint32 cur_tick = ::GetTickCount();

	// [TPT] - Code Improvement
	for (POSITION pos = m_snafulist.GetHeadPosition(); pos != 0;)
	{
		POSITION cur_pos = pos;
		CUpDownClient* client =	m_snafulist.GetNext(pos);
		if (!client)
		{
			m_snafulist.RemoveAt(cur_pos);
			continue;
		}
		try 
		{
			if (client->IsBannedForTriedCrash()) // Maella -AntiCrash/AntiFake handling- (Vorlost/Mortillo)
				continue;
			if (!client->IsSnafu() && !client->IsNotSUI() && !client->IsBanned())
			{
				m_snafulist.RemoveAt(cur_pos);
				continue;
			}
			if (client->IsBanned() && client->GetBannedTime() + CLIENTBANTIME < cur_tick)
			{				
				client->UnBan();
				if (!client->IsSnafu() && !client->IsNotSUI() && !client->IsBanned())
				{
					m_snafulist.RemoveAt(cur_pos);
					continue;
				}
			}
			if (client->IsSnafu() && client->GetSnafuTime() + CLIENTBANTIME < cur_tick)
			{
				theApp.uploadqueue->RemoveFromWaitingQueue(client,true);
				m_snafulist.RemoveAt(cur_pos);
				continue;
			}
			else if (client->IsNotSUI() && client->GetSUITime() + CLIENTBANTIME < cur_tick)
			{
				theApp.uploadqueue->RemoveFromWaitingQueue(client,true);
				m_snafulist.RemoveAt(cur_pos);
				continue;
			}
		} catch(...) {}
	} //for
	
}
//>>> [TPT] - eWombat SNAFU v2

// [TPT] - WebCache
// yonatan - not 2 be confused with the one in CUploadQueue!
CUpDownClient*	CClientList::FindClientByWebCacheUploadId(const uint32 id)
{
	for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
	{
		CUpDownClient* cur_client = list.GetNext(pos);
		if ( cur_client->m_uWebCacheUploadId == id )
			return cur_client;
	}
	return 0;
}
// [TPT] - WebCache

//[TPT] - Webcache 1.9 beta3
// Superlexx - OHCB manager
// returns a list of multi-OHCB-supporting clients ( = v1.9a or newer ) that should receive this OHCB immediately
CUpDownClientPtrList* CClientList::XpressOHCBRecipients(uint16 maxNrOfClients, const Requested_Block_Struct* block)
{
	uint16 part = block->StartOffset / PARTSIZE;
	CUpDownClientPtrList* newClients = new CUpDownClientPtrList;	// supporting multi-OHCB

	for (POSITION pos = list.GetHeadPosition(); pos;)
	{
		CUpDownClient* cur_client = list.GetNext(pos);
		if ( !(cur_client->HasLowID() || (cur_client->socket && cur_client->socket->IsConnected()))
			&& cur_client->SupportsMultiOHCBs()
			&& !cur_client->IsProxy()	// client isn't a proxy
			&& cur_client->m_bIsAcceptingOurOhcbs
			//&& theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID())	// client has requested a file - obsolete with MFR
			//&& !md4cmp(cur_client->GetUploadFileID(), block->FileID ) // file hashes do match - obsolete with MFR
			&& !cur_client->IsPartAvailable(part, block->FileID) // the MFR version
			&& cur_client->IsBehindOurWebCache())	// inefficient
			if (cur_client->socket && cur_client->socket->IsConnected())
				newClients->AddHead(cur_client); // add connected clients to head
			else
				newClients->AddTail(cur_client);
	}

	CUpDownClientPtrList* toReturn = new CUpDownClientPtrList;

	// TODO: optimize this, dependent on further protocol development
	POSITION pos1 = newClients->GetHeadPosition();
	while (pos1 != NULL
		&& toReturn->GetCount() <= maxNrOfClients)
		toReturn->AddTail(newClients->GetNext(pos1));

	delete newClients;
	return toReturn;
}

uint16 CClientList::GetNumberOfClientsBehindOurWebCacheHavingSameFileAndNeedingThisBlock(Pending_Block_Struct* pending, uint16 maxNrOfClients) // Superlexx - COtN
{
	uint16 toReturn = 0;
	uint16 part = pending->block->StartOffset / PARTSIZE;

	for (POSITION pos = list.GetHeadPosition(); pos && toReturn <= maxNrOfClients; list.GetNext(pos))
	{
		CUpDownClient* cur_client = list.GetAt( pos );
		if( cur_client->m_bIsAcceptingOurOhcbs
			&& !cur_client->IsProxy()
			//			&& cur_client != this // 'this' is the client we want to download data from - covered by IsPartAvaiable
			&& cur_client->IsBehindOurWebCache()
			&& !cur_client->IsPartAvailable(part, pending->block->FileID))
			toReturn++;
	}
	return toReturn;
}

void CClientList::SendOHCBs()
{
	const uint32 now = ::GetTickCount();
	if (now - m_dwLastSendOHCBs > WC_SENDOHCBS_INTERVAL)
	{
		m_dwLastSendOHCBs = now;
		CUpDownClient* cur_client = NULL;
		for (POSITION pos = list.GetHeadPosition(); pos;)
		{
			cur_client = list.GetNext(pos);
			if (cur_client->SupportsWebCache()
				&& cur_client->SupportsMultiOHCBs()
				&& now - cur_client->lastMultiOHCBPacketSent > WC_MULTI_OHCB_SEND_TIME
				&& cur_client->IsBehindOurWebCache())
				cur_client->SendOHCBsNow();
		}
	}
}