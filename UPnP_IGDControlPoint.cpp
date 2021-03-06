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
//
// ----------------------------------------------------------------------
//
// emulEspaņa. Added by MoNKi [MoNKi: -UPnPNAT Support-]

#include "StdAfx.h"
#include "emule.h"
#include "preferences.h"
#include "FirewallOpener.h"
#include "upnp_igdcontrolpoint.h"
#include "upnplib\upnp\inc\upnptools.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//Static Variables
CUPnP_IGDControlPoint*					CUPnP_IGDControlPoint::m_IGDControlPoint;
int										CUPnP_IGDControlPoint::m_nInstances;
UpnpClient_Handle						CUPnP_IGDControlPoint::m_ctrlPoint;
bool									CUPnP_IGDControlPoint::m_bInit;
CUPnP_IGDControlPoint::DEVICE_LIST		CUPnP_IGDControlPoint::m_devices;
CUPnP_IGDControlPoint::SERVICE_LIST		CUPnP_IGDControlPoint::m_knownServices;
CCriticalSection						CUPnP_IGDControlPoint::m_devListLock;
CUPnP_IGDControlPoint::MAPPING_LIST		CUPnP_IGDControlPoint::m_Mappings;
CCriticalSection						CUPnP_IGDControlPoint::m_MappingsLock;
CCriticalSection						CUPnP_IGDControlPoint::m_ActionThreadCS;
//-----

CUPnP_IGDControlPoint::CUPnP_IGDControlPoint(void)
{
	// Initialize variables
	m_nInstances = 0;
	m_IGDControlPoint = NULL;
	m_ctrlPoint = 0;
	m_bInit = false;
}

CUPnP_IGDControlPoint::~CUPnP_IGDControlPoint(void)
{
	//Lock devices and mappings before use it
	m_devListLock.Lock();
	m_MappingsLock.Lock();

	//Unregister control point and finish UPnP
	if(m_ctrlPoint){
		UpnpUnRegisterClient(m_ctrlPoint);
		UpnpFinish();
	}

	//Remove devices/services
	POSITION pos = m_devices.GetHeadPosition();
	bool found = false;
	while(pos){
		UPNP_DEVICE *item;
		item = m_devices.GetNext(pos);
		if(item)
			RemoveDevice(item);
	}
	m_devices.RemoveAll();
	m_knownServices.RemoveAll();

	//Remove mappings
	m_Mappings.RemoveAll();

	//Unlock devices and mappings
	m_MappingsLock.Unlock();
	m_devListLock.Unlock();
}

// Initialize all UPnP thing
bool CUPnP_IGDControlPoint::Init(){
	if(m_bInit)
		return true;

	// Init UPnP
	int rc;
	rc = UpnpInit( NULL, thePrefs.GetUPnPPort() );
	if (UPNP_E_SUCCESS != rc) {
		AddLogLine(false, _T("UPnP: Failed initiating UPnP on port %d [%s]"), thePrefs.GetUPnPPort(), GetErrDescription(rc) );
		UpnpFinish();
		return false;
	}

	// Check if you are in a LAN or directly connected to Internet
	if(!IsLANIP(UpnpGetServerIpAddress())){
		AddLogLine(false, _T("UPnP: Your IP is a public internet IP, UPnP will be disabled"));
		UpnpFinish();
		return false;
	}

	// Register us as a Control Point
	rc = UpnpRegisterClient( (Upnp_FunPtr)IGD_Callback, &m_ctrlPoint, &m_ctrlPoint );
	if (UPNP_E_SUCCESS != rc) {
		AddLogLine(false, _T("UPnP: Failed registering control point [%s]"), GetErrDescription(rc) );
		UpnpFinish();
		return false;
	}

	//Starts timer thread
	AfxBeginThread(TimerThreadFunc, this);

	//Open UPnP Server Port on Windows Firewall
	/*
	if(thePrefs.GetICFSupport()){
		if (theApp.m_pFirewallOpener->OpenPort(UpnpGetServerPort(), NAT_PROTOCOL_UDP, EMULE_DEFAULTRULENAME_UPNP_UDP, true))
			theApp.QueueLogLine(false, GetResString(IDS_FO_TEMPUDP_S), UpnpGetServerPort());
		else
			theApp.QueueLogLine(false, GetResString(IDS_FO_TEMPUDP_F), UpnpGetServerPort());

		if (theApp.m_pFirewallOpener->OpenPort(UpnpGetServerPort(), NAT_PROTOCOL_TCP, EMULE_DEFAULTRULENAME_UPNP_TCP, true))
			theApp.QueueLogLine(false, GetResString(IDS_FO_TEMPTCP_S), UpnpGetServerPort());
		else
			theApp.QueueLogLine(false, GetResString(IDS_FO_TEMPTCP_F), UpnpGetServerPort());
	}
*/
	m_bInit = true;
	AddLogLine(false, _T("UPnP: Initiated on %s:%d, searching devices..."), GetLocalIPStr(), UpnpGetServerPort());

	// Search Devices
	// Some routers only reply to one of this SSDP searchs:
	UpnpSearchAsync(m_ctrlPoint, 5, "upnp:rootdevice", &m_ctrlPoint);
	UpnpSearchAsync(m_ctrlPoint, 5, "urn:schemas-upnp-org:device:InternetGatewayDevice:1", &m_ctrlPoint);

	return m_bInit;
}

// Returns the port used by UPnP
unsigned int CUPnP_IGDControlPoint::GetPort(){
	if(!m_bInit)
		return 0;
    
	return UpnpGetServerPort(  );
}

// Handles all UPnP Events
int CUPnP_IGDControlPoint::IGD_Callback( Upnp_EventType EventType, void* Event, void* Cookie ){
	USES_CONVERSION;

	switch (EventType){
	//SSDP Stuff 
		case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
		case UPNP_DISCOVERY_SEARCH_RESULT:
		{
			//New UPnP Device found
			struct Upnp_Discovery *d_event = ( struct Upnp_Discovery * )Event;
			IXML_Document *DescDoc = NULL;
			int ret;

			if( d_event->ErrCode != UPNP_E_SUCCESS ) {
				if(thePrefs.GetUPnPVerboseLog())
					theApp.QueueDebugLogLine(false, _T("UPnP: Error in Discovery Callback [%s]"), GetErrDescription(d_event->ErrCode) );
			}

			CString devType;
			CString location = CA2CT(d_event->Location);
			
			// Download Device description
			if( ( ret = UpnpDownloadXmlDoc( d_event->Location, &DescDoc ) ) != UPNP_E_SUCCESS ) {
				if(thePrefs.GetUPnPVerboseLog())
					theApp.QueueDebugLogLine(false, _T("UPnP: Error obtaining device description from %s [%s]"), location, GetErrDescription(ret));
			}

			if(DescDoc){
				//Checks if is a InternetGatewayDevice and adds it to our list
				devType = GetFirstDocumentItem(DescDoc, _T("deviceType"));
				if(devType.CompareNoCase(IGD_DEVICE_TYPE) == 0){
					AddDevice(DescDoc, location, d_event->Expires);
				}

				ixmlDocument_free( DescDoc );
			}

			break;
		}
		case UPNP_DISCOVERY_SEARCH_TIMEOUT:
			break;
		case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
		{
			// UPnP Device Removed
			struct Upnp_Discovery *d_event = ( struct Upnp_Discovery * )Event;

			if( d_event->ErrCode != UPNP_E_SUCCESS ) {
				if(thePrefs.GetUPnPVerboseLog())
					theApp.QueueDebugLogLine(false, _T("UPnP: Error in Discovery ByeBye Callback [%s]"), GetErrDescription(d_event->ErrCode) );
			}

			CString devType = CA2CT(d_event->DeviceType);
			devType.Trim();

			//Checks if is a InternetGatewayDevice and removes it from our list
			if(devType.CompareNoCase(IGD_DEVICE_TYPE) == 0){
				RemoveDevice( CString(CA2CT(d_event->DeviceId)));
			}

			break;
		}
		case UPNP_EVENT_RECEIVED:
		{
			// Event reveived
			struct Upnp_Event *e_event = ( struct Upnp_Event * )Event;

			// Parses the event
			OnEventReceived( e_event->Sid, e_event->EventKey, e_event->ChangedVariables );

			break;
		}
	}
	return 0;
}

// Adds a port mapping to all known/future devices
// Returns: 
//		UNAT_OK:	If the mapping has been added to our mapping list.
//					(Maybe not to the device, because it runs in a different thread).
//		UNAT_ERROR: If you have not Init() the class.
CUPnP_IGDControlPoint::UPNPNAT_RETURN CUPnP_IGDControlPoint::AddPortMapping(CUPnP_IGDControlPoint::UPNPNAT_MAPPING *mapping){
	if(!m_bInit)
		return UNAT_ERROR;

	if(mapping->externalPort == 0){
		mapping->externalPort = mapping->internalPort;
	}

	m_devListLock.Lock();
	m_MappingsLock.Lock();

	//Checks if we already have this mapping
	bool found = false;
	POSITION pos = m_Mappings.GetHeadPosition();
	while(pos){
		UPNPNAT_MAPPING item;
		item = m_Mappings.GetNext(pos);
		if(item.externalPort == mapping->externalPort){
			found = true;
			pos = NULL;
		}
	}

	//Checks if we have devices
	if(m_devices.GetCount()==0){
		AddLogLine(false, _T("UPnP: Action \"AddPortMapping\" queued until a device is found [%s]"), mapping->description);
		if(!found){
			//If we do not have this mapping, add it to our list
			m_Mappings.AddTail(*mapping);
		}
	}
	else {
		if(!found){
			//If we do not have this mapping, add it to our list
			//and try to add it to all known services.
			m_Mappings.AddTail(*mapping);
			POSITION srvpos = m_knownServices.GetHeadPosition();
			while(srvpos){
				UPNP_SERVICE *srv;
				srv = m_knownServices.GetNext(srvpos);

				if(srv){
					UPNPNAT_ACTIONPARAM *action = new UPNPNAT_ACTIONPARAM;
					if(action){
						action->type = UPNPNAT_ACTION_ADD;
						action->srv = *srv;
						action->mapping = *mapping;
						action->bUpdating = false;
						
						// Starts thread to add the mapping
						AfxBeginThread(ActionThreadFunc, action);
					}
				}
			}
		}
	}

	m_MappingsLock.Unlock();
	m_devListLock.Unlock();
	return UNAT_OK;
}

// Removes a port mapping from all known devices
// Returns: 
//		UNAT_OK:	If the mapping has been removed from our mapping list.
//					(Maybe not from the device, because it runs in a different thread).
//		UNAT_ERROR: If you have not Init() the class.
CUPnP_IGDControlPoint::UPNPNAT_RETURN CUPnP_IGDControlPoint::DeletePortMapping(CUPnP_IGDControlPoint::UPNPNAT_MAPPING mapping, bool removeFromList){
	if(!m_bInit)
		return UNAT_ERROR;

	m_devListLock.Lock();
	m_MappingsLock.Lock();
	if(mapping.externalPort == 0){
		mapping.externalPort = mapping.internalPort;
	}

	bool found = false;
	POSITION old_pos, pos = m_Mappings.GetHeadPosition();
	while(pos){
		old_pos = pos;
		UPNPNAT_MAPPING item;
		item = m_Mappings.GetNext(pos);
		if(item.externalPort == mapping.externalPort){
			POSITION srvpos = m_knownServices.GetHeadPosition();
			while(srvpos){
				UPNP_SERVICE *srv;
				srv = m_knownServices.GetNext(srvpos);
				if(srv){
					//DeletePortMappingFromService(srv, &mapping);
					UPNPNAT_ACTIONPARAM *action = new UPNPNAT_ACTIONPARAM;
					if(action){
						action->type = UPNPNAT_ACTION_DELETE;
						action->srv = *srv;
						action->mapping = mapping;
						action->bUpdating = false;
						AfxBeginThread(ActionThreadFunc, action);
					}
				}
			}
			if(removeFromList)
				m_Mappings.RemoveAt(old_pos);

			pos = NULL;
		}
	}
	m_MappingsLock.Unlock();
	m_devListLock.Unlock();
	return UNAT_OK;
}

// Removes all mapping we have in our list from all known devices
bool CUPnP_IGDControlPoint::RemoveAllMappings(){
	m_devListLock.Lock();
	m_MappingsLock.Lock();

	if(m_devices.GetCount()>0){
		POSITION srvpos = m_knownServices.GetHeadPosition();
		while(srvpos){
			UPNP_SERVICE *srv;
			srv = m_knownServices.GetNext(srvpos);
			if(srv){
				POSITION map_pos = m_Mappings.GetHeadPosition();
				while(map_pos){
					UPNPNAT_MAPPING mapping = m_Mappings.GetNext(map_pos);
					DeletePortMappingFromService(srv, &mapping);
				}
			}
		}
	}

	m_MappingsLock.Unlock();
	m_devListLock.Unlock();

	return true;
}

bool CUPnP_IGDControlPoint::UpdateAllMappings( bool bLockDeviceList, bool bUpdating){
	if(bLockDeviceList)
		m_devListLock.Lock();

	if(m_devices.GetCount()>0){
		POSITION srvpos = m_knownServices.GetHeadPosition();
		while(srvpos){
			UPNP_SERVICE *srv;
			srv = m_knownServices.GetNext(srvpos);
			if(srv){
				if(IsServiceConnected(srv)){
					m_MappingsLock.Lock();
					POSITION map_pos = m_Mappings.GetHeadPosition();
					while(map_pos){
						UPNPNAT_MAPPING mapping = m_Mappings.GetNext(map_pos);

						UPNPNAT_ACTIONPARAM *action = new UPNPNAT_ACTIONPARAM;
						if(action){
							action->type = UPNPNAT_ACTION_ADD;
							action->srv = *srv;
							action->mapping= mapping;
							action->bUpdating = bUpdating;
							AfxBeginThread(ActionThreadFunc, action);
						}
					}
					m_MappingsLock.Unlock();
				}
			}
		}
	}

	if(bLockDeviceList)
		m_devListLock.Unlock();

	return true;
}

CString CUPnP_IGDControlPoint::GetFirstDocumentItem(IXML_Document * doc, CString item ){
	return GetFirstNodeItem( (IXML_Node *)doc, item );
}

CString CUPnP_IGDControlPoint::GetFirstElementItem( IXML_Element * element, CString item ){
	return GetFirstNodeItem( (IXML_Node *)element, item );
}

CString CUPnP_IGDControlPoint::GetFirstNodeItem( IXML_Node * root_node, CString item )
{
    IXML_Node *node;
	CString nodeVal;
	CString node_name;

	USES_CONVERSION;

	node = root_node;
    while( node != NULL ) {
		if (ixmlNode_getNodeType( node ) == eELEMENT_NODE){
			node_name = CA2CT(ixmlNode_getNodeName( node ));
			// match name
			if( node_name.CompareNoCase(item) == 0 ) {
				IXML_Node *text_node = NULL;

				text_node = ixmlNode_getFirstChild( node );
				if( text_node == NULL ) {
					return CString(_T(""));
				}

				nodeVal = CA2CT(ixmlNode_getNodeValue( text_node ));
				return nodeVal;
			}
			else{
				//Checks if we have something like "u:UPnPError" instead of "UPnPError"
				int pos = 0;
				pos = node_name.Find(_T(':'));
				if (pos != -1){
					node_name = node_name.Right(node_name.GetLength() - pos - 1);
					if( node_name.CompareNoCase(item) == 0 ) {
						IXML_Node *text_node = NULL;

						text_node = ixmlNode_getFirstChild( node );
						if( text_node == NULL ) {
							return CString(_T(""));
						}

						nodeVal = CA2CT(ixmlNode_getNodeValue( text_node ));
						return nodeVal;
					}
				}
			}
		}

		nodeVal = GetFirstNodeItem(ixmlNode_getFirstChild(node), item);
		if(!nodeVal.IsEmpty())
			return nodeVal;

        // free and next node
        node = ixmlNode_getNextSibling( node ); // next node
    }

	return CString(_T(""));
}

IXML_NodeList *CUPnP_IGDControlPoint::GetElementsByName(IXML_Document *doc, CString name){
	return GetElementsByName((IXML_Node*)doc, name);
}

IXML_NodeList *CUPnP_IGDControlPoint::GetElementsByName(IXML_Element *element, CString name){
	return GetElementsByName((IXML_Node*)element, name);
}

IXML_NodeList *CUPnP_IGDControlPoint::GetElementsByName(IXML_Node *root_node, CString name){
	IXML_NodeList *node_list = NULL;
	return GetElementsByName(root_node, name, &node_list);
}

IXML_NodeList *CUPnP_IGDControlPoint::GetElementsByName(IXML_Node *root_node, CString name, IXML_NodeList **nodelist){
    IXML_Node *node;

	USES_CONVERSION;

	if(nodelist == NULL)
		return NULL;

	node = root_node;
    while( node != NULL ) {
		if (ixmlNode_getNodeType( node ) == eELEMENT_NODE){
			CString node_name = CA2CT(ixmlNode_getNodeName( node ));
			// match name
			if( node_name.CompareNoCase(name) == 0 ) {
				ixmlNodeList_addToNodeList(nodelist, node);
			}
			else{
				int pos = 0;
				pos = node_name.Find(_T(':'));
				if (pos != -1){
					node_name = node_name.Right(node_name.GetLength() - pos - 1);
					if( node_name.CompareNoCase(name) == 0 ) {
						ixmlNodeList_addToNodeList(nodelist, node);
					}
				}
			}
		}

		GetElementsByName(ixmlNode_getFirstChild(node), name, nodelist);

        // free and next node
        node = ixmlNode_getNextSibling( node ); // next node
    }

	return *nodelist;
}

IXML_NodeList * CUPnP_IGDControlPoint::GetDeviceList( IXML_Document * doc ){
	return GetDeviceList((IXML_Node*)doc);
}

IXML_NodeList * CUPnP_IGDControlPoint::GetDeviceList( IXML_Element * element ){
	return GetDeviceList((IXML_Node*)element);
}

IXML_NodeList * CUPnP_IGDControlPoint::GetDeviceList( IXML_Node * root_node ){
    IXML_NodeList *DeviceList = NULL;
    IXML_NodeList *devlistnodelist = NULL;
    IXML_Node *devlistnode = NULL;

    devlistnodelist = GetElementsByName( root_node, _T("deviceList") );
    if( devlistnodelist && ixmlNodeList_length( devlistnodelist ) ) {
        devlistnode = ixmlNodeList_item( devlistnodelist, 0 );
        DeviceList =  GetElementsByName( devlistnode, _T("device") );
    }

    if( devlistnodelist )
        ixmlNodeList_free( devlistnodelist );

    return DeviceList;
}

IXML_NodeList * CUPnP_IGDControlPoint::GetServiceList( IXML_Element * element ){
	return GetServiceList((IXML_Node*) element);
}

IXML_NodeList * CUPnP_IGDControlPoint::GetServiceList( IXML_Node * root_node ){
    IXML_NodeList *ServiceList = NULL;
    IXML_NodeList *servlistnodelist = NULL;
    IXML_Node *servlistnode = NULL;

    servlistnodelist = GetElementsByName( root_node, _T("serviceList") );
    if( servlistnodelist && ixmlNodeList_length( servlistnodelist ) ) {
        servlistnode = ixmlNodeList_item( servlistnodelist, 0 );
        ServiceList = GetElementsByName( servlistnode, _T("service") );
    }

    if( servlistnodelist )
        ixmlNodeList_free( servlistnodelist );

    return ServiceList;
}

CUPnP_IGDControlPoint * CUPnP_IGDControlPoint::AddInstance(){
	if (m_IGDControlPoint == NULL) {
		m_IGDControlPoint = new CUPnP_IGDControlPoint();
	}
	m_nInstances++;
	return m_IGDControlPoint;
}

void CUPnP_IGDControlPoint::RemoveInstance(){
	m_nInstances--;
	if(m_nInstances == 0)
		delete m_IGDControlPoint;
}

void CUPnP_IGDControlPoint::AddDevice( IXML_Document * doc, CString location, int expires){
	m_devListLock.Lock();
	
	USES_CONVERSION;

	CString UDN;
	UDN = GetFirstDocumentItem(doc, _T("UDN"));
	
	POSITION pos = m_devices.GetHeadPosition();
	bool found = false;
	while(pos){
		UPNP_DEVICE *item;
		item = m_devices.GetNext(pos);
		if(item && item->UDN.CompareNoCase(UDN) == 0){
			found = true;
			//Update advertisement timeout
			item->AdvrTimeOut = expires;
			pos = NULL;
		}
	}
	
	if(!found){
		CString friendlyName = GetFirstDocumentItem(doc, _T("friendlyName"));
		AddLogLine(false, _T("UPnP: New compatible device found: %s"), friendlyName);
		if(thePrefs.GetUPnPVerboseLog())
			theApp.QueueDebugLogLine(false, _T("UPnP: New compatible device found: %s"), friendlyName);
		UPNP_DEVICE *device = new UPNP_DEVICE;
		if(device){
			CString BaseURL;
			m_devices.AddTail(device);
			device->DevType = GetFirstDocumentItem(doc, _T("deviceType"));
			device->UDN = UDN;
			device->DescDocURL = location;
			device->FriendlyName = friendlyName;
			device->AdvrTimeOut = expires;

			BaseURL = GetFirstDocumentItem(doc, _T("URLBase"));
			if(BaseURL.IsEmpty())
				BaseURL = location;

			IXML_NodeList* devlist1 = GetDeviceList(doc);
		    int length1 = ixmlNodeList_length( devlist1 );
			for( int n1 = 0; n1 < length1; n1++ ) {
				IXML_Element *dev1;
		        dev1 = ( IXML_Element * ) ixmlNodeList_item( devlist1, n1 );
				CString devType1 = GetFirstElementItem(dev1, _T("deviceType"));
				if(devType1.CompareNoCase(WAN_DEVICE_TYPE) == 0){
					//WanDevice
					UPNP_DEVICE *wandevice = new UPNP_DEVICE;
					wandevice->DevType = devType1;
					wandevice->UDN = GetFirstElementItem(dev1, _T("UDN"));
					wandevice->FriendlyName = GetFirstElementItem(dev1, _T("friendlyName"));
					device->EmbededDevices.AddTail(wandevice);

					if(thePrefs.GetUPnPVerboseLog())
						theApp.QueueDebugLogLine(false, _T("UPnP: Added embedded device: %s [%s]"), wandevice->FriendlyName, wandevice->DevType);

					IXML_NodeList* devlist2 = GetDeviceList(dev1);
					int length2 = ixmlNodeList_length( devlist2 );
					for( int n2 = 0; n2 < length2; n2++ ) {
						IXML_Element *dev2;
						dev2 = ( IXML_Element * ) ixmlNodeList_item( devlist2, n2 );
						CString devType2 = GetFirstElementItem(dev2, _T("deviceType"));
						if(devType2.CompareNoCase(WANCON_DEVICE_TYPE) == 0){
							//WanConnectionDevice, get services
							UPNP_DEVICE *wancondevice = new UPNP_DEVICE;
							wancondevice->DevType = devType2;
							wancondevice->UDN = GetFirstElementItem(dev2, _T("UDN"));
							wancondevice->FriendlyName = GetFirstElementItem(dev2, _T("friendlyName"));
							wandevice->EmbededDevices.AddTail(wancondevice);

							if(thePrefs.GetUPnPVerboseLog())
								theApp.QueueDebugLogLine(false, _T("UPnP: Added embedded device: %s [%s]"), wancondevice->FriendlyName, wancondevice->DevType);

							IXML_NodeList* services = GetServiceList(dev2);
							int length3 = ixmlNodeList_length( services );
							for( int n3 = 0; n3 < length3; n3++ ) {
								IXML_Element *srv;
								srv = ( IXML_Element * ) ixmlNodeList_item( services, n3 );
								CString srvType = GetFirstElementItem(srv, _T("serviceType"));
								if(srvType.CompareNoCase(WANIP_SERVICE_TYPE) == 0
									|| srvType.CompareNoCase(WANPPP_SERVICE_TYPE) == 0)
								{
									//Compatible Service found
									CString RelURL;
									char *cAbsURL;
									UPNP_SERVICE *service = new UPNP_SERVICE;
									service->ServiceType = srvType;
									service->ServiceID = GetFirstElementItem(srv, _T("serviceId"));

									RelURL = GetFirstElementItem(srv, _T("eventSubURL"));
									cAbsURL = new char[BaseURL.GetLength() + RelURL.GetLength() + 1];
									if(cAbsURL && UpnpResolveURL(CT2CA(BaseURL), CT2CA(RelURL), cAbsURL) == UPNP_E_SUCCESS)
									{
										service->EventURL = CA2CT(cAbsURL);
									}
									else{
										service->EventURL = RelURL;
									}
									delete[] cAbsURL;

									RelURL = GetFirstElementItem(srv, _T("controlURL"));
									cAbsURL = new char[BaseURL.GetLength() + RelURL.GetLength() + 1];
									if(cAbsURL && UpnpResolveURL(CT2CA(BaseURL), CT2CA(RelURL), cAbsURL) == UPNP_E_SUCCESS)
									{
										service->ControlURL = CA2CT(cAbsURL);
									}
									else{
										service->ControlURL = RelURL;
									}
									delete[] cAbsURL;

									service->Connected = -1; //Uninitialized

									wancondevice->Services.AddTail(service);
									m_knownServices.AddTail(service);

									if(IsServiceConnected(service)){
										if(thePrefs.GetUPnPVerboseLog())
											theApp.QueueDebugLogLine(false, _T("UPnP: Added service: %s (Connected)"), service->ServiceType);
									}
									else{
										if(thePrefs.GetUPnPVerboseLog())
											theApp.QueueDebugLogLine(false, _T("UPnP: Added service: %s (Disconnected)"), service->ServiceType);
									}

									//Subscribe to events
									int TimeOut = expires;
									int subsRet;
									subsRet = UpnpSubscribe(m_ctrlPoint, CT2CA(service->EventURL), &TimeOut, service->SubscriptionID);
									if(subsRet == UPNP_E_SUCCESS){
										if(thePrefs.GetUPnPVerboseLog())
											theApp.QueueDebugLogLine(false, _T("UPnP: Subscribed with service \"%s\" [SID=%s]"), service->ServiceType, CA2CT(service->SubscriptionID));
									}
									else{
										if(thePrefs.GetUPnPVerboseLog())
											theApp.QueueDebugLogLine(false, _T("UPnP: Failed to subscribe with service: %s [%s]"), service->ServiceType, GetErrDescription(subsRet));
									}
								}
							}
							ixmlNodeList_free(services);
						}
					}
					ixmlNodeList_free(devlist2);
				}
			}
			ixmlNodeList_free(devlist1);
		}
		UpdateAllMappings(false, false);
	}
	m_devListLock.Unlock();
}

void CUPnP_IGDControlPoint::RemoveDevice( UPNP_DEVICE *dev ){
	//Do not use the mutex here, is a recursive function
	CString fName = dev->FriendlyName;

	POSITION pos = dev->EmbededDevices.GetHeadPosition();
	while(pos){
		UPNP_DEVICE *item;
		item = dev->EmbededDevices.GetNext(pos);
		if(item){
			RemoveDevice(item);
		}
	}

	pos = dev->Services.GetHeadPosition();
	while(pos){
		UPNP_SERVICE *item;
		item = dev->Services.GetNext(pos);

		POSITION srvpos = m_knownServices.GetHeadPosition();
		while(srvpos){
			UPNP_SERVICE *item2;
			POSITION curpos = srvpos;
			item2 = m_knownServices.GetNext(srvpos);
			if(item == item2){
				m_knownServices.RemoveAt(curpos);
				srvpos = NULL;
			}
		}

		if(item)
			delete item;
	}

	delete dev;

	if(thePrefs.GetUPnPVerboseLog())
		theApp.QueueDebugLogLine(false, _T("UPnP: Device removed: %s"), fName);
}

void CUPnP_IGDControlPoint::RemoveDevice( CString devID ){
	m_devListLock.Lock();
	
	POSITION old_pos, pos = m_devices.GetHeadPosition();
	while(pos){
		UPNP_DEVICE *item;
		old_pos = pos;
		item = m_devices.GetNext(pos);
		if(item && item->UDN.CompareNoCase(devID) == 0){
			AddLogLine(false, _T("UPnP: Device removed: %s"), item->FriendlyName);
			RemoveDevice(item);
			m_devices.RemoveAt(old_pos);
			pos = NULL;
		}
	}
	m_devListLock.Unlock();
}

void CUPnP_IGDControlPoint::CheckTimeouts(){
	if(!m_bInit)
		return;

	m_devListLock.Lock();
	
	POSITION old_pos, pos = m_devices.GetHeadPosition();
	while(pos){
		UPNP_DEVICE *item;
		old_pos = pos;
		item = m_devices.GetNext(pos);
		item->AdvrTimeOut -= UPNP_ADVERTISEMENT_DECREMENT;
		if(item->AdvrTimeOut <= 0){
			RemoveDevice(item);
			m_devices.RemoveAt(old_pos);
		}
		else if(item->AdvrTimeOut < UPNP_ADVERTISEMENT_DECREMENT * 2){
			//About to expire, send a search to try to renew
            UpnpSearchAsync( m_ctrlPoint, UPNP_ADVERTISEMENT_DECREMENT,
                                       CT2CA(item->UDN), &m_ctrlPoint);
		}
	}
	m_devListLock.Unlock();
}

UINT CUPnP_IGDControlPoint::TimerThreadFunc( LPVOID pParam ){
	int sleepTime = UPNP_ADVERTISEMENT_DECREMENT * 1000;
	double updateTimeF = UPNP_PORT_LEASETIME * 1000 * 0.8f;
	static long int updateTime = updateTimeF;

	while(1){
		Sleep(sleepTime);
		CheckTimeouts();

		updateTime -= sleepTime;
		if(updateTime <= 0){
			UpdateAllMappings();
			updateTime = updateTimeF;
		}
	};

	return 1;
}

CUPnP_IGDControlPoint::UPNPNAT_RETURN CUPnP_IGDControlPoint::AddPortMappingToService(CUPnP_IGDControlPoint::UPNP_SERVICE *srv, CUPnP_IGDControlPoint::UPNPNAT_MAPPING *mapping, bool bIsUpdating){
	if(!m_bInit)
		return UNAT_ERROR;

	USES_CONVERSION;

	UPNPNAT_RETURN Status = UNAT_ERROR;

	CString protocol;
	CString desc;
	CString intPort, extPort;
	bool bUpdate = false;

	if (mapping->protocol == UNAT_TCP){
		protocol = _T("TCP");
	}
	else {
		protocol = _T("UDP");
	}
		
	desc.Format(UPNP_DESCRIPTION_FORMAT, mapping->description, protocol, mapping->externalPort);
	_itow(mapping->internalPort, intPort.GetBufferSetLength(10), 10);
	intPort.ReleaseBuffer();
	_itow(mapping->externalPort, extPort.GetBufferSetLength(10), 10);
	extPort.ReleaseBuffer();

	//Check if the portmaping already exists
	UPNPNAT_FULLMAPPING fullMapping;
	if(GetSpecificPortMappingEntryFromService(srv, mapping, &fullMapping, false) == UNAT_OK){
		if(fullMapping.internalClient == GetLocalIPStr()){
			if(fullMapping.description.Left(7).MakeLower() != _T("emule (")){
				if(thePrefs.GetUPnPVerboseLog())
					AddLogLine(false, _T("UPnP: Couldn't add mapping: \"%s\". The port %d is already mapped to other application (\"%s\" on %s:%d). [%s]"), desc, mapping->externalPort, fullMapping.description, fullMapping.internalClient, fullMapping.internalPort, srv->ServiceType);
				return UNAT_NOT_OWNED_PORTMAPPING;
			}
			else{
				if(fullMapping.enabled == TRUE && fullMapping.leaseDuration == 0){
					if(bIsUpdating){
						if(thePrefs.GetUPnPVerboseLog())
							theApp.QueueDebugLogLine(false, _T("UPnP: The port mapping \"%s\" don't needs an update. [%s]"), desc, srv->ServiceType);
					}
					else
						AddLogLine(false, _T("UPnP: The port mapping \"%s\" don't needs an update. [%s]"), desc, srv->ServiceType);
					//Mapping is already OK
					return UNAT_OK;
				}
				else{
					bUpdate = true;
				}
			}
		}
		else{
			if(thePrefs.GetUPnPVerboseLog())
				AddLogLine(false, _T("UPnP: Couldn't add mapping: \"%s\". The port %d is already mapped to other application (\"%s\" on %s:%d). [%s]"), desc, mapping->externalPort, fullMapping.description, fullMapping.internalClient, fullMapping.internalPort, srv->ServiceType);
			return UNAT_NOT_OWNED_PORTMAPPING;
		}
	}

	IXML_Document *actionNode = NULL;
	char actionName[] = "AddPortMapping";
	UpnpAddToAction( &actionNode, actionName , CT2CA(srv->ServiceType),
		"NewRemoteHost", "");
	UpnpAddToAction( &actionNode, actionName , CT2CA(srv->ServiceType),
		"NewExternalPort", CT2CA(extPort));
	UpnpAddToAction( &actionNode, actionName , CT2CA(srv->ServiceType),
		"NewProtocol", CT2CA(protocol.GetBuffer()));
	UpnpAddToAction( &actionNode, actionName , CT2CA(srv->ServiceType),
		"NewInternalPort", CT2CA(intPort));
	UpnpAddToAction( &actionNode, actionName , CT2CA(srv->ServiceType),
		"NewInternalClient", CT2CA(GetLocalIPStr().GetBuffer()));
	UpnpAddToAction( &actionNode, actionName , CT2CA(srv->ServiceType),
		"NewEnabled", "1");
	UpnpAddToAction( &actionNode, actionName , CT2CA(srv->ServiceType),
		"NewPortMappingDescription", CT2CA(desc));
	UpnpAddToAction( &actionNode, actionName , CT2CA(srv->ServiceType),
		"NewLeaseDuration", UPNP_PORT_LEASETIME_STR);

	IXML_Document* RespNode = NULL;
	int rc = UpnpSendAction( m_ctrlPoint,CT2CA(srv->ControlURL),
		CT2CA(srv->ServiceType), NULL, actionNode, &RespNode);
	if( rc != UPNP_E_SUCCESS)
	{
		//Maybe the IGD do not support dynamic port mappings,
		//try with an static one (NewLeaseDuration = 0).

	    if( RespNode )
		    ixmlDocument_free( RespNode );
		
		IXML_NodeList *nodeList = NULL;
		IXML_Node *textNode = NULL;
		IXML_Node *tmpNode = NULL;
		nodeList = GetElementsByName( actionNode, _T("NewLeaseDuration"));

		if( nodeList ) {
			if( ( tmpNode = ixmlNodeList_item( nodeList, 0 ) ) ) {
				textNode = ixmlNode_getFirstChild( tmpNode );
				ixmlNode_setNodeValue( textNode , "0");
			}
			ixmlNodeList_free( nodeList );
		}

		rc = UpnpSendAction( m_ctrlPoint,CT2CA(srv->ControlURL),
			CT2CA(srv->ServiceType), NULL, actionNode, &RespNode);
		if(rc == UPNP_E_SUCCESS){
			Status = UNAT_OK;

			if(bUpdate)
				AddLogLine(false, _T("UPnP: Updated port mapping \"%s\" (Static). [%s]"), desc, srv->ServiceType);
			else
				AddLogLine(false, _T("UPnP: Added port mapping \"%s\" (Static). [%s]"), desc, srv->ServiceType);
		}
		else{
			if(bIsUpdating){
				if(thePrefs.GetUPnPVerboseLog())
					theApp.QueueDebugLogLine(false, _T("UPnP: Failed to add port mapping \"%s\" [%s] [%s]"), desc, srv->ServiceType, GetErrDescription(RespNode, rc));
			}
			else
				AddLogLine(false, _T("UPnP: Failed to add port mapping \"%s\". [%s] [%s]"), desc, srv->ServiceType, GetErrDescription(RespNode, rc));
		}
	}
	else{
		Status = UNAT_OK;
		if(bUpdate)
			AddLogLine(false, _T("UPnP: Updated port mapping \"%s\" (Dynamic). [%s]"), desc, srv->ServiceType);
		else
			AddLogLine(false, _T("UPnP: Added port mapping \"%s\" (Dynamic). [%s]"), desc, srv->ServiceType);
	}

    if( RespNode )
        ixmlDocument_free( RespNode );

    if( actionNode )
        ixmlDocument_free( actionNode );

	return Status;
}

CUPnP_IGDControlPoint::UPNPNAT_RETURN CUPnP_IGDControlPoint::DeletePortMappingFromService(CUPnP_IGDControlPoint::UPNP_SERVICE *srv, CUPnP_IGDControlPoint::UPNPNAT_MAPPING *mapping){
	if(!m_bInit)
		return UNAT_ERROR;

	USES_CONVERSION;

	UPNPNAT_RETURN Status = UNAT_ERROR;

	CString protocol;
	CString desc;
	CString extPort;

	if (mapping->protocol == UNAT_TCP){
		protocol = _T("TCP");
	}
	else {
		protocol = _T("UDP");
	}
		
	desc.Format(UPNP_DESCRIPTION_FORMAT, mapping->description, protocol, mapping->externalPort);
	_itow(mapping->externalPort, extPort.GetBufferSetLength(10), 10);
	extPort.ReleaseBuffer();

	//Check if the portmapping belong to us
	UPNPNAT_FULLMAPPING fullMapping;
	UPNPNAT_RETURN ret = GetSpecificPortMappingEntryFromService(srv, mapping, &fullMapping, false);
	if( ret == UNAT_OK ){
		if(fullMapping.internalClient == GetLocalIPStr()){
			if(fullMapping.description.Left(7).MakeLower() != _T("emule (")){
				return UNAT_NOT_OWNED_PORTMAPPING;
			}
		}
		else{
			return UNAT_NOT_OWNED_PORTMAPPING;
		}
	}
	else if(ret == UNAT_NOT_FOUND){
		if(thePrefs.GetUPnPVerboseLog())
			theApp.QueueDebugLogLine(false, _T("UPnP: Deleting port mapping \"%s\" aborted. [%s] [NoSuchEntryInArray (714)]"), desc, srv->ServiceType);
		return UNAT_NOT_FOUND;
	}

	IXML_Document *actionNode = NULL;
	char actionName[] = "DeletePortMapping";
	UpnpAddToAction( &actionNode, actionName , CT2CA(srv->ServiceType),
		"NewRemoteHost", "");
	UpnpAddToAction( &actionNode, actionName , CT2CA(srv->ServiceType),
		"NewExternalPort", CT2CA(extPort));
	UpnpAddToAction( &actionNode, actionName , CT2CA(srv->ServiceType),
		"NewProtocol", CT2CA(protocol.GetBuffer()));

	IXML_Document* RespNode = NULL;
	int rc = UpnpSendAction( m_ctrlPoint,CT2CA(srv->ControlURL),
		CT2CA(srv->ServiceType), NULL, actionNode, &RespNode);
	if( rc != UPNP_E_SUCCESS)
	{
		AddLogLine(false, _T("UPnP: Failed to delete port mapping \"%s\". [%s] [%s]"), desc, srv->ServiceType, GetErrDescription(RespNode, rc));
	}
	else{
		Status = UNAT_OK;
		AddLogLine(false, _T("UPnP: Deleted port mapping \"%s\". [%s]"), desc, srv->ServiceType);
	}

    if( RespNode )
        ixmlDocument_free( RespNode );
    if( actionNode )
        ixmlDocument_free( actionNode );

	return Status;
}

CUPnP_IGDControlPoint::UPNPNAT_RETURN CUPnP_IGDControlPoint::GetSpecificPortMappingEntryFromService(CUPnP_IGDControlPoint::UPNP_SERVICE *srv, UPNPNAT_MAPPING *mapping, UPNPNAT_FULLMAPPING *fullMapping, bool bLog){
	if(!m_bInit)
		return UNAT_ERROR;

	UPNPNAT_RETURN status = UNAT_ERROR;

	USES_CONVERSION;

	CString protocol;
	CString desc;
	CString extPort;

	if (mapping->protocol == UNAT_TCP){
		protocol = _T("TCP");
	}
	else {
		protocol = _T("UDP");
	}
		
	desc.Format(UPNP_DESCRIPTION_FORMAT, mapping->description, protocol, mapping->externalPort);
	_itow(mapping->externalPort, extPort.GetBufferSetLength(10), 10);
	extPort.ReleaseBuffer();

	IXML_Document *actionNode = NULL;
	char actionName[] = "GetSpecificPortMappingEntry";
	UpnpAddToAction( &actionNode, actionName , CT2CA(srv->ServiceType),
		"NewRemoteHost", "");
	UpnpAddToAction( &actionNode, actionName , CT2CA(srv->ServiceType),
		"NewExternalPort", CT2CA(extPort));
	UpnpAddToAction( &actionNode, actionName , CT2CA(srv->ServiceType),
		"NewProtocol", CT2CA(protocol.GetBuffer()));

	IXML_Document* RespNode = NULL;
	int rc = UpnpSendAction( m_ctrlPoint,CT2CA(srv->ControlURL),
		CT2CA(srv->ServiceType), NULL, actionNode, &RespNode);
	if( rc != UPNP_E_SUCCESS)
	{
		if(rc == 714){
			//NoSuchEntryInArray
			status = UNAT_NOT_FOUND;
		}
		else{
			//Other error
			status = UNAT_ERROR;
		}

		if(bLog && thePrefs.GetUPnPVerboseLog())
			theApp.QueueDebugLogLine(false, _T("UPnP: Failed to get specific port mapping entry \"%s\". [%s]"), desc, GetErrDescription(RespNode, rc));
	}
	else{
		fullMapping->externalPort = mapping->externalPort;
		fullMapping->protocol = mapping->protocol;
		fullMapping->internalPort = _ttoi(GetFirstDocumentItem(RespNode, _T("NewInternalPort")));
		fullMapping->description = GetFirstDocumentItem(RespNode, _T("NewPortMappingDescription"));
		fullMapping->enabled = _ttoi(GetFirstDocumentItem(RespNode, _T("NewEnabled"))) == 0 ? false : true;
		fullMapping->leaseDuration = _ttoi(GetFirstDocumentItem(RespNode, _T("NewLeaseDuration")));

		// WinXP returns the host name instead of the ip for the "NewInternalClient" var.
		// Try to get the ip using the host name.
		CString internalClient = GetFirstDocumentItem(RespNode, _T("NewInternalClient"));

		LPHOSTENT lphost;
		lphost = gethostbyname(CT2CA(internalClient));
		if (lphost != NULL){
			internalClient = CA2CT(inet_ntoa(*(in_addr*)lphost->h_addr_list[0]));
		}

		fullMapping->internalClient = internalClient;
		status = UNAT_OK;
	}

    if( RespNode )
        ixmlDocument_free( RespNode );
    if( actionNode )
        ixmlDocument_free( actionNode );

	return status;
}

/////////////////////////////////////////////////////////////////////////////////
// Returns a CString with the local IP in format xxx.xxx.xxx.xxx
/////////////////////////////////////////////////////////////////////////////////
CString CUPnP_IGDControlPoint::GetLocalIPStr()
{
	if(!m_bInit)
		return CString(_T(""));
	else
		return CString(CA2CT(UpnpGetServerIpAddress()));
}

/////////////////////////////////////////////////////////////////////////////////
// Returns true if nIP is a LAN ip, false otherwise
/////////////////////////////////////////////////////////////////////////////////
bool CUPnP_IGDControlPoint::IsLANIP(WORD nIP){
	// filter LAN IP's
	// -------------------------------------------
	// 0.*
	// 10.0.0.0 - 10.255.255.255  class A
	// 172.16.0.0 - 172.31.255.255  class B
	// 192.168.0.0 - 192.168.255.255 class C

	unsigned char nFirst = (unsigned char)nIP;
	unsigned char nSecond = (unsigned char)(nIP >> 8);

	if (nFirst==192 && nSecond==168) // check this 1st, because those LANs IPs are mostly spreaded
		return true;

	if (nFirst==172 && nSecond>=16 && nSecond<=31)
		return true;

	if (nFirst==0 || nFirst==10)
		return true;

	return false; 
}

bool CUPnP_IGDControlPoint::IsLANIP(char *cIP){
	if(cIP == NULL)
		return false;

	USES_CONVERSION;

	CString strIP = CA2CT(cIP);
	CString tok;
	int nIP = 0;
	int curPos= 0;
	int ipPos = 0;

	tok= strIP.Tokenize(_T("."),curPos);
	while (!tok.IsEmpty() && ipPos < 4)
	{
		nIP |= _tstoi(tok) << 8*ipPos;

		ipPos++;
		tok= strIP.Tokenize(_T("."),curPos);
	}
	return IsLANIP(nIP);
}

UINT CUPnP_IGDControlPoint::ActionThreadFunc( LPVOID pParam ){
	m_ActionThreadCS.Lock();
	UPNPNAT_ACTIONPARAM *action	= reinterpret_cast<UPNPNAT_ACTIONPARAM *>(pParam);

	if(action){
		if(IsServiceConnected(&(action->srv))){
			switch(action->type){
				case UPNPNAT_ACTION_ADD:
					AddPortMappingToService(&(action->srv), &(action->mapping), &(action->bUpdating));
					break;
				case UPNPNAT_ACTION_DELETE:
					DeletePortMappingFromService(&(action->srv), &(action->mapping));
					break;
			}
		}
		delete action;
	}
	m_ActionThreadCS.Unlock();
	return 1;
}

bool CUPnP_IGDControlPoint::IsServiceConnected(CUPnP_IGDControlPoint::UPNP_SERVICE *srv){
	if(!m_bInit)
		return false;

	if(srv->Connected != -1)
		return (srv->Connected == 1 ? true : false);

	bool status = false;

	USES_CONVERSION;

	IXML_Document *actionNode = NULL;
	char actionName[] = "GetStatusInfo";
	actionNode = UpnpMakeAction(actionName, CT2CA(srv->ServiceType), 0, NULL);

	IXML_Document* RespNode = NULL;
	int rc = UpnpSendAction( m_ctrlPoint,CT2CA(srv->ControlURL),
		CT2CA(srv->ServiceType), NULL, actionNode, &RespNode);
	if( rc != UPNP_E_SUCCESS)
	{
		if(thePrefs.GetUPnPVerboseLog())
			theApp.QueueDebugLogLine(false, _T("UPnP: Failed to get connection status from [%s] [%s])"), srv->ServiceType, GetErrDescription(RespNode, rc));
	}
	else{
		CString strStatus = GetFirstDocumentItem(RespNode, _T("NewConnectionStatus"));
		if(strStatus.CompareNoCase(_T("Connected")) == 0)
			status = true;
	}

    if( RespNode )
        ixmlDocument_free( RespNode );
    if( actionNode )
        ixmlDocument_free( actionNode );

	srv->Connected = (status == true ? 1 : 0);
	return status;
}

void CUPnP_IGDControlPoint::OnEventReceived(Upnp_SID sid, int evntkey, IXML_Document * changes ){
	bool update = false;
	
	m_devListLock.Lock();

	POSITION srvpos = m_knownServices.GetHeadPosition();
	while(srvpos){
		UPNP_SERVICE *srv;
		srv = m_knownServices.GetNext(srvpos);

		if(srv){
			if(strcmp(srv->SubscriptionID, sid) == 0){
				//if(thePrefs.GetUPnPVerboseLog())
				//	theApp.QueueDebugLogLine(false, _T("UPnP: Event received from service \"%s\" [SID=%s]"), srv->ServiceType, CA2CT(srv->SubscriptionID));
				
				//Parse Event
				IXML_NodeList *properties = NULL;

				properties = GetElementsByName( changes, _T("property") );
				if(properties != NULL){
					int length = ixmlNodeList_length( properties );
					for( int i = 0; i < length; i++ ) {
						IXML_Element *property = NULL;

						property = ( IXML_Element * ) ixmlNodeList_item( properties, i );
						if(property){
							IXML_NodeList *variables = NULL;
							variables = GetElementsByName( property, _T("ConnectionStatus") );
							if(variables){
								int length2 = ixmlNodeList_length( variables );
								if( length2 > 0 ) {
									IXML_Element *variable = NULL;
									CString value;

									variable = ( IXML_Element * ) ixmlNodeList_item( variables, 0 );
										
									IXML_Node *child = ixmlNode_getFirstChild( ( IXML_Node * ) variable );

									if( ( child != 0 ) && ( ixmlNode_getNodeType( child ) == eTEXT_NODE ) ) {
										value = CA2CT(ixmlNode_getNodeValue( child ));
									}

									if(value.CompareNoCase(_T("Connected")) == 0){
										if(thePrefs.GetUPnPVerboseLog())
											theApp.QueueDebugLogLine(false, _T("UPnP: New ConnectionStatus for \"%s\" (Connected) [SID=%s] "), srv->ServiceType, CA2CT(srv->SubscriptionID));
										if(srv->Connected != 1)
											update = true;
										srv->Connected = 1;
									}
									else{
										if(thePrefs.GetUPnPVerboseLog())
											theApp.QueueDebugLogLine(false, _T("UPnP: New ConnectionStatus for \"%s\" (Disconnected) [SID=%s]"), srv->ServiceType, CA2CT(srv->SubscriptionID));
										if(srv->Connected != 0)
											update = true;
										srv->Connected = 0;
									}
								}
								ixmlNodeList_free( variables );
							}
						}
					}
					ixmlNodeList_free( properties );
				}
			}
		}
	}

	m_devListLock.Unlock();

	if(update)
		UpdateAllMappings();
}

CString CUPnP_IGDControlPoint::GetErrDescription(int err){
	CString errDesc;

	USES_CONVERSION;

	if(err < 0){
		errDesc = CA2CT(UpnpGetErrorMessage(err));
	}
	else{
		errDesc = _T("HTTP_ERROR");
	}

	errDesc.AppendFormat(_T(" (%d)"), err);
	return errDesc;
}

CString CUPnP_IGDControlPoint::GetErrDescription(IXML_Document* errDoc, int err){
	CString errDesc;
	int err_n = 0;

	err_n = _ttoi(GetFirstDocumentItem(errDoc, _T("errorCode")));
	if(err_n == 0){
		errDesc = GetErrDescription(err);
	}
	else{
		errDesc = GetFirstDocumentItem(errDoc, _T("errorDescription"));
		if(errDesc.IsEmpty())
			errDesc = _T("Unknown Error");
		errDesc.AppendFormat(_T(" (%d)"), err_n);
	}
	return errDesc;
}