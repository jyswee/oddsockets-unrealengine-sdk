# OddSockets Unreal Engine SDK

Official Unreal Engine plugin for OddSockets real-time messaging platform. Blueprint-ready, pub/sub, presence, message history.

## Install

Copy the `Source/OddSockets/` folder into your UE project's Plugins directory.

## Quick Start

```cpp
AOddSocketsClient* Client = GetWorld()->SpawnActor<AOddSocketsClient>();
FOddSocketsConfig Config;
Config.ApiKey = "YOUR_API_KEY";
Client->Initialize(Config);
Client->ConnectAsync();

UOddSocketsChannel* Channel = Client->GetChannel("my-channel");
Channel->OnMessage.AddDynamic(this, &AMyActor::OnMessageReceived);
Channel->SubscribeAsync();
```

## Get a Free API Key

```bash
curl -X POST https://oddsockets.com/api/agent-signup \
  -H "Content-Type: application/json" \
  -d '{"email": "you@example.com", "agentName": "my-agent", "platform": "unreal"}'
curl -X POST https://oddsockets.com/api/agent-signup/verify \
  -H "Content-Type: application/json" \
  -d '{"email": "you@example.com", "code": "123456", "agentName": "my-agent"}'
```

## Plans

| | Free | Starter | Pro |
|---|---|---|---|
| **Price** | $0/mo | $49.99/mo | $299/mo |
| **MAU** | 100 | 1,000 | 50,000 |
| **Concurrent connections** | 50 | 1,000 | Unlimited |
| **Messages/day** | 10,000 | 4,320,000 | Unlimited |
| **Channels** | 10 | Unlimited | Unlimited |
| **Storage** | 100MB (24h) | 50GB (6 months) | Unlimited |

## Support

- [Documentation](https://docs.oddsockets.com/sdks/unreal)
- [Issue Tracker](https://github.com/jyswee/oddsockets-unrealengine-sdk/issues)
- [Email Support](mailto:support@oddsockets.com)

## License

MIT License - Copyright (c) 2026 Joe Wee, Tyga.Cloud Ltd. See [LICENSE](LICENSE) for details.
