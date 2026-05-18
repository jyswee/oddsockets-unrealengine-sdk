#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Engine.h"
#include "OddSocketsTypes.h"
#include "MessageSizeValidator.generated.h"

/**
 * Message size information structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsMessageSizeInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Message Size")
    int32 SizeBytes;

    UPROPERTY(BlueprintReadOnly, Category = "Message Size")
    float SizeKB;

    UPROPERTY(BlueprintReadOnly, Category = "Message Size")
    bool bIsValid;

    UPROPERTY(BlueprintReadOnly, Category = "Message Size")
    FString ValidationMessage;

    FOddSocketsMessageSizeInfo()
    {
        SizeBytes = 0;
        SizeKB = 0.0f;
        bIsValid = false;
        ValidationMessage = TEXT("");
    }

    FOddSocketsMessageSizeInfo(int32 InSizeBytes, bool InIsValid, const FString& InValidationMessage = TEXT(""))
        : SizeBytes(InSizeBytes)
        , SizeKB(InSizeBytes / 1024.0f)
        , bIsValid(InIsValid)
        , ValidationMessage(InValidationMessage)
    {
    }
};

/**
 * Bulk message validation result structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsBulkValidationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Bulk Validation")
    bool bAllValid;

    UPROPERTY(BlueprintReadOnly, Category = "Bulk Validation")
    int32 ValidCount;

    UPROPERTY(BlueprintReadOnly, Category = "Bulk Validation")
    int32 InvalidCount;

    UPROPERTY(BlueprintReadOnly, Category = "Bulk Validation")
    int32 TotalSizeBytes;

    UPROPERTY(BlueprintReadOnly, Category = "Bulk Validation")
    float TotalSizeKB;

    UPROPERTY(BlueprintReadOnly, Category = "Bulk Validation")
    TArray<FOddSocketsMessageSizeInfo> Results;

    FOddSocketsBulkValidationResult()
    {
        bAllValid = false;
        ValidCount = 0;
        InvalidCount = 0;
        TotalSizeBytes = 0;
        TotalSizeKB = 0.0f;
    }
};

/**
 * Message Size Validator
 * 
 * Provides validation for message sizes according to OddSockets limits.
 * Implements industry standard 32KB message size limit matching PubNub
 * and other real-time messaging platforms.
 */
UCLASS(BlueprintType, Blueprintable)
class ODDSOCKETS_API UMessageSizeValidator : public UObject
{
    GENERATED_BODY()

public:
    UMessageSizeValidator();

    // Static validation methods
    UFUNCTION(BlueprintCallable, Category = "OddSockets Message Validation", CallInEditor = true)
    static bool ValidateMessageSize(const FString& Message);

    UFUNCTION(BlueprintCallable, Category = "OddSockets Message Validation", CallInEditor = true)
    static FOddSocketsMessageSizeInfo GetMessageSizeInfo(const FString& Message);

    UFUNCTION(BlueprintCallable, Category = "OddSockets Message Validation", CallInEditor = true)
    static FOddSocketsBulkValidationResult ValidateBulkMessages(const TArray<FString>& Messages);

    UFUNCTION(BlueprintCallable, Category = "OddSockets Message Validation", CallInEditor = true)
    static FOddSocketsBulkValidationResult ValidateBulkMessageStructs(const TArray<FOddSocketsBulkMessage>& Messages);

    // Size calculation methods
    UFUNCTION(BlueprintPure, Category = "OddSockets Message Validation", CallInEditor = true)
    static int32 CalculateMessageSizeBytes(const FString& Message);

    UFUNCTION(BlueprintPure, Category = "OddSockets Message Validation", CallInEditor = true)
    static float CalculateMessageSizeKB(const FString& Message);

    // Limit information
    UFUNCTION(BlueprintPure, Category = "OddSockets Message Validation", CallInEditor = true)
    static int32 GetMaxMessageSizeBytes();

    UFUNCTION(BlueprintPure, Category = "OddSockets Message Validation", CallInEditor = true)
    static int32 GetMaxMessageSizeKB();

    // Utility methods
    UFUNCTION(BlueprintCallable, Category = "OddSockets Message Validation", CallInEditor = true)
    static FString FormatSizeForDisplay(int32 SizeBytes);

    UFUNCTION(BlueprintCallable, Category = "OddSockets Message Validation", CallInEditor = true)
    static FString GetValidationErrorMessage(int32 MessageSizeBytes);

    UFUNCTION(BlueprintPure, Category = "OddSockets Message Validation", CallInEditor = true)
    static bool IsMessageSizeValid(int32 SizeBytes);

    // Advanced validation
    UFUNCTION(BlueprintCallable, Category = "OddSockets Message Validation", CallInEditor = true)
    static bool ValidateAndLogMessage(const FString& Message, const FString& Context = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "OddSockets Message Validation", CallInEditor = true)
    static FString TruncateMessageToLimit(const FString& Message);

private:
    // Private helper methods
    static int32 CalculateUTF8ByteSize(const FString& Message);
    static FString CreateValidationMessage(int32 SizeBytes, bool bIsValid);
};
