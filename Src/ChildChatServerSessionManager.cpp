#include "stdafx.h"
#include "ChildChatServerSessionManager.h"
#include "Modules.h"
#include "ChildChatServerSession.h"
#include "ChatServerIOCPProcessor.h"

ChildChatServerSessionManager::ChildChatServerSessionManager() :
	m_SessionCount(0)
{
	std::fill(std::begin(m_SessionTable), std::end(m_SessionTable), nullptr);
}


bool ChildChatServerSessionManager::RegisterSession(SharedPtr<ChildChatServerSession>::type session, SessionId& out)
{
	if (nullptr == session)
	{
		return false;
	}
	if (Modules.SessionIdPool->Generate(out) == false)
	{
		return false;
	}
	{
		WriteLock lock(m_Lock);
		if (nullptr != m_SessionTable[out.GetIndex()])
		{
			CHECK_DEBUG(false);		
			return false;
		}
		session->IncReference();
		::InterlockedIncrement(&m_SessionCount);
	
		m_SessionTable[out.GetIndex()] = session;
	}
	
	return true;
}


void ChildChatServerSessionManager::UnregisterSession(SharedPtr<ChildChatServerSession>::type session)
{
	if (nullptr == session)
	{
		CHECK_DEBUG(false);
		return;
	}

	const UINT index = session->GetSessionId().GetIndex();
	if (index == 0 || MAX_GAMESERVER_ID < index)
	{
		return;
	}

	WriteLock lock(m_Lock);

	if (session != m_SessionTable[index])
	{
		return;
	}

	auto copySession = m_SessionTable[index];
	
	m_SessionTable[index] = nullptr;
	Modules.IOCPProcessor->PostTask([copySession]()
		{
			copySession->Release();			
		});
	
	::InterlockedDecrement(&m_SessionCount);
}

bool ChildChatServerSessionManager::SendTo(SessionId session_id, std::function<void(SharedPtr<ChildChatServerSession>::type)> func)
{
	UINT index = session_id.GetIndex();
	if (index == 0 || SessionId::INDEX_MASK < index)
		return false;

	SharedPtr<ChildChatServerSession>::type session;
	{
		ReadLock lock(m_Lock);
		auto session = m_SessionTable[index];
		if (session == nullptr)
			return false;

		session->IncReference();
	}

	func(session);

	session->Release();
	return true;
}
