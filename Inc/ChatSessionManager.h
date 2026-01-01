#pragma once

class ChatSession;
class ChatSessionManager
{
public:
	ChatSessionManager();
	bool RegisterSession(SessionId session_id);
	bool UnregisterSession(SessionId session_id);
	bool ExchangeSession(SessionId old_session_id, SessionId new_session_id);
	SharedPtr<ChatSession>::type GetSession(SessionId session_id);
private:
	Array<SharedPtr<ChatSession>::type, SessionId::INDEX_MASK>::type		m_Sessions;
};
