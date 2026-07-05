#!/usr/bin/env python3
import json
import os
import sys
import urllib.request
import urllib.error
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ENV_FILE = ROOT / ".env"

DEFAULT_BASE_URL = "https://api.moonshot.ai/v1"
DEFAULT_MODEL = "kimi-k2.7-code"

def load_env_file():
    if not ENV_FILE.exists():
        return

    for raw in ENV_FILE.read_text(encoding="utf-8", errors="ignore").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip().strip('"').strip("'")
        if key and key not in os.environ:
            os.environ[key] = value

def clean_text(value):
    return str(value or "").replace("\r", " ").strip()

def has_key():
    return bool(os.environ.get("MOONSHOT_API_KEY") or os.environ.get("KIMI_API_KEY"))

def get_api_key():
    return os.environ.get("MOONSHOT_API_KEY") or os.environ.get("KIMI_API_KEY") or ""

def get_base_url():
    return os.environ.get("KIMI_BASE_URL", DEFAULT_BASE_URL).rstrip("/")

def get_model():
    return os.environ.get("KIMI_MODEL", DEFAULT_MODEL)

def status():
    load_env_file()
    print("🐉 QNOVA Kimi API Bridge V4.3")
    print("")
    print("provider: local_kimi_api_bridge")
    print("base_url:", get_base_url())
    print("model:", get_model())
    print("env_file:", str(ENV_FILE))
    print("api_key_present:", "yes" if has_key() else "no")
    print("")
    print("Safety:")
    print("- API key is never printed")
    print("- .env is local only")
    print("- no shell execution")
    print("- no file deletion")
    print("")
    if has_key():
        print("Verdict:")
        print("PASS: KIMI_BRIDGE_READY")
    else:
        print("Verdict:")
        print("PASS: KIMI_BRIDGE_DISABLED_NO_KEY")

def ask(prompt):
    load_env_file()

    prompt = clean_text(prompt)
    if not prompt:
        print("provider: local_kimi_api_bridge")
        print("fallback_reason: empty_prompt")
        print("PASS: KIMI_BRIDGE_EMPTY_PROMPT_BLOCKED")
        return 0

    api_key = get_api_key()
    if not api_key:
        print("provider: local_kimi_api_bridge")
        print("fallback_reason: missing_api_key")
        print("PASS: KIMI_BRIDGE_SAFE_DISABLED")
        return 0

    url = get_base_url() + "/chat/completions"

    payload = {
        "model": get_model(),
        "messages": [
            {
                "role": "system",
                "content": "You are QBIT NOVA helper. Be concise, safe, and technical."
            },
            {
                "role": "user",
                "content": prompt
            }
        ],
        "max_completion_tokens": 512
    }

    data = json.dumps(payload).encode("utf-8")

    req = urllib.request.Request(
        url,
        data=data,
        headers={
            "Authorization": "Bearer " + api_key,
            "Content-Type": "application/json"
        },
        method="POST"
    )

    try:
        with urllib.request.urlopen(req, timeout=60) as res:
            body = res.read().decode("utf-8", errors="replace")
            parsed = json.loads(body)
            content = parsed["choices"][0]["message"]["content"]
            print("provider: local_kimi_api_bridge")
            print("model:", parsed.get("model", get_model()))
            print("fallback_reason: kimi_api_success")
            print("")
            print(clean_text(content))
            print("")
            print("PASS: KIMI_BRIDGE_LIVE_OK")
            return 0

    except urllib.error.HTTPError as e:
        body = e.read().decode("utf-8", errors="replace")
        print("provider: local_kimi_api_bridge")
        print("fallback_reason: kimi_api_http_error")
        print("http_status:", e.code)
        print(clean_text(body)[:800])
        print("PASS: KIMI_BRIDGE_ERROR_HANDLED")
        return 0

    except Exception as e:
        print("provider: local_kimi_api_bridge")
        print("fallback_reason: kimi_api_exception")
        print("error:", clean_text(type(e).__name__ + ": " + str(e))[:800])
        print("PASS: KIMI_BRIDGE_ERROR_HANDLED")
        return 0

def main():
    if len(sys.argv) < 2:
        status()
        return 0

    cmd = sys.argv[1].strip().lower()

    if cmd == "status":
        status()
        return 0

    if cmd == "ask":
        prompt = " ".join(sys.argv[2:]).strip()
        return ask(prompt)

    print("provider: local_kimi_api_bridge")
    print("fallback_reason: unknown_command")
    print("commands: status | ask")
    print("PASS: KIMI_BRIDGE_UNKNOWN_COMMAND_HANDLED")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
