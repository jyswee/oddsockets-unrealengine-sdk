/**
 * OddSockets UE SDK - Enhanced Features Implementation
 *
 * Send-path for enhanced (Slack-like) events. Each method builds a JSON payload
 * and emits it over the client's Socket.IO connection using the same
 * 42["event",{...}] framing as the core Channel methods. Paired broadcasts are
 * delivered by AOddSocketsClient::RouteEvent to OnEnhancedEvent / On().
 */

#include "OddSocketsEnhancedFeatures.h"
#include "OddSocketsClient.h"
#include "IWebSocket.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void UOddSocketsEnhancedFeatures::Initialize(AOddSocketsClient* InClient)
{
    Client = InClient;
}

void UOddSocketsEnhancedFeatures::Emit(const FString& EventName, const TSharedRef<FJsonObject>& Payload)
{
    if (!Client) return;

    TSharedPtr<IWebSocket> WS = Client->GetWebSocket();
    if (!WS.IsValid() || !WS->IsConnected()) return;

    FString PayloadJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadJson);
    FJsonSerializer::Serialize(Payload, Writer);

    WS->Send(FString::Printf(TEXT("42[\"%s\",%s]"), *EventName, *PayloadJson));
}

// ==================== TYPING INDICATORS ====================

void UOddSocketsEnhancedFeatures::StartTyping(const FString& UserId, const FString& Channel)
{
    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("userId"), UserId);
    Payload->SetStringField(TEXT("channel"), Channel);
    Emit(TEXT("start_typing"), Payload);
}

void UOddSocketsEnhancedFeatures::StopTyping(const FString& UserId, const FString& Channel)
{
    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("userId"), UserId);
    Payload->SetStringField(TEXT("channel"), Channel);
    Emit(TEXT("stop_typing"), Payload);
}

// ==================== REACTIONS ====================

void UOddSocketsEnhancedFeatures::AddReaction(const FString& MessageId, const FString& Channel,
                                              const FString& Emoji, const FString& UserId,
                                              const FString& UserName)
{
    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("messageId"), MessageId);
    Payload->SetStringField(TEXT("channel"), Channel);
    Payload->SetStringField(TEXT("emoji"), Emoji);
    Payload->SetStringField(TEXT("userId"), UserId);
    Payload->SetStringField(TEXT("userName"), UserName);
    Emit(TEXT("add_reaction"), Payload);
}

void UOddSocketsEnhancedFeatures::RemoveReaction(const FString& MessageId, const FString& Channel,
                                                 const FString& Emoji, const FString& UserId)
{
    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("messageId"), MessageId);
    Payload->SetStringField(TEXT("channel"), Channel);
    Payload->SetStringField(TEXT("emoji"), Emoji);
    Payload->SetStringField(TEXT("userId"), UserId);
    Emit(TEXT("remove_reaction"), Payload);
}
