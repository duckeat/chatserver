#pragma once

class ChatServerIOCPProcessor : public IOCPTaskProcessor
{
	DECLARE_CLASS(ChatServerIOCPProcessor, IOCPTaskProcessor);
public:
	ChatServerIOCPProcessor(INT threadCount);

	template<typename T_TASK>
	void PostTask(const T_TASK& task)
	{
		Super::PostTask(CreateDBTask(task));
	}
};											  