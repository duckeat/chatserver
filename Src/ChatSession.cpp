#include "stdafx.h"
#include "ChatSession.h"
#include "Modules.h"
#include "RoomManager.h"
#include "Room.h"

ChatSession::ChatSession(SessionId session_id)
	: m_SessionId(session_id)
{
	m_JoinedRoomID.fill(0);
}
bool   ChatSession::JoinRoom(UINT64 roomID, ERequestChatType chatType)
{
	CHECK_DEBUG(chatType < ERequestChatType_Max);	
	if (chatType >= ERequestChatType_Max)
	{
		return false;
	}	
	
	m_JoinedRoomID[chatType] = roomID;
	return true;
}
UINT64   ChatSession::ExitRoom(ERequestChatType chatType)
{
	CHECK_DEBUG(chatType < ERequestChatType_Max);
	CHECK_DEBUG(m_JoinedRoomID[chatType] != 0);
	if (chatType >= ERequestChatType_Max)
	{
		return 0;		
	}
	UINT64 exitRoomID = m_JoinedRoomID[chatType];
	m_JoinedRoomID[chatType] = 0;	
	return exitRoomID;
}

void ChatSession::ExitAllRoom()
{
	BYTE MaxChatType = ERequestChatType_Zone;
	for(BYTE chatType = ERequestChatType_Normal; chatType < MaxChatType; ++chatType)
	{
		UINT64 roomKey = m_JoinedRoomID[chatType];
		if (roomKey > 0)
		{
			auto room = Modules.RoomManager->GetRoom(roomKey, static_cast<ERequestChatType>(chatType));
			if (room != nullptr)
			{
				room->ExitSession(m_SessionId);
			}
		}
	}
	
}

bool ChatSession::SetSessionId(SessionId session_id)
{
	BYTE MaxChatType = ERequestChatType_Max;
	for (BYTE chatType = ERequestChatType_Normal; chatType < ERequestChatType_Max; chatType++)
	{
		UINT64 roomKey = m_JoinedRoomID[chatType];
		if (roomKey > 0)
		{
			auto room = Modules.RoomManager->GetRoom(roomKey, static_cast<ERequestChatType>(chatType));
			if (room != nullptr)
			{
				room->ExitSession(m_SessionId);
				room->JoinSession(session_id);
			}
		}
	}
	m_SessionId = session_id;
	return true;
}


UINT64 ChatSession::GetRoomID(BYTE chat_type)
{
	if (chat_type >= ERequestChatType_Max) return 0;
	return m_JoinedRoomID[chat_type];
}