#pragma once
#include "RoomManager.h"
#include "ServerShareChatStruct.h"

constexpr UINT64 GLOBAL_CHAT_ROOM_ID = 1;

enum class EChatMessageSource
{
    fromChild,
    fromParent,
};

class ChatService
{
public:
    static ChatService* Get()
    {
        static ChatService instance;
        return &instance;
    }

    void HandleSendChat(const ChatMessage& message, EChatMessageSource source);
    void HandleGlobalMessage(SessionId sessionId, BYTE chatType, BYTE messageType, BYTE messageID, const TArray<FMessageAdditionalInfo>& arguments);
protected:
    void BroadcastChatToRoom(const Room& room, const ChatMessage& message);
    void BroadcastGlobalToRoom(const Room& room, SessionId senderId, BYTE messageType, BYTE messageID, const TArray<FMessageAdditionalInfo>& arguments);
    void SendChatMessageToSession(SessionId targetId, UINT64 roomId, ERequestChatType chatType, const ChatMessage& message);
    void SendGlobalMessageToSession(SessionId targetId, ERequestChatType chatType, BYTE messageType, BYTE messageID, const TArray<FMessageAdditionalInfo>& arguments);
};