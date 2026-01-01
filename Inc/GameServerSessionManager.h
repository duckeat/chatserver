#pragma once

class GameServerSession;
class GameServerSessionManager
{
public:
	GameServerSessionManager();

	bool RegisterSession(SharedPtr<GameServerSession>::type session);
	void UnregisterSession(SharedPtr<GameServerSession>::type session);

	template<typename F>
	bool SendTo(INT index, std::function<void(SharedPtr<GameServerSession>::type)> func);
private:
	RWLock m_Lock;
	SharedPtr<GameServerSession>::type m_GameServerSession[MAX_GAMESERVER_ID + 1];
};
