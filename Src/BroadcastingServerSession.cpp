#include "stdafx.h"
#include "BroadcastingServerSession.h"
#include "Modules.h"
#include "BroadcastingServerSessionManager.h"

IMPLEMENT_CLASS(BroadcastingServerSessionProtocol);
IMPLEMENT_CLASS(BroadcastingServerSession);
IMPLEMENT_BROADCASTINGSERVERSESSIONPROTOCOL_PROTOCOL_DEFINE();

#include "BroadcastingServerSessionBody.generated.inl"
#include "BroadcastingServerSessionBody.clientgame.generated.inl"

bool BroadcastingServerSessionProtocol::MakeMultiCastingHeader(StreamWriter& writer, const Vector<SessionId>::type& idList) const
{
	return MakeBroadCastingHeader(writer, idList);
}

bool BroadcastingServerSessionProtocol::MakeBroadCastingHeader(StreamWriter& writer, const Vector<SessionId>::type& sessionIdList) const
{
	if (sessionIdList.empty())
	{
		return false;
	}

	PacketHeaderType header(BroadcastingServerSessionProtocol::PT_MB_MULTICASTING);
	Vector<SessionId>::type copy = sessionIdList;
	writer << header << copy;

	return true;
}

bool BroadcastingServerSessionProtocol::SendMultiCast(SendBufferStreamWriter& writer, const INT broadcastingServerId) const
{
	CHECK_DEBUG(broadcastingServerId == m_ServerId || broadcastingServerId == INVALID_BROADCASTINGSERVER_ID);

	if (Super::PostBuildMessage(writer))
	{
		return const_cast<BroadcastingServerSessionProtocol*>(this)->PostSend(writer.GetBuffers());
	}
	else
	{
		return false;
	}	
}

void BroadcastingServerSessionProtocol::MakeBypassHeader(const SessionId& sessionId, SendBufferStreamWriter& writer) const
{
	PacketHeaderType header(PT_MB_BYPASS);
	SessionId copy = sessionId;
	writer << header << copy;
}

BroadcastingServerSessionProtocol::BroadcastingServerSessionProtocol()
	: m_State(STATE_NONE)
	, m_ServerId(INVALID_BROADCASTINGSERVER_ID)
{
}

bool BroadcastingServerSessionProtocol::SetServerId(const INT serverId)
{
	// STATE_CONNECTED 상태에서만 STATE_ACQUIRED_ID로 변경
	STATE expected = STATE_CONNECTED;
	if (!m_State.compare_exchange_strong(expected, STATE_ACQUIRED_ID))
	{
		// 이미 STATE_ACQUIRED_ID이거나 STATE_NONE이면 실패
		return false;
	}
	m_ServerId = serverId;
	return true;
}


bool BroadcastingServerSession::OnAccepted()
{
	if (!Super::OnAccepted())
	{
		return false;
	}
	LOG(L"connected Broadcasting Server. ip=" << m_Socket.GetSocketRemoteAddressByString());
	STATE expected = STATE_NONE;
	if (!m_State.compare_exchange_strong(expected, STATE_CONNECTED))
	{
		// 이미 STATE_CONNECTED 이상이면 중복 호출 또는 이상 상태
		CHECK_DEBUG(false);
		return false;
	}

	return true;
}

void BroadcastingServerSession::OnDisconnected()
{
	STATE prevState = m_State.exchange(STATE_NONE);
	if (prevState == STATE_NONE)
		return; // 이미 disconnected 처리됨

	LOG(L"disconnected Broadcasting server");

	auto self = shared_from_this();

	Modules.GlobalTaskProcessor->PostGlobal(CreateGlobalTask([self]()
	{
		Modules.BroadcastingServerSessionManager->UnregisterSession(self);
	}, [](){}));

	Super::OnDisconnected();
						
}

void BroadcastingServerSession::OnRecvMessage(StreamReader& reader)
{
	if (!ProcessPacket(reader))
	{
		CHECK_DEBUG(false);

		WARNING_LOG(L"Invalid packet received from broadcasting server");
		return;
	}
}

bool BroadcastingServerSession::Send(SendBufferStreamWriter& writer) const
{
	if (Super::PostBuildMessage(writer))
	{
		// 기존 framework에서 Send가 const로 되어 있고,
		// 실제 사용하는 PostSend는 내부 Queue를 이용하는 코드가 있어 const로 선언되어 있지 않아
		// 어쩔 수 없이 const_cast를 사용  
		return const_cast<BroadcastingServerSession*>(this)->PostSend(writer.GetBuffers());
	}
	return false;
}
void BroadcastingServerSession::Recv_PT_BM_REQ_SERVER_IDENTITY(const INT& serverId)
{
	if (SetServerId(serverId) == false)
	{
		INFO_LOG(L"Broadcasting server already registered. serverId= " << serverId);
		return;
	}
	if (Modules.BroadcastingServerSessionManager->RegisterSession(shared_from_this()) == false)
	{
		CHECK_DEBUG(false);
		Close(true);
	}
	LOG(L"Broadcasting server session registered. serverId=" << serverId);
}
