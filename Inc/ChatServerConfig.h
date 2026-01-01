#pragma once

class ChatServerConfig : public ServerConfig<ChatServerConfig>
{
public:
	static void BindProperties();

	static const UINT MAX_USER_COUNT_CAP = 10000;
	
	INT m_ThreadCount = 4;
	WString m_ServerName = L"Chatting Server";
	ConnectInfo m_RootChatServerConnectInfo = ConnectInfo(L"", 0);
	ConnectInfo m_CommunityServerConnectInfo = ConnectInfo(L"", 0);
	UINT m_ChildChatPort = 0;
	UINT m_GameServerPort = 0;
	UINT m_BroadcastingServerPort = 0;	
	UINT m_RangeLevel = 0;
};