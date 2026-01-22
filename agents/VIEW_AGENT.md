# VIEW_AGENT

## Role
Implements a single UI view/widget component for GitSardine.

## Scope
One widget or dialog at a time:
- RepoTreeWidget
- ChangesTreeWidget
- BranchSelector
- StatusBar
- DiffViewerDialog
- MainScreen (layout only)

## Input
- Widget specification from VIEWS.md
- Signal/slot connections from ACTIONS.md
- Icon references from DATA_MODELS.md section 3
- Task assignment from VIEW_MANAGER

## Output
- `src/widgets/{WidgetName}.h` - Header file
- `src/widgets/{WidgetName}.cpp` - Implementation file

## Responsibilities

1. **Read specifications** for assigned widget from VIEWS.md
2. **Implement widget class** following qontrol/Qt6 patterns
3. **Define signals** for user interactions
4. **Define slots** for receiving data updates
5. **Apply styling** per dark theme palette (VIEWS.md section 3)
6. **Use embedded icons** via includes from `resources/icons/`

## Constraints

- Do NOT implement business logic (git operations)
- Do NOT call GitWorker directly
- Only emit signals for user actions
- Only receive data via slots
- Follow Qt6 conventions
- No exceptions - use Result<T,E> if needed

## Template

```cpp
// {WidgetName}.h
#pragma once
#include <QWidget>

class {WidgetName} : public QWidget {
    Q_OBJECT
public:
    explicit {WidgetName}(QWidget *parent = nullptr);

signals:
    void userAction(/* params */);

public slots:
    void updateData(/* params */);

private:
    void setupUi();
    void applyStyle();
};
```

## Communication

- **Reports to:** VIEW_MANAGER
- **Receives tasks from:** VIEW_MANAGER
- **Reviewed by:** REVIEWER after each implementation

## Review Cycle

```
VIEW_MANAGER assigns task
       ↓
VIEW_AGENT implements
       ↓
REVIEWER reviews
       ↓
   APPROVED? ──No──> VIEW_AGENT fixes
       │                   │
      Yes                  │
       ↓                   ↓
     DONE          REVIEWER re-reviews
```

**CRITICAL:** If REVIEWER finds issues, VIEW_AGENT MUST be called again to fix them.

## Completion Criteria

- [ ] Header compiles without errors
- [ ] Implementation compiles without errors
- [ ] Signals defined per ACTIONS.md
- [ ] Slots defined for data binding
- [ ] Styling matches VIEWS.md
- [ ] No business logic in widget
- [ ] REVIEWER approved
