#pragma once

class ChildChatServerSession;
class ChildChatServerSessionManager
{
public:
	ChildChatServerSessionManager();

	bool RegisterSession(SharedPtr<ChildChatServerSession>::type session, SessionId& out);
	void UnregisterSession(SharedPtr<ChildChatServerSession>::type session);

	bool SendTo(SessionId session_id, std::function<void(SharedPtr<ChildChatServerSession>::type)> func);
	LONG GetSessionCount() { return m_SessionCount; }

private:
	
private:
	RWLock m_Lock;	
	SharedPtr<ChildChatServerSession>::type m_SessionTable[SessionId::INDEX_MASK];
	volatile LONG m_SessionCount;
};
