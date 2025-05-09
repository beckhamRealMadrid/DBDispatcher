#include "pch.h"
#include "CDBMsg.h"

CDBMsg::CDBMsg()
{
	Reset();
}

CDBMsg::CDBMsg(WORD pCommand, const void* pPayload, DWORD pSize)
{
	assert(pPayload != nullptr);
	assert(pSize > 0);

	mCommand = pCommand;
	mPayload.assign(static_cast<const char*>(pPayload), static_cast<const char*>(pPayload) + pSize);
}

CDBMsg::CDBMsg(const CDBMsg& pOther)
	: mType(pOther.mType), mAccount(pOther.mAccount), mCommand(pOther.mCommand), mPayload(pOther.mPayload)
{

}

CDBMsg& CDBMsg::operator=(const CDBMsg& pOther)
{
	if (this != &pOther)
	{
		mType = pOther.mType;
		mAccount = pOther.mAccount;
		mCommand = pOther.mCommand;
		mPayload = pOther.mPayload;
	}
	
	return *this;
}

CDBMsg::CDBMsg(CDBMsg&& pOther) noexcept
	: mType(pOther.mType), mAccount(pOther.mAccount), mCommand(pOther.mCommand), mPayload(std::move(pOther.mPayload))
{

}

CDBMsg& CDBMsg::operator=(CDBMsg&& pOther) noexcept
{
	if (this != &pOther)
	{
		mType = pOther.mType;
		mAccount = pOther.mAccount;
		mCommand = pOther.mCommand;
		mPayload = std::move(pOther.mPayload);
	}
	
	return *this;
}

CDBMsg::~CDBMsg()
{

}

// 모든 필드를 초기화하는 완전 리셋 함수
VOID CDBMsg::Reset()
{
	mType = eTypeCnt;
	mAccount = 0;
	mCommand = 0;
	mPayload.clear();
}

VOID CDBMsg::SetNonSerialize(WORD pCommand, const void* pPayload, DWORD pSize)
{
	assert(pPayload != nullptr);
	assert(pSize > 0); // payload 크기 유효성 검증

	mType = eTypeNonSerialize;
	mCommand = pCommand;
	mPayload.assign(static_cast<const char*>(pPayload), static_cast<const char*>(pPayload) + pSize);
}

// 직렬화 전용 요청으로 세팅 (payload는 비움)
VOID CDBMsg::SetSerialize(WORD pAccount)
{
	mType = eTypeSerialize;
	mAccount = pAccount;
	mPayload.clear();
}

// 워커 스레드 종료용 메시지로 세팅 (payload는 비움)
VOID CDBMsg::SetStopThread()
{
	mType = eTypeStopThread;
	mPayload.clear();
}