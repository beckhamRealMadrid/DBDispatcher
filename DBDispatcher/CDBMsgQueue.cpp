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

// ���� �޸� ����
VOID CDBMsgQueue::__Dtor()
{
	if (__mArray != nullptr)
	{
		delete[] __mArray;
		__mArray = nullptr;
	}
}

// ���� ũ�� ���� �� �޸� �Ҵ�
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

// ���۰� ���� �ִ��� Ȯ��
bool CDBMsgQueue::IsOpen() const
{
	return __mArray != nullptr;
}

// ���� �ݱ� (�޸� ����)
VOID CDBMsgQueue::Close()
{
	__Dtor();
}

// ���� ũ�� ��ȯ
DWORD CDBMsgQueue::size() const
{
	return __mSz;
}

// �ִ� ũ�� ��ȯ
DWORD CDBMsgQueue::max_size() const
{
	return __mMaxSize;
}

// ����ִ��� Ȯ��
bool CDBMsgQueue::empty() const
{
	return __mSz == 0;
}

// ���� á���� Ȯ��
bool CDBMsgQueue::Full() const
{
	return __mMaxSize == __mSz;
}

// ������ ù ��° ���� ��ȯ
CDBMsg& CDBMsgQueue::front()
{
	assert(IsOpen());
	assert(!empty());

	return __mArray[__mFrontIdx];
}

// ������ ������ ���� ��ȯ
CDBMsg& CDBMsgQueue::back()
{
	assert(IsOpen());
	assert(!empty());

	DWORD aBackIndex = (__mRearIdx > 0) ? (__mRearIdx - 1) : (__mMaxSize - 1);
	return __mArray[aBackIndex];
}

// ���ۿ� ������ �߰�
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

// ���ۿ� ������ �߰�
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

// ���ۿ��� ù ��° ������ ����
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

// ���ۿ��� ������ ������ ����
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

// ���� �ʱ�ȭ
void CDBMsgQueue::clear()
{
	__mSz = 0;
	__mFrontIdx = 0;
	__mRearIdx = 0;
}