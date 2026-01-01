#pragma once

#include "ChatServerConfig.h"

class GameServerSessionManager;
class RootChatServerSession;
class ChildChatServerSessionManager;
class CommunityServerSession;
class BroadcastingServerSessionManager;
class ChatSessionManager;
class ChatService;

class ModuleHandler : public Singleton<ModuleHandler>
{
public:
	ModuleHandler();

	void InitializeInstance() override;
public:
	ChatServerConfig						ChatServerConfig;	

	class ChatServerIOCPProcessor*				IOCPProcessor;
	class GlobalTaskProcessor*					GlobalTaskProcessor;
	class HeartbeatThread*						HeartbeatThread;	
	class SessionIdPool*						SessionIdPool;

	class GameServerSessionManager*				GameServerSessionManager;
	class BroadcastingServerSessionManager*		BroadcastingServerSessionManager;
	class RootChatServerSession*				RootChatServerSession;
	class CommunityServerSession*				CommunityServerSession;
	class ChildChatServerSessionManager*		ChildChatServerSessionManager;
	class RoomManager*							RoomManager;
	class ChatSessionManager*					ChatSessionManager;
	class ChatService*							ChatService;

	//MessageStreamQueue*						MessageStreamQueue;
};


#define Modules ModuleHandler::GetInstance()