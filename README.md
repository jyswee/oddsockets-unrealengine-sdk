# OddSockets Unreal Engine SDK

Official Unreal Engine plugin for OddSockets real-time messaging platform. Blueprint-ready, pub/sub, presence, message history.

## Install

Copy this plugin folder (containing `OddSockets.uplugin` and the `Source/` directory)
into your UE project's `Plugins/OddSockets/` directory, then regenerate your project
files and enable the **OddSockets** plugin in the editor (Edit → Plugins → Networking).

## Quick Start

```cpp
AOddSocketsClient* Client = GetWorld()->SpawnActor<AOddSocketsClient>();
FOddSocketsConfig Config;
Config.ApiKey = "YOUR_API_KEY";
Client->Initialize(Config);
Client->ConnectAsync();

UOddSocketsChannel* Channel = Client->GetChannel("my-channel");
Channel->OnMessage.AddDynamic(this, &AMyActor::OnMessageReceived);
Channel->OnPresenceChange.AddDynamic(this, &AMyActor::OnRosterChanged); // joins/leaves
Channel->OnHistory.AddDynamic(this, &AMyActor::OnHistoryLoaded);       // GetHistoryAsync results
Channel->SubscribeAsync();
```

## Game Features

Scored challenges, leaderboards, achievements and directed 1:1 invites, with
Unity-parity method names (`*Async`) so a project ported from the Unity SDK maps
calls and events 1:1.

```cpp
UOddSocketsEnhancedFeatures* Game = NewObject<UOddSocketsEnhancedFeatures>();
Game->Initialize(Client);

// Typed events (Blueprint-assignable), raw JSON payloads
Game->OnChallengeInvited.AddDynamic(this, &AMyActor::OnInvited);
Game->OnLeaderboardRankChange.AddDynamic(this, &AMyActor::OnRankChange);
Game->OnStandingsData.AddDynamic(this, &AMyActor::OnStandings);

// Versus flow: invite -> reply -> ranked challenge -> progress -> standings
// Payloads are JSON object strings and travel as objects (Unity-compatible).
Game->SendChallengeInviteAsync("player-2", "match", TEXT("{\"arena\":\"vs\"}"));
// Reply: (inviteId, accept, payload, reason) — payload reaches the inviter
// as "replyPayload" on OnChallengeReplyReceived, reason rides "reason".
Game->ReplyChallengeInviteAsync(InviteId, true, TEXT("{\"loadout\":\"axe\"}"));
Game->CreateChallengeAsync("harvest-league", "score", /*bRanked=*/true);
Game->ReportProgressAsync("harvest-league", 250.f);
Game->GetStandingsAsync("harvest-league", 10);
Game->UnlockAchievementAsync("first-win", "First Win", "bronze");
```

Raw named events ride `Client->EmitAsync(EventName, JsonPayload)`; replies and
broadcasts arrive on the typed delegates above or the generic
`Client->OnEnhancedEvent`.

## Token Auth

Game clients can authenticate with a short-lived minted realtime token instead
of an API key (keep keys off player devices):

```cpp
FOddSocketsConfig Config;
Config.Token.Token = MintedToken;        // from your server's /v1/token exchange
Config.Token.ExpiresAt = MintedExpiresAt;
Config.UserId = PlayerId;
```

For a shipping game, set a token provider instead of a static token: it is
called before every (re)connect and again `Config.TokenRefreshLeadSeconds`
before expiry, so a reconnect never reuses a dead token.

```cpp
// C++: fetch a fresh token, then hand it to the callback (game thread).
Client->TokenProvider = [this](TFunction<void(const FOddSocketsToken&)> OnToken)
{
    MintRealtimeToken([OnToken](const FString& Token, const FString& ExpiresAt)
    {
        FOddSocketsToken Fresh; Fresh.Token = Token; Fresh.ExpiresAt = ExpiresAt;
        OnToken(Fresh);
    });
};

// Blueprint: bind OnTokenRequested and answer each firing with ProvideToken.
```

## Get an API Key

```bash
curl -X POST https://oddsockets.com/api/agent-signup \
  -H "Content-Type: application/json" \
  -d '{"email": "you@example.com", "agentName": "my-agent", "platform": "unreal"}'
curl -X POST https://oddsockets.com/api/agent-signup/verify \
  -H "Content-Type: application/json" \
  -d '{"email": "you@example.com", "code": "123456", "agentName": "my-agent"}'
```

## Plans

No free tier — every plan starts with a 7-day free trial.

| | Starter | Pro | Scale | Enterprise |
|---|---|---|---|---|
| **Price** | $29/mo | $99/mo | $299/mo | Contact sales |
| **Messages/mo** | 5M | 25M | 100M | Unlimited |
| **Peak connections** | 200 | 1,000 | 5,000 | Unlimited |
| **MAU** | Unlimited | Unlimited | Unlimited | Unlimited |
| **Extra messages** | $2.50/M | $1.60/M | $1.00/M | Included |

Current pricing: [oddsockets.com/#pricing](https://oddsockets.com/#pricing).

## Get Accredited

<a href="https://tyga.games/accreditation"><img src="https://prodmedia.tyga.host/public/tyga.cloud/landing/tyga.games/tygagames-black-words.svg" alt="tyga.games accreditation" height="44"></a>

Prove you can build and operate real-time features on OddSockets — channels, presence, pub/sub, delivery guarantees and production liveops — on the stack itself. Three tiers (**TCU / TCA / TCP**), certified through **tyga.games** and delivered on ClassaaS.

[**Get accredited on tyga.games →**](https://tyga.games/accreditation)

## Support

- [Documentation](https://docs.oddsockets.com/sdks/unreal)
- [Issue Tracker](https://github.com/jyswee/oddsockets-unrealengine-sdk/issues)
- [Email Support](mailto:support@oddsockets.com)

## License

MIT License - Copyright (c) 2026 Joe Wee, Tyga.Cloud Ltd. See [LICENSE](LICENSE) for details.
