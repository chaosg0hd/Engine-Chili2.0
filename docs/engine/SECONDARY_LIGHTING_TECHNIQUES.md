# Secondary Lighting Techniques

This document defines the intended secondary-lighting family for the engine.

It does not replace the current direct-light law.
It defines the non-direct lighting providers we intend to support next.

Core model:

`Indirect = BaseBounce + ProbeIndirect + ScreenRefinement`

The important architectural rule is:

- treat these as secondary-light providers
- do not treat them as unrelated post effects
- do not force them to compete as equal full replacements
- let each provider contribute a different class of secondary light

## Provider Roles

### 1. Derived Bounce Fill

Purpose:

- provide the guaranteed low-frequency indirect floor
- prevent scenes from collapsing into dead black
- keep some bounced cohesion available even at low quality

Strengths:

- fast
- cheap
- always available
- easy to apply to every frame

Weaknesses:

- coarse
- approximate
- not truly world-aware
- should not be mistaken for full indirect lighting

Inputs:

- direct-light result or direct-light descriptors
- dominant-light color/intensity
- material diffuse/albedo response
- room/environment tint
- optional shadow-awareness hint

Outputs:

- diffuse-only indirect contribution
- stable low-frequency fill term

Role in final composite:

- baseline softness layer
- fallback provider when higher techniques are unavailable

Recommended options:

- bounce strength
- environment/room tint
- light-color influence
- diffuse-only toggle
- shadow-aware weighting
- dominant-light only vs all direct lights later

## 2. Probe-Based Indirect Lighting

Purpose:

- provide stable world-space indirect structure
- represent accumulated soft light in a reusable spatial form

Strengths:

- location-aware
- stable
- reusable across views
- good medium-term engine solution

Weaknesses:

- lower-frequency than full path tracing
- needs data placement/update policy
- can look soft or delayed if probe density is weak

Inputs:

- probe positions / probe grid
- probe radiance or irradiance data
- interpolation policy
- local world position / normal
- probe influence radius and blending rules

Outputs:

- spatially varying indirect contribution
- persistent world-aware bounced light term

Role in final composite:

- stable world-space indirect truth layer
- main reusable indirect-light structure once the provider exists

Recommended options:

- probe spacing/density
- interpolation mode
- update mode
- static
- baked
- dynamic refresh later
- low-frequency only
- probe influence radius
- blending weight

## 3. Screen-Space Indirect Lighting

Purpose:

- provide dynamic visible-scene refinement
- react to view-visible local lighting relationships immediately

Strengths:

- reactive
- detailed
- immediate
- good for visible local richness

Weaknesses:

- view-dependent
- incomplete by nature
- prone to stability/noise issues
- should not be treated as full global truth

Inputs:

- current depth
- normals
- current shaded or lighting buffers
- sampling radius / quality controls
- optional temporal history later

Outputs:

- view-dependent indirect refinement
- local secondary-light detail in visible regions

Role in final composite:

- dynamic correction/detail layer
- refinement over the cheaper and more stable indirect providers

Recommended options:

- resolution scale
- sample count
- radius
- intensity
- temporal accumulation later
- denoise/smoothing later
- edge rejection / stability controls later

## Composite Secondary Lighting

These techniques should not fight as equal replacements.

The intended composition rule is:

- derived bounce fill supplies the baseline floor
- probe indirect supplies stable world-space structure
- screen-space indirect supplies dynamic visible-scene refinement

Conceptual accumulation:

`Indirect = BaseBounce + ProbeIndirect + ScreenRefinement`

This implies a future composite stage that accepts multiple secondary providers and accumulates them deliberately.

Recommended composite controls:

- enabled techniques mask
- per-technique weight
- quality tier
- budget tier
- fallback order
- debug isolation mode

Example quality policy:

- low quality: derived bounce fill only
- medium quality: derived bounce fill + probe indirect
- high quality: derived bounce fill + probe indirect + screen-space indirect
- future ultra: derived bounce fill + probe indirect + screen-space indirect + traced refinement later

## Architectural Rule

Secondary-light techniques should reduce to:

- an indirect-light contribution
- feeding the same accumulation stage

That keeps the system coherent.

The engine should be able to think in terms of:

- direct lighting pass
- secondary-light provider 1
- secondary-light provider 2
- secondary-light provider 3
- composite accumulation

## Recommended Implementation Order

Build in this order:

1. Derived bounce fill
2. Probe-based indirect lighting
3. Screen-space indirect lighting

Reason:

- derived bounce fill gives immediate payoff for very low cost
- probe indirect gives the first reusable world-space secondary-light structure
- screen-space indirect is the most view-dependent and unstable, so it should land after the base is clear

## One-Line Principle

Secondary lighting in this engine should be provided by:

- derived bounce fill
- probe-based indirect lighting
- screen-space indirect lighting

with future support for controlled composite accumulation across multiple secondary-light providers.
