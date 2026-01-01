#include "stdafx.h"
#include "BroadcastingServerSessionManager.h"
#include "BroadcastingServerSession.h"
#include "Modules.h"
#include "ChatServerIOCPProcessor.h"

BroadcastingServerSessionManager::BroadcastingServerSessionManager()
	
{
	for (INT id = MAX_BROADCASTINGSERVER_SESSION - 1; 0 <= id; --id)
	{
		PushBroadcastingServerId(id);
	}

	m_BroadCastingServerSession.fill(nullptr);

}

bool BroadcastingServerSessionManager::RegisterSession(SharedPtr<BroadcastingServerSession>::type session)
{
	if (nullptr == session)
	{
		CHECK_DEBUG(false);
		return false;
	}

	INT serverId = PopBroadcastingServerId();
	if (serverId <= INVALID_BROADCASTINGSERVER_ID || MAX_BROADCASTINGSERVER_SESSION <= serverId)
	{
		CHECK_DEBUG(false);
		session->SetServerId(INVALID_BROADCASTINGSERVER_ID);
		session->Close(true);

		return false;
	}
	{
		WriteLock lock(m_Lock);
		CHECK_DEBUG(m_BroadCastingServerSession[serverId] == nullptr);		
		m_BroadCastingServerSession[serverId] = session;
	}

	session->SetServerId(serverId);

	return true;
}

bool BroadcastingServerSessionManager::UnregisterSession(SharedPtr<BroadcastingServerSession>::type session)
{
	if (session == nullptr)
	{
		CHECK_DEBUG(false);
		return false;
	}

	const INT serverId = session->GetServerId();
	if (serverId <= INVALID_BROADCASTINGSERVER_ID || MAX_BROADCASTINGSERVER_SESSION <= serverId)
	{
		// ServerIdentity 패킷 받기전에 끊어지면 이곳으로 온다
		return false;
	}
	SharedPtr<BroadcastingServerSession>::type copy = nullptr;

	{
		WriteLock lock(m_Lock);
		if (m_BroadCastingServerSession[serverId] != session)
			return false;

		copy = m_BroadCastingServerSession[serverId];
		m_BroadCastingServerSession[serverId] = nullptr;
	}

	PushBroadcastingServerId(serverId);
	Modules.IOCPProcessor->PostTask([copy]()
	{
		copy->Release();
	});


	return true;
}

bool BroadcastingServerSessionManager::SendTo(const SessionId& sessionId, std::function<void(SharedPtr<BroadcastingServerSession>::type)> func)
{
	const INT broadcastingServerId = sessionId.GetBroadcastingServerId();
	if (broadcastingServerId <= INVALID_BROADCASTINGSERVER_ID || MAX_BROADCASTINGSERVER_SESSION <= broadcastingServerId)
		return false;

	SharedPtr<BroadcastingServerSession>::type session = nullptr;

	{
		ReadLock lock(m_Lock);
		session = m_BroadCastingServerSession[broadcastingServerId];
		if (session == nullptr)
			return false;
	}

	Modules.IOCPProcessor->PostTask([session, func]()
	{
		func(session);		
	});

	return true;
}


INT BroadcastingServerSessionManager::PopBroadcastingServerId()
{
	if (auto node = m_BroadcastingServerIdPool.Pop())
	{
		INT id = node->m_Value;
		Destroy(node); // raw pointer 삭제
		return id;
	}
	return INVALID_BROADCASTINGSERVER_ID;
}
void BroadcastingServerSessionManager::PushBroadcastingServerId(const INT id)
{
	CHECK_DEBUG(id > INVALID_BROADCASTINGSERVER_ID);
	if (id <= INVALID_BROADCASTINGSERVER_ID) return;

	auto node = new IntLockfreeNode(id);
	m_BroadcastingServerIdPool.Push(node);
}
