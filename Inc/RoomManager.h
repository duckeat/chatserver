#pragma once

#include "CommonChatDefine.h"

class Room;
class RoomEachTypeManager
{
public:
	RoomEachTypeManager(ERequestChatType chatType) : m_ChatType(chatType){}
	~RoomEachTypeManager();
	SharedPtr<Room>::type CreateRoom(UINT64 roomID = 0);
	SharedPtr<Room>::type GetRoom(UINT64 roomID);
	bool  DestroyRoom(UINT64 roomID);
	void  ReportAllRoomtoRoot();
private:
	UnorderedMap < UINT64, SharedPtr<Room>::type>::type m_MapRoom;
	ERequestChatType m_ChatType;
	SpinLock m_Lock;
};

class ChatSession;
class RoomManager
{
	
public:
	RoomManager();
	~RoomManager();

	bool  Initialize();

	bool  JoinSession(const BYTE& chatType, const UINT64& roomKey, SessionId session_id);
	bool  ExitSession(const BYTE& chatType, const UINT64& roomKey, SessionId session_id);
	SharedPtr<Room>::type CreateRoom(UINT64 roomID, const ERequestChatType chatType);
	SharedPtr<Room>::type GetRoom(const UINT64 room_id, const ERequestChatType chatType);

	int  GetChatRangeLevel();
	bool DestroyRoom(const UINT64 room_id, const ERequestChatType chatType);

	void ReportAllRoomtoRoot();

private:
	SharedPtr<RoomEachTypeManager>::type GetEachTypeManager(const ERequestChatType chatType);
	
private:
	Array<SharedPtr<RoomEachTypeManager>::type, ERequestChatType_Max>::type		m_RoomManager;
};
