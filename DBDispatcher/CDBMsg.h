#pragma once

#include <vector>

/**
 * DB �۾� ��û ������ ��� �޽��� ��ü�� ����ȭ, ������ȭ ���а� ���� �����͸� �����Ѵ�.
 */
class CDBMsg
{
public:
	enum EType { eTypeNonSerialize = 0, eTypeSerialize, eTypeStopThread, eTypeCnt };
public:
	CDBMsg();
	CDBMsg(WORD pCommand, const void* pPayload, DWORD pSize);
	CDBMsg(const CDBMsg& pOther);
	CDBMsg& operator=(const CDBMsg& pOther);
	// ===== �̵� ����ȭ ���� ���� =====
	// CDBMsg�� ���� payload(std::vector<char>)�� �̵� ����ȭ�� ����
	// �̵� ������(CDBMsg(CDBMsg&&))�� �̵� ���� ������(CDBMsg& operator=(CDBMsg&&))�� ��������� ����.
	//
	// - std::vector<char>�� �̵� �� ���� �޸� �����͸� ���� ���� �����ϹǷ�
	//   CDBMsg ���� ��뷮 ������ ���� ���� ������ �̵��� �� �ִ�.
	// - �� �̵��� �߻��Ϸ��� �ݵ�� std::move()�� ����� rvalue�� ĳ�����ؾ� �Ѵ�.
	//   (��: queue.push_back(std::move(msg)))
	// - std::move() ���� �ѱ�� ���� ������/�����ڰ� ȣ��Ǿ� ���ʿ��� ���簡 �߻��� �� �ִ�.
	// ==================================
	CDBMsg(CDBMsg&& pOther) noexcept;
	CDBMsg& operator=(CDBMsg&& pOther) noexcept;
	// ==================================
	~CDBMsg();
public:
	VOID	Reset();
	VOID	SetNonSerialize(WORD pCommand, const void* pPayload, DWORD pSize);
	VOID	SetSerialize(WORD pAccount);
	VOID	SetStopThread();
public:
	EType	mType;
	WORD	mAccount;
	WORD	mCommand;
	std::vector<char> mPayload;
};