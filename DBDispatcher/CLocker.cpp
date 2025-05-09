#include "pch.h"
#include "CLocker.h"

CLocker::CLocker()
{
	__ResetAttr();
}

CLocker::~CLocker()
{
	__Dtor();
}

VOID CLocker::__ResetAttr()
{
	__mIsOpen = false;
}

VOID CLocker::__Dtor()
{
	if (__mIsOpen)
	{
		assert(__mSec.RecursionCount <= 0);
		::DeleteCriticalSection(&__mSec);
		__mIsOpen = false;
	}
}

VOID CLocker::Close()
{
	__Dtor();
	__ResetAttr();
}

bool CLocker::IsOpen() const
{
	return __mIsOpen;
}

DWORD CLocker::Open(BOOL pIsManyRace, DWORD pSpin)
{
	assert(!IsOpen());

	DWORD aRv = 0;	

	// Spin Count는 WIN2K이상에서만 지원하며 만먁 Spin Count를 사용할수 없으면 일반적인 Critical Section을 사용한다.
	#if WINVER >= 0x0500
	if (0 < pSpin)
	{
		aRv = __InitializeCriticalSectionWithSpin(pIsManyRace, pSpin);
		if (aRv == 0)
		{
			return aRv;
		}

		// 실패 시 다음 단계로 진행
		assert(0);
	}
	#endif

	// Spin Count를 사용할 수 없는 경우 일반 Critical Section 초기화
	__try
	{
		::InitializeCriticalSection(&__mSec);
		__mIsOpen = true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		assert(0);
		aRv = STATUS_NO_MEMORY;
	}

	return aRv;
}

DWORD CLocker::__InitializeCriticalSectionWithSpin(BOOL pIsManyRace, DWORD pSpin)
{
	// Thread간의 경쟁이 자주 발생하는지 여부
	if (pIsManyRace)
	{
		pSpin |= MANY_RACE_FLAG;
	}

	// CPU가 1개이면 API자체가 무시하므로 실행될 기기의 CPU수는 고려할 필요가 없다.
	if (::InitializeCriticalSectionAndSpinCount(&__mSec, pSpin))
	{
		__mIsOpen = true;
		return 0;
	}

	return STATUS_NO_MEMORY; // 실패 시 반환
}

VOID CLocker::Lock() const
{
	assert(IsOpen());

	::EnterCriticalSection(&__mSec);	
}

bool CLocker::LockTry() const
{
	assert(IsOpen());

	if (::TryEnterCriticalSection(&__mSec))
	{
		DWORD aThd = GetCurrentThreadId();
		assert(aThd == HandleToUlong(__mSec.OwningThread));
		return true;
	}
	else
	{
		return false;
	}
}

VOID CLocker::Unlock() const
{
	assert(IsOpen());
	assert(0 < __mSec.RecursionCount);
	assert(GetCurrentThreadId() == HandleToUlong(__mSec.OwningThread));

	::LeaveCriticalSection(&__mSec);
}

BOOL CLocker::IsLock() const
{
	assert(IsOpen());

	return (0 < __mSec.RecursionCount) && (GetCurrentThreadId() == HandleToUlong(__mSec.OwningThread));
}