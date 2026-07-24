// OddSockets Unreal Engine SDK - Enhanced Features Example

#include "EnhancedFeaturesExample.h"
#include "Engine/World.h"

AEnhancedFeaturesExample::AEnhancedFeaturesExample()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AEnhancedFeaturesExample::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Log, TEXT("OddSockets UE SDK - Enhanced Features Example"));

    // 1. Spawn the client actor and configure it.
    Client = GetWorld()->SpawnActor<AOddSocketsClient>();
    FOddSocketsConfig Config;
    Config.ApiKey = TEXT("your_api_key_here");
    Config.UserId = TEXT("user_123");
    Config.bAutoConnect = false;
    Client->Initialize(Config);

    // 2. Create the enhanced helper, bound to the client.
    Enhanced = NewObject<UOddSocketsEnhancedFeatures>(this);
    Enhanced->Initialize(Client);

    // 3. Receive enhanced broadcasts (user_typing, reaction_added, ...).
    Client->OnEnhancedEvent.AddDynamic(this, &AEnhancedFeaturesExample::HandleEnhancedEvent);
    Client->OnConnected.AddDynamic(this, &AEnhancedFeaturesExample::HandleConnected);

    // 4. Connect. Enhanced events are sent once we are connected and subscribed.
    Client->ConnectAsync();
}

void AEnhancedFeaturesExample::HandleConnected()
{
    UE_LOG(LogTemp, Log, TEXT("Connected. Subscribing to 'general'..."));

    Channel = Client->GetChannel(TEXT("general"));
    Channel->OnMessage.AddDynamic(this, &AEnhancedFeaturesExample::HandleChannelMessage);
    Channel->SubscribeAsync();

    SendEnhancedEvents();
}

void AEnhancedFeaturesExample::SendEnhancedEvents()
{
    // Typing indicators. Peers receive "user_typing" / "user_stopped_typing".
    Enhanced->StartTyping(TEXT("user_123"), TEXT("general"));
    Enhanced->StopTyping(TEXT("user_123"), TEXT("general"));

    // Reactions. Peers receive "reaction_added" / "reaction_removed".
    Enhanced->AddReaction(TEXT("msg_123"), TEXT("general"), TEXT("thumbsup"), TEXT("user_123"), TEXT("Test User"));
    Enhanced->RemoveReaction(TEXT("msg_123"), TEXT("general"), TEXT("thumbsup"), TEXT("user_123"));

    UE_LOG(LogTemp, Log, TEXT("Sent typing + reaction enhanced events."));
}

void AEnhancedFeaturesExample::HandleChannelMessage(const FOddSocketsChannelMessageData& Data)
{
    UE_LOG(LogTemp, Log, TEXT("[%s] %s"), *Data.Channel, *Data.Message);
}

void AEnhancedFeaturesExample::HandleEnhancedEvent(const FString& EventName, const FString& JsonPayload)
{
    // One handler receives every enhanced broadcast; branch on the event name.
    UE_LOG(LogTemp, Log, TEXT("Enhanced event '%s': %s"), *EventName, *JsonPayload);
}

void AEnhancedFeaturesExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Client)
    {
        Client->Disconnect();
    }
    Super::EndPlay(EndPlayReason);
}
