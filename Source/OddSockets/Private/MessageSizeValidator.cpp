/**
 * OddSockets UE SDK - Message Size Validator Implementation
 */

#include "MessageSizeValidator.h"

const int32 UMessageSizeValidator::MaxMessageSizeBytes = 32768; // 32KB

bool UMessageSizeValidator::ValidateSize(const FString& Message)
{
    return Message.Len() <= MaxMessageSizeBytes;
}

int32 UMessageSizeValidator::GetMessageSize(const FString& Message)
{
    return Message.Len();
}

int32 UMessageSizeValidator::GetMaxMessageSize()
{
    return MaxMessageSizeBytes;
}
