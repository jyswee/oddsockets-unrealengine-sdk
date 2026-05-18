#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Engine.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "OddSocketsTypes.h"
#include "ManagerDiscovery.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnConnectivityTestComplete, bool, bIsReachable);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnManagerInfoReceived, const FOddSocketsWorkerAssignmentInfo&, ManagerInfo);

/**
 * Manager discovery information structure
 */
USTRUCT(BlueprintType)
struct ODDSOCKETS_API FOddSocketsManagerInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Manager Info")
    FString Version;

    UPROPERTY(BlueprintReadOnly, Category = "Manager Info")
    FString Status;

    UPROPERTY(BlueprintReadOnly, Category = "Manager Info")
    int32 ActiveWorkers;

    UPROPERTY(BlueprintReadOnly, Category = "Manager Info")
    FString ManagerUrl;

    FOddSocketsManagerInfo()
    {
        Version = TEXT("");
        Status = TEXT("");
        ActiveWorkers = 0;
        ManagerUrl = TEXT("");
    }
};

/**
 * Manager Discovery Service
 * 
 * Handles discovery of OddSockets manager endpoints and provides
 * connectivity testing functionality. Implements simplified discovery
 * that always returns the main manager endpoint for compatibility.
 */
UCLASS(BlueprintType, Blueprintable)
class ODDSOCKETS_API UManagerDiscovery : public UObject
{
    GENERATED_BODY()

public:
    UManagerDiscovery();

    // Singleton access
    UFUNCTION(BlueprintCallable, Category = "OddSockets Manager Discovery", CallInEditor = true)
    static UManagerDiscovery* Get();

    // Public Methods
    UFUNCTION(BlueprintCallable, Category = "OddSockets Manager Discovery")
    FString GetManagerUrl(const FString& ApiKey);

    UFUNCTION(BlueprintCallable, Category = "OddSockets Manager Discovery")
    void TestConnectivityAsync(const FString& ApiKey, const FOnConnectivityTestComplete& OnComplete);

    UFUNCTION(BlueprintCallable, Category = "OddSockets Manager Discovery")
    void GetManagerInfoAsync(const FString& ApiKey, const FOnManagerInfoReceived& OnComplete);

    UFUNCTION(BlueprintPure, Category = "OddSockets Manager Discovery")
    bool IsManagerReachable() const { return bLastConnectivityTest; }

    UFUNCTION(BlueprintPure, Category = "OddSockets Manager Discovery")
    FString GetDefaultManagerUrl() const;

    // Configuration
    UFUNCTION(BlueprintCallable, Category = "OddSockets Manager Discovery")
    void SetCustomManagerUrl(const FString& CustomUrl);

    UFUNCTION(BlueprintCallable, Category = "OddSockets Manager Discovery")
    void ResetToDefaultManagerUrl();

private:
    // Singleton instance
    static UManagerDiscovery* Instance;

    // Private fields
    UPROPERTY()
    FString CustomManagerUrl;

    UPROPERTY()
    bool bLastConnectivityTest;

    UPROPERTY()
    FOddSocketsManagerInfo CachedManagerInfo;

    UPROPERTY()
    FDateTime LastInfoUpdate;

    // Constants
    static const FString DEFAULT_MANAGER_URL;
    static const int32 INFO_CACHE_DURATION_SECONDS;

    // Private methods
    void OnConnectivityTestResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FOnConnectivityTestComplete OnComplete);
    void OnManagerInfoResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FOnManagerInfoReceived OnComplete);
    bool IsInfoCacheValid() const;
    FString BuildManagerUrl(const FString& ApiKey) const;
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> CreateHttpRequest(const FString& Url, const FString& ApiKey) const;
};
