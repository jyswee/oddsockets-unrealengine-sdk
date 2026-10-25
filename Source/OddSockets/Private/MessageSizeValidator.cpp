/**
 * OddSockets UE SDK - Message Size Validator Implementation
 *
 * The 32KB limit mirrors the platform's transport buffer cap: a frame over it
 * is dropped by the server, so validating client-side is the difference
 * between a clear error and a silent loss.
 *
 * Sizes are UTF-8 byte counts, not FString::Len() — Len() counts TCHARs, and a
 * multi-byte character would pass a Len() check yet exceed the wire limit.
 */

#include "MessageSizeValidator.h"

namespace
{
    constexpr int32 MaxMessageSizeBytes = 32 * 1024;
}

UMessageSizeValidator::UMessageSizeValidator()
{
}

int32 UMessageSizeValidator::CalculateUTF8ByteSize(const FString& Message)
{
    // FTCHARToUTF8 converts without allocating an FString copy of the result.
    return FTCHARToUTF8(*Message).Length();
}

FString UMessageSizeValidator::CreateValidationMessage(int32 SizeBytes, bool bIsValid)
{
    if (bIsValid)
    {
        return FString::Printf(TEXT("Message size %s is within the %dKB limit"),
            *FormatSizeForDisplay(SizeBytes), MaxMessageSizeBytes / 1024);
    }
    return GetValidationErrorMessage(SizeBytes);
}

bool UMessageSizeValidator::ValidateMessageSize(const FString& Message)
{
    return IsMessageSizeValid(CalculateUTF8ByteSize(Message));
}

FOddSocketsMessageSizeInfo UMessageSizeValidator::GetMessageSizeInfo(const FString& Message)
{
    const int32 SizeBytes = CalculateUTF8ByteSize(Message);
    const bool bValid = IsMessageSizeValid(SizeBytes);
    return FOddSocketsMessageSizeInfo(SizeBytes, bValid, CreateValidationMessage(SizeBytes, bValid));
}

FOddSocketsBulkValidationResult UMessageSizeValidator::ValidateBulkMessages(const TArray<FString>& Messages)
{
    FOddSocketsBulkValidationResult Result;
    for (const FString& Message : Messages)
    {
        FOddSocketsMessageSizeInfo Info = GetMessageSizeInfo(Message);
        Result.TotalSizeBytes += Info.SizeBytes;
        if (Info.bIsValid) Result.ValidCount++; else Result.InvalidCount++;
        Result.Results.Add(Info);
    }
    Result.TotalSizeKB = Result.TotalSizeBytes / 1024.0f;
    Result.bAllValid = Result.InvalidCount == 0;
    return Result;
}

FOddSocketsBulkValidationResult UMessageSizeValidator::ValidateBulkMessageStructs(const TArray<FOddSocketsBulkMessage>& Messages)
{
    TArray<FString> Payloads;
    for (const FOddSocketsBulkMessage& Msg : Messages)
    {
        Payloads.Add(Msg.Message);
    }
    return ValidateBulkMessages(Payloads);
}

int32 UMessageSizeValidator::CalculateMessageSizeBytes(const FString& Message)
{
    return CalculateUTF8ByteSize(Message);
}

float UMessageSizeValidator::CalculateMessageSizeKB(const FString& Message)
{
    return CalculateUTF8ByteSize(Message) / 1024.0f;
}

int32 UMessageSizeValidator::GetMaxMessageSizeBytes()
{
    return MaxMessageSizeBytes;
}

int32 UMessageSizeValidator::GetMaxMessageSizeKB()
{
    return MaxMessageSizeBytes / 1024;
}

FString UMessageSizeValidator::FormatSizeForDisplay(int32 SizeBytes)
{
    if (SizeBytes < 1024)
    {
        return FString::Printf(TEXT("%d B"), SizeBytes);
    }
    return FString::Printf(TEXT("%.1f KB"), SizeBytes / 1024.0f);
}

FString UMessageSizeValidator::GetValidationErrorMessage(int32 MessageSizeBytes)
{
    return FString::Printf(
        TEXT("Message size %s exceeds the %dKB platform limit by %s"),
        *FormatSizeForDisplay(MessageSizeBytes),
        MaxMessageSizeBytes / 1024,
        *FormatSizeForDisplay(MessageSizeBytes - MaxMessageSizeBytes));
}

bool UMessageSizeValidator::IsMessageSizeValid(int32 SizeBytes)
{
    return SizeBytes >= 0 && SizeBytes <= MaxMessageSizeBytes;
}

bool UMessageSizeValidator::ValidateAndLogMessage(const FString& Message, const FString& Context)
{
    const FOddSocketsMessageSizeInfo Info = GetMessageSizeInfo(Message);
    if (!Info.bIsValid)
    {
        UE_LOG(LogTemp, Warning, TEXT("[OddSockets]%s%s %s"),
            Context.IsEmpty() ? TEXT("") : TEXT(" "),
            *Context, *Info.ValidationMessage);
    }
    return Info.bIsValid;
}

FString UMessageSizeValidator::TruncateMessageToLimit(const FString& Message)
{
    if (ValidateMessageSize(Message))
    {
        return Message;
    }
    // Trim TCHARs until the UTF-8 encoding fits: byte-slicing UTF-8 could cut a
    // code point in half and produce an invalid string.
    FString Truncated = Message;
    while (!Truncated.IsEmpty() && CalculateUTF8ByteSize(Truncated) > MaxMessageSizeBytes)
    {
        // Oversize ratio gives a near-target first cut; the loop settles the rest.
        const int32 Excess = CalculateUTF8ByteSize(Truncated) - MaxMessageSizeBytes;
        const int32 Chop = FMath::Max(1, Excess / 4); // worst case 4 bytes per TCHAR
        Truncated.LeftChopInline(FMath::Min(Chop, Truncated.Len()));
    }
    return Truncated;
}
