#include "pch.h"
#include "CArrayDBMsg.h"

CArrayDBMsg::CArrayDBMsg()
{
	__ResetAttr();
}

CArrayDBMsg::~CArrayDBMsg()
{
	Close();
}

VOID CArrayDBMsg::__ResetAttr()
{
	__mArray.clear();
	__mSize = 0;
}

DWORD CArrayDBMsg::Open(DWORD pCount)
{
	if (IsOpen() || pCount == 0)
		return ERROR_INVALID_PARAMETER;

	try
	{
		__mArray.resize(pCount, nullptr);  // 모두 nullptr 초기화
		__mSize = pCount;
	}
	catch (...)
	{
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	return 0;
}

VOID CArrayDBMsg::Close()
{
	__ResetAttr();
}

BOOL CArrayDBMsg::IsOpen() const
{
	return !__mArray.empty();
}

const DWORD& CArrayDBMsg::GetSize() const
{
	assert(IsOpen());
	
	return __mSize;
}

CDBMsg* CArrayDBMsg::Get(DWORD pIndex)
{
	assert(IsOpen());
	assert(pIndex < __mSize);
	
	return __mArray[pIndex];
}

const CDBMsg* CArrayDBMsg::Get(DWORD pIndex) const
{
	assert(IsOpen());
	assert(pIndex < __mSize);
	
	return __mArray[pIndex];
}

CDBMsg*& CArrayDBMsg::operator[](int pIndex)
{
	assert(IsOpen());
	assert(pIndex >= 0 && static_cast<DWORD>(pIndex) < __mSize);
	
	return __mArray[pIndex];
}

const CDBMsg* CArrayDBMsg::operator[](int pIndex) const
{
	assert(IsOpen());
	assert(pIndex >= 0 && static_cast<DWORD>(pIndex) < __mSize);
	
	return __mArray[pIndex];
}

VOID CArrayDBMsg::ForEach(const std::function<VOID(CDBMsg*)>& pFunc)
{
	assert(IsOpen());
	
	for (auto& ptr : __mArray)
		pFunc(ptr);
}

VOID CArrayDBMsg::ForEach(const std::function<VOID(const CDBMsg*)>& pFunc) const
{
	assert(IsOpen());
	
	for (auto& ptr : __mArray)
		pFunc(ptr);
}