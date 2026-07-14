<div align="center"><img src="assets/sawyer-superc.svg" alt="Sawyer SuperC" width="600"></div>

# Sawyer SuperC

[![License: Apache 2.0](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)

**Native MoE inference. Single binary. Zero deps.**

Sawyer SuperC is the C-powered upgrade to [Sawyer](https://github.com/drc10101/sawyer-network). Same network, same router, same token economics -- but the local inference engine runs in pure C with int8/int4 kernels, MLA weight absorption, MTP speculative decoding, and DSA sparse attention.

The engine is based on [Colibri](https://github.com/JustVugg/colibri) by Vincenzo/JustVugg, used under Apache 2.0 with gratitude. What Colibri proved -- that a 744B MoE model can run on a 25GB consumer machine by streaming experts from disk -- SuperC extends to the network. When your local cache is cold, the Sawyer router finds the expert on another node. When you have spare GPU, you serve experts back and earn tokens.

**The deal:** Code is free. Earn tokens by serving inference to the network. Spend tokens by running inference. If you can't afford tokens, earn them.

---

## Why SuperC

| | Sawyer (Python) | Sawyer SuperC (C) |
|---|---|---|
| Install | `pip install sawyer-core` | Download one binary |
| Dependencies | Python 3.11+, pip, venv | None |
| Engine | llama.cpp / vLLM | Colibri-derived C engine |
| Cold decode | Depends on backend | ~0.05-0.1 tok/s (disk-bound) |
| Warm decode | Depends on backend | 4+ tok/s (pinned experts) |
| MTP speculation | No | Yes (2.2-2.8 tok/forward) |
| Network routing | Yes | Yes |
| Token earnings | Yes | Yes |

SuperC is for people who want native speed and zero setup. Sawyer (Python) is for people who want the Python ecosystem. Both connect to the same network.

---

## Get Started

Download the binary for your platform:

```bash
# Linux
curl -fsSL https://github.com/drc10101/sawyer-superc/releases/latest/download/superc-linux-x64 -o superc
chmod +x superc

# macOS
curl -fsSL https://github.com/drc10101/sawyer-superc/releases/latest/download/superc-macos-arm64 -o superc
chmod +x superc

# Windows (PowerShell)
irm https://github.com/drc10101/sawyer-superc/releases/latest/download/superc-windows-x64.exe -OutFile superc.exe
```

Then run:

```bash
superc chat              # Talk to the model locally
superc serve             # Start serving experts, earn tokens
superc run               # Chat + serve at the same time
```

---

## Commands

### Using the Agent

You want to talk to an AI. You don't care about servers.

| Command | What it does |
|---------|-------------|
| `superc chat` | Open a chat session. Talk to the model on your machine. |
| `superc chat --model glm-5.2` | Use a specific model. |
| `superc chat --network` | Chat locally, but fall back to the network for experts you don't have. |
| `superc models` | Show what models are available. |
| `superc bench` | Run a speed test on your hardware. |

### Joining the Network

You have a GPU. You want to earn tokens while it sits idle.

| Command | What it does |
|---------|-------------|
| `superc serve` | Start serving. Your GPU runs expert models for others. You earn tokens. |
| `superc serve --model mixtral` | Serve a specific model's experts. |
| `superc status` | See how many tokens you've served, your uptime, and your earnings. |
| `superc register` | Sign up this machine as a network node. |
| `superc register --gpu` | Sign up and tell the network what GPU you have. |
| `superc download` | Download model files so serving starts faster. |
| `superc provider onboarding <id>` | Connect your bank so you can get paid. |

---

## How It Works

```
[You]
 |
 v
[SuperC Engine]  -- Colibri-derived C kernel
 |               -- int8/int4 dot-product kernels
 |               -- MLA weight absorption
 |               -- MTP speculative decoding
 |               -- DSA sparse attention
 |
 +-- Local disk cache (experts you already have)
 |
 +-- Sawyer Router (experts you don't have)
      |
      +--> [Node: Expert 3]  (RTX 4090, Dallas)
      +--> [Node: Expert 7]  (A100, Frankfurt)
      +--> [Node: Expert 15] (RTX 3060, Tokyo)
      |
      v
[Answer comes back to you]
```

Your machine runs the dense layers locally (attention, shared experts, embeddings -- about 9.9 GB at int4). The sparse experts -- the 21,504 routed slices that only activate ~8 per token per layer -- come from disk if you have them cached, or from the network if you don't.

When you run `superc serve`, the same engine runs in reverse: your GPU processes expert requests from other users, and you earn tokens for each one you complete.

---

## Earning on the Network

| Tier | VRAM | Multiplier | Can Host | Monthly Estimate |
|------|------|-----------|----------|-------------------|
| Tier 1 | 4 GB | 1x | Qwen1.5-MoE experts (0.5 GB) | $15-50 |
| Tier 2 | 8 GB | 2x | + DeepSeek-V2 experts (0.8 GB) | $40-120 |
| Tier 3 | 12 GB | 3x | + Mixtral experts (1.5 GB) | $80-250 |
| Tier 4 | 24 GB+ | 4x | All experts | $200-800+ |

Every quarter, 70% of all subscription revenue goes to providers. The pool is split 90% by throughput (tokens served x tier multiplier) and 10% by uptime. Payouts via Stripe Connect, minimum $10, rollover if below.

A kid with a 1060 can't afford tokens. But they can earn them. That's the point.

---

## Pricing

| Tier | Price | Tokens | Best For |
|------|-------|--------|----------|
| Explorer | Free (14 days) | Unlimited | Try it out |
| Pro | $15/mo | 2M tokens | Development, daily use |
| Pioneer | $40/mo | 5M tokens | Scale, growing teams |
| Enterprise | $200/mo | 10M tokens | Teams, custom deployment |

---

## Connect Other Tools

SuperC exposes an OpenAI-compatible API at `http://localhost:8000/v1`. Anything that talks to OpenAI can talk to SuperC.

```bash
# Hermes
hermes config set model.base_url http://localhost:8000/v1
hermes config set model.provider openai_compatible
hermes config set model.default glm-5.2:cloud

# Claude Code
OPENAI_API_KEY=superc OPENAI_BASE_URL=http://localhost:8000/v1 claude
```

```python
from openai import OpenAI

client = OpenAI(api_key="superc", base_url="http://localhost:8000/v1")
response = client.chat.completions.create(
    model="glm-5.2:cloud",
    messages=[{"role": "user", "content": "Hello"}],
)
```

Works with: Hermes, OpenClaw, Claude Code, Cursor, Continue, Aider, Cline, LangChain, LlamaIndex, CrewAI, AutoGPT, and any OpenAI-compatible client.

---

## Supported Models

| Model | Params | Experts | Active/Token | Q4 Size | Expert Size | Best For |
|-------|--------|---------|-------------|---------|-------------|----------|
| GLM-5.2 | 744B | 21,504 | ~40B | ~370 GB | ~19 MB | Chat, Code, Reasoning |
| Mixtral 8x7B | 46.7B | 8 | 2 | ~24 GB | ~1.5 GB | Chat, Code |
| DeepSeek-V2 Lite | 15.7B | 64 | 6 | ~9 GB | ~0.8 GB | Chat |
| Qwen1.5-MoE | 14.3B | 60 | 4 | ~7 GB | ~0.5 GB | Chat (lightweight) |

---

## Attribution

The local inference engine in SuperC is based on [Colibri](https://github.com/JustVugg/colibri) by Vincenzo/JustVugg, used under the [Apache 2.0 License](LICENSE-COLIBRI). Colibri proved that a 744B MoE model can run on consumer hardware by streaming experts from disk. We're grateful for that work.

Key Colibri techniques used in SuperC:
- Expert streaming from disk with per-layer LRU cache
- int8/int4 integer-dot kernels (AVX2 `maddubs`)
- MLA weight absorption for decode
- MTP speculative decoding (2.2-2.8 tok/forward with int8 head)
- DSA sparse attention
- KV-cache persistence across sessions
- Grammar-forced speculative drafts

The network routing, token economics, and provider layer are from [Sawyer](https://github.com/drc10101/sawyer-network) by InFill Systems, LLC.

---

## Building from Source

```bash
git clone https://github.com/drc10101/sawyer-superc.git
cd sawyer-superc
make              # Builds the superc binary
make test         # Runs self-tests
make install      # Installs to /usr/local/bin
```

Requires: gcc or clang with OpenMP support, make.

---

## License

Apache 2.0. See [LICENSE](LICENSE) for details. The Colibri-derived engine code carries additional attribution in [LICENSE-COLIBRI](LICENSE-COLIBRI).

Code is free. Earning on the network is free. If you've got a GPU, you've got income.