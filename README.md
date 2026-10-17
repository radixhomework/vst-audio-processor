# default-template

Base template for new **RadixHomeWork** repositories.

## What it seeds

| File | Purpose |
|---|---|
| `AGENTS.md` | Working rules for AI coding agents: commit policy, quality gates, iteration limits, testing stance, documentation upkeep. |
| `PRODUCT.md` | Stub describing *what* the project is — scope, users, constraints. To be filled in during the first task. |
| `ARCHITECTURE.md` | Stub describing *how* the project is built — components, data flow, decisions, deployment, CI. To be filled in during the first task. |
| `.gitignore` | Excludes agent-local configuration (`.zcode/`, `.claude/`, `.agents/`, …) so machine-local files never get committed. |

## Usage

1. Click **Use this template** → **Create a new repository**.
2. After the first coding task, ask your agent to fill in the
   `PRODUCT.md` and `ARCHITECTURE.md` stubs with real content.
3. Trim the `AGENTS.md` sections that don't apply to the project
   (the file itself says which and how).

## Notes

- `AGENTS.md`, `PRODUCT.md`, and `ARCHITECTURE.md` are living documents:
  they are maintained in the same change that alters behavior, structure,
  or deployment — see `AGENTS.md`, "Product & architecture documentation".
- `openspec/` and `AGENTS.md` are project content and **should** be
  committed; only agent-local directories are ignored.
