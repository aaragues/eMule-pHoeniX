//This file is part of the eMule WebCache mod
//http://ispcachingforemule.de.vu
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
#include <crypto51/arc4.h>
#include "WebCache.h"

USING_NAMESPACE(CryptoPP)

class CWebCacheCryptography
{
public:
	CWebCacheCryptography(void);
	~CWebCacheCryptography(void);
	
	// A keypair is used to create the keys used for en/decryption;
	// this way the webcache owner cannot say what data is stored on the server,
	// if the file name does not contain plaintext information about the data.
	// Also, a harmful client that tries to download a block from the proxy
	// without having received the OP_HTTP_CACHED_BLOCK packet will fail to decrypt it;
	// the key can be used to encrypt the file hash and the offsets in the URL,
	// so the downloaders can be sure that the received key/filehash/offsets are correct.

	// localclient --> remoteclient : encryption
	byte localMasterKey[WC_KEYLENGTH];	// we receive this in the HelloPacket, it's constant
	byte localSlaveKey[WC_KEYLENGTH];	// we receive this key in the HTTP request
	byte localKey[WC_KEYLENGTH];		// this key is generated of the local master and slave keys, we must use it for encryption
	void RefreshLocalKey();			// computes the localkey
	MARC4 encryptor;					// uses localKey

	// localclient <-- remoteclient : decryption
	byte remoteMasterKey[WC_KEYLENGTH];	// this key is sent in the HelloPacket and stays constant
	byte remoteSlaveKey[WC_KEYLENGTH];	// we send this key in the HTTP request
	byte remoteKey[WC_KEYLENGTH];		// this key is generated of the remote master and slave keys, we must use it for decryption and in OP_HTTP_CACHED_BLOCK
	void RefreshRemoteKey();			// computes the remotekey
	MARC4 decryptor;					// uses remoteKey
	bool useNewKey;						// indicates that we need to set a new key for decryption
	void SetRemoteKey(const byte *key) { MEMCOPY( remoteKey, key, WC_KEYLENGTH ); }
	bool isProxy;						// true if it'a pure proxy client
};
