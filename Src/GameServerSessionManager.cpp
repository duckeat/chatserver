#include "stdafx.h"
#include "GameServerSessionManager.h"
#include "GameServerSession.h"

GameServerSessionManager::GameServerSessionManager() 
	:m_GameServerSession{}
{
	
}


bool GameServerSessionManager::RegisterSession(SharedPtr<GameServerSession>::type session)
{
	if (nullptr == session)
	{
		return false;
	}

	const UINT index = session->GetServerIndex();
	if (index <= 0 || MAX_GAMESERVER_ID < index)
	{
		CHECK_DEBUG(false);
		return false;
	}
	
	{
		WriteLock lock(m_Lock);
		if (nullptr != m_GameServerSession[session->GetServerIndex()])
		{
			CHECK_DEBUG(false);
			return false;
		}				
		m_GameServerSession[session->GetServerIndex()] = session;
	}

	return true;
}

void GameServerSessionManager::UnregisterSession(SharedPtr<GameServerSession>::type session)
{
	if (nullptr == session)
	{
		CHECK_DEBUG(false);
		return;
	}

	const UINT index = session->GetServerIndex();
	if (index <= 0 || MAX_GAMESERVER_ID < index)
	{
		return;
	}

	WriteLock lock(m_Lock);

	if (session != m_GameServerSession[index])
	{
		return;
	}

	auto copySession = m_GameServerSession[index];

	m_GameServerSession[index].reset();
}

template<typename F>
bool GameServerSessionManager::SendTo(INT index, std::function<void(SharedPtr<GameServerSession>::type)> func)
{
	if (index <= 0 || MAX_GAMESERVER_ID < index)
	{
		return false;
	}

	SharedPtr<GameServerSession>::type session = nullptr;
	{
		ReadLock lock(m_Lock);
		session = m_GameServerSession[index];
		if (session == nullptr)
			return false;

	}
	std::forward<F>func(session);
	return true;
}
