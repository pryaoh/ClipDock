// Copyright 1997-98 (c) Iuri Apollonio
// given freely to www.codeguru.com

// Ini.cpp: implementation of the CIni class.
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "Ini.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#include <stdlib.h>
#include <fstream>


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIni::CIni()
{
	csLineEnd = "\r\n";
}

CIni::~CIni()
{
	Clear();
}

bool CIni::Read(const TCHAR * cFileName)
{
	/*
	Clear();
	char buf[1024];
	FILE  * pFile;
	pFile =  _wfopen(cFileName,L"rb+");

	ifstream ifs(cFileName);
	
	while (ifs.good())
	{
		ifs.getline(buf, 1023);
		CString cs(buf);
		csList.Add(cs);
	}
	*/

	Clear();
	char buf[2];
	//ifstream ifs(cFileName);
	FILE  * pFile;
	pFile  = fopen(cFileName,"rb+");
	CString cs;
	memset(buf,0x00,sizeof(buf));
	while (!feof(pFile))
	{
		fread(buf,1,1,pFile );
		if(buf[0] != '\r')
		{
			cs += buf;
		} else {
			fread(buf,1,1,pFile );
			csList.Add(cs);
			cs = "";
		}
	}
	fclose(pFile);
	return true;
}

bool CIni::Write(const TCHAR * cFileName)
{
	ASSERT(cFileName);
	ofstream ofs(cFileName);
	int t, max = csList.GetSize();
	for (t = 0; t < max; t++)
	{
		ofs << csList.GetAt(t) << "\n";
	}
	return true;
}

// **********************************************************************************

void CIni::Clear()
{
	csList.RemoveAll();
}

// **********************************************************************************

int CIni::FindSection(const TCHAR * cSection)
{
	int t, max = csList.GetSize();
	CString csSection;
	csSection.Format(L"[%s]", cSection);

	for (t = 0; t < max; t++)
		if (csList.GetAt(t) == csSection) return t;

	return -1;
}

int CIni::InsertSection(const TCHAR * cSection)
{
	ASSERT(cSection);
	if (!cSection) return -1;

	int idx = FindSection(cSection);
	if (idx < 0)
	{
		CString csSection;
		csSection.Format(L"[%s]", cSection);
		idx = csList.Add(csSection);
	}
	return idx;
}

int CIni::FindItem(const int iSection, const TCHAR * cItem, CString &csVal)
{
	ASSERT(iSection >= 0);
	ASSERT(cItem);

	int max = csList.GetSize(), t;
	CString csItem(cItem), csLook;
	csItem += " = ";
	int iLen = csItem.GetLength();

	for (t = iSection; t < max; t++)
	{
		if (!IsSection(t))
		{
			csLook = csList.GetAt(t);
			if (csLook.GetLength() >= iLen)
			{
				if (csLook.Left(iLen) == csItem) 
				{
					if (csLook.GetLength() == iLen) csVal = "";
					else csVal = csLook.Right(csLook.GetLength() - iLen);
					return t;
				}
			}
		}
		else return -1;
	}
	return -1;
}

int CIni::FindMultiItem(const int iSection, const TCHAR * cItem, CString &csVal)
{
	ASSERT(iSection >= 0);
	ASSERT(cItem);

	int max = csList.GetSize(), t, i;
	CString csItem(cItem), csLook;
	csItem += " = \"";
	int iLen = csItem.GetLength();

	for (t = iSection; t < max; t++)
	{
		if (!IsSection(t))
		{
			csLook = csList.GetAt(t);
			if (csLook == csItem)
			{
				csVal = "";
				for (i = t + 1; i < max; i++)
				{
					csLook = csList.GetAt(i);
					if (csLook == '\"' || IsSection(i)) 
					{
						i = max; 
					}
					else 
					{
						if (csVal != "") csVal += csLineEnd;
						csVal += csLook;
					}
				}
				return t;
			}
		}
		else return -1;
	}
	return -1;
}

bool CIni::IsSection(const int iSection)
{
	ASSERT(iSection >= 0 && iSection < csList.GetSize());
	if (iSection >= 0 && iSection < csList.GetSize())
	{
		CString csItem = csList.GetAt(iSection);
		if (csItem.GetLength() > 2 && csItem.Left(1) == '[' && csItem.Right(1) == ']') return true;
	}
	return false;
}

bool CIni::RemoveSection(const TCHAR * cSection)
{
	int idx = FindSection(cSection);
	if (idx >= 0)
	{
		for (;;)
		{
			csList.RemoveAt(idx);
			if (idx >= csList.GetSize()) return true;
			if (IsSection(idx)) return true;
		}
	}
	return true;
}

void CIni::RemoveMultiLineItem(const int idx)
{
	int max = csList.GetSize(), t;
	CString csLook;

	for (t = idx; t < max; t++)
	{
		if (!IsSection(t))
		{
			csLook = csList.GetAt(t);
			if (csLook == '\"')
			{
				csList.RemoveAt(t);
				return;
			}
			csList.RemoveAt(t);
		}
		else return;
	}
}

// **********************************************************************************

bool CIni::SetValue(const TCHAR * cSection, const TCHAR * cItem, const bool bVal)
{
	int idx = InsertSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		int iIdx = FindItem(idx+1, cItem, csVal);
		csVal.Format(_T("%s = %s"), cItem, bVal ? _T("true") : L"false");
		if (iIdx >= 0) csList.SetAt(iIdx, csVal);
		else csList.InsertAt(idx+1, csVal);
		return true;
	}
	return false;
}

bool CIni::SetValue(const TCHAR * cSection, const TCHAR * cItem, const COLORREF crVal)
{
	int idx = InsertSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		int iIdx = FindItem(idx+1, cItem, csVal);
		csVal.Format(L"%s = %d", cItem, (DWORD) crVal);
		if (iIdx >= 0) csList.SetAt(iIdx, csVal);
		else csList.InsertAt(idx+1, csVal);
		return true;
	}
	return false;
}

bool CIni::SetValue(const TCHAR * cSection, const TCHAR * cItem, const char * cVal)
{
	int idx = InsertSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		int iIdx = FindItem(idx+1, cItem, csVal);
		csVal.Format(L"%s = %s", cItem, cVal);
		if (iIdx >= 0) csList.SetAt(iIdx, csVal);
		else csList.InsertAt(idx+1, csVal);
		return true;
	}
	return false;
}

bool CIni::SetValue(const TCHAR * cSection, const TCHAR * cItem, const double dbVal)
{
	int idx = InsertSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		int iIdx = FindItem(idx+1, cItem, csVal);
		csVal.Format(L"%s = %f", cItem, dbVal);
		if (iIdx >= 0) csList.SetAt(iIdx, csVal);
		else csList.InsertAt(idx+1, csVal);
		return true;
	}
	return false;
}

bool CIni::SetValue(const TCHAR * cSection, const TCHAR * cItem, const float fVal)
{
	int idx = InsertSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		int iIdx = FindItem(idx+1, cItem, csVal);
		csVal.Format(L"%s = %f", cItem, fVal);
		if (iIdx >= 0) csList.SetAt(iIdx, csVal);
		else csList.InsertAt(idx+1, csVal);
		return true;
	}
	return false;
}

bool CIni::SetValue(const TCHAR * cSection, const TCHAR * cItem, const long lVal)
{
	int idx = InsertSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		int iIdx = FindItem(idx+1, cItem, csVal);
		csVal.Format(L"%s = %d", cItem, lVal);
		if (iIdx >= 0) csList.SetAt(iIdx, csVal);
		else csList.InsertAt(idx+1, csVal);
		return true;
	}
	return false;
}

bool CIni::SetValue(const TCHAR * cSection, const TCHAR * cItem, const int iVal)
{
	int idx = InsertSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		int iIdx = FindItem(idx+1, cItem, csVal);
		csVal.Format(L"%s = %d", cItem, iVal);
		if (iIdx >= 0) csList.SetAt(iIdx, csVal);
		else csList.InsertAt(idx+1, csVal);
		return true;
	}
	return false;
}

bool CIni::SetMultiValue(const TCHAR * cSection, const TCHAR * cItem, const char * cVal)
{
	int idx = InsertSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		int iIdx = FindItem(idx+1, cItem, csVal);
		csVal.Format(L"%s = %s", cItem, cVal);
		TCHAR * c = csVal.LockBuffer();

		int i = csVal.Find('\r');
		while (i >= 0)
		{
			c[i] = '?';
			i = csVal.Find('\r');
		}
		i = csVal.Find('\n');
		while (i >= 0)
		{
			c[i] = '|';
			i = csVal.Find('\n');
		}

		csVal.UnlockBuffer();
		if (iIdx >= 0) csList.SetAt(iIdx, csVal);
		else csList.InsertAt(idx+1, csVal);
		return true;
	}
	return false;
}

bool CIni::SetValue(const TCHAR * cSection, const TCHAR * cItem, const CRect rcVal)
{
	int idx = InsertSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		int iIdx = FindItem(idx+1, cItem, csVal);
		csVal.Format(L"%s = RECT(%d,%d,%d,%d)", cItem, rcVal.left, rcVal.top, rcVal.right, rcVal.bottom);
		if (iIdx >= 0) csList.SetAt(iIdx, csVal);
		else csList.InsertAt(idx+1, csVal);
		return true;
	}
	return false;
}

bool CIni::SetValue(const TCHAR * cSection, const TCHAR * cItem, const CPoint ptVal)
{
	int idx = InsertSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		int iIdx = FindItem(idx+1, cItem, csVal);
		csVal.Format(L"%s = POINT(%d,%d)", cItem, ptVal.x, ptVal.y);
		if (iIdx >= 0) csList.SetAt(iIdx, csVal);
		else csList.InsertAt(idx+1, csVal);
		return true;
	}
	return false;
}


// **********************************************************************************

bool CIni::GetValue(const TCHAR * cSection, const TCHAR * cItem, COLORREF &crVal)
{
	

	int idx = FindSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		if (FindItem(idx+1, cItem, csVal) > 0)
		{
			char temp[256];
			memset(temp,0x00,sizeof(temp));
			wcstombs(temp,csVal,sizeof(temp));
			crVal = (COLORREF) (DWORD) atol(temp);
			return true;
		}
	}
	return false;
}

bool CIni::GetValue(const TCHAR * cSection, const TCHAR * cItem, bool &bVal)
{
	int idx = FindSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		if (FindItem(idx+1, cItem, csVal) > 0)
		{
			if (csVal.Find(L"true") >= 0) bVal = true; else bVal = false;
			return true;
		}
	}
	return false;
}

bool CIni::GetValue(const TCHAR * cSection, const TCHAR * cItem, CString &cVal)
{
	int idx = FindSection(cSection);
	if (idx >= 0)
	{
		if (FindItem(idx+1, cItem, cVal) > 0)
			return true;
	}
	return false;
}

bool CIni::GetValue(const TCHAR * cSection, const TCHAR * cItem, double &dbVal)
{
	int idx = FindSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		if (FindItem(idx+1, cItem, csVal) > 0)
		{
			char temp[256];
			memset(temp,0x00,sizeof(temp));
			wcstombs(temp,csVal,sizeof(temp));
			dbVal = (double) atof(temp);
			return true;
		}
	}
	return false;
}

bool CIni::GetValue(const TCHAR * cSection, const TCHAR * cItem, float &fVal)
{
	int idx = FindSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		if (FindItem(idx+1, cItem, csVal) > 0)
		{
			char temp[256];
			memset(temp,0x00,sizeof(temp));
			wcstombs(temp,csVal,sizeof(temp));
			fVal = (float) atof(temp);
			return true;
		}
	}
	return false;
}

bool CIni::GetValue(const TCHAR * cSection, const TCHAR * cItem, long &lVal)
{
	int idx = FindSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		if (FindItem(idx+1, cItem, csVal) > 0)
		{
			char temp[256];
			memset(temp,0x00,sizeof(temp));
			wcstombs(temp,csVal,sizeof(temp));
			lVal = (long) atol(temp);
			return true;
		}
	}
	return false;
}

bool CIni::GetValue(const TCHAR * cSection, const TCHAR * cItem, int &iVal)
{
	int idx = FindSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		if (FindItem(idx+1, cItem, csVal) > 0)
		{
			char temp[256];
			memset(temp,0x00,sizeof(temp));
			wcstombs(temp,csVal,sizeof(temp));
			iVal = (int) atoi(temp);
			return true;
		}
	}
	return false;
}

bool CIni::GetMultiValue(const TCHAR * cSection, const TCHAR * cItem, CString &cVal)
{
	int idx = FindSection(cSection);
	if (idx >= 0)
	{
		if (FindItem(idx+1, cItem, cVal) > 0)
		{
			TCHAR * ch = cVal.LockBuffer();
			int i = cVal.Find('?');
			while (i >= 0)
			{
				ch[i] = '\r';
				i = cVal.Find('?');
			}
			i = cVal.Find('|');
			while (i >= 0)
			{
				ch[i] = '\n';
				i = cVal.Find('|');
			}
			cVal.UnlockBuffer();
			return true;
		}
	}
	return false;
}

bool CIni::GetValue(const TCHAR * cSection, const TCHAR * cItem, CRect &rcVal)
{
	int idx = InsertSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		if (FindItem(idx+1, cItem, csVal) > 0)
		{
			TCHAR * pt = csVal.LockBuffer();
			int pf, t = 0, l = 0, r = 0, b = 0;
			pf = wscanf(csVal, L"RECT(%d,%d,%d,%d)", &l, &t, &r, &b);
			ASSERT(pf == 4);
			csVal.UnlockBuffer();
			rcVal.SetRect(l, t, r, b);
			return true;
		}
	}
	return false;
}

bool CIni::GetValue(const TCHAR * cSection, const TCHAR * cItem, CPoint &ptVal)
{
	int idx = InsertSection(cSection);
	if (idx >= 0)
	{
		CString csVal;
		if (FindItem(idx+1, cItem, csVal) > 0)
		{
			TCHAR * pt = csVal.LockBuffer();
			int pf, x = 0, y = 0;
			pf = wscanf(csVal, L"POINT(%d,%d)", &x, &y);
			ASSERT(pf == 2);
			csVal.UnlockBuffer();
			ptVal.x = x;
			ptVal.y = y;
			return true;
		}
	}
	return false;
}

