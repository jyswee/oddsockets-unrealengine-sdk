#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"
#include "Containers/Map.h"
#include "Containers/Array.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "OddSocketsTypes.h"
#include "OddSocketsChannel.h"
#include "ManagerDiscovery.h"
#include "OddSocketsClient.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConnecting);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDisconnected, const FString&, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnError, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorkerAssigned, const FOddSocketsWorkerAssignmentInfo&, WorkerInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReconnecting, const FOddSocketsReconnectInfo&, ReconnectInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMaxReconnectAttemptsReached);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnhancedEvent, const FString&, EventName, const FString&, JsonPayload);

/** Native (C++) handler signature for a raw named server event. */
using FOddSocketsEventHandler = TFunction<void(const FString& /*JsonPayload*/)>;

/**
 * OddSockets Unreal Engine SDK
 * 
 * Provides a simple interface to the OddSockets real-time messaging platform.
 * Automatically handles manager discovery and Worker load balancing internally.
 */
UCLASS(BlueprintType, Blueprintable)
class ODDSOCKETS_API AOddSocketsClient : public AActor
{
    GENERATED_BODY()

public:
    AOddSocketsClient();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void Tick(float DeltaTime) override;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OddSockets Configuration")
    FOddSocketsConfig Config;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "OddSockets Events")
    FOnConnecting OnConnecting;

    UPROPERTY(BlueprintAssignable, Category = "OddSockets Events")
    FOnConnected OnConnected;

    UPROPERTY(BlueprintAssignable, Category = "OddSockets Events")
    FOnDisconnected OnDisconnected;

    UPROPERTY(BlueprintAssignable, Category = "OddSockets Events")
    FOnError OnError;

    UPROPERTY(BlueprintAssignable, Category = "OddSockets Events")
    FOnWorkerAssigned OnWorkerAssigned;

    UPROPERTY(BlueprintAssignable, Category = "OddSockets Events")
    FOnReconnecting OnReconnecting;

    UPROPERTY(BlueprintAssignable, Category = "OddSockets Events")
    FOnMaxReconnectAttemptsReached OnMaxReconnectAttemptsReached;

    // Fires for every enhanced (Slack-like) broadcast the worker delivers,
    // e.g. "user_typing", "reaction_added". Payload is the raw JSON string.
    UPROPERTY(BlueprintAssignable, Category = "OddSockets Enhanced Events")
    FOnEnhancedEvent OnEnhancedEvent;

    // Public Methods
    UFUNCTION(BlueprintCallable, Category = "OddSockets")
    void Initialize(const FOddSocketsConfig& InConfig);

    UFUNCTION(BlueprintCallable, Category = "OddSockets")
    void ConnectAsync();

    UFUNCTION(BlueprintCallable, Category = "OddSockets")
    void Disconnect();

    UFUNCTION(BlueprintCallable, Category = "OddSockets")
    class UOddSocketsChannel* GetChannel(const FString& ChannelName);

    UFUNCTION(BlueprintCallable, Category = "OddSockets")
    void PublishBulkAsync(const TArray<FOddSocketsBulkMessage>& Messages);

    UFUNCTION(BlueprintPure, Category = "OddSockets")
    EOddSocketsConnectionState GetConnectionState() const { return ConnectionState; }

    UFUNCTION(BlueprintPure, Category = "OddSockets")
    FString GetClientIdentifier() const { return ClientIdentifier; }

    UFUNCTION(BlueprintPure, Category = "OddSockets")
    FOddSocketsWorkerInfo GetWorkerInfo() const;

    UFUNCTION(BlueprintPure, Category = "OddSockets")
    FOddSocketsSessionInfo GetSessionInfo() const { return SessionInfo; }

    UFUNCTION(BlueprintPure, Category = "OddSockets")
    bool IsConnected() const;

    // Register a native (C++) handler for a raw named server event (e.g. an
    // enhanced broadcast such as "reaction_added"). Blueprint code should bind
    // OnEnhancedEvent instead. Not a UFUNCTION: TFunction is C++-only.
    void On(const FString& EventName, FOddSocketsEventHandler Handler);

    // Internal methods for channels
    TSharedPtr<IWebSocket> GetWebSocket() const { return WebSocket; }

private:
    // Private fields
    UPROPERTY()
    TMap<FString, class UOddSocketsChannel*> Channels;

    TSharedPtr<IWebSocket> WebSocket;
    FString WorkerUrl;
    FString WorkerId;
    EOddSocketsConnectionState ConnectionState;
    int32 ReconnectAttempts;
    int32 ReconnectDelay;
    FString ClientIdentifier;
    FOddSocketsSessionInfo SessionInfo;
    FTimerHandle ReconnectTimerHandle;

    // Private methods
    void GetWorkerAssignment();
    void OnWorkerAssignmentResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void ConnectToWorker();
    void SetupWebSocketEventHandlers();
    void ScheduleReconnect();
    void OnWebSocketConnected();
    void OnWebSocketConnectionError(const FString& Error);
    void OnWebSocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
    void OnWebSocketMessage(const FString& Message);
    FString GenerateClientIdentifier();
    FString HashString(const FString& Input);
    // Decode a single Engine.IO/Socket.IO frame and route its payload.
    void HandleSocketFrame(const FString& Frame);
    // Dispatch a decoded ["event", data] pair to channels or enhanced handlers.
    void RouteEvent(const FString& EventName, const TSharedPtr<class FJsonObject>& Data, const FString& RawPayload);
    void CleanupResources();

    // Native handlers registered via On(), keyed by event name.
    TMap<FString, TArray<FOddSocketsEventHandler>> NativeEventHandlers;
};
