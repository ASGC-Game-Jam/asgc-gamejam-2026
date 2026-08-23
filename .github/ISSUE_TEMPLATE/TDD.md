---
name: Technical Design Document
title: "[TDD ###] - Name v0.1"
about: Technical Design Document for a feature
labels: "feature"
Source FDS: "[FDS ###](Insert link to FDS)"
Owner / Reviewers: "[Author / Technical Designer] | [Tech Design / Engineering / Design reviewers]"
---

<!-- 
0. Template Guidelines DELETE BEFORE HANDOFF

This section explains how to use the template.
Remove it from the completed TDD before review or implementation planning.

Create the TDD only after the accompanying FDS is approved for technical planning.

This document has exactly three jobs:

1. Translate the approved FDS into a feature-level technical description using
   systems, state, data, ownership, dependencies, and constraints.
2. Identify and justify the independently actionable technical scopes that will
   each receive a TDS.
3. Record feature-wide requirements: core behavior, general rules, edge cases,
   and the rules governing how TDS scopes interact with or limit one another.

Document boundary:
The TDD decides what technical scopes exist and what they must collectively
satisfy.

It does not define Blueprint graphs, assets, functions, folder paths,
per-item implementation strategies, or per-TDS acceptance criteria;
those belong in the TDS documents.

- Create one TDS per actionable implementation outcome, not automatically one
  TDS per Blueprint or asset. A coherent TDS may own several closely related assets.
- Give every requirement a stable ID (for example BHV-01, RULE-01, EDGE-01,
  INT-01, NFR-01) and assign it to at least one TDS.
- Keep shared Blueprint changes minimal and visible.
  Record cross-TDS contracts and merge risks before implementation begins.
- Use the [Tech Design Bible](https://doc.clickup.com/90141389150/p/h/2kydgway-2794/110b321bd1f86e0) for naming, folder, architecture, review,
  and Blueprint standards.
-->

# 1. Technical Feature Description

<!--
Describe the feature as an engineering system: what initiates it,
which systems participate, and what technical result must be produced.

Exclude design rationale, player-facing pitch language,
and implementation detail that belongs in a TDS.
-->

## 1.1 Technical Purpose

<!--
State the technical problem, the systems affected, and the result
the project must support. Keep this implementation-neutral.

Example form:
This feature introduces a system that receives ..., validates ...,
maintains ..., produces ..., and exposes ... to the rest of the project.
-->

## 1.2 Operation

| Step | System / Actor | Technical Action | Required Result |
|---|---|---|---|
| 1 | [Initiating system] | [Trigger or request] | [Initial state or output] |
| 2 | [Responsible system] | [Primary processing or state change] | [Required state or data] |
| 3 | [Dependent system] | [Consume, present, or continue] | [Observable completion result] |

# 2. Required Behavior and Rules

<!--
Ownership rule:
These requirements belong to the TDD.

A TDS references and implements them; it must not silently redefine them.

If implementation reveals a rule change, update and re-review
the parent TDD as needed.

List only requirements needed for the intended implementation flow.
Use stable IDs so each rule can be assigned to a TDS and referenced
in its acceptance criteria.
-->

| ID | Requirement | Applies To |
|---|---|---|
| REQ-01 | [Core behavior that must occur] | [TDS-01] |
| REQ-02 | [General rule or constraint] | [TDS-01, TDS-02] |
| REQ-03 | [Required interaction or ordering rule] | [TDS-02] |
| REQ-04 | [Required completion result or feedback] | [TDS-03] |

# 3. TDS Decomposition

<!--
Break the feature into technical outcomes that can be assigned,
implemented, reviewed, and tested separately while still respecting
the feature-level contracts above.
-->

## 3.1 Decomposition Rules

<!--
- Each TDS produces one clear technical outcome with a named owner
  and completion boundary.
- A TDS may contain multiple Blueprints or assets when they form
  one cohesive implementation responsibility.
- Separate scopes when they have different owners, review paths,
  deployment/merge risks, test strategies, or can be delivered independently.
- Do not create a miscellaneous or integration TDS merely to absorb
  unclear ownership. Assign every contract endpoint to a real producer
  and consumer.
- Every TDS must cite the requirement IDs it owns and the interaction
  IDs it participates in.
-->

## 3.2 TDS Index

| TDS ID | Actionable Item | Owner | Status | Justification |
|---|---|---|---|---|
| TDS-###-001 | [Outcome-oriented actionable item name] | [Person / discipline] | [Not Started / Draft / Ready / Done] | [Ownership, dependency, or independent delivery boundary] |
| TDS-###-002 | [Actionable item] | [Owner] | [Status] | [Reason for separate implementation] |
| TDS-###-003 | [Actionable item] | [Owner] | [Status] | [Reason for separate implementation] |

## 3.3 TDS Dependency and Implementation Order

| TDS | Depends On | Reason / Required Contract | Can Run in Parallel? | Blocking Condition |
|---|---|---|---|---|
| [TDS-02] | [TDS-01 / existing system] | [Data, interface, state, or asset required] | [Yes / Partially / No] | [What must exist first] |
| [TDS-03] | [Dependency] | [Reason] | [Yes / No] | [Blocker] |

# 4. Review and Approval

## 4.1 Acceptance Criteria Checklist

- [ ] The approved FDS and version are linked
- [ ] The feature has a technical description and explicit boundary
- [ ] Core behavior, rules, edge cases, interaction limits, and non-functional constraints have stable IDs
- [ ] Every requirement has a primary TDS owner
- [ ] Every TDS has an actionable outcome, a justification, boundaries, dependencies, and an owner
- [ ] Cross-TDS contracts, implementation order, shared-asset risks, and integration acceptance criteria are documented
- [ ] Open questions and blockers have owners and due dates
- [ ] Tech Design, Engineering, and Game Design have reviewed the plan

## 4.2 Signatures

| Role | Reviewer | Status | Date | Notes |
|---|---|---|---|---|
| Design | [Name] | [Pending / Approved] | [YYYY-MM-DD] | [Notes] |
| Tech Design | [Name] | [Pending / Approved] | [YYYY-MM-DD] | [Notes] |
| Tech | [Name] | [Pending / Approved] | [YYYY-MM-DD] | [Notes] |
| [Discipline] | [Name] | [Pending / Approved] | [YYYY-MM-DD] | [Notes] |
