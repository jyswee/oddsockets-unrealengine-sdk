#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Engine.h"
#include "Containers/Map.h"
#include "Containers/Array.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "OddSocketsTypes.h"
#include "OddSocketsChannel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelMessage, const FOddSocketsChannelMessageData&, MessageData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelSubscribed, const FOddSocketsChannelSubscriptionData&, SubscriptionData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelUnsubscribed, const FOddSocketsChannelSubscriptionData&, SubscriptionData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelPublished, const FOddSocketsChannelPublishData&, PublishData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelPresence, const FOddSocketsChannelPresenceData&, PresenceData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelPresenceChange, const FOddSocketsChannelPresenceChangeData&, PresenceChangeData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelHistory, const FOddSocketsChannelHistoryData&, HistoryData);

/**
 * Channel class for pub/sub messaging
 * 
 * Provides methods for subscribing, publishing, and managing presence
 * on a specific channel within the OddSockets platform.
 */
UCLASS(BlueprintType, Blueprintable)
class ODDSOCKETS_API UOddSocketsChannel : public UObject
{
    GENERATED_BODY()

public:
    UOddSocketsChannel();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "OddSockets Channel Events")
    FOnChannelMessage OnMessage;

    UPROPERTY(BlueprintAssignable, Category = "OddSockets Channel Events")
    FOnChannelSubscribed OnSubscribed;

    UPROPERTY(BlueprintAssignable, Category = "OddSockets Channel Events")
    FOnChannelUnsubscribed OnUnsubscribed;

    UPROPERTY(BlueprintAssignable, Category = "OddSockets Channel Events")
    FOnChannelPublished OnPublished;

    UPROPERTY(BlueprintAssignable, Category = "OddSockets Channel Events")
    FOnChannelPresence OnPresence;

    UPROPERTY(BlueprintAssignable, Category = "OddSockets Channel Events")
    FOnChannelPresenceChange OnPresenceChange;

    UPROPERTY(BlueprintAssignable, Category = "OddSockets Channel Events")
    FOnChannelHistory OnHistory;

    // Public Methods
    UFUNCTION(BlueprintCallable, Category = "OddSockets Channel")
    void SubscribeAsync(const FOddSocketsSubscriptionOptions& Options = FOddSocketsSubscriptionOptions());

    UFUNCTION(BlueprintCallable, Category = "OddSockets Channel")
    void UnsubscribeAsync();

    UFUNCTION(BlueprintCallable, Category = "OddSockets Channel")
    void PublishAsync(const FString& Message, const FOddSocketsPublishOptions& Options = FOddSocketsPublishOptions());

    UFUNCTION(BlueprintCallable, Category = "OddSockets Channel")
    void GetHistoryAsync(const FOddSocketsHistoryOptions& Options = FOddSocketsHistoryOptions());

    UFUNCTION(BlueprintCallable, Category = "OddSockets Channel")
    void GetPresenceAsync();

    UFUNCTION(BlueprintCallable, Category = "OddSockets Channel")
    void UpdateStateAsync(const TMap<FString, FString>& State);

    UFUNCTION(BlueprintPure, Category = "OddSockets Channel")
    FString GetChannelName() const { return ChannelName; }

    UFUNCTION(BlueprintPure, Category = "OddSockets Channel")
    bool IsSubscribed() const { return bSubscribed; }

    UFUNCTION(BlueprintPure, Category = "OddSockets Channel")
    TMap<FString, FOddSocketsPresenceUser> GetPresenceMap() const { return PresenceMap; }

    UFUNCTION(BlueprintPure, Category = "OddSockets Channel")
    TArray<FOddSocketsChannelMessageData> GetCachedHistory() const { return MessageHistory; }

    // Internal methods
    void Initialize(const FString& InChannelName, class AOddSocketsClient* InClient);
    void HandleMessage(const FOddSocketsChannelMessageData& Data);
    void HandleSubscribed(const FOddSocketsChannelSubscriptionData& Data);
    void HandleUnsubscribed(const FOddSocketsChannelSubscriptionData& Data);
    void HandlePublished(const FOddSocketsChannelPublishData& Data);
    void HandlePresence(const FOddSocketsChannelPresenceData& Data);
    void HandlePresenceChange(const FOddSocketsChannelPresenceChangeData& Data);
    void HandleHistory(const FOddSocketsChannelHistoryData& Data);

private:
    UPROPERTY()
    FString ChannelName;

    UPROPERTY()
    class AOddSocketsClient* Client;

    UPROPERTY()
    bool bSubscribed;

    UPROPERTY()
    bool bSubscribing;

    UPROPERTY()
    FOddSocketsSubscriptionOptions Options;

    UPROPERTY()
    TMap<FString, FOddSocketsPresenceUser> PresenceMap;

    UPROPERTY()
    TArray<FOddSocketsChannelMessageData> MessageHistory;

    UPROPERTY()
    int32 MaxHistorySize;

    // Private methods
    void ValidateMessageSize(const FString& Message);
    bool IsClientConnected() const;
    TSharedPtr<IWebSocket> GetWebSocket() const;
};
