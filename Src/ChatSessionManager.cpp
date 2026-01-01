#include "stdafx.h"
#include "ChatSessionManager.h"
#include "ChatSession.h"
#include "Modules.h"
#include "ChatServerIOCPProcessor.h"

ChatSessionManager::ChatSessionManager():
	m_Sessions{}
{
	
}

bool ChatSessionManager::RegisterSession(SessionId session_id)
{
	if(nullptr != m_Sessions[session_id.GetIndex()])
	{
		ERROR_LOG(L"sesssion id table is not nullptr in RegisterSession [" << session_id);
		CHECK_DEBUG(false);
		return false;
	}
	m_Sessions[session_id.GetIndex()] = std::make_shared<ChatSession>(session_id);	
	
	return true;
}
bool ChatSessionManager::UnregisterSession(SessionId session_id)
{
	auto session = m_Sessions[session_id.GetIndex()];
	if (nullptr == session)
	{
		ERROR_LOG(L"sesssion id table is nullptr in UnregisterSession [" << session_id);
		return false;
	}
	
	m_Sessions[session_id.GetIndex()].reset();
	Modules.IOCPProcessor->PostTask([session]()
	{
		session->ExitAllRoom();
		
	});
	return true;
}
SharedPtr<ChatSession>::type ChatSessionManager::GetSession(SessionId session_id)
{
	return m_Sessions[session_id.GetIndex()];
}

bool ChatSessionManager::ExchangeSession(SessionId old_session_id, SessionId new_session_id)
{
	auto chat_session = GetSession(old_session_id);
	CHECK_DEBUG(chat_session != nullptr);
	if(chat_session == nullptr)				 	{
		
		return false;
	}	
	return chat_session->SetSessionId(new_session_id);	
}