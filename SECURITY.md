# Security Policy

## Scope

Security reports are welcome for the QBIT NOVA C source, its public proof/demo scripts, the workload-admission interface, the userspace QCPU protocol, or release automation.

The project is a software Virtual QCPU running on classical hardware. The approval gate and bounded-I/O tests are not a guarantee of complete isolation from a hostile local user, operating system, or process. Do not run unreviewed workloads with elevated privileges. The public Build Week demo is designed not to require root, load a kernel module, create device nodes, access GPIO, or expose a TCP/UDP listener.

## Reporting a vulnerability

- Do **not** publish passwords, API tokens, private keys, device credentials, personal logs, or exploit details in a public issue or pull request.
- Use GitHub's **Security → Report a vulnerability** private-reporting flow when it is enabled for this repository.
- If private reporting is not available, open a brief public issue asking the maintainer for a private reporting channel. Do not include the sensitive details in that issue.
- Include the affected commit or file, impact, reproducible steps, and a proposed mitigation when safe to do so.

There is no guaranteed response-time SLA. Please allow the maintainer time to validate the report before public disclosure.

## Repository hygiene

Do not commit local `.env` files, `.qcpu/` runtime state, `build/` or `logs/` output, backups, compiled binaries, device dumps, or private keys. The CI public-hygiene check is a safety net, not a substitute for reviewing staged changes.
