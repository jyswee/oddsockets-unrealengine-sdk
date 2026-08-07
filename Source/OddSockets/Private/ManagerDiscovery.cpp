/**
 * OddSockets UE SDK - Manager Discovery Implementation
 */

#include "ManagerDiscovery.h"

const FString UManagerDiscovery::DefaultManagerUrl = TEXT("https://connect.oddsockets.tyga.network");

FString UManagerDiscovery::GetManagerUrl(const FString& ConfiguredUrl, FString& OutError)
{
    OutError.Empty();

    const FString Url = ConfiguredUrl.IsEmpty() ? DefaultManagerUrl : ConfiguredUrl;

    // Reject anything that is not an absolute http(s) URL up front, rather than
    // letting a malformed value surface later as a confusing request error.
    if (!Url.StartsWith(TEXT("http://")) && !Url.StartsWith(TEXT("https://")))
    {
        OutError = FString::Printf(TEXT("Invalid ManagerUrl: %s (expected http:// or https://)"), *Url);
        return FString();
    }

    const FString Authority = Url.RightChop(Url.Find(TEXT("://")) + 3);
    if (Authority.IsEmpty() || Authority.StartsWith(TEXT("/")))
    {
        OutError = FString::Printf(TEXT("Invalid ManagerUrl: %s (missing host)"), *Url);
        return FString();
    }

    FString Normalised = Url;
    while (Normalised.EndsWith(TEXT("/")))
    {
        Normalised.LeftChopInline(1);
    }
    return Normalised;
}
