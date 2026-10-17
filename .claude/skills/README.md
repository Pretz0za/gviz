# gviz project skills

Human-facing index. Agents don't need this file — every skill's `name` +
`description` frontmatter is already surfaced automatically to Claude at the
start of every conversation, and that's what actually drives whether a skill
gets used. This file exists so a person browsing the repo can see what's
here without opening each `SKILL.md`.

- **gviz-force-model** — how to add a new `ForceModel` (repulsion/attraction
  law) that plugs into `ForceDirectedLayoutAlgorithm`. Concept contract,
  Configuration pattern, distance-vector conventions, numerical-safety
  idioms, testing.
- **gviz-debug** — diagnosing runtime/visual bugs in the layout engine and
  renderer: screenshots, camera lock, and charting any `ComponentPool` over
  time via `GVIZ_DEBUG_CHARTS`.

## Conventions for adding a new skill

- Directory: `.claude/skills/<name>/SKILL.md`, name prefixed `gviz-` for
  anything specific to this codebase (as opposed to a general-purpose skill).
- Always include YAML frontmatter with `name` and `description` — the
  description is the entire triggering mechanism, so make it specific: name
  concrete trigger phrases, file paths, and symbols, not just a topic label.
  See either existing skill for the bar.
- One skill per *pattern*, not per file. `gviz-force-model` covers every
  force model, not one skill per algorithm variant. Split into a new skill
  only when the audience/trigger context is genuinely different (e.g.
  debugging vs. authoring).
- Update this README's bullet list when you add or retire a skill — it's
  documentation, not a mechanism, so nothing breaks if it goes stale, but
  keep it honest anyway.
