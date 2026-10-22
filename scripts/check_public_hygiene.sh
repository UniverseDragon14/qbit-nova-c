#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

failures=0
block() {
  printf 'BLOCKED: tracked public-release artifact: %s\n' "$1" >&2
  failures=1
}

while IFS= read -r -d '' path; do
  base="${path##*/}"
  case "$base" in
    .env|.env.*)
      case "$base" in
        .env.example|.env.sample|.env.template) ;;
        *) block "$path" ;;
      esac
      ;;
  esac

  case "$path" in
    build/*|*/build/*|logs/*|*/logs/*|.qcpu/*|*/.qcpu/*|.safe_patch_backup/*|*/.safe_patch_backup/*|stable_backups/*|*/stable_backups/*)
      block "$path"
      ;;
  esac

  case "$base" in
    *.pem|*.key|*.p12|*.pfx|id_rsa|id_ed25519|*.sqlite|*.sqlite3|*.db|*.log|*.pyc)
      block "$path"
      ;;
  esac
done < <(git ls-files -z)

if (( failures != 0 )); then
  printf '%s\n' 'FAIL: PUBLIC_REPOSITORY_HYGIENE' >&2
  exit 1
fi

printf '%s\n' 'PASS: PUBLIC_REPOSITORY_HYGIENE'
