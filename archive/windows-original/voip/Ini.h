// Copyright 1997-98 (c) Iuri Apollonio
// given freely to www.codeguru.com


// Ini.h: interface for the CIni class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INI_H__4A8E7C07_902F_11D1_8B70_0000B43382FE__INCLUDED_)
#define AFX_INI_H__4A8E7C07_902F_11D1_8B70_0000B43382FE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


//#include <afxcoll.h>

// *****************************************************************************
// Explanation of file structure:
// [A Section]
// A double Field = 0.23123
// A int Field = 132432
// A string Field = This is a string!
// A multicolumn string Field = This is a 1st line string?This is a 2nd line string?This is a 3rd line string
// [New Section]


class CIni  
{
public:
	void RemoveMultiLineItem(const int idx);
	CString csLineEnd;
	int FindItem(const int iSection, const TCHAR * cItem, CString &csVal);
	int FindMultiItem(const int iSection, const TCHAR * cItem, CString &csVal);
	bool RemoveSection(const TCHAR * cSection);
	bool IsSection(const int iSection);
	int InsertSection(const TCHAR * cSection);
	int FindSection(const TCHAR * cSection);

	bool SetMultiValue(const TCHAR * cSection, const TCHAR * cItem, const char * cVal);

	bool SetValue(const TCHAR * cSection, const TCHAR * cItem, const COLORREF crVal);
	bool SetValue(const TCHAR * cSection, const TCHAR * cItem, const bool bVal);
	bool SetValue(const TCHAR * cSection, const TCHAR * cItem, const char * cVal);
	bool SetValue(const TCHAR * cSection, const TCHAR * cItem, const double dbVal);
	bool SetValue(const TCHAR * cSection, const TCHAR * cItem, const float fVal);
	bool SetValue(const TCHAR * cSection, const TCHAR * cItem, const long lVal);
	bool SetValue(const TCHAR * cSection, const TCHAR * cItem, const int iVal);
	bool SetValue(const TCHAR * cSection, const TCHAR * cItem, const CRect rcVal);
	bool SetValue(const TCHAR * cSection, const TCHAR * cItem, const CPoint ptVal);

	bool GetMultiValue(const TCHAR * cSection, const TCHAR * cItem, CString &cVal);

	bool GetValue(const TCHAR * cSection, const TCHAR * cItem, COLORREF &crVal);
	bool GetValue(const TCHAR * cSection, const TCHAR * cItem, bool &bVal);
	bool GetValue(const TCHAR * cSection, const TCHAR * cItem, CString &cVal);
	bool GetValue(const TCHAR * cSection, const TCHAR * cItem, double &dbVal);
	bool GetValue(const TCHAR * cSection, const TCHAR * cItem, float &fVal);
	bool GetValue(const TCHAR * cSection, const TCHAR * cItem, long &lVal);
	bool GetValue(const TCHAR * cSection, const TCHAR * cItem, int &iVal);
	bool GetValue(const TCHAR * cSection, const TCHAR * cItem, CRect &rcVal);
	bool GetValue(const TCHAR * cSection, const TCHAR * cItem, CPoint &ptVal);

	void Clear();
	bool Write(const TCHAR * cFileName);
	bool Read(const TCHAR * cFileName);
	CIni();
	virtual ~CIni();
	CStringArray	csList;

};

#endif // !defined(AFX_INI_H__4A8E7C07_902F_11D1_8B70_0000B43382FE__INCLUDED_)
