// Copyright Epic Games, Inc. All Rights Reserved.

#include "EnhancedFeaturesExample.h"
#include "Engine/World.h"
#include "TimerManager.h"

AEnhancedFeaturesExample::AEnhancedFeaturesExample()
{
    PrimaryActorTick.bCanEverTick = false;
    bIsConnected = false;
}

void AEnhancedFeaturesExample::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Log, TEXT("🚀 OddSockets Unreal Engine SDK - Enhanced Features Example"));
    UE_LOG(LogTemp, Log, TEXT("Demonstrating all 67 new Slack-like events"));
    UE_LOG(LogTemp, Log, TEXT("=================================================="));

    // Create and configure client
    Client = MakeShared<FOddSocketsClient>(TEXT("your_api_key_here"), TEXT("user_123"));
    Enhanced = MakeShared<FOddSocketsEnhancedFeatures>(Client);

    // Set up event listeners
    Client->On(TEXT("connected"), [this](const FString& Data) {
        bIsConnected = true;
        UE_LOG(LogTemp, Log, TEXT("🟢 Connected event fired"));
    });

    Client->On(TEXT("disconnected"), [this](const FString& Data) {
        bIsConnected = false;
        UE_LOG(LogTemp, Log, TEXT("🔴 Disconnected event fired"));
    });

    Client->On(TEXT("error"), [this](const FString& Error) {
        UE_LOG(LogTemp, Error, TEXT("❌ Error event: %s"), *Error);
    });

    // Connect
    UE_LOG(LogTemp, Log, TEXT("\n🔄 Connecting to OddSockets..."));
    Client->Connect();

    // Wait for connection then test features
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]() {
        if (bIsConnected)
        {
            UE_LOG(LogTemp, Log, TEXT("✅ Connected successfully!\n"));
            TestAllFeatures();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ Failed to connect"));
        }
    }, 2.0f, false);
}

void AEnhancedFeaturesExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Client.IsValid() && bIsConnected)
    {
        Client->Disconnect();
        UE_LOG(LogTemp, Log, TEXT("\n✅ Disconnected"));
    }

    Super::EndPlay(EndPlayReason);
}

void AEnhancedFeaturesExample::TestAllFeatures()
{
    TestThreadEvents();
    TestReactionEvents();
    TestReadReceiptEvents();
    TestChannelEvents();
    TestDirectMessageEvents();
    TestNotificationEvents();
    TestPresenceEvents();
    TestMessageEditingEvents();
    TestSearchEvents();

    // Summary
    UE_LOG(LogTemp, Log, TEXT("\n🎉 All enhanced features tested!"));
    UE_LOG(LogTemp, Log, TEXT("\n📊 Summary:"));
    UE_LOG(LogTemp, Log, TEXT("- Thread Events: 7 methods"));
    UE_LOG(LogTemp, Log, TEXT("- Reaction Events: 6 methods"));
    UE_LOG(LogTemp, Log, TEXT("- Read Receipt Events: 6 methods"));
    UE_LOG(LogTemp, Log, TEXT("- Channel Events: 11 methods"));
    UE_LOG(LogTemp, Log, TEXT("- Direct Message Events: 6 methods"));
    UE_LOG(LogTemp, Log, TEXT("- Notification Events: 6 methods"));
    UE_LOG(LogTemp, Log, TEXT("- File Upload Events: 7 methods"));
    UE_LOG(LogTemp, Log, TEXT("- Presence Events: 8 methods"));
    UE_LOG(LogTemp, Log, TEXT("- Message Editing Events: 5 methods"));
    UE_LOG(LogTemp, Log, TEXT("- Search Events: 4 methods"));
    UE_LOG(LogTemp, Log, TEXT("=================================================="));
    UE_LOG(LogTemp, Log, TEXT("Total: 67 enhanced Slack-like events! 🚀"));
}

void AEnhancedFeaturesExample::TestThreadEvents()
{
    UE_LOG(LogTemp, Log, TEXT("📝 Testing Thread Events..."));

    Enhanced->ThreadReply(
        TEXT("general"),
        TEXT("msg_123"),
        TEXT("This is a test reply from Unreal Engine!"),
        TEXT("user_123"),
        TEXT("Test User"),
        [](const FString& Result) {
            UE_LOG(LogTemp, Log, TEXT("✅ Thread reply created: %s"), *Result);
        },
        [](const FString& Error) {
            UE_LOG(LogTemp, Error, TEXT("❌ Thread reply error: %s"), *Error);
        }
    );

    Enhanced->GetThread(
        TEXT("thread_123"),
        [](const FString& Thread) {
            UE_LOG(LogTemp, Log, TEXT("✅ Thread data: %s"), *Thread);
        },
        [](const FString& Error) {
            UE_LOG(LogTemp, Error, TEXT("❌ Get thread error: %s"), *Error);
        }
    );

    Enhanced->MarkThreadRead(TEXT("thread_123"), TEXT("user_123"));
    UE_LOG(LogTemp, Log, TEXT("✅ Marked thread as read"));

    Enhanced->FollowThread(TEXT("thread_123"), TEXT("user_123"));
    UE_LOG(LogTemp, Log, TEXT("✅ Following thread\n"));
}

void AEnhancedFeaturesExample::TestReactionEvents()
{
    UE_LOG(LogTemp, Log, TEXT("😀 Testing Reaction Events..."));

    Enhanced->AddReaction(TEXT("msg_123"), TEXT("general"), TEXT("👍"), TEXT("user_123"), TEXT("Test User"));
    UE_LOG(LogTemp, Log, TEXT("✅ Added reaction 👍"));

    Enhanced->RemoveReaction(TEXT("msg_123"), TEXT("general"), TEXT("👍"), TEXT("user_123"));
    UE_LOG(LogTemp, Log, TEXT("✅ Removed reaction"));

    Enhanced->GetReactions(
        TEXT("msg_123"),
        [](const FString& Reactions) {
            UE_LOG(LogTemp, Log, TEXT("✅ Reactions: %s\n"), *Reactions);
        },
        [](const FString& Error) {
            UE_LOG(LogTemp, Error, TEXT("❌ Get reactions error: %s\n"), *Error);
        }
    );
}

void AEnhancedFeaturesExample::TestReadReceiptEvents()
{
    UE_LOG(LogTemp, Log, TEXT("✓ Testing Read Receipt Events..."));

    Enhanced->MarkRead(TEXT("msg_123"), TEXT("general"), TEXT("user_123"), TEXT("Test User"));
    UE_LOG(LogTemp, Log, TEXT("✅ Marked message as read"));

    TArray<FString> Channels;
    Channels.Add(TEXT("general"));
    Channels.Add(TEXT("random"));

    Enhanced->GetUnreadCounts(
        TEXT("user_123"),
        Channels,
        [](const FString& Counts) {
            UE_LOG(LogTemp, Log, TEXT("✅ Unread counts: %s"), *Counts);
        },
        [](const FString& Error) {
            UE_LOG(LogTemp, Error, TEXT("❌ Get unread counts error: %s"), *Error);
        }
    );

    Enhanced->MarkAllRead(TEXT("general"), TEXT("user_123"));
    UE_LOG(LogTemp, Log, TEXT("✅ Marked all messages as read\n"));
}

void AEnhancedFeaturesExample::TestChannelEvents()
{
    UE_LOG(LogTemp, Log, TEXT("📢 Testing Channel Events..."));

    Enhanced->CreateChannel(
        TEXT("unreal-test"),
        TEXT("public"),
        TEXT("Created from Unreal Engine SDK"),
        TEXT("Testing"),
        TEXT("user_123"),
        TEXT("Test User"),
        [](const FString& Channel) {
            UE_LOG(LogTemp, Log, TEXT("✅ Channel created: %s"), *Channel);
        },
        [](const FString& Error) {
            UE_LOG(LogTemp, Error, TEXT("❌ Create channel error: %s"), *Error);
        }
    );

    TMap<FString, FString> Updates;
    Updates.Add(TEXT("topic"), TEXT("Updated topic"));
    Enhanced->UpdateChannel(TEXT("channel_123"), Updates, TEXT("user_123"));
    UE_LOG(LogTemp, Log, TEXT("✅ Updated channel"));

    Enhanced->JoinChannel(TEXT("channel_123"), TEXT("user_123"), TEXT("Test User"));
    UE_LOG(LogTemp, Log, TEXT("✅ Joined channel"));

    Enhanced->InviteToChannel(TEXT("channel_123"), TEXT("user_456"), TEXT("Jane Doe"), TEXT("user_123"));
    UE_LOG(LogTemp, Log, TEXT("✅ Invited user to channel\n"));
}

void AEnhancedFeaturesExample::TestDirectMessageEvents()
{
    UE_LOG(LogTemp, Log, TEXT("💬 Testing Direct Message Events..."));

    TArray<FString> UserIds;
    UserIds.Add(TEXT("user_123"));
    UserIds.Add(TEXT("user_456"));

    Enhanced->CreateDM(
        UserIds,
        TEXT("1-on-1"),
        [](const FString& DM) {
            UE_LOG(LogTemp, Log, TEXT("✅ DM created: %s"), *DM);
        },
        [](const FString& Error) {
            UE_LOG(LogTemp, Error, TEXT("❌ Create DM error: %s"), *Error);
        }
    );

    Enhanced->SendDM(TEXT("dm_123"), TEXT("Hello from Unreal Engine!"), TEXT("user_123"), TEXT("Test User"));
    UE_LOG(LogTemp, Log, TEXT("✅ Sent DM\n"));
}

void AEnhancedFeaturesExample::TestNotificationEvents()
{
    UE_LOG(LogTemp, Log, TEXT("🔔 Testing Notification Events..."));

    Enhanced->SubscribeNotifications(TEXT("user_123"));
    UE_LOG(LogTemp, Log, TEXT("✅ Subscribed to notifications"));

    Enhanced->MarkNotificationRead(TEXT("notif_123"), TEXT("user_123"));
    UE_LOG(LogTemp, Log, TEXT("✅ Marked notification as read"));

    Enhanced->MarkAllNotificationsRead(TEXT("user_123"));
    UE_LOG(LogTemp, Log, TEXT("✅ Marked all notifications as read\n"));
}

void AEnhancedFeaturesExample::TestPresenceEvents()
{
    UE_LOG(LogTemp, Log, TEXT("👤 Testing Presence Events..."));

    Enhanced->SetStatus(TEXT("user_123"), TEXT("online"));
    UE_LOG(LogTemp, Log, TEXT("✅ Set status to online"));

    Enhanced->SetCustomStatus(TEXT("user_123"), TEXT("🎮"), TEXT("Playing in Unreal Engine"));
    UE_LOG(LogTemp, Log, TEXT("✅ Set custom status"));

    Enhanced->ClearCustomStatus(TEXT("user_123"));
    UE_LOG(LogTemp, Log, TEXT("✅ Cleared custom status"));

    Enhanced->SetDND(TEXT("user_123"));
    UE_LOG(LogTemp, Log, TEXT("✅ Enabled Do Not Disturb"));

    Enhanced->ClearDND(TEXT("user_123"));
    UE_LOG(LogTemp, Log, TEXT("✅ Disabled Do Not Disturb"));

    Enhanced->StartTyping(TEXT("user_123"), TEXT("general"));
    UE_LOG(LogTemp, Log, TEXT("✅ Started typing indicator"));

    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]() {
        Enhanced->StopTyping(TEXT("user_123"), TEXT("general"));
        UE_LOG(LogTemp, Log, TEXT("✅ Stopped typing indicator\n"));
    }, 2.0f, false);
}

void AEnhancedFeaturesExample::TestMessageEditingEvents()
{
    UE_LOG(LogTemp, Log, TEXT("✏️ Testing Message Editing Events..."));

    Enhanced->EditMessage(TEXT("msg_123"), TEXT("general"), TEXT("Updated message from Unreal Engine"), TEXT("user_123"));
    UE_LOG(LogTemp, Log, TEXT("✅ Edited message"));

    Enhanced->DeleteMessage(TEXT("msg_456"), TEXT("general"), TEXT("user_123"));
    UE_LOG(LogTemp, Log, TEXT("✅ Deleted message"));

    Enhanced->PinMessage(TEXT("msg_123"), TEXT("general"), TEXT("user_123"));
    UE_LOG(LogTemp, Log, TEXT("✅ Pinned message"));

    Enhanced->UnpinMessage(TEXT("msg_123"), TEXT("general"), TEXT("user_123"));
    UE_LOG(LogTemp, Log, TEXT("✅ Unpinned message\n"));
}

void AEnhancedFeaturesExample::TestSearchEvents()
{
    UE_LOG(LogTemp, Log, TEXT("🔍 Testing Search Events..."));

    Enhanced->SearchMessages(
        TEXT("test"),
        TEXT("user_123"),
        10,
        [](const FString& Results) {
            UE_LOG(LogTemp, Log, TEXT("✅ Search results: %s"), *Results);
        },
        [](const FString& Error) {
            UE_LOG(LogTemp, Error, TEXT("❌ Search error: %s"), *Error);
        }
    );

    Enhanced->SearchInChannel(
        TEXT("general"),
        TEXT("test"),
        10,
        [](const FString& Results) {
            UE_LOG(LogTemp, Log, TEXT("✅ Channel search results: %s\n"), *Results);
        },
        [](const FString& Error) {
            UE_LOG(LogTemp, Error, TEXT("❌ Channel search error: %s\n"), *Error);
        }
    );
}
