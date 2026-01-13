#pragma once

class RootChatServerSessionProtocol : public SessionDecorator<Connector>
{
	DECLARE_CLASS(RootChatServerSessionProtocol, SessionDecorator<Connector>);
public:
	RootChatServerSessionProtocol(IOCPProcessor* iocpProcessor) : Super(iocpProcessor) {}

#include "RootChatServerSession.generated.inl"
};

class RootChatServerSession : public RootChatServerSessionProtocol, public std::enable_shared_from_this<RootChatServerSession>
{
	DECLARE_CLASS(RootChatServerSession, RootChatServerSessionProtocol);
public:
	RootChatServerSession(IOCPProcessor* iocpProcessor);
	~RootChatServerSession();
	void OnConnectFailed() override;
	bool OnConnected() override;
	void OnDisconnected() override;

	void OnRecvMessage(StreamReader& reader) override;
	bool Send(SendBufferStreamWriter& writer) const override;
	bool IsConnected() { return m_bConnect; }

	void Recv_PT_RC_NFY_SEND_CHAT(const UINT64& roomID, const BYTE& chatType, const ChatMessage& message) override;
	void Recv_PT_RC_ANS_CREATE_CHATROOM(const UINT& result, const UINT64& roomID, const BYTE& chatType) override;
	void Recv_PT_RC_ANS_REGISTER_CHILDCHAT_RESULT(const UINT& result) override;
private:
	void RetryConnect();
private:
	std::atomic<bool> m_IsAlive{ true };
	std::atomic<bool> m_IsRetrying{ false };
	std::atomic<bool> m_bConnect{ false };

};