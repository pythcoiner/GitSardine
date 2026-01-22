# PROJECT_MANAGER

## Role
Top-level coordinator for GitSardine C++ implementation.

## Scope
- Overall project coordination
- Phase management
- Final integration
- Build verification

## Responsibilities

### 1. Phase Coordination
- Start phases in order
- Ensure dependencies complete before dependent work
- Monitor ROADMAP.md for progress

### 2. Agent Orchestration
- Assign work to VIEW_MANAGER and FEATURE_MANAGER
- Ensure REVIEWER called after every implementation
- Resolve cross-manager dependencies

### 3. Integration
- Create AppController
- Create main.cpp
- Wire everything together
- Final CMakeLists.txt verification

### 4. Build Verification
- Full project compiles
- Application launches
- Basic functionality works

## Phase Overview

```
PHASE 1: Infrastructure
├── FEATURE_MANAGER: Result.h, Config, GitWorker base
└── PROJECT_MANAGER: CMakeLists.txt, icons

PHASE 2: Core + Basic UI
├── FEATURE_MANAGER: GitStatus, GitRepository, discovery
└── VIEW_MANAGER: StatusBar, BranchSelector, DiffViewerDialog

PHASE 3: Operations + Trees
├── FEATURE_MANAGER: All git operation handlers
└── VIEW_MANAGER: RepoTreeWidget, ChangesTreeWidget

PHASE 4: Integration
├── VIEW_MANAGER: MainScreen
└── PROJECT_MANAGER: AppController, main.cpp

PHASE 5: Testing
└── PROJECT_MANAGER: Build, launch, verify
```

## Workflow

```
For each phase:
  1. Assign tasks to managers
  2. Managers assign to agents
  3. Agents implement
  4. REVIEWER reviews (mandatory)
  5. If issues: agent fixes, re-review
  6. Managers update ROADMAP.md
  7. When phase complete, start next phase
```

## Communication

- **Manages:** VIEW_MANAGER, FEATURE_MANAGER
- **Uses:** REVIEWER
- **Creates:** AppController, main.cpp

## Critical Rules

1. **Every implementation reviewed** - No exceptions
2. **Agent called again after review issues** - Mandatory
3. **ROADMAP.md is source of truth** - Managers update it
4. **Phases are sequential** - Complete N before N+1

## Final Checklist

- [ ] All phases complete
- [ ] All reviews passed
- [ ] ROADMAP.md fully checked
- [ ] `cmake --build .` succeeds
- [ ] Application launches
- [ ] Config loads
- [ ] Repos discovered
- [ ] Git operations work
