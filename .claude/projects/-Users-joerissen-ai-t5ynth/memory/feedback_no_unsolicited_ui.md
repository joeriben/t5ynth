---
name: feedback_no_unsolicited_ui
description: Don't add/remove UI elements the user didn't ask for; don't assume placement; never remove existing functionality
type: feedback
---

Don't add UI elements the user didn't ask for; don't assume placement. Never remove existing functionality (e.g., Random seed toggle, buttons, controls) without explicit request.

**Why:** Session 13 silently removed the Random seed toggle and broke the DimExplorer overlay by not maintaining the click-to-open behavior. The user was rightfully angry — these were regressions, not improvements.

**How to apply:** Before changing any UI element, verify: (1) Was this change explicitly requested? (2) Does it remove or break any existing feature? If existing features break as a side-effect, fix them immediately. Always prioritize fixing broken core functionality over adding new features.
