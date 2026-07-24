#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Dom/JsonObject.h"
#include "OddSocketsEnhancedFeatures.generated.h"

class AOddSocketsClient;

/**
 * Enhanced (Slack-like) real-time features for the OddSockets Unreal Engine SDK.
 *
 * These are send-path helpers: each method emits an enhanced event over the
 * client's existing Socket.IO connection. The paired broadcast arrives back on
 * AOddSocketsClient::OnEnhancedEvent (Blueprint) or a native handler registered
 * via AOddSocketsClient::On (C++), keyed by the broadcast event name.
 *
 * Scope: typing indicators and message reactions. These are the enhanced events
 * with a verified send path in this SDK. Additional Slack-like surfaces (threads,
 * read receipts, presence, search, etc.) will be added here as each is
 * implemented and tested end-to-end, rather than declared ahead of support.
 */
UCLASS(BlueprintType)
class ODDSOCKETS_API UOddSocketsEnhancedFeatures : public UObject
{
    GENERATED_BODY()

public:
    /** Bind this helper to a connected client before calling any method. */
    void Initialize(AOddSocketsClient* InClient);

    // ==================== TYPING INDICATORS ====================

    /**
     * Notify a channel that the user has started typing.
     * Wire event: start_typing. Paired broadcast: "user_typing".
     */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Typing")
    void StartTyping(const FString& UserId, const FString& Channel);

    /**
     * Notify a channel that the user has stopped typing.
     * Wire event: stop_typing. Paired broadcast: "user_stopped_typing".
     */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Typing")
    void StopTyping(const FString& UserId, const FString& Channel);

    // ==================== REACTIONS ====================

    /**
     * Add an emoji reaction to a message.
     * Wire event: add_reaction. Paired broadcast: "reaction_added".
     */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Reactions")
    void AddReaction(const FString& MessageId, const FString& Channel, const FString& Emoji,
                     const FString& UserId, const FString& UserName);

    /**
     * Remove an emoji reaction from a message.
     * Wire event: remove_reaction. Paired broadcast: "reaction_removed".
     */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Reactions")
    void RemoveReaction(const FString& MessageId, const FString& Channel, const FString& Emoji,
                        const FString& UserId);

private:
    /** Serialize Payload and emit it as a Socket.IO event over the client socket. */
    void Emit(const FString& EventName, const TSharedRef<FJsonObject>& Payload);

    UPROPERTY()
    AOddSocketsClient* Client = nullptr;
};
