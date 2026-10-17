# AGENTS.md — working rules for coding agents

Rules for AI agents (and anyone acting as one) working in this repository.
Project-specific details live in the repo's own documentation (see
PRODUCT.md / ARCHITECTURE.md) — read them before touching code.

## Commit policy

- **Commit and push only on the user's explicit demand.** Finishing a
  task or passing tests is never consent to commit.
- **Sole exception — quality-check rounds**: when the user asks for a
  quality check (or when the quality-fix loop below is running), the agent
  is autonomous: it commits and pushes its fixes on its own so the fresh
  analyses (Sonar, CodeQL) run, without asking each time.
- If the user asks to hold for local testing, report "done, ready to test"
  and stop — don't ask again; wait for an explicit go.
- Conventional-commit style, English (`feat:`, `fix:`, `refactor:`,
  `docs:` …), body bullets explaining the why. Feature work on `feat/*`
  branches opened as PRs. Quality-fix iterations on the same branch/PR.
- When the working tree contains files the agent did not create, inspect
  them and say so before staging everything.

## Agent-local files are never committed

- **Never stage or commit agent-specific directories and files**
  (`.zcode/`, `.claude/`, `.agents/`, `.cursor/`, `.aider*`, and the like).
  They are machine-local configuration, not project content. The repo
  `.gitignore` covers them; if it doesn't yet, propose adding it rather
  than committing these paths.

## Quality gates

Before calling implementation work done, check all three sources and fix
what they report:

1. **SonarCloud / SonarQube** — quality gate passing; no new bugs,
   vulnerabilities, or smells on changed code.
2. **CodeQL** — no new code-scanning alerts.
3. **PR comments** — bot and human review comments addressed.

## Iteration policy

Fix what was found, push, wait for fresh analyses, then re-check all three
sources — repeat until clean.

**During these fix/verify rounds the agent is autonomous**: it commits and
pushes each fix itself (this is the only case where committing without an
explicit user demand is allowed — see Commit policy), within the
quality-check scope only. It does not use that autonomy to commit anything
unrelated to the findings.

**Stop rule: 3 iterations maximum, autonomously.** After 3 fix/verify
iterations, stop and report remaining findings and what was tried; wait for
the user's decision.

## Testing stance

- Do not build heavy test suites unless asked. Verify by compiling,
  building, and exercising the real endpoints/pages.
- When the user says to do fewer tests and move on, move on — don't stall
  on ceremony.

## Workflow

- If the repo uses OpenSpec (`openspec/`), feature work goes through it:
  `/opsx:propose` (planning only — never implement in the same turn),
  `/opsx:apply` (task-by-task, tick checkboxes), `/opsx:archive` when done.
  Spec scenarios are the acceptance criteria.
- **When OpenSpec is deployed in a repo, documentation is part of the
  workflow**: planning artifacts, spec updates, and the PRODUCT.md /
  ARCHITECTURE.md changes implied by a change are maintained as the change
  progresses — not deferred to "later". `/opsx:archive` is only done once
  the documentation reflects the implemented behavior.

## Product & architecture documentation

**`PRODUCT.md` and `ARCHITECTURE.md` are mandatory in every repository and
must be created and kept up to date as part of the development process —
not as an afterthought.**

- If either file is missing, create it as part of the first task that
  touches the repo:
  - `PRODUCT.md` — what the project is, who it's for, what it must and
    must not do (scope, key features, constraints).
  - `ARCHITECTURE.md` — how it is built: components, data flow, tech
    choices (with brief rationale), deployment, and quality/CI setup.
- **Maintain them continuously**: any feature, refactor, or infrastructure
  change that alters behavior, structure, or deployment includes the
  corresponding doc update in the same change — same PR, same commit
  series. Documentation drift is treated as incomplete work.
- When starting a task, read both files first; if the code and the docs
  disagree, surface the discrepancy to the user instead of silently
  trusting either one.

## Documentation edits need approval

`AGENTS.md`, `PRODUCT.md`, and `ARCHITECTURE.md` are never edited silently
beyond the upkeep duty above: substantive changes (new decisions, scope
changes, removed sections) are proposed to the user and applied after
approval. Routine sync of facts that the approved change already implies
(e.g. documenting the feature being merged) goes in directly.

## Keeping this file honest

Delete any section above that doesn't apply to this repository, and add
repo-specific sections (stack conventions, build/test commands) below.
