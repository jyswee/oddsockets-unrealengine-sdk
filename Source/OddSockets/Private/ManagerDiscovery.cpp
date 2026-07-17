/**
 * OddSockets UE SDK - Manager Discovery Implementation
 */

#include "ManagerDiscovery.h"

const FString UManagerDiscovery::DefaultManagerUrl = TEXT("https://connect.oddsockets.tyga.network");

FString UManagerDiscovery::GetManagerUrl()
{
    return DefaultManagerUrl;
}

void UManagerDiscovery::ClearCache()
{
    // No cache in simplified version
}
