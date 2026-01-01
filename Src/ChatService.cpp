#include "stdafx.h"
#include "ChatService.h"
#include "Modules.h"
#include "RoomManager.h"
#include "Room.h"
#include "RootChatServerSession.h"
#include "ChildChatServerSessionManager.h"
#include "BroadcastingServerSessionManager.h"
#include "BroadcastingServerSession.h"
#include "ChildChatServerSession.h"
#include "ChatSessionManager.h"
#include "ChatSession.h"


void ChatService::HandleSendChat(const ChatMessage& message, EChatMessageSource source)
{
	auto chat_session = Modules.ChatSessionManager->GetSession(message.m_SessionId);
	if (chat_session == nullptr)
	{
		WARNING_LOG(L"REQ_SEND_CHAT - NOT exist sessionId " << message.m_SessionId);
		return;
	}
	UINT64 roomID = chat_session->GetRoomID(message.m_ChatType);
	auto room = Modules.RoomManager->GetRoom(roomID, static_cast<ERequestChatType>(message.m_ChatType));
	if (room == nullptr)
	{
		WARNING_LOG(L"REQ_SEND_CHAT - NOT exist RoomID [" << roomID);
		return;
	}
	
	// sender가 존재하면 상위 채팅룸이 있는지 확인
	// 상위 채팅룸이 있으면 상위 채팅룸에도 메시지를 전달하도록 한다.
	if (room->HasParentRoom() == true)
	{
		switch (source)
		{
		case EChatMessageSource::fromChild:
			LOG(L"Send Packet to parent RoomID : " << room->GetRoomID() << L", chatType : " << message.m_ChatType);
			Modules.RootChatServerSession->Send_PT_CR_REQ_SEND_CHAT(room->GetRoomID(), message.m_ChatType, message);
			break;
		default:
			// Child 통해 온것 이외는 상위에 전달하지 않는다.
			break;
		}
		
	}

	BroadcastChatToRoom(*room, message);
}

void ChatService::HandleGlobalMessage(SessionId senderId, BYTE chatType, BYTE messageType, BYTE messageID, const TArray<FMessageAdditionalInfo>& arguments)
{
	auto chat_session = Modules.ChatSessionManager->GetSession(senderId);
	if (chat_session == nullptr)
	{
		WARNING_LOG(L"REQ_SEND_GLOBAL_MESSAGE - NOT exist sessionId " << senderId);
		return;
	}

    auto room = Modules.RoomManager->GetRoom(chat_session->GetRoomID(chatType), static_cast<ERequestChatType>(chatType));
	if (room == nullptr)
	{
		WARNING_LOG(L"GlobalMessage - Global Room not found");
		return;
	}

	// sender가 존재하면 상위 채팅룸이 있는지 확인
	// 상위 채팅룸이 있으면 상위 채팅룸에도 메시지를 전달하도록 한다.
	if (senderId != SessionId::Nil && room->HasParentRoom() == true)
	{
		LOG(L"Send Packet to parent RoomID : " << room->GetRoomID() << L", chatType : " << chatType);
		Modules.RootChatServerSession->Send_PT_CR_REQ_GLOBAL_MESSAGE(chatType, messageType, messageID, arguments);
	}

	BroadcastGlobalToRoom(*room, senderId, messageType, messageID, arguments);
}

void ChatService::BroadcastChatToRoom(const Room& room, const ChatMessage& message)
{
	const bool includeSender = (Modules.ChatServerConfig.m_RangeLevel == 1);

	for (UINT index = 0; index < room.GetMaxIDinRoom(); ++index)
	{
		SessionId targetId = room.GetJoinedChatSession(index);
		if (targetId == SessionId::Nil)
			continue;

		if (room.GetChatType() == ERequestChatType_Whisper)
		{
			if (targetId != message.m_TargetSessionID)
				continue;
		}
		else
		{
			if (!includeSender && targetId == message.m_SessionId)
				continue;
		}

		SendChatMessageToSession(
			targetId,
			room.GetRoomID(),
			room.GetChatType(),
			message);
	}
}

void ChatService::BroadcastGlobalToRoom(
	const Room& room,
	SessionId senderId,
	BYTE messageType,
	BYTE messageID,
	const TArray<FMessageAdditionalInfo>& arguments)
{
	const bool includeSender =
		(Modules.ChatServerConfig.m_RangeLevel == 1);

	for (UINT index = 0; index <  room.GetMaxIDinRoom(); ++index)
	{
		SessionId targetId = room.GetJoinedChatSession(index);

		if (targetId == SessionId::Nil)
			continue;

		if (!includeSender && targetId == senderId)
			continue;

		SendGlobalMessageToSession(
			targetId,
			room.GetChatType(),
			messageType,
			messageID,
			arguments);
	}
}

void ChatService::SendChatMessageToSession(
	SessionId targetId,
	UINT64 roomId, 
	ERequestChatType chatType,
	const ChatMessage& message)
{
	if (targetId == SessionId::Nil)
		return;

	auto msg = std::make_shared<ChatMessage>(message);
	// ChildChatServer 경유
	if (Modules.ChildChatServerSessionManager != nullptr)
	{
		Modules.ChildChatServerSessionManager->SendTo(
			targetId,
			[targetId, roomId, chatType, msg](SharedPtr<ChildChatServerSession>::type session)
			{
				if (session == nullptr)
					return;

				LOG(L"Send Chat Message to ChildChatServer "
					<< L"[TargetSession: " << targetId
					<< L", ChatType: " << chatType << L"]");

				session->Send_PT_RC_NFY_SEND_CHAT(roomId, chatType, *msg);
			});
	}
	// BroadcastingServer 직접 전송
	else
	{
		Modules.BroadcastingServerSessionManager->SendTo(
			targetId,
			[targetId, chatType, msg](SharedPtr<BroadcastingServerSession>::type session)
			{
				if (session == nullptr)
					return;

				LOG(L"Send Chat Message to Client via Broadcasting "
					<< L"[TargetSession: " << targetId
					<< L", ChatType: " << chatType << L"]");

				TArray<FMessageAdditionalInfo> arguments;

				session->SendBypass_PT_GC_NFY_CHAT_MESSAGE(
					chatType,
					FMiniPlayerInfo(
						static_cast<ECharacterJobClass>(
							static_cast<uint8>(msg->m_PlayerInfo.PlayerJobClass)),
						msg->m_PlayerInfo.PlayerWorldID,
						msg->m_PlayerInfo.PlayerName,
						msg->m_PlayerInfo.GuildName,
						msg->m_PlayerInfo.GuildEmblemIconID,
						msg->m_PlayerInfo.UnionEmblemIconID),
					msg->m_Message,
					arguments,
					msg->m_SessionId);
			});
	}
}

void ChatService::SendGlobalMessageToSession(
	SessionId targetId,
	ERequestChatType chatType,
	BYTE messageType,
	BYTE messageID,
	const TArray<FMessageAdditionalInfo>& arguments)
{
	if (Modules.ChildChatServerSessionManager != nullptr)
	{
		Modules.ChildChatServerSessionManager->SendTo(
			targetId,
			[chatType, messageType, messageID, arguments](SharedPtr<ChildChatServerSession>::type session)
			{
				LOG(L"Send Global Message [" << messageID << L"] to ChildChatServer");				
				session->Send_PT_RC_NFY_GLOBAL_MESSAGE(
					chatType, messageType, messageID, arguments);
			});
	}
	else
	{
		Modules.BroadcastingServerSessionManager->SendTo(
			targetId,
			[targetId, messageType, messageID, arguments](SharedPtr<BroadcastingServerSession>::type session)
			{
				LOG(L"Send Global Message [" << messageID << L"] to Client via Broadcasting [SessionId : " << targetId << L"]");

				session->SendBypass_PT_GC_NFY_CHAT_BY_ID(
					messageType,
					0,
					FMiniPlayerInfo(),
					GlobalSystemMessageIds[messageID],
					arguments,
					false,
					targetId);
			});
	}
}