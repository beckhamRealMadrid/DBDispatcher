#include "pch.h"
#include "CDBDispatcher.h"
#include "CMssqlConnection.h"

template <typename T>
constexpr const T& Clamp(const T& v, const T& lo, const T& hi)
{
	return (v < lo) ? lo : (hi < v) ? hi : v;
}

CDBDispatcher CDBDispatcher::__mSingleton;

unsigned int __stdcall CDBDispatcher::ThreadEntryPoint(VOID* pArg)
{
	CDBDispatcher* aDBDispatcher = static_cast<CDBDispatcher*>(pArg);
	if (aDBDispatcher)
		return aDBDispatcher->RunDispatchLoop();

	return 1;
}

CDBDispatcher::CDBDispatcher()
{
	__ResetAttr();
}

CDBDispatcher::~CDBDispatcher()
{
	__Dtor();
}

CDBDispatcher& CDBDispatcher::This()
{
	return __mSingleton;
}

VOID CDBDispatcher::__ResetAttr()
{
	__mLinkCnt = 0;
	__mWorkerCount = 0;
	__mWorkers.clear();
	__mSemaphoreChannel.Close();
	__mUserDBQueueMap.clear();
}

VOID CDBDispatcher::__Dtor()
{
	for (auto& aWorker : __mWorkers)
	{
		aWorker.Close();
	}
	__mWorkers.clear();

	for (auto& aPair : __mUserDBQueueMap)
	{
		CUserDBQueue* aUserDBQueue = aPair.second;
		if (aUserDBQueue)
		{
			aUserDBQueue->Close();
			delete aUserDBQueue;
		}
	}
	__mUserDBQueueMap.clear();
}

DWORD CDBDispatcher::Open()
{
	DWORD aRv = 0;
	aRv = __mLocker.Open(TRUE);
	if (0 < aRv)
	{
		return aRv;
	}

	aRv = __mSemaphoreChannel.Open();
	if (0 < aRv)
	{
		return aRv;
	}

	aRv = __mDBMsgPool.Open(DB_REQUEST_POOL_CAPACITY);
	if (0 < aRv)
	{
		return aRv;
	}

	// 유저 큐를 동적으로 생성하고 맵에 등록
	for (DWORD aAccount = 1; aAccount <= ON_MAX_CONNECTION; ++aAccount)
	{
		try
		{
			CUserDBQueue* aUserDBQueue = new CUserDBQueue();
			aRv = aUserDBQueue->Open(USER_DB_QUEUE_CAPACITY, static_cast<unsigned short>(aAccount));
			if (aRv != 0)
			{
				printf("CUserDBQueue::Open 실패 (Account: %u)\n", aAccount);
				delete aUserDBQueue;
				return aRv;
			}
			__mUserDBQueueMap[aAccount] = aUserDBQueue;
		}
		catch (const std::bad_alloc&)
		{
			printf("CUserDBQueue 메모리 부족! (Account: %u)\n", aAccount);
			return ERROR_OUTOFMEMORY;
		}
	}

	const unsigned int aCoreCount = std::thread::hardware_concurrency();
	__mWorkerCount = Clamp(aCoreCount, 4u, 8u); // 최소 4, 최대 8

	// 워커 수 + 1 (메인스레드용) = 풀사이즈
	const int aSqlPoolSize = __mWorkerCount + 1;
	__mSqlPool.reserve(aSqlPoolSize);

	for (int i = 0; i < aSqlPoolSize; ++i)
	{
		try
		{
			CMssqlConnection* aDB = new CMssqlConnection("Server", "Database", "User", "Password");

			// 예외 기반이므로 성공하면 아무 문제 없음, 실패 시 예외 발생
			aDB->Connect();

			__mSqlPool.push_back(aDB);
		}
		catch (const std::bad_alloc&)
		{
			printf("SQL DB 객체 생성 실패! (%d)\n", i);
			return ERROR_OUTOFMEMORY;
		}
		catch (const std::exception& e)
		{
			printf("SQL DB 연결 실패 (%d): %s\n", i, e.what());
			return ERROR_CONNECTION_REFUSED;
		}
	}

	// 메인스레드용 SQL 등록
	__BindDB();
	
	for (int i = 0; i < __mWorkerCount; ++i)
	{
		unsigned int aThreadID = 0;
		HANDLE aThread = (HANDLE)_beginthreadex(NULL, 0, ThreadEntryPoint, this, 0, &aThreadID);
		if (aThread)
		{
			__mWorkers.emplace_back(aThread, aThreadID);			
		}
		else
		{
			return GetLastError();
		}
	}

	printf("CDBDispatcher - %d개의 워커 스레드를 생성했습니다.\n", __mWorkerCount);
	return 0;
}

VOID CDBDispatcher::Close()
{
	// 워커 스레드 종료 요청
	for (int i = 0; i < __mWorkerCount; ++i)
	{
		CDBMsg* aMsg = __GetFromPool();
		aMsg->SetStopThread();
		DWORD aRv = __PushRequestToWorker(aMsg);
		if (0 < aRv)
		{
			printf("스레드 종료 요청 실패 (워커 %d)\n", i);
			// 실패하더라도 다음 워커에 계속 시도
		}
	}

	// 워커 스레드 종료 대기
	for (auto& aWorker : __mWorkers)
	{
		WaitForSingleObject(aWorker.GetHandle(), INFINITE);
		aWorker.Close();

		printf("DB Worker 종료: Thread ID = %u\n", aWorker.GetThreadId());
	}
	__mWorkers.clear();

	// 유저 큐 처리 및 해제
	for (auto& aPair : __mUserDBQueueMap)
	{
		CUserDBQueue* aUserDBQueue = aPair.second;
		if (aUserDBQueue)
		{
			if (aUserDBQueue->LockAccess())
			{
				while (true)
				{
					CDBMsg aMsg;
					if (aUserDBQueue->Dequeue(&aMsg))
					{
						__ExecuteRequest(aMsg.mCommand, aMsg.mPayload.data());
					}
					else
					{
						aUserDBQueue->UnlockAccess();
						break;
					}
				}
			}

			aUserDBQueue->Close();
			delete aUserDBQueue;			
		}
	}
	__mUserDBQueueMap.clear();

	// 남은 비직렬화 요청 처리
	CDBMsg* aMsg = nullptr;
	while (__mSemaphoreChannel.Pop(aMsg))
	{
		if (!aMsg)
		{
			continue;
		}

		if (aMsg->mType == CDBMsg::eTypeNonSerialize)
		{
			__ExecuteRequest(aMsg->mCommand, aMsg->mPayload.data());
		}

		__ReleaseToPool(aMsg);
	}

	// SQL 커넥션 풀 정리
	for (CMssqlConnection* aSQL : __mSqlPool)
	{
		if (aSQL)
		{
			delete aSQL;
		}
	}
	__mSqlPool.clear();

	__mDBMsgPool.Close();
	__mSemaphoreChannel.Close();	
	__mLocker.Close();
}

VOID CDBDispatcher::__BindDB()
{
	CLockerAuto<CLocker> aLocker(__mLocker);

	DWORD aThreadId = GetCurrentThreadId();
	if (__mLinker.find(aThreadId) != __mLinker.end())
	{
		return;
	}

	if (__mLinker.size() >= __mSqlPool.size())
	{
		printf("[CDBDispatcher] ERROR: DB 커넥션 풀 초과 (Thread %u)\n", aThreadId);
		return;
	}

	__mLinker.emplace(aThreadId, __mLinkCnt);
	printf("Thread %u linked to DB index %u\n", aThreadId, __mLinkCnt);
	__mLinkCnt++;
}

CMssqlConnection* CDBDispatcher::__GetSql()
{
	CLockerAuto<CLocker> aLocker(__mLocker);

	auto ait = __mLinker.find(GetCurrentThreadId());
	if (ait == __mLinker.end())
	{
		assert(false && "Thread not linked to DB");
		return nullptr;
	}

	return __mSqlPool[ait->second];
}

DWORD CDBDispatcher::QueueUserRequest(WORD pAccount, WORD pCommand, const void* pPayload, DWORD pSize)
{
	CUserDBQueue* aUserDBQueue = __GetUserDBQueueByAccount(pAccount);
	if (nullptr == aUserDBQueue)
	{
		return ERROR_INVALID_INDEX;
	}

	// 1. 워커 알림 메시지 확보 (먼저 확보해야 유저 큐 상태를 더럽히지 않음)
	CDBMsg* aMsg = __GetFromPool();
	if (!aMsg)
	{
		return ERROR_OUTOFMEMORY;
	}

	// 2. 유저 큐에 요청 저장
	DWORD aRv = aUserDBQueue->Enqueue(pCommand, pPayload, pSize);
	if (0 < aRv)
	{
		__ReleaseToPool(aMsg); // 메시지 반환
		printf("[CDBDispatcher] Enqueue 실패 - Account:%u, Command:%u, Size:%u, ErrorCode:%u\n", pAccount, pCommand, pSize, aRv);
		return aRv;
	}

	// 3. 알림 메시지 세팅
	aMsg->SetSerialize(pAccount);

	// 4. 워커 스레드 큐에 푸시
	aRv = __PushRequestToWorker(aMsg);
	if (0 < aRv)
	{
		// 실패 시 유저 큐도 정리
		aUserDBQueue->PopBack();
		__ReleaseToPool(aMsg);
		printf("[CDBDispatcher] 워커 알림 실패 - Account:%u, ErrorCode:%u\n", pAccount, aRv);
		return aRv;
	}

	return aRv;
}

DWORD CDBDispatcher::QueueSharedRequest(WORD pCommand, const void* pPayload, DWORD pSize)
{
	// 1. 워커 알림 메시지 확보
	CDBMsg* aMsg = __GetFromPool();
	if (!aMsg)
	{
		printf("[CDBDispatcher] 공유 요청 메시지 풀 고갈 - Command:%u, Size:%u\n", pCommand, pSize);
		return ERROR_OUTOFMEMORY;
	}

	// 2. 메시지 구성 (비직렬화)
	aMsg->SetNonSerialize(pCommand, pPayload, pSize);

	// 3. 워커 큐에 푸시
	DWORD aRv = __PushRequestToWorker(aMsg);
	if (0 < aRv)
	{
		__ReleaseToPool(aMsg);
		printf("[CDBDispatcher] 공유 요청 푸시 실패 - Command:%u, Size:%u, ErrorCode:%u\n", pCommand, pSize, aRv);
		return aRv;
	}

	return aRv;
}

DWORD CDBDispatcher::__PushRequestToWorker(CDBMsg* pMsg)
{
	if (NULL == pMsg)
	{
		return ERROR_INVALID_PARAMETER;
	}

	// 워커 스레드를 깨우기 위해 세마포어로 신호 전달
	DWORD aRv = __mSemaphoreChannel.PushAndSignal(pMsg);
	if (0 < aRv)
	{
		return aRv;
	}
	
	return aRv;
}

CDBMsg* CDBDispatcher::__GetFromPool()
{
	CDBMsg* aMsg = __mDBMsgPool.Get(0);
	if (nullptr == aMsg)
	{
		printf("[CDBDispatcher::__GetFromPool] 요청 객체 할당 실패\n");
		return nullptr;
	}
	
	return aMsg;
}

VOID CDBDispatcher::__ReleaseToPool(CDBMsg* pMsg)
{
	pMsg->Reset();
	DWORD aRv = __mDBMsgPool.Release(pMsg);
	if (0 < aRv)
	{
		printf("[CDBDispatcher::__ReturnToPool] Push 실패! ErrNo = %u\n", aRv);
		assert(0);
	}
}

DWORD CDBDispatcher::RunDispatchLoop()
{
	__BindDB(); // 현재 스레드에 DB 인덱스 바인딩

	DWORD aThreadId = GetCurrentThreadId();
	DWORD aTick = GetTickCount();
	srand(static_cast<unsigned int>(aTick ^ aThreadId));

	bool aRunning = true;
	CDBMsg*	aMsg = NULL;

	while (aRunning)
	{
		aMsg = NULL;
		__mSemaphoreChannel.WaitAndPop(aMsg, INFINITE); // 비직렬화 or 알림 메시지 대기
		if (NULL != aMsg)
		{
			switch (aMsg->mType)
			{
				case CDBMsg::eTypeNonSerialize:	
					__ExecuteRequest(aMsg->mCommand, aMsg->mPayload.data());
					break;
				case CDBMsg::eTypeSerialize:
					__ProcessUserQueue(aMsg->mAccount); // 유저 전용 큐 처리
					break;
				case CDBMsg::eTypeStopThread:
					aRunning = false;
					break;
			}
			__ReleaseToPool(aMsg);
		}
	}

	return 0;
}

VOID CDBDispatcher::__ExecuteRequest(WORD pCommand, const char* pData)
{
	CMssqlConnection* aSQL = __GetSql();
	aSQL->Execute(pData);
}

// 동시에 두 스레드가 같은 유저 큐를 처리하는 일은 절대 없음(직렬화 보장)
VOID CDBDispatcher::__ProcessUserQueue(DWORD pAccount)
{
	CUserDBQueue* aUserDBQueue = __GetUserDBQueueByAccount(pAccount);
	if (nullptr == aUserDBQueue)
	{
		printf("[__ProcessUserQueue] 유저 큐 존재하지 않음 - Account:%u\n", pAccount);
		return;
	}

	// 유저 단위로 단 하나의 스레드만 이 큐를 처리하도록 제한
	if (aUserDBQueue->LockAccess())
	{
		// 권한 획득 성공: 큐에서 데이터 꺼내고 처리
		while (true)
		{
			CDBMsg aMsg;
			if (aUserDBQueue->Dequeue(&aMsg))
			{
				__ExecuteRequest(aMsg.mCommand, aMsg.mPayload.data());
			}
			else
			{
				aUserDBQueue->UnlockAccess(); // 처리 완료 후 해제
				break;
			}
		}
	}
	else
	{
		// 이미 다른 스레드가 처리 중 -> 아무것도 하지 않음
	}
}

CUserDBQueue* CDBDispatcher::__GetUserDBQueueByAccount(DWORD pAccount)
{
	auto ait = __mUserDBQueueMap.find(pAccount);
	return (ait != __mUserDBQueueMap.end()) ? ait->second : nullptr;
}