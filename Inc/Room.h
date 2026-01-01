#pragma once

#include "CommonChatDefine.h"
#include "ServerShareChatStruct.h"

const UINT MAX_CHAT_SESSIONCOUNT = 10000;  // User Session 혹은 Child Room Session 최대 갯수
class ChatSession;
class Room
{
	
public:
	Room(UINT64 room_id, ERequestChatType chatType);
	~Room();

	ERequestChatType GetChatType() const { return m_ChatType; }
	bool SetChatType(ERequestChatType chatType);
	bool JoinSession(SessionId session_id);
	bool ExitSession(SessionId session_id);
	void Initialize();
	void ReporttoRoot();
	UINT64 GetRoomID() const { return m_RoomID; }
	
	INT  GetIDinRoom();	
	void ReleaseIDinRoom(const INT id, bool& emptyRoom);
	bool HasParentRoom();
	UINT GetMaxIDinRoom() const { return m_MaxIDinRoom; }
	SessionId GetJoinedChatSession(UINT index) const { return m_JoinedChatSessions[index]; }

protected:
	bool IsExistSessionId(SessionId session_id);
	bool JoinSessionId(SessionId session_id, UINT ID);
	bool ExitSessionId(SessionId session_id);
	UINT GetIDinRoom(SessionId session_id);

	void Release();
	
private:
	UINT64 m_RoomID;	
	ERequestChatType m_ChatType;
	Array<SessionId, MAX_CHAT_SESSIONCOUNT>::type		m_JoinedChatSessions;
	Map<SessionId, UINT>::type m_SessionMap;
	Vector<INT>::type m_UsedIDinRoom;	
	UINT m_MaxIDinRoom;
	UINT m_SessionCount;
	RWLock m_Lock;

};
