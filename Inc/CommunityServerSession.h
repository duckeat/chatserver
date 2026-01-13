#pragma once

#include "CommonAuctionStruct.h"

class CommunityServerSessionProtocol : public SessionDecorator<Connector>
{
	DECLARE_CLASS(CommunityServerSessionProtocol, SessionDecorator<Connector>);
public:
	CommunityServerSessionProtocol(IOCPProcessor* iocpProcessor) : Super(iocpProcessor) {}

#include "CommunityServerSession.generated.inl"
};

class CommunityServerSession : public CommunityServerSessionProtocol, public std::enable_shared_from_this<CommunityServerSession>
{
	DECLARE_CLASS(CommunityServerSession, CommunityServerSessionProtocol);
public:
	CommunityServerSession(IOCPProcessor* iocpProcessor);
	~CommunityServerSession();

	bool OnConnected() override;
	void OnConnectFailed() override;
	void OnDisconnected() override;

	void OnRecvMessage(StreamReader& reader) override;
	bool Send(SendBufferStreamWriter& writer) const override;

	void Recv_PT_TM_ANS_REGIST_SERVER(const UINT& result) override;
	void Recv_PT_TM_REQ_ENTER_USER(const SessionId& session_id) override;
	void Recv_PT_TM_REQ_LEAVE_USER(const SessionId& session_id) override;
	void Recv_PT_TM_REQ_EXCHANGE_USER(const SessionId& old_session_id, const SessionId& new_session_id) override;
private:
	void RetryConnect();
private:
	std::atomic<bool> m_IsAlive{ true };
	std::atomic<bool> m_IsRetrying{ false };
};