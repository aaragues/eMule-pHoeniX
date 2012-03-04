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
#include "MapKey.h"

class CKnownFile;
typedef CMap<CCKey,const CCKey&,CKnownFile*,CKnownFile*> CKnownFilesMap;

class CKnownFileList 
{
	friend class CSharedFilesWnd;
	friend class CFileStatistic;
public:
	CKnownFileList();
	~CKnownFileList();

	bool	SafeAddKFile(CKnownFile* toadd);
	bool	Init();
	void	Save();
	void	Clear();
	void	Process();

	CKnownFile* FindKnownFile(LPCTSTR filename, uint32 date, uint32 size) const;
	CKnownFile* FindKnownFileByID(const uchar* hash) const;
	void	MergePartFileStats(CKnownFile* original);	// [TPT] - SLUGFILLER: mergeKnown - retrieve part file stats from known file
	bool	IsKnownFile(const CKnownFile* file) const;
	bool	IsFilePtrInList(const CKnownFile* file) const;
	const CKnownFilesMap& GetKnownFiles() const { return m_Files_map; }

private:
	uint16 	requested;
	uint16 	accepted;
	uint64 	transferred;
	uint32 	m_nLastSaved;
	CKnownFilesMap m_Files_map;
//emulEspa�a
	//Added by MoNKi [MoNKi: -Check already downloaded files-]
public:
	// [TPT] - MoNKi: -Check already downloaded files-
	int CheckAlreadyDownloadedFile(const uchar* hash, CString filename= _T(""), CArray<CKnownFile*,CKnownFile*> *files = NULL);
	bool CheckAlreadyDownloadedFileQuestion(const uchar* hash, CString filename);

	CKnownFilesMap* GetDownloadedFiles();
	bool RemoveKnownFile(CKnownFile *toRemove);
	void ClearHistory();
	// [TPT] - MoNKi: -Check already downloaded files-end
};
