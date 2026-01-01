#pragma once

#include "CommonChatDefine.h"

class ChatSession
{
	
public:
	ChatSession(SessionId session_id);

	
	SessionId GetSessionId() { return m_SessionId; }
	bool      JoinRoom(UINT64 roomID, ERequestChatType chatType);
	UINT64    ExitRoom(ERequestChatType chatType);
	void      ExitAllRoom();
	UINT64    GetRoomID(BYTE chat_type);

	
private:
	SessionId m_SessionId;	
	Array<UINT64, static_cast<size_t>(ERequestChatType::ERequestChatType_Max)>::type		m_JoinedRoomID; // 각 타입별로 배정된 방의 고유번호
	friend class ChatSessionManager;
	bool      SetSessionId(SessionId session_id);
	
};
