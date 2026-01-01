#pragma once

class ChildChatServerSessionProtocol : public SessionDecorator<Session>
{
	DECLARE_CLASS(ChildChatServerSessionProtocol, SessionDecorator<Session>);
public:

#include "ChildChatServerSession.generated.inl"
};

class ChildChatServerSession : public ChildChatServerSessionProtocol, std::enable_shared_from_this<ChildChatServerSession>
{
	DECLARE_CLASS(ChildChatServerSession, ChildChatServerSessionProtocol);
public:
	ChildChatServerSession();

	bool OnAccepted() override;
	void OnDisconnected() override;

	void OnRecvMessage(StreamReader& reader) override;
	bool Send(SendBufferStreamWriter& writer) const override;

	const SessionId& GetSessionId() const { return m_SessionId; }

	void Recv_PT_CR_REQ_SEND_CHAT(const UINT64& roomID, const BYTE& chatType, const ChatMessage& message);
	void Recv_PT_CR_REQ_GLOBAL_MESSAGE(const BYTE& chatType, const BYTE& messageType, const BYTE& messageID, const TArray<struct FMessageAdditionalInfo>& arguments);
	void Recv_PT_CR_REQ_CREATE_CHATROOM(const UINT64& roomID, const BYTE& chatType);
	void Recv_PT_CR_REQ_DESTROY_CHATROOM(const UINT64& roomID, const BYTE& chatType);
private:
	SessionId m_SessionId;	
};

class ChildChatServerSessionFactory : public SessionFactory, public Singleton<ChildChatServerSessionFactory>
{
public:
	Session* CreateSession() override
	{
		return Create<ChildChatServerSession>();
	}
};