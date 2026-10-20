
#include "stdafx.h"
#include "ScreenEncoder.h"
#include "MyComboBox.h"


// CMyComboBox

IMPLEMENT_DYNAMIC(CMyComboBox, CComboBoxEx)

CMyComboBox::CMyComboBox()
{

}

CMyComboBox::~CMyComboBox()
{
}

BEGIN_MESSAGE_MAP(CMyComboBox, CComboBoxEx)
	ON_CONTROL_REFLECT(CBN_EDITCHANGE, &CMyComboBox::OnCbnEditchange)
	ON_WM_CREATE()
	ON_NOTIFY_REFLECT(CBEN_ENDEDIT, &CMyComboBox::OnCbenEndedit)
	ON_CONTROL_REFLECT(CBN_EDITUPDATE, &CMyComboBox::OnCbnEditupdate)
END_MESSAGE_MAP()

// CMyComboBox message handlers



void CMyComboBox::OnCbnEditchange()
{
	int a=0;
	// TODO: Add your control notification handler code here
}

int CMyComboBox::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CComboBoxEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	::SetWindowLong(this->GetEditCtrl()->GetSafeHwnd(),GWL_STYLE,::GetWindowLong(this->GetEditCtrl()->GetSafeHwnd(),GWL_STYLE) | ES_MULTILINE|ES_WANTRETURN|ES_AUTOVSCROLL);

	// TODO:  Add your specialized creation code here

	return 0;
}

BOOL CMyComboBox::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	// TODO: Add your specialized code here and/or call the base class

	return CComboBoxEx::Create(dwStyle, rect, pParentWnd, nID);
}

void CMyComboBox::OnCbenEndedit(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

LRESULT CMyComboBox::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: Add your specialized code here and/or call the base class

	if(message == CB_GETDROPPEDSTATE)
	{
		bool bRet = CComboBoxEx::WindowProc(message, wParam, lParam);
		this->GetParent()->SendMessage(WM_COMMAND,ID_UPDATED,(LPARAM)this->GetSafeHwnd());
		//this->SendMessage(WM_KEYUP,'\n',0);
		return  bRet;
	}

	return CComboBoxEx::WindowProc(message, wParam, lParam);
}

void CMyComboBox::OnCbnEditupdate()
{	
	int a=0;
	// TODO: Add your control notification handler code here
}
