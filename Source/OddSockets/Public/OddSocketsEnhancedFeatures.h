#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Dom/JsonObject.h"
#include "OddSocketsEnhancedFeatures.generated.h"

class AOddSocketsClient;

/** Payload for every typed game event: the broadcast's raw JSON string. The
 *  game parses it itself (same contract as the Unity SDK's game events). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOddSocketsGameEvent, const FString&, JsonPayload);

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
    /** Bind this helper to a connected client before calling any method. Also
     *  wires the typed game-event delegates below to their broadcasts. */
    void Initialize(AOddSocketsClient* InClient);

    // ==================== TYPED GAME EVENTS (Unity parity) =====================
    //
    // Blueprint-assignable delegates for the game-feature broadcasts, named as
    // the Unity SDK names them so a converted project binds 1:1. Each fires with
    // the broadcast's raw JSON payload. The generic AOddSocketsClient::
    // OnEnhancedEvent still fires for every broadcast, these included.

    /** Inbound event "challenge_invited": a directed invite arrived for this player. */
    UPROPERTY(BlueprintAssignable, Category = "OddSockets|Enhanced|Challenges")
    FOddSocketsGameEvent OnChallengeInvited;

    /** Inbound event "challenge_reply_received": your invite was accepted or declined. */
    UPROPERTY(BlueprintAssignable, Category = "OddSockets|Enhanced|Challenges")
    FOddSocketsGameEvent OnChallengeReplyReceived;

    /** Inbound event "challenge_invite_cancelled": an invite sent to you was withdrawn. */
    UPROPERTY(BlueprintAssignable, Category = "OddSockets|Enhanced|Challenges")
    FOddSocketsGameEvent OnChallengeInviteCancelled;

    /** Ack "challenge_invites": the pending-invite inbox (GetChallengeInvitesAsync). */
    UPROPERTY(BlueprintAssignable, Category = "OddSockets|Enhanced|Challenges")
    FOddSocketsGameEvent OnChallengeInvites;

    /** Ack "achievement_state": persisted achievement state (GetAchievementsAsync). */
    UPROPERTY(BlueprintAssignable, Category = "OddSockets|Enhanced|Challenges")
    FOddSocketsGameEvent OnAchievementState;

    /** Ack "challenge_standings_success": leaderboard standings (GetStandingsAsync). */
    UPROPERTY(BlueprintAssignable, Category = "OddSockets|Enhanced|Challenges")
    FOddSocketsGameEvent OnStandingsData;

    /** Inbound event "leaderboard_rank_change": a ranked challenge's order moved. */
    UPROPERTY(BlueprintAssignable, Category = "OddSockets|Enhanced|Challenges")
    FOddSocketsGameEvent OnLeaderboardRankChange;

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

    // ==================== CHALLENGES / LEADERBOARDS / ACHIEVEMENTS ====================
    //
    // Game Center parity: scored challenges + leaderboards + achievements, plus a
    // directed 1:1 invite/accept/decline handshake. These are send-path helpers in
    // the same style as the reaction methods above: each builds a JSON payload and
    // emits it. Responses and broadcasts arrive back on
    // AOddSocketsClient::OnEnhancedEvent / On(), keyed by the broadcast event name.
    //
    // Request/ack events (success + error) delivered as enhanced broadcasts:
    //   challenge_create        -> "challenge_create_success"        (err "challenge_create")
    //   challenge_complete      -> "challenge_complete_success"       (err "challenge_complete")
    //   challenge_standings     -> "challenge_standings_success"      (err "challenge_standings")
    //   achievement_query       -> "achievement_state"                (err "achievement_query")
    //   challenge_invite        -> "challenge_invite_success"         (err "challenge_invite")
    //   challenge_reply         -> "challenge_reply_success"          (err "challenge_reply")
    //   challenge_invite_cancel -> "challenge_invite_cancel_success"  (err "challenge_invite_cancel")
    //   challenge_invites_query -> "challenge_invites"                (err "challenge_invites_query")
    // Fire-and-forget: challenge_progress, achievement_unlock.
    //
    // Params follow the reaction methods' style: individual typed args (Blueprint-
    // compatible FString/typed values). Optional fields default to empty/sentinel
    // and are only written into the payload when supplied.

    /**
     * Open a challenge. ranked enables a shared leaderboard so progress publishes
     * "leaderboard_rank_change". Pass empty strings for optional URL fields to omit.
     * Wire event: challenge_create. Paired ack: "challenge_create_success" (err "challenge_create").
     */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void CreateChallenge(const FString& ChallengeId, const FString& Metric, bool bRanked = false,
                         const FString& Channel = TEXT(""), const FString& ResultWebhookUrl = TEXT(""),
                         const FString& StandingsUrl = TEXT(""));

    /**
     * Report a participant's value. Broadcasts "challenge_progress" (and
     * "leaderboard_rank_change" on ranked challenges). Pass a stable EventId for
     * idempotent retries.
     * Wire event: challenge_progress. Fire-and-forget.
     */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void ReportProgress(const FString& ChallengeId, float Value, const FString& Metric = TEXT(""),
                        const FString& EventId = TEXT(""), const FString& Cohort = TEXT(""),
                        const FString& Platform = TEXT(""), const FString& Channel = TEXT(""));

    /**
     * Finalize a participant. Outcome in { completed, failed, expired, conceded, tied }.
     * Wire event: challenge_complete. Paired ack: "challenge_complete_success" (err "challenge_complete").
     */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void CompleteChallenge(const FString& ChallengeId, const FString& Outcome,
                           const FString& EventId = TEXT(""), const FString& Reward = TEXT(""));

    /**
     * Unlock or advance an achievement. PercentComplete < 100 broadcasts
     * "achievement_progress"; >= 100 (or negative to omit) broadcasts
     * "achievement_unlock".
     * Wire event: achievement_unlock. Fire-and-forget.
     */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void UnlockAchievement(const FString& AchievementId, const FString& Name = TEXT(""),
                           const FString& Tier = TEXT(""), float PercentComplete = -1.0f,
                           const FString& ChallengeId = TEXT(""), const FString& Channel = TEXT(""));

    /**
     * Fetch top-N leaderboard standings + the caller's own rank.
     * Wire event: challenge_standings. Paired ack: "challenge_standings_success" (err "challenge_standings").
     */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void GetStandings(const FString& ChallengeId, int32 Limit = 20, int32 Offset = 0);

    /**
     * Query persisted achievement state for this player. Pass empty AchievementId
     * for all.
     * Wire event: achievement_query. Paired ack: "achievement_state" (err "achievement_query").
     */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void GetAchievements(const FString& AchievementId = TEXT(""));

    /**
     * Send a directed 1:1 invite to another player. Payload is an arbitrary JSON
     * string (<=8KB). Pass empty InviteId to let the server assign one.
     * Wire event: challenge_invite. Paired ack: "challenge_invite_success" (err "challenge_invite").
     */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void SendChallengeInvite(const FString& ToUserId, const FString& Type = TEXT("match"),
                             const FString& Payload = TEXT(""), int32 Ttl = 300,
                             const FString& Channel = TEXT(""), const FString& InviteId = TEXT(""));

    /**
     * Accept or decline an invite you received. Payload (a JSON object string)
     * reaches the inviter as "replyPayload" on OnChallengeReplyReceived; Reason
     * rides the wire's "reason" field alongside it.
     * Wire event: challenge_reply. Paired ack: "challenge_reply_success" (err "challenge_reply").
     */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void ReplyChallengeInvite(const FString& InviteId, bool bAccept,
                              const FString& Payload = TEXT(""), const FString& Reason = TEXT(""));

    /**
     * Cancel an invite you sent (inviter only).
     * Wire event: challenge_invite_cancel. Paired ack: "challenge_invite_cancel_success" (err "challenge_invite_cancel").
     */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void CancelChallengeInvite(const FString& InviteId);

    /**
     * Pull this player's pending invites (offline-capable inbox). Empty payload.
     * Wire event: challenge_invites_query. Paired ack: "challenge_invites" (err "challenge_invites_query").
     */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void GetChallengeInvites();

    // ==================== UNITY-NAMED ALIASES (*Async) ========================
    //
    // The Unity SDK's method names, so unity2unreal-style converters map calls
    // 1:1 without a rename table. Each delegates to the method above it aliases;
    // "Async" describes the delivery contract (the result arrives on the typed
    // delegate / OnEnhancedEvent, never as a return value), same as in Unity.

    /** Unity alias for CreateChallenge. Ack arrives on OnEnhancedEvent ("challenge_create_success"). */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void CreateChallengeAsync(const FString& ChallengeId, const FString& Metric, bool bRanked = false)
    {
        CreateChallenge(ChallengeId, Metric, bRanked);
    }

    /** Unity alias for ReportProgress. Ranked challenges broadcast OnLeaderboardRankChange. */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void ReportProgressAsync(const FString& ChallengeId, float Value)
    {
        ReportProgress(ChallengeId, Value);
    }

    /** Unity alias for GetStandings. Result arrives on OnStandingsData. */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void GetStandingsAsync(const FString& ChallengeId, int32 Limit = 20)
    {
        GetStandings(ChallengeId, Limit, 0);
    }

    /** Unity alias for UnlockAchievement. State broadcasts arrive on OnAchievementState after a query. */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void UnlockAchievementAsync(const FString& AchievementId, const FString& Name = TEXT(""),
                                const FString& Tier = TEXT(""), float PercentComplete = -1.0f)
    {
        UnlockAchievement(AchievementId, Name, Tier, PercentComplete);
    }

    /** Unity alias for GetAchievements (all achievements). Result arrives on OnAchievementState. */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void GetAchievementsAsync()
    {
        GetAchievements();
    }

    /** Unity alias for SendChallengeInvite. Invitee receives OnChallengeInvited. */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void SendChallengeInviteAsync(const FString& ToUserId, const FString& Type = TEXT("match"),
                                  const FString& Payload = TEXT(""), int32 TtlSec = 300)
    {
        SendChallengeInvite(ToUserId, Type, Payload, TtlSec);
    }

    /**
     * Unity alias for ReplyChallengeInvite — same argument order as the Unity
     * SDK: (inviteId, accept, payload, reason). Payload is a JSON object string
     * sent as the wire's "payload" and relayed to the inviter as "replyPayload"
     * on OnChallengeReplyReceived (the original invite payload is echoed back
     * alongside it); Reason rides the wire's "reason" field.
     */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void ReplyChallengeInviteAsync(const FString& InviteId, bool bAccept,
                                   const FString& Payload = TEXT(""), const FString& Reason = TEXT(""))
    {
        ReplyChallengeInvite(InviteId, bAccept, Payload, Reason);
    }

    /** Unity alias for CancelChallengeInvite. Invitee receives OnChallengeInviteCancelled. */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void CancelChallengeInviteAsync(const FString& InviteId)
    {
        CancelChallengeInvite(InviteId);
    }

    /** Unity alias for GetChallengeInvites. Result arrives on OnChallengeInvites. */
    UFUNCTION(BlueprintCallable, Category = "OddSockets|Enhanced|Challenges")
    void GetChallengeInvitesAsync()
    {
        GetChallengeInvites();
    }

private:
    /** Serialize Payload and emit it as a Socket.IO event over the client socket. */
    void Emit(const FString& EventName, const TSharedRef<FJsonObject>& Payload);

    UPROPERTY()
    AOddSocketsClient* Client = nullptr;

    /** Typed delegates are registered on the client once; see Initialize. */
    bool bEventsWired = false;
};
