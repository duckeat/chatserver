#pragma once

#include "ServerSharePartyStruct.h"
#include "ServerShareZoneStruct.h"

class GameServerSessionProtocol : public SessionDecorator<Session>
{
	DECLARE_CLASS(GameServerSessionProtocol, SessionDecorator<Session>);
public:

#include "GameServerSession.generated.inl"
};

class GameServerSession : public GameServerSessionProtocol, public std::enable_shared_from_this<GameServerSession>
{
	DECLARE_CLASS(GameServerSession, GameServerSessionProtocol);
public:
	GameServerSession();

	bool OnAccepted() override;
	void OnDisconnected() override;

	void OnRecvMessage(StreamReader& reader) override;
	bool Send(SendBufferStreamWriter& writer) const override;

	UINT GetServerIndex() const { return m_Index; }

	void Recv_PT_GM_REQ_JOIN_CHATROOM(const BYTE& chatType, const UINT64& roomID, const SessionId& session_id) override;
	void Recv_PT_GM_REQ_EXIT_CHATROOM(const BYTE& chatType, const UINT64& roomID, const SessionId& session_id) override;
	void Recv_PT_GM_REQ_GLOBAL_MESSAGE(const SessionId& userSessionId, const BYTE& chatType, const BYTE& messageType, const BYTE& messageID, const TArray<struct FMessageAdditionalInfo>& arguments) override;
	void Recv_PT_GM_REQ_SEND_CHAT(const ChatMessage& message) override;
private:
	UINT m_Index;
	SessionId m_SessionId;
};

class GameServerSessionFactory : public SessionFactory, public Singleton<GameServerSessionFactory>
{
public:
	Session* CreateSession() override
	{
		return Create<GameServerSession>();
	}
};