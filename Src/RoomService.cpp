#include "stdafx.h"
#include "ChatService.h"

#include "Modules.h"
#include "RoomManager.h"
#include "Room.h"
#include "RoomService.h"


RoomService::FRoomResult RoomService::CreateOrJoinRoom(BYTE chatType, UINT64 roomID, SessionId sessionId)
{
    if (chatType >= ERequestChatType_Max)
        return { EServerError_CHAT_CreateRoom_OverflowChatType };

    auto room = Modules.RoomManager->GetRoom(roomID, static_cast<ERequestChatType>(chatType));
    if (room == nullptr)
    {
        LOG(L"Create new Room ID : " << roomID << L", chatType : " << chatType);
        room = Modules.RoomManager->CreateRoom(roomID, static_cast<ERequestChatType>(chatType));
    }

    if (room->JoinSession(sessionId))
    {
        LOG(L"Session [" << sessionId << "] joined Room ID : " << roomID << ", chatType : " << chatType);
        return { EServerError_SUCCESS };
    }
    else
    {
        WARNING_LOG(L"Session [" << sessionId << "] failed to join Room ID : " << roomID << ", chatType : " << chatType);
        return { EServerError_CHAT_CreateRoom_JoinRoom };
    }
}

void RoomService::ExitRoom(BYTE chatType, UINT64 roomID, SessionId sessionId)
{
    auto room = Modules.RoomManager->GetRoom(roomID, static_cast<ERequestChatType>(chatType));
    if (room)
    {
        if (room->ExitSession(sessionId))
        {
            LOG(L"Session [" << sessionId << "] exited Room ID : " << roomID);
        }
        else
        {
            WARNING_LOG(L"Session [" << sessionId << "] failed to exit Room ID : " << roomID);
        }
    }
    else
    {
        WARNING_LOG(L"No Room found for exit [Room ID : " << roomID << ", chatType : " << chatType << "]");
    }
}
