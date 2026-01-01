#include "stdafx.h"
#include "ChatServerConfig.h"

void ChatServerConfig::BindProperties()
{
	DataPathConfig::BindProperties();

	auto binder = BeginPropertyMapBind<ChatServerConfig>()
		.BindProperty(L"ThreadCount", &ChatServerConfig::m_ThreadCount)
		.BindProperty(L"ServerName", &ChatServerConfig::m_ServerName)
		.BindProperty(L"RootChatServerConnectInfo", &ChatServerConfig::m_RootChatServerConnectInfo)
		.BindProperty(L"CommunityServerConnectInfo", &ChatServerConfig::m_CommunityServerConnectInfo)
		.BindProperty(L"ChildChatPort", &ChatServerConfig::m_ChildChatPort)
		.BindProperty(L"GameServerPort", &ChatServerConfig::m_GameServerPort)
		.BindProperty(L"BroadcastingServerPort", &ChatServerConfig::m_BroadcastingServerPort)
		;
}