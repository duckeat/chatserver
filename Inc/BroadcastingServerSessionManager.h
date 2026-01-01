#pragma once

class BroadcastingServerSession;
class BroadcastingServerSessionManager
{
public:
	BroadcastingServerSessionManager();

	bool RegisterSession(SharedPtr<BroadcastingServerSession>::type session);
	bool UnregisterSession(SharedPtr<BroadcastingServerSession>::type session);

	bool SendTo(const SessionId& sessionId, std::function<void(SharedPtr<BroadcastingServerSession>::type)> func);
private:
	INT PopBroadcastingServerId();
	void PushBroadcastingServerId(const INT id);
private:
	struct IntLockfreeNode : public SLIST_ENTRY
	{
		INT m_Value;

		IntLockfreeNode(const INT value) : m_Value(value) {}
	};
	LockfreeNodeStack<IntLockfreeNode> m_BroadcastingServerIdPool;
	Array<SharedPtr<BroadcastingServerSession>::type, MAX_BROADCASTINGSERVER_SESSION>::type m_BroadCastingServerSession;
	RWLock m_Lock;
};
