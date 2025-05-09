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

// ��� �ʵ带 �ʱ�ȭ�ϴ� ���� ���� �Լ�
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
	assert(pSize > 0); // payload ũ�� ��ȿ�� ����

	mType = eTypeNonSerialize;
	mCommand = pCommand;
	mPayload.assign(static_cast<const char*>(pPayload), static_cast<const char*>(pPayload) + pSize);
}

// ����ȭ ���� ��û���� ���� (payload�� ���)
VOID CDBMsg::SetSerialize(WORD pAccount)
{
	mType = eTypeSerialize;
	mAccount = pAccount;
	mPayload.clear();
}

// ��Ŀ ������ ����� �޽����� ���� (payload�� ���)
VOID CDBMsg::SetStopThread()
{
	mType = eTypeStopThread;
	mPayload.clear();
}