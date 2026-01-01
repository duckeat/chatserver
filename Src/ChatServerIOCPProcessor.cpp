#include "stdafx.h"
#include "ChatServerIOCPProcessor.h"

IMPLEMENT_CLASS(ChatServerIOCPProcessor);

ChatServerIOCPProcessor::ChatServerIOCPProcessor(INT threadCount)
	: Super(threadCount)
{
}

