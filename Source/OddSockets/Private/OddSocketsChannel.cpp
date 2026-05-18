/**
 * OddSockets UE SDK - Channel Implementation
 */

#include "OddSocketsChannel.h"
#include "OddSocketsClient.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

UOddSocketsChannel::UOddSocketsChannel()
    : bSubscribed(false)
    , bSubscribing(false)
    , MaxHistorySize(100)
{
}

void UOddSocketsChannel::Initialize(const FString& InChannelName, AOddSocketsClient* InClient)
{
    ChannelName = InChannelName;
    Client = InClient;
}

void UOddSocketsChannel::SubscribeAsync(const FOddSocketsSubscriptionOptions& InOptions)
{
    if (bSubscribed || bSubscribing || !IsClientConnected()) return;
    bSubscribing = true;
    Options = InOptions;

    TSharedPtr<FJsonObject> Msg = MakeShareable(new FJsonObject);
    Msg->SetStringField(TEXT("channel"), ChannelName);
    TSharedPtr<FJsonObject> Opts = MakeShareable(new FJsonObject);
    Opts->SetNumberField(TEXT("maxHistory"), Options.MaxHistory);
    Opts->SetBoolField(TEXT("retainHistory"), Options.bRetainHistory);
    Opts->SetBoolField(TEXT("enablePresence"), Options.bEnablePresence);
    Msg->SetObjectField(TEXT("options"), Opts);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Msg.ToSharedRef(), Writer);

    if (TSharedPtr<IWebSocket> WS = GetWebSocket())
        WS->Send(FString::Printf(TEXT("42[\"subscribe\",%s]"), *Output));
}

void UOddSocketsChannel::UnsubscribeAsync()
{
    if (!bSubscribed || !IsClientConnected()) return;
    if (TSharedPtr<IWebSocket> WS = GetWebSocket())
        WS->Send(FString::Printf(TEXT("42[\"unsubscribe\",{\"channel\":\"%s\"}]"), *ChannelName));
}

void UOddSocketsChannel::PublishAsync(const FString& Message, const FOddSocketsPublishOptions& PubOptions)
{
    if (!IsClientConnected()) return;
    ValidateMessageSize(Message);

    TSharedPtr<FJsonObject> Msg = MakeShareable(new FJsonObject);
    Msg->SetStringField(TEXT("channel"), ChannelName);
    Msg->SetStringField(TEXT("message"), Message);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Msg.ToSharedRef(), Writer);

    if (TSharedPtr<IWebSocket> WS = GetWebSocket())
        WS->Send(FString::Printf(TEXT("42[\"publish\",%s]"), *Output));
}

void UOddSocketsChannel::GetHistoryAsync(const FOddSocketsHistoryOptions& HistOptions)
{
    if (!IsClientConnected()) return;
    if (TSharedPtr<IWebSocket> WS = GetWebSocket())
        WS->Send(FString::Printf(TEXT("42[\"get_history\",{\"channel\":\"%s\",\"count\":%d}]"), *ChannelName, HistOptions.Count));
}

void UOddSocketsChannel::GetPresenceAsync()
{
    if (!IsClientConnected()) return;
    if (TSharedPtr<IWebSocket> WS = GetWebSocket())
        WS->Send(FString::Printf(TEXT("42[\"get_presence\",{\"channel\":\"%s\"}]"), *ChannelName));
}

void UOddSocketsChannel::UpdateStateAsync(const TMap<FString, FString>& State)
{
    if (!IsClientConnected()) return;
    TSharedPtr<FJsonObject> StateObj = MakeShareable(new FJsonObject);
    for (const auto& KV : State) StateObj->SetStringField(KV.Key, KV.Value);
    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(StateObj.ToSharedRef(), Writer);
    if (TSharedPtr<IWebSocket> WS = GetWebSocket())
        WS->Send(FString::Printf(TEXT("42[\"update_state\",%s]"), *Output));
}

void UOddSocketsChannel::HandleMessage(const FOddSocketsChannelMessageData& Data) { MessageHistory.Add(Data); if (MessageHistory.Num() > MaxHistorySize) MessageHistory.RemoveAt(0); OnMessage.Broadcast(Data); }
void UOddSocketsChannel::HandleSubscribed(const FOddSocketsChannelSubscriptionData& Data) { bSubscribed = true; bSubscribing = false; OnSubscribed.Broadcast(Data); }
void UOddSocketsChannel::HandleUnsubscribed(const FOddSocketsChannelSubscriptionData& Data) { bSubscribed = false; OnUnsubscribed.Broadcast(Data); }
void UOddSocketsChannel::HandlePublished(const FOddSocketsChannelPublishData& Data) { OnPublished.Broadcast(Data); }
void UOddSocketsChannel::HandlePresence(const FOddSocketsChannelPresenceData& Data) { OnPresence.Broadcast(Data); }
void UOddSocketsChannel::HandlePresenceChange(const FOddSocketsChannelPresenceChangeData& Data) { OnPresenceChange.Broadcast(Data); }
void UOddSocketsChannel::HandleHistory(const FOddSocketsChannelHistoryData& Data) { OnHistory.Broadcast(Data); }

void UOddSocketsChannel::ValidateMessageSize(const FString& Message)
{
    if (Message.Len() > 32768)
        UE_LOG(LogTemp, Error, TEXT("OddSockets: Message exceeds 32KB limit (%d bytes)"), Message.Len());
}

bool UOddSocketsChannel::IsClientConnected() const { return Client && Client->IsConnected(); }
TSharedPtr<IWebSocket> UOddSocketsChannel::GetWebSocket() const { return Client ? Client->GetWebSocket() : nullptr; }
