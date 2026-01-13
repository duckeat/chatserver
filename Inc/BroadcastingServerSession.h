#pragma once

class BroadcastingServerSessionProtocol : public SessionDecorator<Session>
{
	typedef FPacketHeader ClientPacketHeader;
	DECLARE_CLASS(BroadcastingServerSessionProtocol, SessionDecorator<Session>);
protected:
	bool MakeMultiCastingHeader(StreamWriter& writer, const Vector<SessionId>::type& idList) const;
	bool MakeBroadCastingHeader(StreamWriter& writer, const Vector<SessionId>::type& sessionIdList) const;
	bool SendMultiCast(SendBufferStreamWriter& writer, const INT broadcastingServerId) const;
public:
	BroadcastingServerSessionProtocol();
	void MakeBypassHeader(const SessionId& sessionId, SendBufferStreamWriter& writer) const;
	INT  GetServerId() { return m_ServerId; };
	bool SetServerId(INT server_id);
#include "BroadcastingServerSession.generated.inl"
#include "BroadCastingServerSession.clientgame.generated.inl"

protected:
	enum STATE
	{
		STATE_NONE,										// 연결되지 않은 상태
		STATE_CONNECTED,								// 연결된 상태
		STATE_ACQUIRED_ID,								// ID 를 발급받은 상태
		STATE_INITIALIZED,								// Broadcasting 초기화 완료
	};
	std::atomic<STATE> m_State{ STATE_NONE };
	INT m_ServerId;
};

class BroadcastingServerSession : public BroadcastingServerSessionProtocol, std::enable_shared_from_this<BroadcastingServerSession>
{
	DECLARE_CLASS(BroadcastingServerSession, BroadcastingServerSessionProtocol);
public:
	bool OnAccepted() override;
	void OnDisconnected() override;

	void OnRecvMessage(StreamReader& reader) override;
	bool Send(SendBufferStreamWriter& writer) const override;
	void Recv_PT_BM_REQ_SERVER_IDENTITY(const INT& serverId) override;
};

class BroadcastingServerSessionFactory : public SessionFactory, public Singleton<BroadcastingServerSessionFactory>
{
public:
	Session* CreateSession() override
	{
		return Create<BroadcastingServerSession>();
	}
};