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
    if (Config.IsValid() && Config.bAutoConnect)
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

    // Token mode with a provider: fetch a fresh token before every (re)connect
    // so a reconnect after expiry never reuses a dead token. The connect
    // resumes in ProvideToken.
    if (Config.ApiKey.IsEmpty() && HasTokenProvider())
    {
        bAwaitingToken = true;
        RequestFreshToken();
        return;
    }
    GetWorkerAssignment();
}

bool AOddSocketsClient::HasTokenProvider() const
{
    return static_cast<bool>(TokenProvider) || OnTokenRequested.IsBound();
}

void AOddSocketsClient::RequestFreshToken()
{
    if (TokenProvider)
    {
        TWeakObjectPtr<AOddSocketsClient> Self(this);
        TokenProvider([Self](const FOddSocketsToken& FreshToken)
        {
            if (Self.IsValid())
            {
                Self->ProvideToken(FreshToken);
            }
        });
    }
    else
    {
        OnTokenRequested.Broadcast();
    }
}

void AOddSocketsClient::ProvideToken(const FOddSocketsToken& FreshToken)
{
    Config.Token = FreshToken;
    ScheduleTokenRefresh();

    if (bAwaitingToken)
    {
        bAwaitingToken = false;
        if (ConnectionState == EOddSocketsConnectionState::Connecting)
        {
            GetWorkerAssignment();
        }
    }
}

void AOddSocketsClient::ScheduleTokenRefresh()
{
    if (!GetWorld()) return;
    GetWorld()->GetTimerManager().ClearTimer(TokenRefreshTimerHandle);
    if (!HasTokenProvider()) return;

    FDateTime Expiry;
    if (!FDateTime::ParseIso8601(*Config.Token.ExpiresAt, Expiry)) return;

    const double SecondsUntilRefresh =
        (Expiry - FDateTime::UtcNow()).GetTotalSeconds() - Config.TokenRefreshLeadSeconds;
    GetWorld()->GetTimerManager().SetTimer(
        TokenRefreshTimerHandle, this, &AOddSocketsClient::RequestFreshToken,
        FMath::Max(1.0f, static_cast<float>(SecondsUntilRefresh)), false);
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

void AOddSocketsClient::EmitAsync(const FString& EventName, const FString& JsonPayload)
{
    if (EventName.IsEmpty() || !WebSocket.IsValid() || !WebSocket->IsConnected()) return;

    // Validate the payload parses as a JSON object so a malformed string never
    // reaches the wire as a broken frame; an empty payload emits {}.
    FString Body = JsonPayload.IsEmpty() ? TEXT("{}") : JsonPayload;
    TSharedPtr<FJsonObject> Parsed;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
    if (!FJsonSerializer::Deserialize(Reader, Parsed) || !Parsed.IsValid())
    {
        OnError.Broadcast(FString::Printf(TEXT("EmitAsync(%s): payload is not a JSON object"), *EventName));
        return;
    }

    FString EscapedEvent = EventName.ReplaceCharWithEscapedChar();
    WebSocket->Send(FString::Printf(TEXT("42[\"%s\",%s]"), *EscapedEvent, *Body));
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
        ConnectionState = EOddSocketsConnectionState::Failed;
        OnError.Broadcast(DiscoveryError);
        return;
    }

    FString UserId = Config.UserId.IsEmpty() ? ClientIdentifier : Config.UserId;
    // Token clients present the minted token instead of an apiKey; the manager
    // accepts either (same contract as the JS SDK's selectParams).
    const FString AuthParam = (!Config.ApiKey.IsEmpty() || !Config.Token.IsSet())
        ? FString::Printf(TEXT("apiKey=%s"), *Config.ApiKey)
        : FString::Printf(TEXT("token=%s"), *Config.Token.Token);
    FString Url = FString::Printf(
        TEXT("%s/api/cluster/select-worker?%s&userId=%s&clientIdentifier=%s"),
        *ManagerUrl, *AuthParam, *UserId, *ClientIdentifier);

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
            ConnectionState = EOddSocketsConnectionState::Failed;
            OnMaxReconnectAttemptsReached.Broadcast();
        }
        return;
    }

    TSharedPtr<FJsonObject> Json;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
    if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
    {
        OnError.Broadcast(TEXT("Invalid worker response JSON"));
        ConnectionState = EOddSocketsConnectionState::Failed;
        return;
    }

    WorkerUrl = Json->GetStringField(TEXT("url"));
    WorkerId = Json->GetStringField(TEXT("workerId"));

    if (WorkerUrl.IsEmpty() || WorkerId.IsEmpty())
    {
        OnError.Broadcast(TEXT("Missing worker URL/ID"));
        ConnectionState = EOddSocketsConnectionState::Failed;
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
    // Transport is up, but the client is not usable until the Socket.IO
    // namespace CONNECT is acknowledged ("40" ack in HandleSocketFrame). The
    // auth payload rides that CONNECT packet — the worker's io.use middleware
    // reads handshake.auth.apiKey / handshake.auth.token from it and rejects
    // the namespace otherwise ("API key required"). OnConnected is broadcast
    // on the ack, not here.
}

void AOddSocketsClient::OnWebSocketConnectionError(const FString& Error)
{
    ConnectionState = EOddSocketsConnectionState::Failed;
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

    // Engine.IO OPEN ("0{...}"): the transport is ready for Socket.IO packets.
    // Send the namespace CONNECT ("40{auth}") carrying our credentials — the
    // worker authenticates from handshake.auth and will not deliver or accept
    // any event until this handshake completes.
    if (Frame.StartsWith(TEXT("0")))
    {
        TSharedRef<FJsonObject> Auth = MakeShared<FJsonObject>();
        if (!Config.ApiKey.IsEmpty())
            Auth->SetStringField(TEXT("apiKey"), Config.ApiKey);
        else if (Config.Token.IsSet())
            Auth->SetStringField(TEXT("token"), Config.Token.Token);
        Auth->SetStringField(TEXT("userId"),
            Config.UserId.IsEmpty() ? ClientIdentifier : Config.UserId);

        FString AuthJson;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&AuthJson);
        FJsonSerializer::Serialize(Auth, Writer);
        if (WebSocket.IsValid() && WebSocket->IsConnected())
            WebSocket->Send(FString::Printf(TEXT("40%s"), *AuthJson));
        return;
    }

    // Socket.IO CONNECT ack ("40{...}"): authenticated and joined — NOW the
    // client is usable.
    if (Frame.StartsWith(TEXT("40")))
    {
        ConnectionState = EOddSocketsConnectionState::Connected;
        ReconnectAttempts = 0;
        OnConnected.Broadcast();
        return;
    }

    // Socket.IO CONNECT_ERROR ("44{\"message\":...}"): auth was refused.
    if (Frame.StartsWith(TEXT("44")))
    {
        FString Reason = Frame.Mid(2);
        TSharedPtr<FJsonObject> Err;
        TSharedRef<TJsonReader<>> ErrReader = TJsonReaderFactory<>::Create(Reason);
        if (FJsonSerializer::Deserialize(ErrReader, Err) && Err.IsValid())
            Err->TryGetStringField(TEXT("message"), Reason);
        ConnectionState = EOddSocketsConnectionState::Failed;
        OnError.Broadcast(FString::Printf(TEXT("Connection refused: %s"), *Reason));
        return;
    }

    // Only Socket.IO EVENT frames (Engine.IO msg "4" + Socket.IO event "2")
    // carry application payloads: 42["event",{...}]. Ignore other ack frames.
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
        if (EventName == TEXT("presence_change") && Found)
        {
            FOddSocketsChannelPresenceChangeData Change;
            Change.Channel = ChannelName;
            Data->TryGetStringField(TEXT("action"), Change.Action);
            const TSharedPtr<FJsonObject>* UserObj = nullptr;
            if (Data->TryGetObjectField(TEXT("user"), UserObj))
            {
                (*UserObj)->TryGetStringField(TEXT("userId"), Change.User.UserId);
                FString JoinedAt;
                if ((*UserObj)->TryGetStringField(TEXT("joinedAt"), JoinedAt))
                {
                    FDateTime::ParseIso8601(*JoinedAt, Change.User.JoinedAt);
                }
            }
            (*Found)->HandlePresenceChange(Change);
            return;
        }
        if (EventName == TEXT("presence") && Found)
        {
            FOddSocketsChannelPresenceData Pres;
            Pres.Channel = ChannelName;
            const TArray<TSharedPtr<FJsonValue>>* Occupants = nullptr;
            if (Data->TryGetArrayField(TEXT("occupants"), Occupants))
            {
                for (const TSharedPtr<FJsonValue>& Entry : *Occupants)
                {
                    FOddSocketsPresenceUser User;
                    const TSharedPtr<FJsonObject>* EntryObj = nullptr;
                    if (Entry->TryGetObject(EntryObj))
                    {
                        (*EntryObj)->TryGetStringField(TEXT("userId"), User.UserId);
                        FString JoinedAt;
                        if ((*EntryObj)->TryGetStringField(TEXT("joinedAt"), JoinedAt))
                        {
                            FDateTime::ParseIso8601(*JoinedAt, User.JoinedAt);
                        }
                    }
                    else
                    {
                        Entry->TryGetString(User.UserId);
                    }
                    Pres.Occupants.Add(User);
                }
            }
            Pres.Count = Pres.Occupants.Num();
            Data->TryGetNumberField(TEXT("occupancy"), Pres.Count);
            (*Found)->HandlePresence(Pres);
            return;
        }
        if (EventName == TEXT("history") && Found)
        {
            FOddSocketsChannelHistoryData Hist;
            Hist.Channel = ChannelName;
            const TArray<TSharedPtr<FJsonValue>>* Msgs = nullptr;
            if (Data->TryGetArrayField(TEXT("messages"), Msgs))
            {
                for (const TSharedPtr<FJsonValue>& Entry : *Msgs)
                {
                    FOddSocketsChannelMessageData Msg;
                    Msg.Channel = ChannelName;
                    const TSharedPtr<FJsonObject>* EntryObj = nullptr;
                    if (Entry->TryGetObject(EntryObj))
                    {
                        // A stored message body may itself be an object; hand it
                        // to the game re-serialized, same as a live frame would.
                        const TSharedPtr<FJsonObject>* BodyObj = nullptr;
                        if ((*EntryObj)->TryGetObjectField(TEXT("message"), BodyObj))
                        {
                            const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Msg.Message);
                            FJsonSerializer::Serialize(BodyObj->ToSharedRef(), Writer);
                        }
                        else
                        {
                            (*EntryObj)->TryGetStringField(TEXT("message"), Msg.Message);
                        }
                        (*EntryObj)->TryGetStringField(TEXT("sender"), Msg.Sender);
                        FString Timestamp;
                        if ((*EntryObj)->TryGetStringField(TEXT("timestamp"), Timestamp))
                        {
                            FDateTime::ParseIso8601(*Timestamp, Msg.Timestamp);
                        }
                    }
                    Hist.Messages.Add(Msg);
                }
            }
            Hist.Count = Hist.Messages.Num();
            (*Found)->HandleHistory(Hist);
            return;
        }
        if (EventName == TEXT("published") && Found)
        {
            FOddSocketsChannelPublishData Pub;
            Pub.Channel = ChannelName;
            Pub.bSuccess = true;
            Data->TryGetStringField(TEXT("messageId"), Pub.MessageId);
            FString Timestamp;
            if (Data->TryGetStringField(TEXT("timestamp"), Timestamp))
            {
                FDateTime::ParseIso8601(*Timestamp, Pub.Timestamp);
            }
            (*Found)->HandlePublished(Pub);
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
    Info.Delay = Delay;
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
    {
        GetWorld()->GetTimerManager().ClearTimer(ReconnectTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(TokenRefreshTimerHandle);
    }
    bAwaitingToken = false;
    if (WebSocket.IsValid()) { WebSocket->Close(); WebSocket.Reset(); }
    WorkerUrl.Empty();
    WorkerId.Empty();
}
