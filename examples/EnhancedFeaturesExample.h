// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OddSocketsClient.h"
#include "OddSocketsEnhancedFeatures.h"
#include "EnhancedFeaturesExample.generated.h"

/**
 * OddSockets Unreal Engine SDK - Enhanced Features Example
 * Demonstrates all 67 new Slack-like events with Unreal Engine patterns
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
    TSharedPtr<FOddSocketsClient> Client;
    TSharedPtr<FOddSocketsEnhancedFeatures> Enhanced;
    bool bIsConnected;

    void TestAllFeatures();
    void TestThreadEvents();
    void TestReactionEvents();
    void TestReadReceiptEvents();
    void TestChannelEvents();
    void TestDirectMessageEvents();
    void TestNotificationEvents();
    void TestPresenceEvents();
    void TestMessageEditingEvents();
    void TestSearchEvents();
};
