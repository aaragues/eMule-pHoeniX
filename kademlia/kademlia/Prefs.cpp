/*
Copyright (C)2003 Barry Dunne (http://www.emule-project.net)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/
#include "stdafx.h"
#include "Prefs.h"
#include "../utils/UInt128.h"
#include "../utils/MiscUtils.h"
#include "../kademlia/SearchManager.h"
#include "../../opcodes.h"
#include "../Routing/RoutingZone.h"
#include "../kademlia/kademlia.h"
#include "../kademlia/indexed.h"
#include "preferences.h"
#include "emule.h"
#include "emuledlg.h"
#include "SafeFile.h"
#include "serverlist.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CPrefs::CPrefs()
{
	CString filename = CMiscUtils::getAppDir();
	filename.Append(CONFIGFOLDER);
	filename.Append(_T("preferencesKad.dat"));
	init(filename.GetBuffer(0));
}

CPrefs::~CPrefs()
{
	if (m_filename.GetLength() > 0)
		writeFile();
}

void CPrefs::init(LPCTSTR filename)
{
	m_clientID.setValueRandom();
	m_lastContact = 0;
	m_recheckip = 0;
	m_firewalled = 0;
	m_totalFile = 0;
	m_totalStoreSrc = 0;
	m_totalStoreKey = 0;
	m_totalSource = 0;
	m_totalNotes = 0;
	m_totalStoreNotes = 0;
	m_Publish = false;
	m_clientHash.setValue((uchar*)thePrefs.GetUserHash());
	m_ip			= 0;
	m_ipLast		= 0;
	m_recheckip		= 0;
	m_firewalled	= 0;
	m_findBuddy		= false;
	m_kademliaUsers	= 0;
	m_kademliaFiles	= 0;
	m_filename = filename;
	m_lastFirewallState = true;
	readFile();
}

void CPrefs::readFile()
{
	try
	{
		CSafeBufferedFile file;
		CFileException fexp;
		if (file.Open(m_filename.GetBuffer(0),CFile::modeRead|CFile::osSequentialScan|CFile::typeBinary|CFile::shareDenyWrite, &fexp))
		{
			setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
			m_ip = file.ReadUInt32();
			file.ReadUInt16();
			file.ReadUInt128(&m_clientID);
			file.Close();
		}
	}
	//TODO: Make this catch an CFileException..
	catch (...) 
	{
		TRACE("Exception in CPrefs::readFile\n");
	}
}

void CPrefs::writeFile()
{
	try
	{
		CSafeBufferedFile file;
		CFileException fexp;
		if (file.Open(m_filename.GetBuffer(0), CFile::modeWrite|CFile::modeCreate|CFile::typeBinary|CFile::shareDenyWrite, &fexp))
		{
			setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
			file.WriteUInt32(m_ip);
			file.WriteUInt16(0); //This is no longer used.
			file.WriteUInt128(&m_clientID);
			file.WriteUInt8(0); //This is to tell older clients there are no tags..
			file.Close();
		}
	} 
	//TODO: Make this catch an CFileException 
	catch (...) 
	{
		TRACE("Exception in CPrefs::writeFile\n");
	}
}

void CPrefs::setIPAddress(uint32 val)
{
	//This is our first check on connect, init our IP..
	if( !val || !m_ipLast )
	{
		m_ip = val;
		m_ipLast = val;
	}
	//If the last check matches this one, reset our current IP.
	//If the last check does not match, wait for our next incoming IP.
	//This happens for two reasons.. We just changed our IP, or a client responsed with a bad IP.
	if( val == m_ipLast )
		m_ip = val;
	else
		m_ipLast = val;
}


bool CPrefs::hasLostConnection() const
{
	if( m_lastContact )
		return !((time(NULL) - m_lastContact) < KADEMLIADISCONNECTDELAY);
	return false;
}

bool CPrefs::hasHadContact() const
{
	if( m_lastContact )
	return ((time(NULL) - m_lastContact) < KADEMLIADISCONNECTDELAY);
	return false;
}

bool CPrefs::getFirewalled() const
{
	if( m_firewalled<2 )
	{
		//Not enough people have told us we are open but we may be doing a recheck
		//at the moment which will give a false lowID.. Therefore we check to see
		//if we are still rechecking and will report our last known state..
		if(getRecheckIP())
			return m_lastFirewallState;
		return true;
	}
	//We had enough tell us we are not firewalled..
	return false;
}
void CPrefs::setFirewalled()
{
	//Are are checking our firewall state.. Let keep a snapshot of our
	//current state to prevent false reports during the recheck..
	m_lastFirewallState = (m_firewalled<2);
	m_firewalled = 0;
	theApp.emuledlg->ShowConnectionState();
}

void CPrefs::incFirewalled()
{
	m_firewalled++;
	theApp.emuledlg->ShowConnectionState();
}

bool CPrefs::getFindBuddy() /*const*/
{
	if( m_findBuddy )
	{
		m_findBuddy = false;
		return true;
	}
	return false;
}

void CPrefs::setKademliaFiles()
{
	//There is no real way to know how many files are in the Kad network..
	//So we first try to see how many files per user are in the ED2K network..
	//If that fails, we use a set value based on previous tests..
	uint32 nServerAverage = 0;
	theApp.serverlist->GetAvgFile( nServerAverage );
	uint32 nKadAverage = Kademlia::CKademlia::getIndexed()->GetFileKeyCount();

	#ifdef _DEBUG
	CString method;
	#endif
	if( nServerAverage > nKadAverage )
	{
		#ifdef _DEBUG
		method.Format(_T("Kad file estimate used Server avg(%u)"), nServerAverage);
		#endif
		nKadAverage = nServerAverage;
	}
	#ifdef _DEBUG
	else
	{
		method.Format(_T("Kad file estimate used Kad avg(%u)"), nKadAverage);
	}
	#endif
	if( nKadAverage < 108 )
	{
		#ifdef _DEBUG
		method.Format(_T("Kad file estimate used default avg(108)"));
		#endif
		nKadAverage = 108;
	}
	#ifdef _DEBUG
	AddDebugLogLine(false, method);
	#endif
	m_kademliaFiles = nKadAverage*m_kademliaUsers;
}