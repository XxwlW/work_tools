# Workflow Miner

Workflow Miner is a Codex skill for identifying repeated work patterns from user-provided conversations, task records, project notes, or exported context.

It helps decide which repeated tasks are worth turning into reusable prompt templates, workflows, Codex skills, agent roles, or automations.

The goal is not to summarize everything. The goal is to find work that is repeated, costly, fragile, context-heavy, or easy to standardize.

## Use Cases

Use Workflow Miner when you want to:

- Review recent AI-assisted work and find recurring patterns.
- Decide which tasks should become reusable prompts or skills.
- Convert repeated manual workflows into clearer procedures.
- Identify existing workflows that should be extended instead of recreated.
- Build a private or team-specific library of reusable AI workflows.
- Avoid creating oversized or low-value automations.

## What It Looks For

Workflow Miner scans for repeated:

- Tasks the user asks for multiple times.
- Corrections the user gives more than once.
- Output formats, tone preferences, and acceptance criteria.
- Tool-specific procedures that are easy to forget or damage.
- Manual steps that could become a prompt, workflow, skill, agent, or automation.

## Repository Structure

```text
workflow-miner/
├── SKILL.md
├── agents/
│   └── openai.yaml
├── scripts/
│   └── mine_patterns.py
├── prompt.md
└── README.md
```

## Install As A Codex Skill

Clone this repository into your Codex skills directory:

```bash
git clone https://github.com/your-name/workflow-miner.git ~/.codex/skills/workflow-miner
```

Or copy an existing checkout:

```bash
cp -R workflow-miner ~/.codex/skills/workflow-miner
```

Then restart Codex so the skill can be discovered.

## Use The Skill

After installation, invoke it like:

```text
Use $workflow-miner to review these exported task notes and suggest which repeated workflows should become prompts, skills, agents, or automations.
```

You can provide:

- Conversation exports.
- Task logs.
- Project notes.
- Existing prompt libraries.
- Existing skill or agent descriptions.
- A short manually written summary of repeated work.

## Optional Script

The bundled script provides a first-pass scan over user-provided text files:

```bash
python3 scripts/mine_patterns.py path/to/history.md
```

Scan a directory:

```bash
python3 scripts/mine_patterns.py path/to/exports/
```

Emit JSON:

```bash
python3 scripts/mine_patterns.py path/to/exports/ --json
```

The script is only a starting point. Review the evidence manually before creating reusable assets.

## Prompt-Only Version

If you do not want to install a Codex skill, use [prompt.md](Prompt.md) directly in any AI assistant.

The prompt-only version is useful when:

- You are using an assistant that does not support Codex skills.
- You want to quickly test the workflow before installing it.
- You want to paste the prompt into a temporary chat.
- You want a portable version for another AI tool.

The prompt-only version has the same core behavior as the skill:

- It reviews only the material the user provides or authorizes.
- It identifies repeated work patterns.
- It recommends the smallest useful package: prompt, workflow, skill, agent, automation, or skip.
- It asks for evidence and confidence before generating reusable assets.
- It includes privacy checks before turning private history into reusable public material.

You can use it like this:

```text
Paste the prompt from prompt.md, then attach or paste conversation exports, task notes, or project records.
```

For convenience, here is a compact English version:

```text
You are Workflow Miner.

Your job is to review the user-provided conversations, task records, project notes, or exported context and identify repeated work patterns that are worth standardizing.

Do not summarize everything. Focus on repeated, costly, fragile, context-heavy, or easy-to-standardize work.

Privacy rules:
- Use only the material the user provides or authorizes.
- Do not expose raw private conversation snippets, names, local paths, secrets, client names, internal project names, URLs, tokens, or identifiers in public artifacts.
- Redact examples before turning them into reusable templates.
- Mark weak evidence as "inferred".
- Do not fabricate history when context is missing.

Look for repeated:
- Tasks the user asks for multiple times.
- Corrections the user gives more than once.
- Output formats, tone preferences, and acceptance criteria.
- Tool-specific procedures that are easy to forget or damage.
- Manual steps that could become a prompt, workflow, skill, agent, or automation.
- Existing prompts, templates, skills, agents, or automations that should be extended instead of recreated.

Recommend a reusable workflow only when most of these are true:
- It appeared at least twice, or future repetition is very likely and costly.
- Inputs are reasonably stable.
- Steps can be reused.
- The output or completion standard is clear.
- Standardization would improve speed, quality, consistency, or reliability.
- Existing reusable assets do not already cover it well.
- It can be described without exposing private details.

Choose the smallest useful package:
- Prompt template: simple one-shot tasks with predictable inputs.
- Workflow or skill: multi-step tasks with validation, tools, or reusable procedure.
- Agent role: bounded delegated work that needs judgment over a domain.
- Automation: recurring checks, reports, reminders, monitors, or scheduled work.
- Skip: one-off, vague, sensitive, low-evidence, or already covered.

Process:
1. Build an evidence map from the provided material.
2. Group repeated work by intent, tool/carrier, output format, and failure mode.
3. Generate a candidate list first; do not immediately write full assets for every idea.
4. Filter hard for high-value, clear-boundary, enough-evidence candidates.
5. Prefer extending existing assets over creating duplicates.
6. Draft concrete reusable assets only for the strongest candidates.
7. Check the final output for private details before returning it.

Candidate format:

## Candidate Workflows

1. Repeated workflow:
   Evidence:
   Frequency / confidence:
   Recommended package:
   Worth creating:
   Reason:

Final format:

## Recommended To Create Or Extend

1.

## Skipped

1.

## Needs More Evidence

1.

If the user asks for concrete assets, include sanitized prompt templates, workflow drafts, skill drafts, agent briefs, or automation specs after the recommendation list.

Be concise and direct. Write like a practical workflow review, not a consulting report.
```

## Privacy Notes

This skill is designed to work from user-provided material. It should not require private memory access, hidden logs, or account-specific paths.

Before publishing any generated workflow or skill, check for:

- Personal names.
- Local filesystem paths.
- Private project names.
- Internal URLs.
- Access tokens or API keys.
- Raw conversation quotes.
- Client or company identifiers.

## License

Choose a license before publishing. MIT is a common default for small open-source utility skills.
