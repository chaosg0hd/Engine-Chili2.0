# Visual Language

This document defines the shared UI direction for interactive surfaces in this repo.
It is meant to outlive any one implementation, whether the UI is built as HTML/CSS,
embedded web content, or native C++ widgets later on.

## Purpose

Use this as the design contract for:

- engine tools
- Studio surfaces
- runtime utilities
- docs-adjacent interactive pages
- future native editor UI

The goal is consistent visual behavior, not strict visual cloning.

## Core Principles

- Prefer calm, readable interfaces over flashy presentation.
- Favor paper-like surfaces over glassy or heavily outlined panels.
- Keep depth through elevation and tone separation, not thick borders.
- Support narrow docked layouts first, then scale upward.
- Let contrast, hierarchy, and motion guide attention without noise.
- Build around semantic tokens so themes stay coherent across implementations.

## Visual Character

The desired look is:

- paper-like
- calm
- dense but breathable
- modern Material-inspired
- rounded, but restrained
- clear in both dark and light themes

Avoid:

- oversized border radii
- heavy edge strokes
- plastic-looking cards
- flat undifferentiated surfaces
- browser-default controls when custom surfaces are expected
- decorative motion that does not improve hierarchy

## Color System

Color should come from the shared `--md-sys-color-*` token language in `docs/css`.

Use tokens semantically:

- `background` for app/page backdrops
- `surface` and `surface-container*` for panels and cards
- `primary` and `secondary` for emphasis and active surfaces
- `on-*` tokens for readable foreground content
- `outline` and `outline-variant` only for subtle structure
- `error` tokens only for destructive or failure states

Do not make individual screens by picking ad hoc colors first.

## Shape

Shape should feel intentional and restrained.

- Reduce border radius compared with common web defaults.
- Keep major containers slightly softer than inner components.
- Prefer a small stepped shape scale over many arbitrary radii.
- Menus, cards, buttons, and fields should feel related, not random.

Current direction:

- hero / major container: soft but not oversized
- panels / cards: moderately rounded
- inputs / buttons / tabs / menus: tighter rounding
- chips / pills: fully rounded only when the component meaning benefits from it

## Elevation

Depth should feel like stacked paper, not floating plastic.

- Use tonal separation first.
- Add shadow to reinforce hierarchy.
- Keep borders weak or nearly invisible.
- Elevation changes should be more noticeable than border changes.

Surfaces should still separate clearly in dark themes without relying on harsh outlines.

## Density And Spacing

The UI should feel compact enough for tool work, but never cramped.

- Favor tighter padding than marketing-style layouts.
- Keep spacing consistent and token-like.
- Use smaller gaps in control groups than in section-level layout.
- In docked views, reduce padding before sacrificing readability.

## Typography

Typography should remain readable inside constrained panels.

- Text must never overflow its container in normal use.
- Headings, labels, and code-like strings must wrap safely.
- Font sizing should adapt to tighter spaces before content breaks.
- Strong hierarchy should come from scale, weight, and spacing together.
- Avoid giant hero type in embedded tools.

Expected direction:

- concise headings
- compact labels
- readable body copy
- safe wrapping for paths, states, and long tokens

## Layout

Layout should be dock-first and responsive.

- Design for narrow side panels first.
- Expand gracefully to larger windows.
- Collapse multi-column regions early.
- Avoid horizontal scrolling in normal workflows.
- Use stacked modules and short line lengths in utility surfaces.
- Hero areas must still work when the width is heavily constrained.

## Components

Common controls should share one family resemblance.

Expected component traits:

- buttons should feel compact, tactile, and readable
- tabs should read as segmented navigation, not oversized pills
- fields should feel integrated with surrounding surfaces
- menus should reflect the rounded language of their trigger
- toggles should feel stable and legible in all themes
- accordions and expandable sections should stay compact
- progress indicators should be clear without becoming visually loud

Avoid generic browser-default presentation when a control is part of the designed surface language.

## Motion

Motion should support clarity and hierarchy.

- Entry motion should be visible enough to feel intentional.
- Hero transitions can be slightly stronger than the rest of the UI.
- Hover and pressed states should be noticeable but quick.
- Motion should never make the interface feel sluggish.
- Use motion to reinforce elevation, focus, and state changes.

Avoid motion that is so weak it feels accidental, or so strong it becomes theatrical.

## Theme Expectations

This visual language must survive theme changes cleanly.

- dark theme should feel rich and readable
- high-contrast dark should keep hierarchy while improving legibility
- light theme should remain paper-like, not washed out
- all themes should preserve the same component identity

Theme swaps should not require redesigning component structure.

## Behavior Expectations

Across all UI surfaces, the design language should support:

- responsive reflow from narrow docks to wider panels
- visible empty, loading, error, and missing-asset states
- clear focus, hover, pressed, and selected states
- reliable interaction for buttons, tabs, menus, inputs, toggles, and collapsible regions
- safe text flow inside cards, menus, chips, labels, and headings
- normal keyboard, pointer, text-selection, and scroll behavior

## Native Rebuild Guidance

When rebuilding this language in native C++:

- preserve the token semantics even if the underlying styling system changes
- preserve the shape scale, density, and elevation logic
- rebuild component behavior before polishing rare edge visuals
- use this document as the appearance baseline, not the current HTML structure

The important part is the system:

- restrained shape
- paper-like elevation
- responsive compact layout
- semantic color mapping
- dependable interaction states

## Relationship To Theme Files

The theme files in `docs/css/` remain the palette source of truth.
This document explains how that palette should feel when turned into actual UI.
