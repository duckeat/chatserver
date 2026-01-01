#include "stdafx.h"
#include "ServerErrorCode.h"
#include "CommunityServerSession.h"

#include <thread>

#include "Modules.h"
#include "ChatSessionManager.h"

IMPLEMENT_CLASS(CommunityServerSessionProtocol);
IMPLEMENT_CLASS(CommunityServerSession);
IMPLEMENT_COMMUNITYSERVERSESSIONPROTOCOL_PROTOCOL_DEFINE();

#include "CommunityServerSessionBody.generated.inl"

CommunityServerSession::CommunityServerSession(IOCPProcessor* iocpProcessor) : Super(iocpProcessor)
{
}
CommunityServerSession::~CommunityServerSession()
{
	m_IsAlive.store(false);
}



bool CommunityServerSession::OnConnected()
{
	if (!Super::OnConnected())
	{
		return false;
	}

	LOG(L"Success to connect CommunityServer");

	return true;
}

void CommunityServerSession::OnConnectFailed()
{
	WARNING_LOG(L"Failed to connect CommunityServer");
	RetryConnect();
	
}

void CommunityServerSession::OnDisconnected()
{
	LOG(L"Disconnected CommunityServer");
	Super::OnDisconnected();
	RetryConnect();
}

void CommunityServerSession::RetryConnect()
{
	bool expected = false;
	if (!m_IsRetrying.compare_exchange_strong(expected, true))
		return;

	auto weakSelf = weak_from_this();

	std::thread([weakSelf]()
		{
			int retryDelay = 500;

			while (true)
			{
				auto self = weakSelf.lock();
				if (!self || !self->m_IsAlive.load())
					break;

				if (self->TryConnect())
					break;

				std::this_thread::sleep_for(std::chrono::milliseconds(retryDelay));
				retryDelay = std::min(retryDelay * 2, 10000);
			}

			if (auto self = weakSelf.lock())
				self->m_IsRetrying.store(false);

		}).detach();
}

void CommunityServerSession::OnRecvMessage(StreamReader& reader)
{
	if (!ProcessPacket(reader))
	{
		CHECK_DEBUG(false);

		INFO_LOG(L"Invalid Packet - Protocol Function");
		return;
	}
}

bool CommunityServerSession::Send(SendBufferStreamWriter& writer) const
{
	if (Super::PostBuildMessage(writer))
	{
		return const_cast<CommunityServerSession*>(this)->PostSend(writer.GetBuffers());
	}
	return false;
}

void CommunityServerSession::Recv_PT_TM_ANS_REGIST_SERVER(const UINT& result)
{
	// 성공 패킷이 오지 않으면 서버를 종료한다.
	 if(result != EServerError_SUCCESS)
	 {
		 CHECK_CRASH(false);
		 exit(-1);
	 }
}

void CommunityServerSession::Recv_PT_TM_REQ_ENTER_USER(const SessionId& session_id)
{
	if (false == Modules.ChatSessionManager->RegisterSession(session_id))
	{
		LOG(L"Failed to register clinet Session ID : " << session_id);
		return;
	}

	LOG(L"Success to register clinet Session ID : " << session_id);
																											   

}
void CommunityServerSession::Recv_PT_TM_REQ_LEAVE_USER(const SessionId& session_id)							    
{
	if (false == Modules.ChatSessionManager->UnregisterSession(session_id))
	{
		LOG(L"Failed to unregister clinet Session ID : " << session_id);
		return;
	}
	LOG(L"Success to unregister clinet Session ID : " << session_id);
	
}
void CommunityServerSession::Recv_PT_TM_REQ_EXCHANGE_USER(const SessionId& old_session_id, const SessionId& new_session_id)
{
	if( false == Modules.ChatSessionManager->ExchangeSession(old_session_id, new_session_id))
	{
		LOG(L"Failed to exchange clinet Session ID : " << new_session_id << L", Old session ID : " << old_session_id);
		return;
	}
	LOG(L"Success to exchange clinet Session ID : " << new_session_id << L", Old session ID : " << old_session_id);
}
