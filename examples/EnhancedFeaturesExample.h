// OddSockets Unreal Engine SDK - Enhanced Features Example

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OddSocketsClient.h"
#include "OddSocketsChannel.h"
#include "OddSocketsEnhancedFeatures.h"
#include "OddSocketsTypes.h"
#include "EnhancedFeaturesExample.generated.h"

/**
 * OddSockets Unreal Engine SDK - Enhanced Features Example
 *
 * Demonstrates the enhanced (Slack-like) send/receive surface against the real
 * client API: spawn an AOddSocketsClient actor, subscribe to a channel, then
 * send typing indicators and reactions. Incoming enhanced broadcasts arrive on
 * AOddSocketsClient::OnEnhancedEvent (e.g. "user_typing", "reaction_added").
 */
UCLASS()
class ODDSOCKETS_API AEnhancedFeaturesExample : public AActor
{
    GENERATED_BODY()

public:
    AEnhancedFeaturesExample();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    // The client is an Actor, spawned into the world (not a shared pointer).
    UPROPERTY()
    AOddSocketsClient* Client = nullptr;

    // Enhanced helper is a UObject bound to the client.
    UPROPERTY()
    UOddSocketsEnhancedFeatures* Enhanced = nullptr;

    UPROPERTY()
    UOddSocketsChannel* Channel = nullptr;

    // Delegate handlers must be UFUNCTIONs to bind to dynamic multicast delegates.
    UFUNCTION()
    void HandleConnected();

    UFUNCTION()
    void HandleChannelMessage(const FOddSocketsChannelMessageData& Data);

    UFUNCTION()
    void HandleEnhancedEvent(const FString& EventName, const FString& JsonPayload);

    void SendEnhancedEvents();
};
