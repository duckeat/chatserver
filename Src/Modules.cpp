
#include "stdafx.h"
#include "Modules.h"

ModuleHandler::ModuleHandler()	:
	IOCPProcessor(nullptr) ,
	GlobalTaskProcessor(nullptr) ,
	HeartbeatThread(nullptr) ,	
	SessionIdPool(nullptr) ,
	GameServerSessionManager(nullptr),
	BroadcastingServerSessionManager(nullptr),
	RootChatServerSession(nullptr),	
	CommunityServerSession(nullptr),
	ChildChatServerSessionManager(nullptr)
{
	
}

void ModuleHandler::InitializeInstance()
{
	
}