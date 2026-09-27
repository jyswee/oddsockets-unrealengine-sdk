/**
 * OddSockets UE SDK - Client Implementation
 */

#include "OddSocketsClient.h"
#include "ManagerDiscovery.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "TimerManager.h"

AOddSocketsClient::AOddSocketsClient()
{
    PrimaryActorTick.bCanEverTick = true;
    ConnectionState = EOddSocketsConnectionState::Disconnected;
    ReconnectAttempts = 0;
    ReconnectDelay = 1000;
}

void AOddSocketsClient::BeginPlay()
{
    Super::BeginPlay();
    if (!Config.ApiKey.IsEmpty() && Config.bAutoConnect)
    {
        Initialize(Config);
        ConnectAsync();
    }
}

void AOddSocketsClient::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CleanupResources();
    Super::EndPlay(EndPlayReason);
}

void AOddSocketsClient::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AOddSocketsClient::Initialize(const FOddSocketsConfig& InConfig)
{
    Config = InConfig;
    ClientIdentifier = GenerateClientIdentifier();
}

void AOddSocketsClient::ConnectAsync()
{
    if (ConnectionState == EOddSocketsConnectionState::Connected ||
        ConnectionState == EOddSocketsConnectionState::Connecting) return;

    ConnectionState = EOddSocketsConnectionState::Connecting;
    OnConnecting.Broadcast();
    GetWorkerAssignment();
}

void AOddSocketsClient::Disconnect()
{
    CleanupResources();
    ConnectionState = EOddSocketsConnectionState::Disconnected;
    OnDisconnected.Broadcast(TEXT("Manual disconnect"));
}

UOddSocketsChannel* AOddSocketsClient::GetChannel(const FString& ChannelName)
{
    if (ChannelName.IsEmpty()) return nullptr;
    if (UOddSocketsChannel** Found = Channels.Find(ChannelName)) return *Found;

    UOddSocketsChannel* Ch = NewObject<UOddSocketsChannel>(this);
    Ch->Initialize(ChannelName, this);
    Channels.Add(ChannelName, Ch);
    return Ch;
}

void AOddSocketsClient::PublishBulkAsync(const TArray<FOddSocketsBulkMessage>& Messages)
{
    for (const auto& Msg : Messages)
    {
        if (UOddSocketsChannel* Ch = GetChannel(Msg.Channel))
        {
            Ch->PublishAsync(Msg.Message, Msg.Options);
        }
    }
}

void AOddSocketsClient::GetUsageStats()
{
    // Requires an API key. A client with no key has no owner scope to attribute
    // metrics to, so fail fast (mirrors the JS SDK's token-client guard) rather
    // than sending an unauthenticated request the manager would reject anyway.
    if (Config.ApiKey.IsEmpty())
    {
        FOddSocketsUsageStats Stats;
        Stats.bSuccess = false;
        Stats.Error = TEXT("getUsageStats requires an apiKey (keyless/token clients have no owner scope to query)");
        OnUsageStats.Broadcast(Stats);
        return;
    }

    // Same manager-discovery helper the worker-selection call uses.
    FString DiscoveryError;
    FString ManagerUrl = UManagerDiscovery::GetManagerUrl(Config.ManagerUrl, DiscoveryError);
    if (!DiscoveryError.IsEmpty())
    {
        FOddSocketsUsageStats Stats;
        Stats.bSuccess = false;
        Stats.Error = DiscoveryError;
        OnUsageStats.Broadcast(Stats);
        return;
    }

    FString Url = FString::Printf(TEXT("%s/api/tenant/usage"), *ManagerUrl);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET"));
    Request->SetHeader(TEXT("X-API-Key"), Config.ApiKey);
    Request->SetHeader(TEXT("User-Agent"), TEXT("OddSockets-UE-SDK/1.0.0"));
    Request->OnProcessRequestComplete().BindUObject(this, &AOddSocketsClient::OnUsageStatsResponse);
    Request->ProcessRequest();
}

void AOddSocketsClient::OnUsageStatsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FOddSocketsUsageStats Stats;

    if (!bWasSuccessful || !Response.IsValid() || Response->GetResponseCode() != 200)
    {
        Stats.bSuccess = false;
        Stats.Error = Response.IsValid()
            ? FString::Printf(TEXT("Usage stats request failed: HTTP %d"), Response->GetResponseCode())
            : TEXT("Usage stats request failed: no response");
        OnUsageStats.Broadcast(Stats);
        return;
    }

    TSharedPtr<FJsonObject> Json;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
    if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
    {
        Stats.bSuccess = false;
        Stats.Error = TEXT("Invalid usage stats response JSON");
        OnUsageStats.Broadcast(Stats);
        return;
    }

    // Shape: { ownerScope, tiles: { mau, dau, totalMessages, errorRate }, detail, timestamp }.
    // A tile absent (or JSON null) stays absent — bHas... = false — never coerced
    // to 0, so a not-yet-live metric can never masquerade as real zero activity.
    const TSharedPtr<FJsonObject>* TilesPtr = nullptr;
    if (Json->TryGetObjectField(TEXT("tiles"), TilesPtr) && TilesPtr && TilesPtr->IsValid())
    {
        const TSharedPtr<FJsonObject>& Tiles = *TilesPtr;

        // TryGetNumberField returns false for both an absent key AND a JSON null,
        // so a null tile leaves bHas... = false — exactly the "preserve null,
        // never coerce to 0" contract.
        double NumValue = 0.0;
        if (Tiles->TryGetNumberField(TEXT("mau"), NumValue))
        {
            Stats.Mau = static_cast<int64>(NumValue);
            Stats.bHasMau = true;
        }
        if (Tiles->TryGetNumberField(TEXT("dau"), NumValue))
        {
            Stats.Dau = static_cast<int64>(NumValue);
            Stats.bHasDau = true;
        }
        if (Tiles->TryGetNumberField(TEXT("totalMessages"), NumValue))
        {
            Stats.TotalMessages = static_cast<int64>(NumValue);
            Stats.bHasTotalMessages = true;
        }
        if (Tiles->TryGetNumberField(TEXT("errorRate"), NumValue))
        {
            Stats.ErrorRate = static_cast<float>(NumValue);
            Stats.bHasErrorRate = true;
        }
    }

    Json->TryGetStringField(TEXT("ownerScope"), Stats.OwnerScope);
    Json->TryGetStringField(TEXT("detail"), Stats.Detail);
    Json->TryGetStringField(TEXT("timestamp"), Stats.Timestamp);

    Stats.bSuccess = true;
    OnUsageStats.Broadcast(Stats);
}

FOddSocketsWorkerInfo AOddSocketsClient::GetWorkerInfo() const
{
    FOddSocketsWorkerInfo Info;
    Info.WorkerId = WorkerId;
    Info.WorkerUrl = WorkerUrl;
    return Info;
}

bool AOddSocketsClient::IsConnected() const
{
    return ConnectionState == EOddSocketsConnectionState::Connected;
}

void AOddSocketsClient::GetWorkerAssignment()
{
    // The configured manager is honoured verbatim. A malformed value fails here
    // instead of silently retargeting the default manager, which would make a
    // misconfigured client look healthy.
    FString DiscoveryError;
    FString ManagerUrl = UManagerDiscovery::GetManagerUrl(Config.ManagerUrl, DiscoveryError);
    if (!DiscoveryError.IsEmpty())
    {
        ConnectionState = EOddSocketsConnectionState::Error;
        OnError.Broadcast(DiscoveryError);
        return;
    }

    FString UserId = Config.UserId.IsEmpty() ? ClientIdentifier : Config.UserId;
    FString Url = FString::Printf(
        TEXT("%s/api/cluster/select-worker?apiKey=%s&userId=%s&clientIdentifier=%s"),
        *ManagerUrl, *Config.ApiKey, *UserId, *ClientIdentifier);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET"));
    Request->SetHeader(TEXT("User-Agent"), TEXT("OddSockets-UE-SDK/1.0.0"));
    Request->OnProcessRequestComplete().BindUObject(this, &AOddSocketsClient::OnWorkerAssignmentResponse);
    Request->ProcessRequest();
}

void AOddSocketsClient::OnWorkerAssignmentResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid() || Response->GetResponseCode() != 200)
    {
        OnError.Broadcast(TEXT("Worker assignment failed"));
        if (ReconnectAttempts < Config.ReconnectAttempts)
            ScheduleReconnect();
        else
        {
            ConnectionState = EOddSocketsConnectionState::Error;
            OnMaxReconnectAttemptsReached.Broadcast();
        }
        return;
    }

    TSharedPtr<FJsonObject> Json;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
    if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
    {
        OnError.Broadcast(TEXT("Invalid worker response JSON"));
        ConnectionState = EOddSocketsConnectionState::Error;
        return;
    }

    WorkerUrl = Json->GetStringField(TEXT("url"));
    WorkerId = Json->GetStringField(TEXT("workerId"));

    if (WorkerUrl.IsEmpty() || WorkerId.IsEmpty())
    {
        OnError.Broadcast(TEXT("Missing worker URL/ID"));
        ConnectionState = EOddSocketsConnectionState::Error;
        return;
    }

    FOddSocketsWorkerAssignmentInfo Info;
    Info.WorkerId = WorkerId;
    Info.WorkerUrl = WorkerUrl;
    OnWorkerAssigned.Broadcast(Info);
    ConnectToWorker();
}

void AOddSocketsClient::ConnectToWorker()
{
    if (WorkerUrl.IsEmpty()) return;

    FString WsUrl = WorkerUrl.Replace(TEXT("https://"), TEXT("wss://"))
                              .Replace(TEXT("http://"), TEXT("ws://"));
    WsUrl += TEXT("/socket.io/?EIO=4&transport=websocket");

    if (!FModuleManager::Get().IsModuleLoaded(TEXT("WebSockets")))
        FModuleManager::Get().LoadModule(TEXT("WebSockets"));

    TMap<FString, FString> Headers;
    Headers.Add(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Config.ApiKey));

    WebSocket = FWebSocketsModule::Get().CreateWebSocket(WsUrl, TEXT("wss"), Headers);
    SetupWebSocketEventHandlers();
    WebSocket->Connect();
}

void AOddSocketsClient::SetupWebSocketEventHandlers()
{
    if (!WebSocket.IsValid()) return;
    WebSocket->OnConnected().AddUObject(this, &AOddSocketsClient::OnWebSocketConnected);
    WebSocket->OnConnectionError().AddUObject(this, &AOddSocketsClient::OnWebSocketConnectionError);
    WebSocket->OnClosed().AddUObject(this, &AOddSocketsClient::OnWebSocketClosed);
    WebSocket->OnMessage().AddUObject(this, &AOddSocketsClient::OnWebSocketMessage);
}

void AOddSocketsClient::OnWebSocketConnected()
{
    ConnectionState = EOddSocketsConnectionState::Connected;
    ReconnectAttempts = 0;
    OnConnected.Broadcast();
}

void AOddSocketsClient::OnWebSocketConnectionError(const FString& Error)
{
    ConnectionState = EOddSocketsConnectionState::Error;
    OnError.Broadcast(Error);
    if (ReconnectAttempts < Config.ReconnectAttempts)
        ScheduleReconnect();
    else
        OnMaxReconnectAttemptsReached.Broadcast();
}

void AOddSocketsClient::OnWebSocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
    ConnectionState = EOddSocketsConnectionState::Disconnected;
    OnDisconnected.Broadcast(FString::Printf(TEXT("Code:%d %s"), StatusCode, *Reason));
    if (ReconnectAttempts < Config.ReconnectAttempts)
        ScheduleReconnect();
}

void AOddSocketsClient::OnWebSocketMessage(const FString& Message)
{
    HandleSocketFrame(Message);
}

void AOddSocketsClient::HandleSocketFrame(const FString& Frame)
{
    if (Frame.IsEmpty()) return;

    // Engine.IO ping ("2") -> reply pong ("3") so the worker keeps us alive.
    if (Frame == TEXT("2"))
    {
        if (WebSocket.IsValid() && WebSocket->IsConnected())
            WebSocket->Send(TEXT("3"));
        return;
    }

    // Only Socket.IO EVENT frames (Engine.IO msg "4" + Socket.IO event "2")
    // carry application payloads: 42["event",{...}]. Ignore handshake/ack frames.
    if (!Frame.StartsWith(TEXT("42"))) return;

    // Skip the "42" prefix plus any numeric ack id before the payload array.
    int32 BracketIndex = INDEX_NONE;
    if (!Frame.FindChar(TEXT('['), BracketIndex) || BracketIndex == INDEX_NONE) return;

    const FString ArrayJson = Frame.Mid(BracketIndex);
    TArray<TSharedPtr<FJsonValue>> Parsed;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ArrayJson);
    if (!FJsonSerializer::Deserialize(Reader, Parsed) || Parsed.Num() == 0) return;

    const FString EventName = Parsed[0]->AsString();

    TSharedPtr<FJsonObject> DataObj;
    FString RawPayload;
    if (Parsed.Num() > 1 && Parsed[1].IsValid())
    {
        DataObj = Parsed[1]->AsObject();
        if (DataObj.IsValid())
        {
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RawPayload);
            FJsonSerializer::Serialize(DataObj.ToSharedRef(), Writer);
        }
    }

    RouteEvent(EventName, DataObj, RawPayload);
}

void AOddSocketsClient::RouteEvent(const FString& EventName, const TSharedPtr<FJsonObject>& Data, const FString& RawPayload)
{
    // Core channel events are dispatched to the owning UOddSocketsChannel.
    if (Data.IsValid())
    {
        FString ChannelName;
        Data->TryGetStringField(TEXT("channel"), ChannelName);
        UOddSocketsChannel** Found = ChannelName.IsEmpty() ? nullptr : Channels.Find(ChannelName);

        if (EventName == TEXT("message") && Found)
        {
            FOddSocketsChannelMessageData Msg;
            Msg.Channel = ChannelName;
            Data->TryGetStringField(TEXT("message"), Msg.Message);
            Data->TryGetStringField(TEXT("sender"), Msg.Sender);
            (*Found)->HandleMessage(Msg);
            return;
        }
        if (EventName == TEXT("subscribed") && Found)
        {
            FOddSocketsChannelSubscriptionData Sub;
            Sub.Channel = ChannelName;
            Sub.bSuccess = true;
            (*Found)->HandleSubscribed(Sub);
            return;
        }
        if (EventName == TEXT("unsubscribed") && Found)
        {
            FOddSocketsChannelSubscriptionData Sub;
            Sub.Channel = ChannelName;
            Sub.bSuccess = true;
            (*Found)->HandleUnsubscribed(Sub);
            return;
        }
    }

    // Everything else is an enhanced (Slack-like) broadcast: deliver generically
    // to Blueprint listeners and any native handlers registered via On(). This
    // includes reaction/typing events (e.g. "reaction_added") and the challenge /
    // leaderboard / achievement inbound events: "challenge_progress",
    // "leaderboard_rank_change", "challenge_complete", "achievement_unlock",
    // "achievement_progress", "challenge_invited", "challenge_reply_received" and
    // "challenge_invite_cancelled".
    OnEnhancedEvent.Broadcast(EventName, RawPayload);
    if (TArray<FOddSocketsEventHandler>* Handlers = NativeEventHandlers.Find(EventName))
    {
        for (const FOddSocketsEventHandler& Handler : *Handlers)
        {
            if (Handler) Handler(RawPayload);
        }
    }
}

void AOddSocketsClient::On(const FString& EventName, FOddSocketsEventHandler Handler)
{
    NativeEventHandlers.FindOrAdd(EventName).Add(MoveTemp(Handler));
}

void AOddSocketsClient::ScheduleReconnect()
{
    ReconnectAttempts++;
    ConnectionState = EOddSocketsConnectionState::Reconnecting;
    int32 Delay = FMath::Min(ReconnectDelay * (1 << (ReconnectAttempts - 1)), 30000);

    FOddSocketsReconnectInfo Info;
    Info.Attempt = ReconnectAttempts;
    Info.MaxAttempts = Config.ReconnectAttempts;
    Info.DelayMs = Delay;
    OnReconnecting.Broadcast(Info);

    GetWorld()->GetTimerManager().SetTimer(
        ReconnectTimerHandle, this, &AOddSocketsClient::ConnectAsync,
        static_cast<float>(Delay) / 1000.0f, false);
}

FString AOddSocketsClient::GenerateClientIdentifier()
{
    return FString::Printf(TEXT("%s_%s"), *HashString(Config.ApiKey),
        Config.UserId.IsEmpty() ? TEXT("default") : *Config.UserId);
}

FString AOddSocketsClient::HashString(const FString& Input)
{
    uint32 Hash = 0;
    for (int32 i = 0; i < Input.Len(); i++)
        Hash = ((Hash << 5) - Hash) + static_cast<uint32>(Input[i]);
    return FString::Printf(TEXT("%08x"), Hash);
}

void AOddSocketsClient::CleanupResources()
{
    if (GetWorld())
        GetWorld()->GetTimerManager().ClearTimer(ReconnectTimerHandle);
    if (WebSocket.IsValid()) { WebSocket->Close(); WebSocket.Reset(); }
    WorkerUrl.Empty();
    WorkerId.Empty();
}
