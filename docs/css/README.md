# CSS Design Guidelines

`docs/css/` is now the meta design reference for UI work in this repo.

Related guidance:

- `visual-language.md`
  - repo-wide visual design direction for current web surfaces and future native UI

Use these files as the palette and contrast source of truth moving forward:

- `theme-light.css`
  - baseline light theme
- `theme-light-medium-contrast.css`
  - light medium-contrast theme
- `theme-light-high-contrast.css`
  - light high-contrast theme
- `theme-dark.css`
  - baseline dark theme
- `theme-dark-medium-contrast.css`
  - dark medium-contrast theme
- `theme-dark-high-contrast.css`
  - dark high-contrast theme

## Intent

These files define the repo's shared color language using `--md-sys-color-*` tokens.
New UI work should prefer these tokens over ad hoc color picking.

That means:

- start from the documented token set first
- keep component styling mapped to semantic tokens like `primary`, `surface`, `on-surface`, `outline`, and `error`
- treat contrast variants as first-class, not as afterthought overrides

## Naming

Theme files map to these selector names:

- `theme-light.css` -> `.light`
- `theme-light-medium-contrast.css` -> `.light-medium-contrast`
- `theme-light-high-contrast.css` -> `.light-high-contrast`
- `theme-dark.css` -> `.dark`
- `theme-dark-medium-contrast.css` -> `.dark-medium-contrast`
- `theme-dark-high-contrast.css` -> `.dark-high-contrast`

When introducing theme switching, prefer these exact class names so app code and docs stay aligned.

## File Convention

Theme files should follow this naming pattern:

- `theme-<mode>.css`
- `theme-<mode>-medium-contrast.css`
- `theme-<mode>-high-contrast.css`

That keeps them easy to scan and leaves room for future families without shorthand like `mc` or `hc`.

## Usage Rules

- Prefer semantic variables, not hard-coded hex or rgb values, in new UI code.
- Use `surface` and `surface-container*` tokens for large layout areas and cards.
- Use `primary` and `secondary` for emphasis, navigation, and interactive states.
- Use `on-*` tokens for text and icon contrast on top of their paired surfaces.
- Use `outline` and `outline-variant` for borders, dividers, and subtle structure.
- Use `error` tokens only for destructive or failure states.

## Layout Guidance

- Default to light theme unless a product area already establishes another mode.
- Support narrow dock/sidebar widths first, then expand upward for larger surfaces.
- Avoid single fixed-width hero layouts for embedded tools.
- Preserve readable contrast in every mode before adding extra visual complexity.
- Keep spacing, radius, and typography responsive with fluid sizing where practical.

## When Adding UI

Before introducing a new screen or component:

1. choose the closest theme mode from this folder
2. map the component colors to existing `--md-sys-color-*` tokens
3. verify the same structure still reads well in medium and high contrast variants
4. only add new custom variables when the existing token set cannot express the need cleanly

## TODO: Expected Behaviors

Behavior checks this visual language should support across the repo:

- layouts should reflow cleanly from narrow dock/sidebar widths to larger desktop panels
- keyboard focus, pointer interaction, scroll, and text selection should behave normally in embedded and standalone surfaces
- common control types should remain visually and behaviorally dependable:
  - buttons
  - tabs
  - text inputs
  - selects
  - toggles
  - progress indicators
  - collapsible sections
- empty, loading, error, and missing-asset states should fail visibly instead of silently appearing blank
- bundled runtimes should preserve the same design language as source-driven surfaces
- dark, medium-contrast, and high-contrast variants should keep equivalent hierarchy and readability
- paper-like surfaces should keep readable separation with restrained borders, smaller radii, and preserved shadow depth
- token-based styling should remain the default so shared themes stay coherent across engine tools, docs, and studio surfaces

## Scope

This folder is guidance, not a framework requirement.
Apps can still layer their own component tokens on top, but these files should remain the repo-wide visual baseline.
