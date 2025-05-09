#pragma once

#include <deque>
#include "CLocker.h"

class CDBMsg;

/**
 * 세마포어를 사용해 스레드 간 메시지를 안전하게 송수신하는 동기화 큐 클래스이다.

   WaitForSingleObjectEx, ReleaseSemaphore 동기화 개념
   1. ReleaseSemaphore() : 카운트를 증가 (신호 발생)
   2. WaitForSingleObjectEx() : 카운트를 감소하면서 신호 대기

   Scenario 1: WaitForSingleObjectEx()가 먼저 대기 중
   1. 스레드가 WaitForSingleObjectEx(__mSemaphore, INFINITE, TRUE)로 대기
   2. 아직 세마포어 카운트는 0이라서 잠듬
   3. 누군가 ReleaseSemaphore(__mSemaphore, 1, NULL) 호출
   4. 대기 중인 스레드가 즉시 깨어남
   5. 세마포어 카운트는 다시 0이 됨

   초기 상태: [세마포어 카운트: 0]
   Thread A: WaitForSingleObjectEx(...)  -> 대기
   Thread B: ReleaseSemaphore(..., 1)    -> Thread A 깨어남
      
   Scenario 2: ReleaseSemaphore()가 먼저 실행됨
   1. ReleaseSemaphore(__mSemaphore, 1, NULL) 호출 -> 세마포어 카운트는 1이 됨
   2. 아직 아무도 WaitForSingleObjectEx() 호출 안 함
   3. 나중에 스레드가 WaitForSingleObjectEx(...) 호출
   4. 세마포어 카운트가 1이므로 즉시 반환됨 (대기 없이)
   5. 카운트는 다시 0으로 감소

   초기 상태: [세마포어 카운트: 0]
   Thread B: ReleaseSemaphore(..., 1)    -> [카운트 = 1]
   Thread A: WaitForSingleObjectEx(...)  -> 즉시 반환
 */
class CSemaphoreChannel
{
public:
	CSemaphoreChannel();
	~CSemaphoreChannel();
public:
	DWORD	Open();
	VOID	Close();
	BOOL	IsOpen() const;
	VOID	Clear();
	DWORD	PushAndSignal(CDBMsg* pMsg);
	DWORD	WaitAndPop(CDBMsg*& pMsg, DWORD pTimeout = INFINITE);
	DWORD	GetSz() const;
	BOOL	IsEmpty() const;
	BOOL	IsExist(CDBMsg* pMsg) const;
	bool	Pop(CDBMsg*& pMsg);
private:
	VOID	__ResetAttr();
	VOID	__Dtor();
private:
	typedef CLockerAuto<CLocker>	__TLockerAuto;
private:
	__TLockerAuto::TLocker	__mLocker;		// 스레드 동기화 객체
	HANDLE					__mSemaphore;	// 세마포어 핸들
	std::deque<CDBMsg*>		__mQueue;		// 작업을 저장하는 큐
};