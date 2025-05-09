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

	// ���� ť�� �������� �����ϰ� �ʿ� ���
	for (DWORD aAccount = 1; aAccount <= ON_MAX_CONNECTION; ++aAccount)
	{
		try
		{
			CUserDBQueue* aUserDBQueue = new CUserDBQueue();
			aRv = aUserDBQueue->Open(USER_DB_QUEUE_CAPACITY, static_cast<unsigned short>(aAccount));
			if (aRv != 0)
			{
				printf("CUserDBQueue::Open ���� (Account: %u)\n", aAccount);
				delete aUserDBQueue;
				return aRv;
			}
			__mUserDBQueueMap[aAccount] = aUserDBQueue;
		}
		catch (const std::bad_alloc&)
		{
			printf("CUserDBQueue �޸� ����! (Account: %u)\n", aAccount);
			return ERROR_OUTOFMEMORY;
		}
	}

	const unsigned int aCoreCount = std::thread::hardware_concurrency();
	__mWorkerCount = Clamp(aCoreCount, 4u, 8u); // �ּ� 4, �ִ� 8

	// ��Ŀ �� + 1 (���ν������) = Ǯ������
	const int aSqlPoolSize = __mWorkerCount + 1;
	__mSqlPool.reserve(aSqlPoolSize);

	for (int i = 0; i < aSqlPoolSize; ++i)
	{
		try
		{
			CMssqlConnection* aDB = new CMssqlConnection("Server", "Database", "User", "Password");

			// ���� ����̹Ƿ� �����ϸ� �ƹ� ���� ����, ���� �� ���� �߻�
			aDB->Connect();

			__mSqlPool.push_back(aDB);
		}
		catch (const std::bad_alloc&)
		{
			printf("SQL DB ��ü ���� ����! (%d)\n", i);
			return ERROR_OUTOFMEMORY;
		}
		catch (const std::exception& e)
		{
			printf("SQL DB ���� ���� (%d): %s\n", i, e.what());
			return ERROR_CONNECTION_REFUSED;
		}
	}

	// ���ν������ SQL ���
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

	printf("CDBDispatcher - %d���� ��Ŀ �����带 �����߽��ϴ�.\n", __mWorkerCount);
	return 0;
}

VOID CDBDispatcher::Close()
{
	// ��Ŀ ������ ���� ��û
	for (int i = 0; i < __mWorkerCount; ++i)
	{
		CDBMsg* aMsg = __GetFromPool();
		aMsg->SetStopThread();
		DWORD aRv = __PushRequestToWorker(aMsg);
		if (0 < aRv)
		{
			printf("������ ���� ��û ���� (��Ŀ %d)\n", i);
			// �����ϴ��� ���� ��Ŀ�� ��� �õ�
		}
	}

	// ��Ŀ ������ ���� ���
	for (auto& aWorker : __mWorkers)
	{
		WaitForSingleObject(aWorker.GetHandle(), INFINITE);
		aWorker.Close();

		printf("DB Worker ����: Thread ID = %u\n", aWorker.GetThreadId());
	}
	__mWorkers.clear();

	// ���� ť ó�� �� ����
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

	// ���� ������ȭ ��û ó��
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

	// SQL Ŀ�ؼ� Ǯ ����
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
		printf("[CDBDispatcher] ERROR: DB Ŀ�ؼ� Ǯ �ʰ� (Thread %u)\n", aThreadId);
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

	// 1. ��Ŀ �˸� �޽��� Ȯ�� (���� Ȯ���ؾ� ���� ť ���¸� �������� ����)
	CDBMsg* aMsg = __GetFromPool();
	if (!aMsg)
	{
		return ERROR_OUTOFMEMORY;
	}

	// 2. ���� ť�� ��û ����
	DWORD aRv = aUserDBQueue->Enqueue(pCommand, pPayload, pSize);
	if (0 < aRv)
	{
		__ReleaseToPool(aMsg); // �޽��� ��ȯ
		printf("[CDBDispatcher] Enqueue ���� - Account:%u, Command:%u, Size:%u, ErrorCode:%u\n", pAccount, pCommand, pSize, aRv);
		return aRv;
	}

	// 3. �˸� �޽��� ����
	aMsg->SetSerialize(pAccount);

	// 4. ��Ŀ ������ ť�� Ǫ��
	aRv = __PushRequestToWorker(aMsg);
	if (0 < aRv)
	{
		// ���� �� ���� ť�� ����
		aUserDBQueue->PopBack();
		__ReleaseToPool(aMsg);
		printf("[CDBDispatcher] ��Ŀ �˸� ���� - Account:%u, ErrorCode:%u\n", pAccount, aRv);
		return aRv;
	}

	return aRv;
}

DWORD CDBDispatcher::QueueSharedRequest(WORD pCommand, const void* pPayload, DWORD pSize)
{
	// 1. ��Ŀ �˸� �޽��� Ȯ��
	CDBMsg* aMsg = __GetFromPool();
	if (!aMsg)
	{
		printf("[CDBDispatcher] ���� ��û �޽��� Ǯ �� - Command:%u, Size:%u\n", pCommand, pSize);
		return ERROR_OUTOFMEMORY;
	}

	// 2. �޽��� ���� (������ȭ)
	aMsg->SetNonSerialize(pCommand, pPayload, pSize);

	// 3. ��Ŀ ť�� Ǫ��
	DWORD aRv = __PushRequestToWorker(aMsg);
	if (0 < aRv)
	{
		__ReleaseToPool(aMsg);
		printf("[CDBDispatcher] ���� ��û Ǫ�� ���� - Command:%u, Size:%u, ErrorCode:%u\n", pCommand, pSize, aRv);
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

	// ��Ŀ �����带 ����� ���� ��������� ��ȣ ����
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
		printf("[CDBDispatcher::__GetFromPool] ��û ��ü �Ҵ� ����\n");
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
		printf("[CDBDispatcher::__ReturnToPool] Push ����! ErrNo = %u\n", aRv);
		assert(0);
	}
}

DWORD CDBDispatcher::RunDispatchLoop()
{
	__BindDB(); // ���� �����忡 DB �ε��� ���ε�

	DWORD aThreadId = GetCurrentThreadId();
	DWORD aTick = GetTickCount();
	srand(static_cast<unsigned int>(aTick ^ aThreadId));

	bool aRunning = true;
	CDBMsg*	aMsg = NULL;

	while (aRunning)
	{
		aMsg = NULL;
		__mSemaphoreChannel.WaitAndPop(aMsg, INFINITE); // ������ȭ or �˸� �޽��� ���
		if (NULL != aMsg)
		{
			switch (aMsg->mType)
			{
				case CDBMsg::eTypeNonSerialize:	
					__ExecuteRequest(aMsg->mCommand, aMsg->mPayload.data());
					break;
				case CDBMsg::eTypeSerialize:
					__ProcessUserQueue(aMsg->mAccount); // ���� ���� ť ó��
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

// ���ÿ� �� �����尡 ���� ���� ť�� ó���ϴ� ���� ���� ����(����ȭ ����)
VOID CDBDispatcher::__ProcessUserQueue(DWORD pAccount)
{
	CUserDBQueue* aUserDBQueue = __GetUserDBQueueByAccount(pAccount);
	if (nullptr == aUserDBQueue)
	{
		printf("[__ProcessUserQueue] ���� ť �������� ���� - Account:%u\n", pAccount);
		return;
	}

	// ���� ������ �� �ϳ��� �����常 �� ť�� ó���ϵ��� ����
	if (aUserDBQueue->LockAccess())
	{
		// ���� ȹ�� ����: ť���� ������ ������ ó��
		while (true)
		{
			CDBMsg aMsg;
			if (aUserDBQueue->Dequeue(&aMsg))
			{
				__ExecuteRequest(aMsg.mCommand, aMsg.mPayload.data());
			}
			else
			{
				aUserDBQueue->UnlockAccess(); // ó�� �Ϸ� �� ����
				break;
			}
		}
	}
	else
	{
		// �̹� �ٸ� �����尡 ó�� �� -> �ƹ��͵� ���� ����
	}
}

CUserDBQueue* CDBDispatcher::__GetUserDBQueueByAccount(DWORD pAccount)
{
	auto ait = __mUserDBQueueMap.find(pAccount);
	return (ait != __mUserDBQueueMap.end()) ? ait->second : nullptr;
}