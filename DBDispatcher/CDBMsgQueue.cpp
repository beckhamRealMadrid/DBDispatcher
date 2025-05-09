#include "pch.h"
#include "CDBMsgQueue.h"

CDBMsgQueue::CDBMsgQueue() 
{
	__ResetAttr();
}

CDBMsgQueue::~CDBMsgQueue()
{
	__Dtor();
}

VOID CDBMsgQueue::__ResetAttr()
{
	__mMaxSize = 0;
	__mSz = 0;
	__mFrontIdx = 0;
	__mRearIdx = 0;
	__mArray = nullptr;
}

// 내부 메모리 해제
VOID CDBMsgQueue::__Dtor()
{
	if (__mArray != nullptr)
	{
		delete[] __mArray;
		__mArray = nullptr;
	}
}

// 고정 크기 설정 및 메모리 할당
DWORD CDBMsgQueue::Open(DWORD pCapacity)
{
	assert(!IsOpen());
	assert(pCapacity > 0);

	__mArray = new CDBMsg[pCapacity];
	if (__mArray != nullptr)
	{
		__mMaxSize = pCapacity;
		__mSz = 0;
		__mFrontIdx = 0;
		__mRearIdx = 0;
	}

	return (__mArray != nullptr) ? 0 : ERROR_NOT_ENOUGH_MEMORY;
}

// 버퍼가 열려 있는지 확인
bool CDBMsgQueue::IsOpen() const
{
	return __mArray != nullptr;
}

// 버퍼 닫기 (메모리 해제)
VOID CDBMsgQueue::Close()
{
	__Dtor();
}

// 현재 크기 반환
DWORD CDBMsgQueue::size() const
{
	return __mSz;
}

// 최대 크기 반환
DWORD CDBMsgQueue::max_size() const
{
	return __mMaxSize;
}

// 비어있는지 확인
bool CDBMsgQueue::empty() const
{
	return __mSz == 0;
}

// 가득 찼는지 확인
bool CDBMsgQueue::Full() const
{
	return __mMaxSize == __mSz;
}

// 버퍼의 첫 번째 원소 반환
CDBMsg& CDBMsgQueue::front()
{
	assert(IsOpen());
	assert(!empty());

	return __mArray[__mFrontIdx];
}

// 버퍼의 마지막 원소 반환
CDBMsg& CDBMsgQueue::back()
{
	assert(IsOpen());
	assert(!empty());

	DWORD aBackIndex = (__mRearIdx > 0) ? (__mRearIdx - 1) : (__mMaxSize - 1);
	return __mArray[aBackIndex];
}

// 버퍼에 데이터 추가
bool CDBMsgQueue::push_back(const CDBMsg& pMsg)
{
	assert(IsOpen());
	assert(!Full());
	
	if (Full())
	{
		return false;
	}

	__mArray[__mRearIdx] = pMsg;
	__mRearIdx = (__mRearIdx + 1) % __mMaxSize;
	++__mSz;

	return true;
}

// 버퍼에 데이터 추가
bool CDBMsgQueue::push_back(CDBMsg&& pMsg)
{
	assert(IsOpen());
	assert(!Full());

	if (Full())
	{
		return false;
	}

	__mArray[__mRearIdx] = std::move(pMsg);
	__mRearIdx = (__mRearIdx + 1) % __mMaxSize;
	++__mSz;

	return true;
}

// 버퍼에서 첫 번째 데이터 제거
bool CDBMsgQueue::pop_front()
{
	assert(IsOpen());
	assert(!empty());

	if (empty())
	{
		return false;
	}

	__mFrontIdx = (__mFrontIdx + 1) % __mMaxSize;
	--__mSz;

	return true;
}

// 버퍼에서 마지막 데이터 제거
bool CDBMsgQueue::pop_back()
{
	assert(IsOpen());
	assert(!empty());

	if (empty())
	{
		return false;
	}

	__mRearIdx = (__mRearIdx == 0) ? (__mMaxSize - 1) : (__mRearIdx - 1);
	--__mSz;

	return true;
}

// 버퍼 초기화
void CDBMsgQueue::clear()
{
	__mSz = 0;
	__mFrontIdx = 0;
	__mRearIdx = 0;
}