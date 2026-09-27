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
Channel->SubscribeAsync();
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
