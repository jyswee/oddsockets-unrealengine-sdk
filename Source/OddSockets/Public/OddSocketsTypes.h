#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "OddSocketsTypes.generated.h"

/**
 * Connection states for the OddSockets client
 */
UENUM(BlueprintType)
enum class EOddSocketsConnectionState : uint8
{
    Disconnected    UMETA(DisplayName = "Disconnected"),
    Connecting      UMETA(DisplayName = "Connecting"),
    Connected       UMETA(DisplayName = "Connected"),
    Reconnecting    UMETA(DisplayName = "Reconnecting"),
    Failed          UMETA(DisplayName = "Failed")
};

/**
 * Log levels for the SDK
 */
UENUM(BlueprintType)
enum class EOddSocketsLogLevel : uint8
{
    None        UMETA(DisplayName = "None"),
    Error       UMETA(DisplayName = "Error"),
    Warning     UMETA(DisplayName = "Warning"),
    Info        UMETA(DisplayName = "Info"),
    Debug       UMETA(DisplayName = "Debug")
};

/**
 * Configuration structure for OddSockets client
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsConfig
{
    GENERATED_BODY()

    /** Your OddSockets API key */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Required Settings")
    FString ApiKey;

    /** User identifier (auto-generated if empty) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optional Settings")
    FString UserId;

    /**
     * Manager endpoint that assigns a worker. Leave empty to use the default.
     * When set it is used verbatim: if it is unreachable the connection fails
     * rather than silently falling back to the default manager.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optional Settings")
    FString ManagerUrl;

    /** Automatically connect on initialization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optional Settings")
    bool bAutoConnect = true;

    /** Maximum reconnection attempts */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection Settings", meta = (ClampMin = "0", ClampMax = "10"))
    int32 ReconnectAttempts = 5;

    /** Connection timeout in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection Settings", meta = (ClampMin = "5", ClampMax = "60"))
    int32 Timeout = 10;

    /** Heartbeat interval in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection Settings", meta = (ClampMin = "10", ClampMax = "300"))
    int32 HeartbeatInterval = 30;

    /** Logging level for SDK operations */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logging")
    EOddSocketsLogLevel LogLevel = EOddSocketsLogLevel::Info;

    FOddSocketsConfig()
    {
        ApiKey = TEXT("");
        UserId = TEXT("");
        bAutoConnect = true;
        ReconnectAttempts = 5;
        Timeout = 10;
        HeartbeatInterval = 30;
        LogLevel = EOddSocketsLogLevel::Info;
    }

    /** Validates the configuration */
    bool IsValid() const
    {
        return !ApiKey.IsEmpty() && Timeout > 0 && ReconnectAttempts >= 0 && HeartbeatInterval >= 0;
    }
};

/**
 * Session information structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsSessionInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    FString SessionId;

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    FString ClientIdentifier;

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    FDateTime CreatedAt;

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    FDateTime LastActivity;

    FOddSocketsSessionInfo()
    {
        SessionId = TEXT("");
        ClientIdentifier = TEXT("");
        CreatedAt = FDateTime::Now();
        LastActivity = FDateTime::Now();
    }
};

/**
 * Worker information structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsWorkerInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Worker")
    FString WorkerId;

    UPROPERTY(BlueprintReadOnly, Category = "Worker")
    FString WorkerUrl;

    FOddSocketsWorkerInfo()
    {
        WorkerId = TEXT("");
        WorkerUrl = TEXT("");
    }

    FOddSocketsWorkerInfo(const FString& InWorkerId, const FString& InWorkerUrl)
        : WorkerId(InWorkerId), WorkerUrl(InWorkerUrl)
    {
    }
};

/**
 * Worker assignment information structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsWorkerAssignmentInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Assignment")
    FString WorkerId;

    UPROPERTY(BlueprintReadOnly, Category = "Assignment")
    FString WorkerUrl;

    UPROPERTY(BlueprintReadOnly, Category = "Assignment")
    FOddSocketsSessionInfo Session;

    UPROPERTY(BlueprintReadOnly, Category = "Assignment")
    FString ClientIdentifier;

    UPROPERTY(BlueprintReadOnly, Category = "Assignment")
    FString ManagerUrl;

    FOddSocketsWorkerAssignmentInfo()
    {
        WorkerId = TEXT("");
        WorkerUrl = TEXT("");
        ClientIdentifier = TEXT("");
        ManagerUrl = TEXT("");
    }
};

/**
 * Reconnection information structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsReconnectInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Reconnect")
    int32 Attempt;

    UPROPERTY(BlueprintReadOnly, Category = "Reconnect")
    int32 MaxAttempts;

    UPROPERTY(BlueprintReadOnly, Category = "Reconnect")
    int32 Delay;

    FOddSocketsReconnectInfo()
    {
        Attempt = 0;
        MaxAttempts = 0;
        Delay = 0;
    }
};

/**
 * Publish options structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsPublishOptions
{
    GENERATED_BODY()

    /** Time to live in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Options")
    int32 Ttl = 0;

    /** Additional message metadata */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Options")
    TMap<FString, FString> Metadata;

    FOddSocketsPublishOptions()
    {
        Ttl = 0;
    }
};

/**
 * Bulk message structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsBulkMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Message")
    FString Channel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Message")
    FString Message;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Message")
    FOddSocketsPublishOptions Options;

    FOddSocketsBulkMessage()
    {
        Channel = TEXT("");
        Message = TEXT("");
    }
};

/**
 * Publish result structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsPublishResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Result")
    bool bSuccess;

    UPROPERTY(BlueprintReadOnly, Category = "Result")
    FString Result;

    UPROPERTY(BlueprintReadOnly, Category = "Result")
    FString Error;

    FOddSocketsPublishResult()
    {
        bSuccess = false;
        Result = TEXT("");
        Error = TEXT("");
    }
};

/**
 * Channel message data structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsChannelMessageData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Message")
    FString Channel;

    UPROPERTY(BlueprintReadOnly, Category = "Message")
    FString Message;

    UPROPERTY(BlueprintReadOnly, Category = "Message")
    FDateTime Timestamp;

    UPROPERTY(BlueprintReadOnly, Category = "Message")
    FString Sender;

    UPROPERTY(BlueprintReadOnly, Category = "Message")
    TMap<FString, FString> Metadata;

    FOddSocketsChannelMessageData()
    {
        Channel = TEXT("");
        Message = TEXT("");
        Timestamp = FDateTime::Now();
        Sender = TEXT("");
    }
};

/**
 * Channel subscription data structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsChannelSubscriptionData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Subscription")
    FString Channel;

    UPROPERTY(BlueprintReadOnly, Category = "Subscription")
    bool bSuccess;

    UPROPERTY(BlueprintReadOnly, Category = "Subscription")
    FString Message;

    FOddSocketsChannelSubscriptionData()
    {
        Channel = TEXT("");
        bSuccess = false;
        Message = TEXT("");
    }
};

/**
 * Channel publish data structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsChannelPublishData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Publish")
    FString Channel;

    UPROPERTY(BlueprintReadOnly, Category = "Publish")
    bool bSuccess;

    UPROPERTY(BlueprintReadOnly, Category = "Publish")
    FString MessageId;

    UPROPERTY(BlueprintReadOnly, Category = "Publish")
    FDateTime Timestamp;

    FOddSocketsChannelPublishData()
    {
        Channel = TEXT("");
        bSuccess = false;
        MessageId = TEXT("");
        Timestamp = FDateTime::Now();
    }
};

/**
 * Presence user structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsPresenceUser
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Presence")
    FString UserId;

    UPROPERTY(BlueprintReadOnly, Category = "Presence")
    TMap<FString, FString> State;

    UPROPERTY(BlueprintReadOnly, Category = "Presence")
    FDateTime JoinedAt;

    FOddSocketsPresenceUser()
    {
        UserId = TEXT("");
        JoinedAt = FDateTime::Now();
    }
};

/**
 * Channel presence data structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsChannelPresenceData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Presence")
    FString Channel;

    UPROPERTY(BlueprintReadOnly, Category = "Presence")
    TArray<FOddSocketsPresenceUser> Occupants;

    UPROPERTY(BlueprintReadOnly, Category = "Presence")
    int32 Count;

    FOddSocketsChannelPresenceData()
    {
        Channel = TEXT("");
        Count = 0;
    }
};

/**
 * Channel presence change data structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsChannelPresenceChangeData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Presence")
    FString Channel;

    UPROPERTY(BlueprintReadOnly, Category = "Presence")
    FString Action; // "join" or "leave"

    UPROPERTY(BlueprintReadOnly, Category = "Presence")
    FOddSocketsPresenceUser User;

    FOddSocketsChannelPresenceChangeData()
    {
        Channel = TEXT("");
        Action = TEXT("");
    }
};

/**
 * Channel history data structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsChannelHistoryData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "History")
    FString Channel;

    UPROPERTY(BlueprintReadOnly, Category = "History")
    TArray<FOddSocketsChannelMessageData> Messages;

    UPROPERTY(BlueprintReadOnly, Category = "History")
    int32 Count;

    FOddSocketsChannelHistoryData()
    {
        Channel = TEXT("");
        Count = 0;
    }
};

/**
 * Subscription options structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsSubscriptionOptions
{
    GENERATED_BODY()

    /** Maximum history messages to retain */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Options")
    int32 MaxHistory = 100;

    /** Whether to retain message history */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Options")
    bool bRetainHistory = true;

    /** Whether to enable presence tracking */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Options")
    bool bEnablePresence = false;

    FOddSocketsSubscriptionOptions()
    {
        MaxHistory = 100;
        bRetainHistory = true;
        bEnablePresence = false;
    }
};

/**
 * History options structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsHistoryOptions
{
    GENERATED_BODY()

    /** Number of messages to retrieve */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Options")
    int32 Count = 50;

    /** Start time */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Options")
    FDateTime Start;

    /** End time */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Options")
    FDateTime End;

    FOddSocketsHistoryOptions()
    {
        Count = 50;
        Start = FDateTime::MinValue();
        End = FDateTime::MinValue();
    }
};

/**
 * Message size limits (32KB - industry standard)
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsMessageSizeLimits
{
    GENERATED_BODY()

    static constexpr int32 MAX_MESSAGE_SIZE = 32768; // 32KB in bytes
    static constexpr int32 MAX_MESSAGE_SIZE_KB = 32;
};
