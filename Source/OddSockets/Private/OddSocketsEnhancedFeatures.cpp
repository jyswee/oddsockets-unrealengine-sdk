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

// ==================== CHALLENGES / LEADERBOARDS / ACHIEVEMENTS ====================

void UOddSocketsEnhancedFeatures::CreateChallenge(const FString& ChallengeId, const FString& Metric,
                                                  bool bRanked, const FString& Channel,
                                                  const FString& ResultWebhookUrl, const FString& StandingsUrl)
{
    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("challengeId"), ChallengeId);
    Payload->SetStringField(TEXT("metric"), Metric);
    Payload->SetBoolField(TEXT("ranked"), bRanked);
    if (!Channel.IsEmpty()) Payload->SetStringField(TEXT("channel"), Channel);
    if (!ResultWebhookUrl.IsEmpty()) Payload->SetStringField(TEXT("resultWebhookUrl"), ResultWebhookUrl);
    if (!StandingsUrl.IsEmpty()) Payload->SetStringField(TEXT("standingsUrl"), StandingsUrl);
    Emit(TEXT("challenge_create"), Payload);
}

void UOddSocketsEnhancedFeatures::ReportProgress(const FString& ChallengeId, float Value,
                                                 const FString& Metric, const FString& EventId,
                                                 const FString& Cohort, const FString& Platform,
                                                 const FString& Channel)
{
    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("challengeId"), ChallengeId);
    Payload->SetNumberField(TEXT("value"), Value);
    if (!Metric.IsEmpty()) Payload->SetStringField(TEXT("metric"), Metric);
    if (!EventId.IsEmpty()) Payload->SetStringField(TEXT("eventId"), EventId);
    if (!Cohort.IsEmpty()) Payload->SetStringField(TEXT("cohort"), Cohort);
    if (!Platform.IsEmpty()) Payload->SetStringField(TEXT("platform"), Platform);
    if (!Channel.IsEmpty()) Payload->SetStringField(TEXT("channel"), Channel);
    Emit(TEXT("challenge_progress"), Payload);
}

void UOddSocketsEnhancedFeatures::CompleteChallenge(const FString& ChallengeId, const FString& Outcome,
                                                    const FString& EventId, const FString& Reward)
{
    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("challengeId"), ChallengeId);
    Payload->SetStringField(TEXT("outcome"), Outcome);
    if (!EventId.IsEmpty()) Payload->SetStringField(TEXT("eventId"), EventId);
    if (!Reward.IsEmpty()) Payload->SetStringField(TEXT("reward"), Reward);
    Emit(TEXT("challenge_complete"), Payload);
}

void UOddSocketsEnhancedFeatures::UnlockAchievement(const FString& AchievementId, const FString& Name,
                                                    const FString& Tier, float PercentComplete,
                                                    const FString& ChallengeId, const FString& Channel)
{
    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("achievementId"), AchievementId);
    if (!Name.IsEmpty()) Payload->SetStringField(TEXT("name"), Name);
    if (!Tier.IsEmpty()) Payload->SetStringField(TEXT("tier"), Tier);
    if (PercentComplete >= 0.0f) Payload->SetNumberField(TEXT("percentComplete"), PercentComplete);
    if (!ChallengeId.IsEmpty()) Payload->SetStringField(TEXT("challengeId"), ChallengeId);
    if (!Channel.IsEmpty()) Payload->SetStringField(TEXT("channel"), Channel);
    Emit(TEXT("achievement_unlock"), Payload);
}

void UOddSocketsEnhancedFeatures::GetStandings(const FString& ChallengeId, int32 Limit, int32 Offset)
{
    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("challengeId"), ChallengeId);
    Payload->SetNumberField(TEXT("limit"), Limit);
    Payload->SetNumberField(TEXT("offset"), Offset);
    Emit(TEXT("challenge_standings"), Payload);
}

void UOddSocketsEnhancedFeatures::GetAchievements(const FString& AchievementId)
{
    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    if (!AchievementId.IsEmpty()) Payload->SetStringField(TEXT("achievementId"), AchievementId);
    Emit(TEXT("achievement_query"), Payload);
}

void UOddSocketsEnhancedFeatures::SendChallengeInvite(const FString& ToUserId, const FString& Type,
                                                      const FString& Payload_, int32 Ttl,
                                                      const FString& Channel, const FString& InviteId)
{
    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("toUserId"), ToUserId);
    if (!Type.IsEmpty()) Payload->SetStringField(TEXT("type"), Type);
    if (!Payload_.IsEmpty()) Payload->SetStringField(TEXT("payload"), Payload_);
    Payload->SetNumberField(TEXT("ttl"), Ttl);
    if (!Channel.IsEmpty()) Payload->SetStringField(TEXT("channel"), Channel);
    if (!InviteId.IsEmpty()) Payload->SetStringField(TEXT("inviteId"), InviteId);
    Emit(TEXT("challenge_invite"), Payload);
}

void UOddSocketsEnhancedFeatures::ReplyChallengeInvite(const FString& InviteId, bool bAccept,
                                                       const FString& Reason)
{
    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("inviteId"), InviteId);
    Payload->SetBoolField(TEXT("accept"), bAccept);
    if (!Reason.IsEmpty()) Payload->SetStringField(TEXT("reason"), Reason);
    Emit(TEXT("challenge_reply"), Payload);
}

void UOddSocketsEnhancedFeatures::CancelChallengeInvite(const FString& InviteId)
{
    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("inviteId"), InviteId);
    Emit(TEXT("challenge_invite_cancel"), Payload);
}

void UOddSocketsEnhancedFeatures::GetChallengeInvites()
{
    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Emit(TEXT("challenge_invites_query"), Payload);
}
