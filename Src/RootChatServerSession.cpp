#include "stdafx.h"
#include "ServerErrorCode.h"
#include "RootChatServerSession.h"

#include <thread>
#include "Modules.h"
#include "RoomManager.h"
#include "Room.h"
#include "ChatService.h"

IMPLEMENT_CLASS(RootChatServerSessionProtocol);
IMPLEMENT_CLASS(RootChatServerSession);
IMPLEMENT_ROOTCHATSERVERSESSIONPROTOCOL_PROTOCOL_DEFINE();

#include "RootChatServerSessionBody.generated.inl"

RootChatServerSession::RootChatServerSession(IOCPProcessor* iocpProcessor) : Super(iocpProcessor) 
{
}
RootChatServerSession::~RootChatServerSession()
{
	m_IsAlive.store(false);
}


bool RootChatServerSession::OnConnected()
{
	if (!Super::OnConnected())
	{
		return false;
	}

	m_bConnect.store(true);
	m_IsRetrying.store(false);
	LOG(L"connect success RootChatServer");
	// 만일 늦게 Chat서버가 실행되었다면 이미 방이 생긴것이 있을 수 있으니 보고 할 수 있도록 한다.
	Modules.RoomManager->ReportAllRoomtoRoot();

	return true;
}

void RootChatServerSession::OnConnectFailed()
{									
	WARNING_LOG(L"connect failed to RootChatServer");
	m_bConnect.store(false);
	RetryConnect();
}

void RootChatServerSession::OnDisconnected()
{
	LOG(L"disconnected RootChatServer");
	m_bConnect.store(false);
	Super::OnDisconnected();
	m_bConnect = false;
	RetryConnect();
}

void RootChatServerSession::RetryConnect()
{
	// 이미 재접속 중이면 무시
	bool expected = false;
	if (!m_IsRetrying.compare_exchange_strong(expected, true))
		return;
	auto weakSelf = weak_from_this();
	std::thread([weakSelf]()
		{
			int retryDelay = 500; // 초기 500ms
			// 객체가 살아있는 경우에만 재접속 시도
			while (true)
			{
				auto self = weakSelf.lock();
				if (!self || !self->m_IsAlive.load())
					break;

				if (self->TryConnect())
					break;


				std::this_thread::sleep_for(std::chrono::milliseconds(retryDelay));
				retryDelay = std::min(retryDelay * 2, 10000); // 최대 10초 지수 백오프
			}
			if (auto self = weakSelf.lock())
				self->m_IsRetrying.store(false);
		}).detach();
}


void RootChatServerSession::OnRecvMessage(StreamReader& reader)
{
	if (!ProcessPacket(reader))
	{
		CHECK_DEBUG(false);

		INFO_LOG(L"Invalid Packet - Protocol Function");
		return;
	}
}

bool RootChatServerSession::Send(SendBufferStreamWriter& writer) const
{
	if (Super::PostBuildMessage(writer))
	{
		// 기존 framework에서 Send가 const로 되어 있고,
		// 실제 사용하는 PostSend는 내부 Queue를 이용하는 코드가 있어 const로 선언되어 있지 않아
		// 어쩔 수 없이 const_cast를 사용 
		return const_cast<RootChatServerSession*>(this)->PostSend(writer.GetBuffers());
	}
	return false;
}


void RootChatServerSession::Recv_PT_RC_NFY_SEND_CHAT(const UINT64& roomID, const BYTE& chatType, const ChatMessage& message)
{
	try
	{
		ChatService::Get()->HandleSendChat(message, EChatMessageSource::fromParent);
	}
	catch (const std::exception& e)
	{
		ERROR_LOG(L"ChatService exception: " << e.what() << L" in [Recv_PT_RC_NFY_SEND_CHAT]");		
	}
}


void RootChatServerSession::Recv_PT_RC_ANS_CREATE_CHATROOM(const UINT& result, const UINT64& roomID, const BYTE& chatType)
{
	
	if (result == EServerError_SUCCESS)
	{
		// Root에서 방이 생성되면 후속 조취를 취할 경우 아래에서 처리한다.
		
	}	
}

void RootChatServerSession::Recv_PT_RC_ANS_REGISTER_CHILDCHAT_RESULT(const UINT& result)
{
	if(result != EServerError_SUCCESS)
	{
		ERROR_LOG("RootServer could NOT registry Child ChatServer.")
		CHECK_CRASH(false);						
	}
}

