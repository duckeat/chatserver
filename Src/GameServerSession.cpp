#include "stdafx.h"
#include "ServerErrorCode.h"
#include "GameServerSession.h"
#include "ServerShareChatStruct.h"
#include "Modules.h"
#include "GameServerSessionManager.h"
#include "ChatSession.h"
#include "ChatSessionManager.h"
#include "RoomManager.h"
#include "Room.h"
#include "RootChatServerSession.h"
#include "ChatService.h"
#include "RoomService.h"

IMPLEMENT_CLASS(GameServerSessionProtocol);
IMPLEMENT_CLASS(GameServerSession);
IMPLEMENT_GAMESERVERSESSIONPROTOCOL_PROTOCOL_DEFINE();

#include "GameServerSessionBody.generated.inl"

GameServerSession::GameServerSession() : m_Index(INVALID_GAMESERVER_ID)
{
}

bool GameServerSession::OnAccepted()
{
	if (!Super::OnAccepted())
	{
		Close();
		return false;
	}
	if (false == Modules.GameServerSessionManager->RegisterSession(shared_from_this()))
	{	
		Close();
		return false;
	}
	LOG(L"GameServer connected");

	return true;
}

void GameServerSession::OnDisconnected()
{
	LOG(L"disconnected GameServer");

	if (auto self = weak_from_this().lock())
	{
		Modules.GameServerSessionManager->UnregisterSession(self);
	}	

	Super::OnDisconnected();
}

void GameServerSession::OnRecvMessage(StreamReader& reader)
{
	if (!ProcessPacket(reader))
	{
		CHECK_DEBUG(false);

		INFO_LOG(L"Invalid Packet - Protocol Function");
		return;
	}
}

bool GameServerSession::Send(SendBufferStreamWriter& writer) const
{
	if (Super::PostBuildMessage(writer))
	{
		return const_cast<GameServerSession*>(this)->PostSend(writer.GetBuffers());
	}
	return false;
}
void GameServerSession::Recv_PT_GM_REQ_JOIN_CHATROOM(const BYTE& chatType, const UINT64& roomID, const SessionId& session_id)
{
	try
	{
		auto ret = RoomService::Get()->CreateOrJoinRoom(chatType, roomID, session_id);
		Send_PT_MG_ANS_JOIN_CHATROOM(ret.ErrorCode, chatType, roomID, session_id);
	}
	catch (const std::exception& e)
	{
		ERROR_LOG(L"ChatService exception: " << e.what() << L" in [Recv_PT_GM_REQ_JOIN_CHATROOM]");		
	}

}

void GameServerSession::Recv_PT_GM_REQ_EXIT_CHATROOM(const BYTE& chatType, const UINT64& roomID, const SessionId& session_id)
{
	try
	{
		RoomService::Get()->ExitRoom(chatType, roomID, session_id);
	}
	catch (const std::exception& e)
	{
		ERROR_LOG(L"ChatService exception: " << e.what() << L" in [Recv_PT_GM_REQ_EXIT_CHATROOM]");		
	}

}

void GameServerSession::Recv_PT_GM_REQ_GLOBAL_MESSAGE(const SessionId& userSessionId, const BYTE& chatType, const BYTE& messageType, const BYTE& messageID, const TArray<struct FMessageAdditionalInfo>& arguments)
{
	try
	{
		ChatService::Get()->HandleGlobalMessage(userSessionId, chatType, messageType, messageID, arguments);
	}
	catch (const std::exception & e)
	{
		ERROR_LOG(L"ChatService exception: " << e.what() << L" in [Recv_PT_GM_REQ_GLOBAL_MESSAGE]");		
	}
}

void GameServerSession::Recv_PT_GM_REQ_SEND_CHAT(const ChatMessage& message)
{
	try
	{
		ChatService::Get()->HandleSendChat(message, EChatMessageSource::fromChild);
	}
	catch (const std::exception& e)
	{
		ERROR_LOG(L"ChatService exception: " << e.what() << L" in [Recv_PT_GM_REQ_SEND_CHAT]");		
	}
}
