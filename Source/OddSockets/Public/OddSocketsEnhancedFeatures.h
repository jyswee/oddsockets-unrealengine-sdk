// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OddSocketsClient.h"
#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"

/**
 * Enhanced Features for OddSockets Unreal Engine SDK
 * Provides 67 new Slack-like events with Unreal Engine async patterns and delegates
 */
class ODDSOCKETS_API FOddSocketsEnhancedFeatures
{
public:
    FOddSocketsEnhancedFeatures(TSharedPtr<FOddSocketsClient> InClient);
    ~FOddSocketsEnhancedFeatures();

    // ==================== THREAD EVENTS ====================

    void ThreadReply(const FString& Channel, const FString& ParentMessageId, const FString& Message, 
                    const FString& UserId, const FString& UserName, 
                    TFunction<void(const FString&)> OnSuccess, TFunction<void(const FString&)> OnError);

    void GetThread(const FString& ThreadId, 
                  TFunction<void(const FString&)> OnSuccess, TFunction<void(const FString&)> OnError);

    void SubscribeThread(const FString& ThreadId, const FString& UserId, 
                        TFunction<void(const FString&)> OnSuccess, TFunction<void(const FString&)> OnError);

    void MarkThreadRead(const FString& ThreadId, const FString& UserId);
    void FollowThread(const FString& ThreadId, const FString& UserId);
    void UnfollowThread(const FString& ThreadId, const FString& UserId);

    // ==================== REACTION EVENTS ====================

    void AddReaction(const FString& MessageId, const FString& Channel, const FString& Emoji, 
                    const FString& UserId, const FString& UserName);

    void RemoveReaction(const FString& MessageId, const FString& Channel, const FString& Emoji, 
                       const FString& UserId);

    void GetReactions(const FString& MessageId, 
                     TFunction<void(const FString&)> OnSuccess, TFunction<void(const FString&)> OnError);

    // ==================== READ RECEIPT EVENTS ====================

    void MarkRead(const FString& MessageId, const FString& Channel, const FString& UserId, 
                 const FString& UserName);

    void GetUnreadCounts(const FString& UserId, const TArray<FString>& Channels, 
                        TFunction<void(const FString&)> OnSuccess, TFunction<void(const FString&)> OnError);

    void MarkAllRead(const FString& Channel, const FString& UserId);

    // ==================== CHANNEL EVENTS ====================

    void CreateChannel(const FString& Name, const FString& Type, const FString& Description, 
                      const FString& Topic, const FString& CreatedBy, const FString& CreatedByName, 
                      TFunction<void(const FString&)> OnSuccess, TFunction<void(const FString&)> OnError);

    void UpdateChannel(const FString& ChannelId, const TMap<FString, FString>& Updates, 
                      const FString& UserId);

    void ArchiveChannel(const FString& ChannelId, const FString& UserId);

    void InviteToChannel(const FString& ChannelId, const FString& InvitedUserId, 
                        const FString& InvitedUserName, const FString& InvitedBy);

    void RemoveFromChannel(const FString& ChannelId, const FString& RemovedUserId, 
                          const FString& RemovedBy);

    void JoinChannel(const FString& ChannelId, const FString& UserId, const FString& UserName);
    void LeaveChannel(const FString& ChannelId, const FString& UserId);

    void GetChannelMembers(const FString& ChannelId, 
                          TFunction<void(const FString&)> OnSuccess, TFunction<void(const FString&)> OnError);

    // ==================== DIRECT MESSAGE EVENTS ====================

    void CreateDM(const TArray<FString>& UserIds, const FString& Type, 
                 TFunction<void(const FString&)> OnSuccess, TFunction<void(const FString&)> OnError);

    void SendDM(const FString& ConversationId, const FString& Message, const FString& UserId, 
               const FString& UserName);

    void GetDMConversations(const FString& UserId, bool bIncludeArchived, 
                           TFunction<void(const FString&)> OnSuccess, TFunction<void(const FString&)> OnError);

    // ==================== NOTIFICATION EVENTS ====================

    void SubscribeNotifications(const FString& UserId);
    void MarkNotificationRead(const FString& NotificationId, const FString& UserId);
    void MarkAllNotificationsRead(const FString& UserId);
    void ClearNotifications(const FString& UserId);

    void GetNotifications(const FString& UserId, int32 Limit, const FString& Status, 
                         TFunction<void(const FString&)> OnSuccess, TFunction<void(const FString&)> OnError);

    // ==================== PRESENCE EVENTS ====================

    void SetStatus(const FString& UserId, const FString& Status);
    void SetCustomStatus(const FString& UserId, const FString& Emoji, const FString& Text, 
                        const FString& ExpiresAt = TEXT(""));
    void ClearCustomStatus(const FString& UserId);
    void SetDND(const FString& UserId, const FString& Until = TEXT(""));
    void ClearDND(const FString& UserId);
    void StartTyping(const FString& UserId, const FString& Channel);
    void StopTyping(const FString& UserId, const FString& Channel);

    void GetUserPresence(const TArray<FString>& UserIds, 
                        TFunction<void(const FString&)> OnSuccess, TFunction<void(const FString&)> OnError);

    // ==================== MESSAGE EDITING EVENTS ====================

    void EditMessage(const FString& MessageId, const FString& Channel, const FString& NewContent, 
                    const FString& UserId);

    void DeleteMessage(const FString& MessageId, const FString& Channel, const FString& UserId);
    void PinMessage(const FString& MessageId, const FString& Channel, const FString& UserId);
    void UnpinMessage(const FString& MessageId, const FString& Channel, const FString& UserId);

    void GetPinnedMessages(const FString& Channel, 
                          TFunction<void(const FString&)> OnSuccess, TFunction<void(const FString&)> OnError);

    // ==================== SEARCH EVENTS ====================

    void SearchMessages(const FString& Query, const FString& UserId, int32 Limit, 
                       TFunction<void(const FString&)> OnSuccess, TFunction<void(const FString&)> OnError);

    void FilterMessages(const TMap<FString, FString>& Filters, 
                       TFunction<void(const FString&)> OnSuccess, TFunction<void(const FString&)> OnError);

    void SearchInChannel(const FString& Channel, const FString& Query, int32 Limit, 
                        TFunction<void(const FString&)> OnSuccess, TFunction<void(const FString&)> OnError);

    void SearchByUser(const FString& UserId, const FString& Query, int32 Limit, 
                     TFunction<void(const FString&)> OnSuccess, TFunction<void(const FString&)> OnError);

private:
    TSharedPtr<FOddSocketsClient> Client;
    float Timeout;

    void EmitWithResponse(const FString& EventName, const TSharedPtr<FJsonObject>& Data, 
                         const FString& ResponseEvent, 
                         TFunction<void(const FString&)> OnSuccess, 
                         TFunction<void(const FString&)> OnError);

    TSharedPtr<FJsonObject> CreateJsonObject();
};
