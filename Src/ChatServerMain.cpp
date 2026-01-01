#include "stdafx.h"
#include "Modules.h"
#include "ChildChatServerSessionManager.h"
#include "RoomManager.h"
#include "GameServerSession.h"
#include "BroadcastingServerSession.h"
#include "RootChatServerSession.h"
#include "CommunityServerSession.h"
#include "ChildChatServerSession.h"
#include "ChatServerIOCPProcessor.h"
#include "GameServerSessionManager.h"
#include "BroadcastingServerSessionManager.h"
#include "ChatSessionManager.h"
#include "ChatService.h"

namespace
{
	void PerformanceStatus()
	{
		if (Modules.ChildChatServerSessionManager != nullptr)
		{
			INFO_LOG(L"---------------- Status" << std::endl
				<< L"---> Child Chetting Server session count : " << Modules.ChildChatServerSessionManager->GetSessionCount() << std::endl
				<< L"----------------"
			);
		}

		ServerConsoleHandler::GetInstance().ShowMememoryPerformance();
	}
};

bool LoadConfigData()
{
	// XML 구조 Mapping
	ChatServerConfig::BindProperties();

	// 데이터 로드
	const WString serverConfigName = ServerCommon::CommandLineParser::GetArgumentString(L"CONFIGFILE", L"ChatServerConfig");
	if(Modules.ChatServerConfig.LoadServerConfig(serverConfigName) == false)
	{
		return false;
	}
	{
		/** 
		채팅서버는 우선 3단계 구조로 이루어져 있다.
		1단계 - Community, GameServer와 연결을 하면서 같은 월드내에서 통신을 하기 위한 범위
		2단계 - 1단계의 상위 레벨로서, 월드군 내에서 통신을 하기 위한 범위
		3단계 - 2단계보다 상위 레베로서, 전체 유저들에게 채팅 내용을 전달하기 위한 broker 영할을 한다.
		(만일 그 윗단계가 더 필요하다면 level을 4, 5 이런식으로 더 올릴 수가 있다.
		*/
		auto& config = Modules.ChatServerConfig;
		if (config.m_RootChatServerConnectInfo.Port == 0 && config.m_ChildChatPort != 0)
			config.m_RangeLevel = 3;
		else if (config.m_RootChatServerConnectInfo.Port != 0 && config.m_ChildChatPort != 0)
			config.m_RangeLevel = 2;
		else if (config.m_RootChatServerConnectInfo.Port != 0 && config.m_BroadcastingServerPort != 0)
			config.m_RangeLevel = 1;
		else
			return false;
	}
	
	return true;
}
bool InitializeLoggingSystem()
{
	// Log 관련 등록
	LogController::LogWaiter logWaiter;
	LogController::GetInstance().Regist(Create<FileLogPrinter>(L"ChatServer"));
	LogController::GetInstance().Regist(Create<ConsoleLogPrinter>());
	ServerCommonConfig::GetInstance().ApplyLogLevel();
	ServerConsoleHandler::GetInstance().Regist(ServerConsoleHandler::KEY_F2, &PerformanceStatus, L"display performance status");

	return true;
}

bool BindModules()
{
	auto& config = Modules.ChatServerConfig;
	ChatServerIOCPProcessor* iocpProcessor = Create<ChatServerIOCPProcessor>(config.m_ThreadCount);
	Modules.IOCPProcessor = iocpProcessor;
	Modules.GameServerSessionManager = Create<GameServerSessionManager>();
	Modules.BroadcastingServerSessionManager = Create <BroadcastingServerSessionManager>();
	
	Modules.GlobalTaskProcessor = Create<GlobalTaskProcessor>(config.m_ThreadCount);
	Modules.SessionIdPool = Create<SessionIdPool>();
	Modules.RoomManager = Create<RoomManager>();
	Modules.ChatService = Create<ChatService>();

	if(config.m_RangeLevel > 1)
		Modules.ChildChatServerSessionManager = Create<ChildChatServerSessionManager>();
	if(config.m_RangeLevel == 1)
		Modules.ChatSessionManager = Create<ChatSessionManager>();


	Modules.RoomManager->Initialize();
	

	return true;
}

// 월드 기준 채팅서버 생성
bool OpenServerPort_Level1() 
{
	auto& config = Modules.ChatServerConfig;

	// Game & Broadcasting 서버 port 정보가 0이면 서버를 종료한다.
	if (config.m_GameServerPort == 0 || config.m_BroadcastingServerPort == 0)
	{
		WARNING_LOG(L"There are no port number for GameServer[ "<< config.m_GameServerPort << L" ] or Broadcasting Server [ " << config.m_BroadcastingServerPort << L" ]");
		return false;
	}
	// Root Chat 서버가 없으면 RootChatServerSession 객체를 생성하지 않고 서버를 종료하도록 한다.
	if (config.m_RootChatServerConnectInfo.Port == 0)
	{
		ERROR_LOG(L"There is no port for RootCharServer[" << config.m_RootChatServerConnectInfo.Port << L" ]");
		return false;
	}

	// Game & broadcasting 용 Listen port 생성
	{
		Listener* gameServerListener = Create<Listener>(Modules.IOCPProcessor, Create<GameServerSessionFactory>());
		if (false == gameServerListener->Listen(config.m_GameServerPort))
		{
			ERROR_LOG(L"Can't open for listen port [" << config.m_GameServerPort << L" ], for gameServer");
			return false;
		}
		if (false == gameServerListener->AcceptEx())
		{
			ERROR_LOG(L"Can't wait accept socket [ " << config.m_GameServerPort << L" ], for gameServer");
			return false;
		}
		Listener* broadcastingServerListener = Create<Listener>(Modules.IOCPProcessor, Create<BroadcastingServerSessionFactory>());
		if (false == broadcastingServerListener->Listen(config.m_BroadcastingServerPort))
		{
			ERROR_LOG(L"Can't open for listen port [" << config.m_BroadcastingServerPort << L" ], for broadcastingServer");
			return false;
		}
		if (false == broadcastingServerListener->AcceptEx())
		{
			ERROR_LOG(L"Can't wait accept socket[ " << config.m_BroadcastingServerPort << L" ], for broadcastingServer");
			return false;
		}
	}				

	// Root Chat 서버 접속 시도
	{
		Modules.RootChatServerSession = Create<RootChatServerSession>(Modules.IOCPProcessor);
		Modules.RootChatServerSession->IncReference();
		if (false == Modules.RootChatServerSession->Connect(config.m_RootChatServerConnectInfo.IP, config.m_RootChatServerConnectInfo.Port))
		{
			ERROR_LOG(L"Fail to connect, to Root Chatting Server - [" << config.m_RootChatServerConnectInfo.IP << L":" << config.m_RootChatServerConnectInfo.Port << L"]");
			return false;
		}
	}
	// Community 서버 접속 시도
	{
		Modules.CommunityServerSession = Create<CommunityServerSession>(Modules.IOCPProcessor);
		Modules.CommunityServerSession->IncReference();
		if (false == Modules.CommunityServerSession->Connect(config.m_CommunityServerConnectInfo.IP, config.m_CommunityServerConnectInfo.Port))
		{
			ERROR_LOG(L"Fail to connect, to Community Server - [" << config.m_CommunityServerConnectInfo.IP << L":" << config.m_CommunityServerConnectInfo.Port << L"]");
			return false;
		}
	}
	return true;
}
// 월드군 기준 채팅서버 생성
bool OpenServerPort_Level2()
{
	auto& config = Modules.ChatServerConfig;

	// Child Chat Port가 정의되지 않으면 서버를 종료한다.
	if (config.m_ChildChatPort == 0)
	{
		ERROR_LOG(L"There is no port for ChildChatPort[" << config.m_ChildChatPort << L" ]");
		return false;
	}
	// Root Chat 서버가 없으면 RootChatServerSession 객체를 생성하지 않고 서버를 종료하도록 한다.
	if (config.m_RootChatServerConnectInfo.Port == 0)
	{
		ERROR_LOG(L"There is no port for RootChatServerPort[" << config.m_RootChatServerConnectInfo << L" ]");
		return false;
	}

	{
		Listener* childChatServercListener = Create<Listener>(Modules.IOCPProcessor, Create<ChildChatServerSessionFactory>());
		if (false == childChatServercListener->Listen(config.m_ChildChatPort))
		{
			ERROR_LOG(L"Can't listen port " << config.m_ChildChatPort << L", for child chatting server");
			return false;
		}
		if (false == childChatServercListener->AcceptEx())
		{
			ERROR_LOG(L"Can't create child socket " << config.m_ChildChatPort << L", for child chatting server");
			return false;
		}	
	}


	// Root Chat 서버 접속 시도
	{
		Modules.RootChatServerSession = Create<RootChatServerSession>(Modules.IOCPProcessor);
		Modules.RootChatServerSession->IncReference();
		if (false == Modules.RootChatServerSession->Connect(config.m_RootChatServerConnectInfo.IP, config.m_RootChatServerConnectInfo.Port))
		{
			ERROR_LOG(L"Fail to connect, to Root Chatting Server");
			return false;
		}
	}

	return true;
}
//  채팅서버 생성
bool OpenServerPort_Level3()
{
	auto& config = Modules.ChatServerConfig;

	// Child Chat Port가 정의되지 않으면 서버를 종료한다.
	if (config.m_ChildChatPort == 0)
	{
		ERROR_LOG(L"There is no port for ChildChatPort[" << config.m_ChildChatPort << L" ]");
		return false;
	}
	
	{
		Listener* childChatServercListener = Create<Listener>(Modules.IOCPProcessor, Create<ChildChatServerSessionFactory>());
		if (false == childChatServercListener->Listen(config.m_ChildChatPort))
		{
			ERROR_LOG(L"Can't listen port " << config.m_ChildChatPort << L", for child chatting server");
			return false;
		}
		if (false == childChatServercListener->AcceptEx())
		{
			ERROR_LOG(L"Can't create child socket " << config.m_ChildChatPort << L", for child chatting server");
			return false;
		}
	}

	return true;
}
bool StartServer()
{
	auto& config = Modules.ChatServerConfig;

	const INT threadGroup = 1;
	const INT threadCount = config.m_ThreadCount;
	INT createThreadCount = threadCount - 1;


	Vector<IProcessor*>::type processors;
	processors.push_back(Modules.GlobalTaskProcessor);
	processors.push_back(Modules.IOCPProcessor);

	ThreadController threadController(processors);
	threadController.Initialize(threadGroup, createThreadCount);
	threadController.Start();

	ServerCommon::StartServer(Modules.ChatServerConfig.m_ServerName.c_str());

	// 범위별 서버 포트 열기 및 접속 시도

	switch(config.m_RangeLevel)
	{
	case 1:
		if (OpenServerPort_Level1() == false)		return false;
		break;
	case 2:
		if (OpenServerPort_Level2() == false)		return false;		
		break;
	case 3:
		if (OpenServerPort_Level3() == false)		return false;
		break;
	default:
		break;
	}
	
	if (false == ServerCommon::OpenProcessEvent())
	{
		return false;
	}
	LOG(L"Chat Server Level : [" << config.m_RangeLevel << L"]");
	threadController.ThreadInitialization();
	while (threadController.Process())
	{
	}

	return true;
}

int _tmain(int argc, _TCHAR* argv[])
{
	GluonNet::Initialize();
	if (!ServerCommon::Initialize(argc, argv))
	{
		return EProcessExitCode_InvalidRun;
	}

	// 로깅 시스템 초기화
	if(InitializeLoggingSystem() == false)
	{
		CHECK_DEBUG(false);
		return EProcessExitCode_InvalidRun;
	}
	ServerCommon::GenerateProcessEventName(L"Chatting.Event");

	// Config 관련 로딩
	if(LoadConfigData() == false)
	{
		CHECK_DEBUG(false);
		return EProcessExitCode_InvalidRun;
	}

	// 통신할 서버 프로토콜 등록
	{
		CommunityServerSession::InitializeProtocol();
		BroadcastingServerSession::InitializeProtocol();
		GameServerSession::InitializeProtocol();
													  
		RootChatServerSession::InitializeProtocol();
		ChildChatServerSession::InitializeProtocol();		
	}

	// 모듈 연동
	if(BindModules() == false)
	{
		CHECK_DEBUG(false);
		return EProcessExitCode_InvalidRun;
	}

	// 서버 시작
	if(StartServer() == false)
	{
		CHECK_DEBUG(false);
		return EProcessExitCode_InvalidRun;
	}
	return EProcessExitCode_None;			  
}
