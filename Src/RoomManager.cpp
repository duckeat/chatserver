#include "stdafx.h"
#include "RoomManager.h"
#include "Modules.h"
#include "ChatServerIOCPProcessor.h"
#include "Room.h"

RoomEachTypeManager::~RoomEachTypeManager()
{
	AutoLock lock(m_Lock);	
	m_MapRoom.clear();
}
SharedPtr<Room>::type RoomEachTypeManager::CreateRoom(UINT64 roomID)
{
	AutoLock lock(m_Lock);
	if (roomID == 0)   // roomID가 0이면 컨텐츠와 연결된 room이 아님 (예 : 파티 등...)
	{
		roomID = m_MapRoom.size() + 1;
	}	
	
	if(auto create_room = std::make_shared<Room>(roomID, m_ChatType))
	{
		m_MapRoom[roomID] = create_room;		
		Modules.IOCPProcessor->PostTask([create_room]()
			{
				create_room->Initialize();
			});
		return create_room;
	}	
	CHECK_DEBUG(false);
	return nullptr;
	
}
bool  RoomEachTypeManager::DestroyRoom(UINT64 roomID)
{
	AutoLock lock(m_Lock);
	auto room = m_MapRoom.find(roomID);
	if (room == m_MapRoom.end())
	{
		return false;
	}
	auto destroyRoom = room->second;
	m_MapRoom.erase(room);
	return true;
}
SharedPtr<Room>::type RoomEachTypeManager::GetRoom(UINT64 roomID)
{		
	AutoLock lock(m_Lock);
	auto room = m_MapRoom.find(roomID);
	if(room == m_MapRoom.end())
	{
		return nullptr;
	}
	return room->second;
}

void RoomEachTypeManager::ReportAllRoomtoRoot()
{
	AutoLock lock(m_Lock);
	for(auto room : m_MapRoom)
	{
		room.second->ReporttoRoot();
	}	
}


RoomManager::RoomManager()
{
	m_RoomManager.fill(nullptr);	
}
RoomManager::~RoomManager()
{
	// 월드 ChatType 매니저 삭제
	for(auto it : m_RoomManager)
	{
		it.reset();
	}
	m_RoomManager.fill(nullptr);
}
bool  RoomManager::Initialize()
{
	// Chat Range Level이 낮을수록 상위 채널을 가지고 있어야 함
	switch(GetChatRangeLevel())
	{
	case 1: // 월드 
		m_RoomManager[ERequestChatType_World] = std::make_shared<RoomEachTypeManager>(ERequestChatType_World);
		m_RoomManager[ERequestChatType_Friendly] = std::make_shared<RoomEachTypeManager>(ERequestChatType_Friendly);
		m_RoomManager[ERequestChatType_Guild] = std::make_shared<RoomEachTypeManager>(ERequestChatType_Guild);
		m_RoomManager[ERequestChatType_Party] = std::make_shared<RoomEachTypeManager>(ERequestChatType_Party);
		m_RoomManager[ERequestChatType_Whisper] = std::make_shared<RoomEachTypeManager>(ERequestChatType_Whisper);
	case 2: // 월드군
		m_RoomManager[ERequestChatType_WorldGroup] = std::make_shared <RoomEachTypeManager>(ERequestChatType_WorldGroup);
		m_RoomManager[ERequestChatType_G1] = std::make_shared<RoomEachTypeManager>(ERequestChatType_G1);
		m_RoomManager[ERequestChatType_G2] = std::make_shared<RoomEachTypeManager>(ERequestChatType_G2);
		m_RoomManager[ERequestChatType_G3] = std::make_shared<RoomEachTypeManager>(ERequestChatType_G3);
		m_RoomManager[ERequestChatType_G4] = std::make_shared<RoomEachTypeManager>(ERequestChatType_G4);
		m_RoomManager[ERequestChatType_G5] = std::make_shared<RoomEachTypeManager>(ERequestChatType_G5);
		m_RoomManager[ERequestChatType_Union] = std::make_shared<RoomEachTypeManager>(ERequestChatType_Union);
	case 3: // 유니버스
		m_RoomManager[ERequestChatType_Universe] = std::make_shared <RoomEachTypeManager>(ERequestChatType_Universe);
		break;
	}
	
	return true;
}

// 0 아무것도 아님, 1: 월드, 2:월드군, 3:유니버스
int RoomManager::GetChatRangeLevel()
{
	int chatRangeLevel = 0;
	if (Modules.BroadcastingServerSessionManager != nullptr)
	{
		chatRangeLevel = 1;
	}
	else if(Modules.ChildChatServerSessionManager != nullptr)
	{
		if(Modules.RootChatServerSession != nullptr)
		{
			chatRangeLevel = 2;
		}
		else
		{
			chatRangeLevel = 3;
		}
	}	
	return chatRangeLevel;
}

SharedPtr<RoomEachTypeManager>::type RoomManager::GetEachTypeManager(const ERequestChatType chatType)
{		
	return m_RoomManager[chatType];	
}

SharedPtr<Room>::type RoomManager::CreateRoom(UINT64 roomID, const ERequestChatType chatType)
{
	if(chatType >= ERequestChatType_Max)
	{
		CHECK_DEBUG(false);
		return nullptr;
	}
	auto each_type_manager = GetEachTypeManager(chatType);	
	if(each_type_manager == nullptr)
	{
		CHECK_DEBUG(false);
		return nullptr;
	}
	auto room = each_type_manager->CreateRoom(roomID);
	return room;
}
bool RoomManager::DestroyRoom(const UINT64 room_id, const ERequestChatType chatType)
{
	if (chatType >= ERequestChatType_Max)
	{
		CHECK_DEBUG(false);
		return false;
	}
	auto each_type_manager = GetEachTypeManager(chatType);
	if (each_type_manager == nullptr)
	{
		CHECK_DEBUG(false);
		return false;
	}
	return each_type_manager->DestroyRoom(room_id);	
}

SharedPtr<Room>::type RoomManager::GetRoom(const UINT64 room_id, const ERequestChatType chatType)
{
	if (chatType >= ERequestChatType_Max)
	{
		CHECK_DEBUG(false);
		return nullptr;
	}
	auto each_type_manager = GetEachTypeManager(chatType);
	if (each_type_manager == nullptr)
	{
		CHECK_DEBUG(false);
		return nullptr;
	}
	return each_type_manager->GetRoom(room_id);
}


bool  RoomManager::JoinSession(const BYTE& chatType, const UINT64& roomKey, SessionId session_id)
{
	auto room_each_type_manager = GetEachTypeManager(static_cast<ERequestChatType>(chatType));
	if (room_each_type_manager == nullptr)
		return false;

	auto room = room_each_type_manager->GetRoom(roomKey);
	if (nullptr == room)
	{
		room = room_each_type_manager->CreateRoom(roomKey);
		if(nullptr == room)
		{
			return false;
		}
	}

	if(false == room->JoinSession(session_id))
	{
		return false;
	}
	return true;

}
bool  RoomManager::ExitSession(const BYTE& chatType, const UINT64& roomKey, SessionId session_id)
{
	auto room_each_type_manager = GetEachTypeManager(static_cast<ERequestChatType>(chatType));
	if (room_each_type_manager == nullptr)
		return false;

	auto room = room_each_type_manager->GetRoom(roomKey);
	if (nullptr == room)
	{
		return false;		
	}

	if (false == room->ExitSession(session_id))
	{
		return false;
	}	
	return true;
}

void RoomManager::ReportAllRoomtoRoot()
{
	for(BYTE chatType = ERequestChatType::ERequestChatType_Normal; chatType < ERequestChatType::ERequestChatType_Max; chatType++)
	{
		auto room_each_type_manager = GetEachTypeManager(static_cast<ERequestChatType>(chatType));
		if (room_each_type_manager == nullptr)
			continue;

		room_each_type_manager->ReportAllRoomtoRoot();		
	}

}