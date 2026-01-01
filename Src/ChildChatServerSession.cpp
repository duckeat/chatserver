#include "stdafx.h"
#include "ServerErrorCode.h"
#include "ChildChatServerSession.h"
#include "ServerShareChatStruct.h"
#include "Modules.h"
#include "ChildChatServerSessionManager.h"
#include "ChatService.h"
#include "RoomService.h"

IMPLEMENT_CLASS(ChildChatServerSessionProtocol);
IMPLEMENT_CLASS(ChildChatServerSession);
IMPLEMENT_CHILDCHATSERVERSESSIONPROTOCOL_PROTOCOL_DEFINE();

#include "ChildChatServerSessionBody.generated.inl"

ChildChatServerSession::ChildChatServerSession()
{
}

bool ChildChatServerSession::OnAccepted()
{
	if (!Super::OnAccepted())
	{
		return false;
	}
	auto self = shared_from_this();

	if(false == Modules.ChildChatServerSessionManager->RegisterSession(self, m_SessionId))
	{
		Send_PT_RC_ANS_REGISTER_CHILDCHAT_RESULT(EServerError_CHAT_Registered_SessionID);
		WARNING_LOG(L"Fail to regist ChildChatServer by accept.");
		return false;
	}
	Send_PT_RC_ANS_REGISTER_CHILDCHAT_RESULT(EServerError_SUCCESS);
	LOG(L"ChildChatServer connected");
	
	return true;
}

void ChildChatServerSession::OnDisconnected()
{
	LOG(L"disconnected GameServer");

	auto self = shared_from_this();
	Modules.ChildChatServerSessionManager->UnregisterSession(self);
	Super::OnDisconnected();
}

void ChildChatServerSession::OnRecvMessage(StreamReader& reader)
{
	if (!ProcessPacket(reader))
	{
		CHECK_DEBUG(false);

		INFO_LOG(L"Invalid Packet - Protocol Function");
		return;
	}
}

bool ChildChatServerSession::Send(SendBufferStreamWriter& writer) const
{
	if (IsClose() == true)
	{
		return false;
	}
	try
	{
		return const_cast<ChildChatServerSession*>(this)->PostSend(writer.GetBuffers());
	}
	catch (...)
	{
		const_cast<ChildChatServerSession*>(this)->Close();
		return false;
	}
	return false;
}

void ChildChatServerSession::Recv_PT_CR_REQ_SEND_CHAT(const UINT64& roomID, const BYTE& chatType, const ChatMessage& message)
{
	try
	{
		ChatService::Get()->HandleSendChat(message, EChatMessageSource::fromChild);
	}
	catch (const std::exception& e)
	{
		ERROR_LOG(L"ChatService exception: " << e.what() << L" in [Recv_PT_CR_REQ_SEND_CHAT]");
		Close();
	}
	
}

void ChildChatServerSession::Recv_PT_CR_REQ_GLOBAL_MESSAGE(const BYTE& chatType, const BYTE& messageType, const BYTE& messageID, const TArray<struct FMessageAdditionalInfo>& arguments)
{
	try
	{
		ChatService::Get()->HandleGlobalMessage(GetSessionId(), chatType, messageType, messageID, arguments);
	}
	catch (const std::exception& e)
	{
		ERROR_LOG(L"ChatService exception: " << e.what() << L" in [Recv_PT_CR_REQ_GLOBAL_MESSAGE]");
		Close();
	}
}

void ChildChatServerSession::Recv_PT_CR_REQ_CREATE_CHATROOM(const UINT64& roomID, const BYTE& chatType)
{
	try
	{
		auto result = RoomService::Get()->CreateOrJoinRoom(chatType, roomID, GetSessionId());
		Send_PT_RC_ANS_CREATE_CHATROOM(result.ErrorCode, roomID, chatType);
	}
	catch (const std::exception& e)
	{
		ERROR_LOG(L"ChatService exception: " << e.what() << L" in [Recv_PT_CR_REQ_CREATE_CHATROOM]");
		Close();
	}

}

void ChildChatServerSession::Recv_PT_CR_REQ_DESTROY_CHATROOM(const UINT64& roomID, const BYTE& chatType)
{		
	try
	{
		RoomService::Get()->ExitRoom(chatType, roomID, GetSessionId());
	}
	catch (const std::exception& e)
	{
		ERROR_LOG(L"ChatService exception: " << e.what() << L" in [Recv_PT_CR_REQ_DESTROY_CHATROOM]");
		Close();
	}
}

