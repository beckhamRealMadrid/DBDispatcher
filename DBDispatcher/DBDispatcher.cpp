#include "pch.h"
#include "CDBDispatcher.h"
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
	CDBDispatcher aDispatcher;

	// Dispatcher 초기화
	if (aDispatcher.Open() != 0)
	{
		std::cerr << "Failed to initialize DBDispatcher!" << std::endl;
		return -1;
	}
	std::cout << "DBDispatcher started." << std::endl;

	// 유저별 직렬화 요청 테스트 (계정 번호: 1001)
	for (int aIdx = 0; aIdx < 3; ++aIdx)
	{
		std::string aPayload = "UserRequestPayload_" + std::to_string(aIdx);
		DWORD aRv = aDispatcher.QueueUserRequest(
			1001,                      // 계정 번호
			100 + aIdx,                // 커맨드 번호
			aPayload.data(),
			static_cast<DWORD>(aPayload.size())
		);

		if (aRv != 0)
		{
			std::cerr << "QueueUserRequest failed (" << aRv << ")" << std::endl;
		}
		else
		{
			std::cout << "Queued user request: " << aPayload << std::endl;
		}
	}

	// 비직렬화 요청 테스트
	for (int aIdx = 0; aIdx < 2; ++aIdx)
	{
		std::string aPayload = "SharedRequestPayload_" + std::to_string(aIdx);
		DWORD aRv = aDispatcher.QueueSharedRequest(
			200 + aIdx,               // 커맨드 번호
			aPayload.data(),
			static_cast<DWORD>(aPayload.size())
		);

		if (aRv != 0)
		{
			std::cerr << "QueueSharedRequest failed (" << aRv << ")" << std::endl;
		}
		else
		{
			std::cout << "Queued shared request: " << aPayload << std::endl;
		}
	}

	// 워커들이 처리할 시간 확보 (3초)
	std::this_thread::sleep_for(std::chrono::seconds(3));

	// Dispatcher 종료
	aDispatcher.Close();
	std::cout << "DBDispatcher stopped." << std::endl;

	return 0;
}