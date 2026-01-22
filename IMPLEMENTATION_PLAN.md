# GitSardine C++ Implementation Plan

## Overview

Agent-based implementation strategy for GitSardine C++ port.

---

## Agent Hierarchy

```
                    ┌─────────────────┐
                    │ PROJECT_MANAGER │
                    └────────┬────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
              ▼              ▼              ▼
     ┌────────────────┐  ┌──────────┐  ┌─────────────────┐
     │  VIEW_MANAGER  │  │ REVIEWER │  │ FEATURE_MANAGER │
     └───────┬────────┘  └──────────┘  └────────┬────────┘
             │                                   │
     ┌───────┼───────┐               ┌───────────┼───────────┐
     │       │       │               │           │           │
     ▼       ▼       ▼               ▼           ▼           ▼
  ┌─────┐ ┌─────┐ ┌─────┐       ┌─────────┐ ┌─────────┐ ┌─────────┐
  │VIEW │ │VIEW │ │VIEW │       │FEATURE  │ │FEATURE  │ │FEATURE  │
  │AGENT│ │AGENT│ │AGENT│       │AGENT    │ │AGENT    │ │AGENT    │
  └─────┘ └─────┘ └─────┘       └─────────┘ └─────────┘ └─────────┘
```

---

## Agent Definitions

| Agent | File | Purpose |
|-------|------|---------|
| PROJECT_MANAGER | `agents/PROJECT_MANAGER.md` | Top-level coordination |
| VIEW_MANAGER | `agents/VIEW_MANAGER.md` | UI layer coordination, updates ROADMAP |
| FEATURE_MANAGER | `agents/FEATURE_MANAGER.md` | Backend coordination, updates ROADMAP |
| VIEW_AGENT | `agents/VIEW_AGENT.md` | Single widget implementation |
| FEATURE_AGENT | `agents/FEATURE_AGENT.md` | Single feature implementation |
| REVIEWER | `agents/REVIEWER.md` | Quality assurance for ALL work |

---

## Specification Documents

| Document | Content |
|----------|---------|
| `GITSARDINE.md` | Project structure, CMake, libgit2, GitWorker architecture |
| `VIEWS.md` | UI layout, coordinates, colors, widget specs |
| `FEATURES.md` | Git operations, status checking, threading |
| `ACTIONS.md` | User interactions, signal/slot connections, execution chains |
| `DATA_MODELS.md` | Config schema, constants, messages, icons |

---

## Progress Tracking

| Document | Purpose |
|----------|---------|
| `ROADMAP.md` | Detailed checklist - **managers mark items complete** |

---

## Implementation Phases

### Phase 1: Infrastructure
```
FEATURE_MANAGER assigns:
├── FEATURE_AGENT → Result.h (error handling)
├── FEATURE_AGENT → Config.h/.cpp (configuration)
└── FEATURE_AGENT → GitWorker.h/.cpp (thread infrastructure)

PROJECT_MANAGER creates:
├── CMakeLists.txt
└── resources/icons/*.h (embedded icons)
```

### Phase 2: Core Features + Basic UI
```
FEATURE_MANAGER assigns:          VIEW_MANAGER assigns:
├── GitStatus.h                   ├── StatusBar
├── GitRepository.h/.cpp          ├── BranchSelector
├── Repository discovery          └── DiffViewerDialog
└── Status task handlers
```

### Phase 3: Git Operations + Tree Widgets
```
FEATURE_MANAGER assigns:          VIEW_MANAGER assigns:
├── Branch operation handlers     ├── RepoTreeWidget
├── File operation handlers       └── ChangesTreeWidget
├── Network operation handlers
├── RepoModel
└── FolderTreeModel
```

### Phase 4: Advanced + Integration
```
FEATURE_MANAGER assigns:          VIEW_MANAGER assigns:
├── Merge handler                 └── MainScreen
└── Diff handler

PROJECT_MANAGER creates:
├── AppController.h/.cpp
└── main.cpp
```

### Phase 5: Final Integration
```
PROJECT_MANAGER:
├── Wire all components
├── Final CMakeLists.txt
├── Build test
└── Launch test
```

---

## Mandatory Review Cycle

**EVERY implementation must be reviewed by REVIEWER.**

```
Manager assigns task to Agent
       ↓
Agent implements
       ↓
REVIEWER reviews
       ↓
   ┌───┴───┐
   │       │
APPROVED  ISSUES FOUND
   │       │
   ↓       ↓
Manager   Agent MUST be called again
updates   to fix issues
ROADMAP         │
   │            ↓
   │      REVIEWER re-reviews
   │            │
   │            ↓
   │      (repeat until APPROVED)
   ↓
DONE
```

**CRITICAL RULES:**
1. No implementation complete without REVIEWER approval
2. If REVIEWER finds issues, Agent MUST be called again
3. Managers update ROADMAP.md when tasks pass review
4. Phases are sequential - complete N before N+1

---

## Final File Structure

```
gitsardine-cpp/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── core/
│   │   └── Result.h
│   ├── controller/
│   │   └── AppController.h/.cpp
│   ├── screens/
│   │   └── MainScreen.h/.cpp
│   ├── widgets/
│   │   ├── RepoTreeWidget.h/.cpp
│   │   ├── ChangesTreeWidget.h/.cpp
│   │   ├── BranchSelector.h/.cpp
│   │   ├── StatusBar.h/.cpp
│   │   └── DiffViewerDialog.h/.cpp
│   ├── git/
│   │   ├── GitRepository.h/.cpp
│   │   ├── GitManager.h/.cpp
│   │   └── GitStatus.h
│   ├── config/
│   │   └── Config.h/.cpp
│   ├── models/
│   │   ├── RepoModel.h/.cpp
│   │   └── FolderTreeModel.h/.cpp
│   └── workers/
│       └── GitWorker.h/.cpp
├── resources/
│   └── icons/
│       ├── icons.h
│       └── *.h (embedded PNGs)
└── deps/
    └── qontrol/ (git submodule)
```

---

## Success Criteria

- [ ] All phases complete
- [ ] All ROADMAP.md items checked
- [ ] All reviews passed (REVIEWER approved everything)
- [ ] `cmake --build .` succeeds
- [ ] Application launches
- [ ] Config loads correctly
- [ ] Repositories discovered
- [ ] Status updates work
- [ ] Branch operations work
- [ ] Git operations work (commit, pull, push)
