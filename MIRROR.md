 Sawyer SuperC Model Mirror

Download GLM-5.2 from our mirror instead of HuggingFace. Faster and always available.

**Direct download:**

```bash
# Option 1: Git LFS (recommended)
git lfs install
git clone https://mirror.infill.systems/GLM-5.2-colibri-int4-with-int8-mtp

# Option 2: HuggingFace CLI
huggingface-cli download mateogrgic/GLM-5.2-colibri-int4-with-int8-mtp \
  --local-dir ./GLM-5.2-colibri-int4-with-int8-mtp \
  --endpoint https://mirror.infill.systems

# Option 3: Direct HTTPS (curl/wget)
curl -L -O https://mirror.infill.systems/GLM-5.2-colibri-int4-with-int8-mtp/config.json
```

**After downloading, set the model path:**

```bash
export COLI_MODEL=/path/to/GLM-5.2-colibri-int4-with-int8-mtp
```

**Mirror status:** The mirror is synced from the HuggingFace source repository. Files are served at datacenter speeds from Hetzner (1Gbps+).