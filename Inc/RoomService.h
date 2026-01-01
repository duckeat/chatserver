#pragma once
#include "RoomManager.h"
#include "ServerErrorCode.h"

class RoomService
{
public:
    struct FRoomResult
    {
        EServerError ErrorCode;
    };

    static RoomService* Get()
    {
        static RoomService instance;
        return &instance;
    }

    FRoomResult CreateOrJoinRoom(BYTE chatType, UINT64 roomID, SessionId sessionId);
    void ExitRoom(BYTE chatType, UINT64 roomID, SessionId sessionId);
};