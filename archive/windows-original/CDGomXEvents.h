// CDGomXEvents.h  : Microsoft Visual C++로 만든 ActiveX 컨트롤 래퍼 클래스의 선언입니다.

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDGomXEvents

class CDGomXEvents : public COleDispatchDriver
{
public:
	CDGomXEvents() {}		// COleDispatchDriver 기본 생성자를 호출합니다.
	CDGomXEvents(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	CDGomXEvents(const CDGomXEvents& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// 특성입니다.
public:

// 작업입니다.
public:

	void EndOfStream(long Result)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x1, DISPATCH_METHOD, VT_EMPTY, NULL, parms, Result);
	}
	void GomxError(long ErrorCode)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x2, DISPATCH_METHOD, VT_EMPTY, NULL, parms, ErrorCode);
	}
	void GomxState(long State)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x3, DISPATCH_METHOD, VT_EMPTY, NULL, parms, State);
	}


};
