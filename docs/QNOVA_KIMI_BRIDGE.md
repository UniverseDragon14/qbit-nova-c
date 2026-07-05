# QNOVA Kimi API Bridge V4.3

This bridge connects QBIT NOVA C tooling to Kimi / Moonshot API without exposing secrets.

## Files

- `scripts/qnova_kimi_bridge.py`
- `scripts/test_kimi_bridge.sh`

## Environment

Use `.env` locally.

Required:

`MOONSHOT_API_KEY=your_key_here`

Optional:

`KIMI_BASE_URL=https://api.moonshot.ai/v1`
`KIMI_MODEL=kimi-k2.7-code`

## Commands

Status:

`python3 scripts/qnova_kimi_bridge.py status`

Ask:

`python3 scripts/qnova_kimi_bridge.py ask "Explain Bell state shortly"`

## Safety

- API key is never printed.
- `.env` must not be committed.
- The bridge does not execute shell commands.
- The bridge does not delete files.
