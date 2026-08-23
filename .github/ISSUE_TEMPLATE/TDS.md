---
name: Technical Design Specification
title: "[TDS ###-###] Name v0.1"
about: Define one independently implementable technical outcome from an approved TDD
labels: "tech design"
assignees: ""
---

<!--
TECHNICAL DESIGN SPECIFICATION (TDS)

A TDS defines HOW one actionable technical outcome identified by the parent TDD will be built
and how we will prove that it works

The parent TDD owns feature-level behavior, rules, boundaries, and cross-system contracts
This TDS inherits those requirements and should not silently redefine them

If implementation reveals that a feature rule, boundary, or cross-TDS contract needs to change,
update and re-review the parent TDD before continuing

Keep this scope independently implementable, reviewable, and testable

Delete comments as the issue is filled out
-->

# Technical Design Specification

## 1. Actionable Item Definition

### 1.1 Technical Outcome

<!--
Describe the concrete technical result that should exist when this TDS is complete

Think of this as the deliverable itself rather than the actions required to build it

A useful format is

"When complete, this TDS will provide [capability] by creating/modifying [systems/assets]
and exposing [state/events/interfaces] to [consumers]."

Someone reading this should be able to understand what this slice produces without reading
the implementation tasks
-->



### 1.2 Dependencies and Prerequisites

<!--
List dependencies that materially affect implementation or sequencing.

Examples
- another TDS
- an existing gameplay system
- plugin or engine functionality
- required asset/content
- interface or data contract
- design decision that must exist before implementation can continue

-->

| Dependency | Type | Required Input / Contract | Availability / Version | Owner |
| --- | --- | --- | --- | --- |
|  |  |  |  |  |

---

## 2. Technical Architecture

### 2.1 Implementation Approach and Rationale

<!--
Describe the selected Unreal approach and justify decisions that are not obvious
Mention rejected alternatives only when the tradeoff affects maintenance, performance, ownership, or integration

Examples
- What owns the state?
- Where does the logic live?
- How do systems communicate?
- What data/configuration is exposed?
- What is the lifecycle?
- What are the important integration boundaries?

Implement the outcome using ___ because ___
The scope communicates with ___ through ___
This keeps ___ isolated and avoids ___
-->



### 2.2 Blueprint / Object Responsibilities

<!--
List logic-bearing objects created or materially modified by this TDS

"Must Not Own" is meant to make system boundaries explicit

Example
A Ballast Component may own Ballast State and buoyancy behavior,
but should not own the player's breathable Oxygen resource
-->

| Blueprint / Object | Class / Type | Responsibilities | Must Not Own |
| --- | --- | --- | --- |
|  |  |  |  |

### 2.3 Assets / Folder Structure

<!--
Show only folders and assets owned or modified by this TDS
Follow project naming and folder standards as defined on the Tech Wiki

/ProjectName/
  Core/[Feature]/
    BPC_[ActionableItem]Component
    BPI_[ActionableItem]Interface
  Data/[Feature]/
    DA_[ActionableItem]Config
  QA/[Feature]/
    BP_Test_[ActionableItem]

-->

---

## 3. Primary Implementation Flow

<!--
Describe the intended runtime sequence from the initiating condition to the required result.
-->

| Step | Owner / Asset | Technical Action | Expected Result |
| --- | --- | --- | --- |
|  |  |  |  |



## 4. Validation and Delivery

### 4.1 Debug Tools and Test Assets

| Tool / Asset | Purpose | Location / Invocation | Owner | Shipping Behavior |
| --- | --- | --- | --- | --- |
| [BP_Test_ActionableItem / debug toggle / force result command] | [State inspection or scenario control] | [Map path / console command / setting] | [Tech / QA] | [Editor-only / disabled / removed] |
| [Tool] | [Purpose] | [Location] | [Owner] | [Shipping behavior] |

### 4.2 Test Environment

| Field | Value |
| --- | --- |
| **Primary Test Map** | [/Game/Maps/L_[ActionableItem]_Test or existing integration map] |
| **Required Setup** | [Actors, data, input, game mode, network mode, platform, or content] |
| **Instrumentation** | [Logs, stats, telemetry viewer, profiler, or debug widget] |
| **Target Build** | [PIE / standalone / client-server / packaged platform build] |

### 4.3 Acceptance Criteria

<!--
Write criteria that can pass or fail. Avoid generic statements such as "works as intended." Include the evidence or review method.*
-->

| AC ID | Acceptance Criterion | Verification Method | Requirement IDs | Status |
| --- | --- | --- | --- | --- |
| AC-01 | [Specific observable implementation outcome] | [TC-## / code review / asset inspection / target-build test] | [BHV-## / RULE-##] | [Not Run / Pass / Fail] |
| AC-02 | [Configuration is editable and invalid values are constrained] | [Asset inspection + boundary test] | [CFG-##] | [Status] |
| AC-03 | [Cross-TDS contract is implemented without redefining it] | [Integration test / IAC-##] | [INT-##] | [Status] |

### 4.4 Handoff Checklist

- [ ] All traced TDD requirements and interaction contracts are implemented or explicitly deferred with approval.
- [ ] New and modified assets follow naming and folder standards.
- [ ] Designer-exposed values are editable without logic changes and reject invalid values.
- [ ] Shared Blueprint changes are minimal, documented, and coordinated with the named owner.
- [ ] Required placeholder UI, audio, VFX, animation, and telemetry hooks are connected.
- [ ] Debug tools and test assets behave correctly and do not ship unintentionally.
- [ ] Performance targets have evidence in the required environment.
- [ ] Test cases and acceptance criteria pass; remaining issues have owners.
- [ ] Tech Design, Engineering, Game Design, QA, and affected disciplines have completed the required review.

### 4.5 Review and Approval

| Role | Reviewer | Status | Date | Notes |
| --- | --- | --- | --- | --- |
|  Design | [Name] | [Pending / Approved] | [YYYY-MM-DD] | [Notes] |
| Tech Design | [Name] | [Pending / Approved] | [YYYY-MM-DD] | [Notes] |
| Tech | [Name] | [Pending / Approved] | [YYYY-MM-DD] | [Notes] |
| [Discipline] | [Name] | [Pending / Approved] | [YYYY-MM-DD] | [Notes] |
