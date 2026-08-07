#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ManagerDiscovery.generated.h"

/**
 * Manager Discovery
 *
 * Resolves the manager endpoint that assigns a worker. There is deliberately
 * no fallback: an explicitly configured manager is used verbatim, and if it is
 * unreachable the connection fails. Silently substituting the default manager
 * would let a client aimed at a broken or non-production endpoint appear to
 * work, hiding both misconfiguration and outages.
 */
UCLASS(BlueprintType)
class ODDSOCKETS_API UManagerDiscovery : public UObject
{
    GENERATED_BODY()

public:
    /** Manager used when none was configured at all. */
    static const FString DefaultManagerUrl;

    /**
     * Resolve the manager URL to use.
     *
     * The default applies only when ConfiguredUrl is empty — it is never used
     * to recover from a configured-but-failing manager.
     *
     * @param ConfiguredUrl Manager URL from FOddSocketsConfig (may be empty)
     * @param OutError      Set when the configured URL is malformed
     * @return Normalised manager URL, or an empty string when OutError is set
     */
    UFUNCTION(BlueprintCallable, Category = "OddSockets Manager Discovery")
    static FString GetManagerUrl(const FString& ConfiguredUrl, FString& OutError);
};
