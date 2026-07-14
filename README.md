<div align="center"><img src="assets/sawyer-superc.svg" alt="Sawyer SuperC" width="600"></div>

# Sawyer SuperC

[![License: Apache 2.0](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)

**8GB of RAM. A 744-billion-parameter model. For real.**

Sawyer SuperC runs the biggest AI models on the weakest hardware by doing what no single machine can do alone -- sharing the load across a network of computers, each one handling just the pieces it can hold.

You don't need a data center. You don't need a GPU. You need 8GB of RAM and an internet connection.

---

## The idea in 30 seconds

Big AI models are called "MoE" -- Mixture of Experts. Only a few pieces activate per word you type. The rest sit on disk until they're needed.

Sawyer SuperC keeps the shared pieces in your RAM (about 9.9 GB at int4). The expert pieces -- 21,504 of them, each about 19 MB -- stream from disk if you have them, or from the network if you don't. Your machine runs the parts it can. Other machines run the parts you can't.

If you have a GPU, you earn tokens by running experts for other people. If you don't have a GPU, you spend tokens to use the network. Either way, it works.

**The deal: code is free. Earn tokens by serving. Spend tokens by chatting. If you can't afford tokens, earn them.**

---

## What it does

```
Your laptop (8GB RAM)
  |
  |--- Runs dense layers locally (9.9 GB int4, fits in RAM)
  |
  |--- Needs expert #47? Check local disk cache.
  |      Got it? Use it.
  |      Don't got it? Ask the Sawyer network.
  |                     |
  |                     +--> Some kid in Mumbai has expert #47
  |                     +--> A miner in Texas has expert #47
  |                     +--> They run it, send it back, you see the answer
  |
  v
Answer on your screen.
```

---

## Get Started

Download one file. No Python. No pip. No dependencies.

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

Then:

```bash
superc chat              # Talk to the model
superc serve             # Start earning tokens with your GPU
superc run               # Chat + earn at the same time
```

---

## Using the Agent

You want to talk to an AI. You don't care about how it works.

| Command | What it does |
|---------|-------------|
| `superc chat` | Talk to the model. Opens a chat in your terminal. |
| `superc chat --network` | Same thing, but pulls experts from the network when your cache is cold. |
| `superc chat --model glm-5.2` | Use a specific model. |
| `superc models` | Show what models are available. |
| `superc bench` | Run a speed test. See how fast your machine is. |

---

## Earning on the Network

You have a GPU sitting idle. You want to make money with it.

| Command | What it does |
|---------|-------------|
| `superc serve` | Start serving. Your GPU runs expert pieces for other people. You earn tokens. |
| `superc serve --model mixtral` | Serve a specific model's experts. |
| `superc status` | See how many tokens you've served, your uptime, and your earnings. |
| `superc register --gpu` | Sign up and tell the network what GPU you have. |
| `superc download` | Download model files so serving starts faster. |
| `superc provider onboarding <id>` | Connect your bank so you can get paid. |

### How much can you earn?

| Your GPU | VRAM | What you can host | What you can earn |
|----------|------|-------------------|--------------------|
| GTX 1060 | 4 GB | Small experts | $15-50/month |
| RTX 3060 | 8 GB | Medium experts | $40-120/month |
| RTX 4070 | 12 GB | Most experts | $80-250/month |
| RTX 4090 | 24 GB | All experts | $200-800/month |

70% of all subscription revenue goes to providers. The pool is split 90% by how much you compute, 10% by how long you're online. Payouts every quarter via Stripe. Minimum $10, below that rolls over.

---

## How it works (the simple version)

A 744-billion-parameter model sounds impossible. It's not. Here's why:

The model has 21,504 expert pieces. For every word you type, only about 8 of them activate. That's roughly 19 MB of expert data per word -- your SSD can handle that.

But if your SSD is slow, or you don't have the expert at all, the Sawyer network finds someone who does. Your laptop runs the shared part. The network runs the specialist part. You get the answer.

No single computer needs to hold the whole model. That's the point.

---

## How it works (the technical version)

- **Engine:** Based on [Colibri](https://github.com/JustVugg/colibri) by Vincenzo/JustVugg (Apache 2.0). Pure C, zero dependencies, int8/int4 integer-dot kernels with AVX2, MLA weight absorption, DSA sparse attention.
- **MTP speculative decoding:** Drafts 2-3 tokens per forward pass. Needs int8 draft head (int4 gives 0% acceptance -- use the int8 model files).
- **Expert streaming:** 21,504 experts live on disk (~370 GB total). Per-layer LRU cache keeps the hot ones in RAM. Cold ones stream from disk, or from the Sawyer network if you're connected.
- **Network routing:** When your local cache misses, the Sawyer router finds the nearest node that has that expert. Latency target: under 100ms for a network expert fetch.
- **Token economics:** Same as Sawyer. 70% of revenue to providers. 4 hardware tiers. Quarterly payouts.
- **KV cache:** Compressed MLA format, 576 floats/token instead of 32,768. Persists across sessions so conversations reopen warm.

---

## For anyone with 8 GB of RAM

You don't have a GPU. Most AI tools won't even install on your machine. Here's what happens when you run `superc chat --network`:

1. SuperC loads the shared layers into your 8 GB. It fits.
2. You type a question. The model activates 8 experts per layer per token.
3. If an expert is in your disk cache, it runs locally. Fast.
4. If it's not cached, SuperC asks the Sawyer network. Someone else's GPU runs it and sends the result back. Takes ~100ms.
5. You see the answer. Same quality as running on a data center. Your machine did the easy part. The network did the hard part.
6. If you ever get a GPU, you switch from spending tokens to earning them. Same software. Just run `superc serve` instead of `superc chat`.

---

## Pricing

| Tier | Price | Tokens | Best For |
|------|-------|--------|----------|
| Explorer | Free (14 days) | Unlimited | Try it out |
| Pro | $15/mo | 2M tokens | Daily use |
| Pioneer | $40/mo | 5M tokens | Growing teams |
| Enterprise | $200/mo | 10M tokens | Teams, custom setup |

14-day free trial. No credit card. Then pick a plan. Or just serve and earn enough tokens to never pay.

---

## Connect other tools

SuperC gives you an OpenAI-compatible API at `http://localhost:8000/v1`. Anything that talks to OpenAI can talk to SuperC.

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

## Attribution

The local inference engine is based on [Colibri](https://github.com/JustVugg/colibri) by Vincenzo/JustVugg, used under the [Apache 2.0 License](LICENSE-COLIBRI). Colibri proved that a 744B MoE model can run on consumer hardware. We're grateful for that work.

The network routing, token economics, and provider layer are from [Sawyer](https://github.com/drc10101/sawyer-network) by InFill Systems, LLC.

---

## Building from source

```bash
git clone https://github.com/drc10101/sawyer-superc.git
cd sawyer-superc
make
./superc selftest
```

Requires: gcc or clang with OpenMP, make.

---

## License

Apache 2.0. See [LICENSE](LICENSE) and [LICENSE-COLIBRI](LICENSE-COLIBRI).

Code is free. Earning on the network is free. If you've got hardware, you've got income.