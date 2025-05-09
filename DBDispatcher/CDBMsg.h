#pragma once

#include <vector>

/**
 * DB 작업 요청 정보를 담는 메시지 객체로 직렬화, 비직렬화 구분과 실행 데이터를 포함한다.
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
	// ===== 이동 최적화 관련 설명 =====
	// CDBMsg는 내부 payload(std::vector<char>)의 이동 최적화를 위해
	// 이동 생성자(CDBMsg(CDBMsg&&))와 이동 대입 연산자(CDBMsg& operator=(CDBMsg&&))를 명시적으로 구현.
	//
	// - std::vector<char>는 이동 시 내부 메모리 포인터를 복사 없이 전달하므로
	//   CDBMsg 역시 대용량 데이터 복사 없이 빠르게 이동할 수 있다.
	// - 단 이동이 발생하려면 반드시 std::move()를 사용해 rvalue로 캐스팅해야 한다.
	//   (예: queue.push_back(std::move(msg)))
	// - std::move() 없이 넘기면 복사 생성자/대입자가 호출되어 불필요한 복사가 발생할 수 있다.
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