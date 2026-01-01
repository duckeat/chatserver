#include "stdafx.h"
#include "Room.h"
#include "Modules.h"
#include "ChatServerIOCPProcessor.h"
#include "RootChatServerSession.h"
#include "RoomManager.h"
#include "ChildChatServerSessionManager.h"
#include "ChildChatServerSession.h"
#include "BroadcastingServerSessionManager.h"
#include "BroadcastingServerSession.h"

Room::Room(UINT64 room_id, ERequestChatType chatType) : m_RoomID(room_id), m_MaxIDinRoom(0), m_ChatType(chatType), m_SessionCount(0)
{
	m_JoinedChatSessions.fill(SessionId::Nil);
}
Room::~Room()
{
	Release();
}

void Room::Release()
{
	m_JoinedChatSessions.fill(SessionId::Nil);

	// 부모 Chat서버가 있는지 체크하고 있으면 보고 
	if (HasParentRoom() == true)
	{
		UINT64 roomID = m_RoomID;
		ERequestChatType   chatType = m_ChatType;
		m_SessionCount = 0;
		Modules.IOCPProcessor->PostTask([roomID, chatType]()
			{
				Modules.RootChatServerSession->Send_PT_CR_REQ_DESTROY_CHATROOM(roomID, chatType);
			});
	}
}

void Room::Initialize()
{
	ReporttoRoot();
}

void Room::ReporttoRoot()
{
	// 부모 Chat서버가 있는지 체크하고 있으면 보고
	if (HasParentRoom() == true)
	{
		Room* self = this;
		Modules.IOCPProcessor->PostTask([self]()
			{
				Modules.RootChatServerSession->Send_PT_CR_REQ_CREATE_CHATROOM(self->GetRoomID(), self->GetChatType());
			});
	}
}


bool Room::JoinSession(SessionId session_id)
{
	if(IsExistSessionId(session_id) == true)
	{
		return false;
	}

	UINT IDinRoom = GetIDinRoom();
	CHECK_DEBUG(m_JoinedChatSessions[IDinRoom] == SessionId::Nil);
	if(JoinSessionId(session_id, IDinRoom) == false)
	{
		bool bEmptyRoom = false;
		ReleaseIDinRoom(IDinRoom, bEmptyRoom);
		CHECK_DEBUG(false);
		if (true == bEmptyRoom)
		{
			Modules.RoomManager->DestroyRoom(m_RoomID, m_ChatType);
		}
		return false;
	}
	m_JoinedChatSessions[IDinRoom] = session_id;
	return true; 

}
bool Room::ExitSession(SessionId session_id)
{
	if (IsExistSessionId(session_id) == false)
	{
		return false;
	}
	UINT IDinRoom = GetIDinRoom(session_id);
	m_JoinedChatSessions[IDinRoom] = SessionId::Nil;

	bool bEmptyRoom = false;
	ReleaseIDinRoom(IDinRoom, bEmptyRoom);
	ExitSessionId(session_id);
	// 해당 방에 연결된 Session이 없다면 방을 폭파하도록 한다.
	if(bEmptyRoom == true)
	{
		Modules.RoomManager->DestroyRoom(m_RoomID, m_ChatType);
	}
	return true;
}

bool Room::HasParentRoom()
{
	// RootChatServerSession이 nullptr 혹은 Root에 접속되지 않았다면 상위 방위 없다고 가정한다.
	if (Modules.RootChatServerSession == nullptr || Modules.RootChatServerSession->IsConnected() == false)
		return false;

	auto config = Modules.ChatServerConfig;
	bool isExistParent = false;
	switch(m_ChatType)
	{
		case ERequestChatType_WorldGroup   :  // 월드군 채팅
		case ERequestChatType_G1		   :  // 그룹1 (월드군)
		case ERequestChatType_G2		   :  // 그룹2 (월드군)
		case ERequestChatType_G3		   :  // 그룹3 (월드군)
		case ERequestChatType_G4		   :  // 그룹4 (월드군)
		case ERequestChatType_G5		   :  // 그룹5 (월드군)
		case ERequestChatType_Union		   :  // 연합  (월드군)
		{
			if (config.m_RangeLevel == 1)	// Level 1일때만 부모가 있다.
			{
				isExistParent = true;	// RootChatServerSession가 있으면 월드군용 쳇 서버
			}			
			break;
		}
		case ERequestChatType_Universe		: // 전체 채팅
		{
			isExistParent = true;
		}
	}
	
	return isExistParent;
}

INT Room::GetIDinRoom()
{
	INT IDinRoom = -1;
	WriteLock lock(m_Lock);
	if(false == m_UsedIDinRoom.empty())
	{
		IDinRoom = m_UsedIDinRoom.back();
		m_UsedIDinRoom.pop_back();		
	}
	else
	{
		IDinRoom = m_MaxIDinRoom;
		m_MaxIDinRoom++;
	}
	m_SessionCount++;
	return IDinRoom;
}
void Room::ReleaseIDinRoom(const INT id, bool& emptyRoom)
{
	WriteLock lock(m_Lock);
	m_SessionCount--;
	m_UsedIDinRoom.push_back(id);
	if (m_SessionCount <= 0)
		emptyRoom = true;
	
}


bool Room::IsExistSessionId(SessionId session_id)
{
	bool bFind = false;
	WriteLock lock(m_Lock);
	auto find_it = m_SessionMap.find(session_id);
	if (find_it != m_SessionMap.end())
		bFind = true;
	return bFind;

}
bool Room::JoinSessionId(SessionId session_id, UINT ID)
{					
	if (IsExistSessionId(session_id) == true)
		return false;
	WriteLock lock(m_Lock);
	m_SessionMap.insert(std::make_pair(session_id, ID));
	return true;
}
bool Room::ExitSessionId(SessionId session_id)
{
	bool bErase = false;
	WriteLock lock(m_Lock);
	auto find_it = m_SessionMap.find(session_id);
	if (find_it != m_SessionMap.end())
	{
		m_SessionMap.erase(find_it);
		bErase = true;
	}
	return bErase;
}

UINT Room::GetIDinRoom(SessionId session_id)
{
	UINT IDinRoom = 0;
	WriteLock lock(m_Lock);
	auto find_it = m_SessionMap.find(session_id);
	if (find_it != m_SessionMap.end())
		IDinRoom = find_it->second;
	return IDinRoom;
}
