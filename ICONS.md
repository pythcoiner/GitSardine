# GitSardine Icon Mapping

All icons used by GitSardine are located in `used_icons/`.

---

## 1. Button Icons (Static)

| Icon | Widget | Action |
|------|--------|--------|
| ![](used_icons/arrow-circle-315.png) `arrow-circle-315.png` | `update_tree`, `b_update` | Refresh repository tree / Update status |
| ![](used_icons/cross-button.png) `cross-button.png` | `b_delete` | Delete branch |
| ![](used_icons/arrow-skip-270.png) `arrow-skip-270.png` | `b_pull` | Pull from remote |
| ![](used_icons/arrow-skip-090.png) `arrow-skip-090.png` | `b_push` | Push to remote |
| ![](used_icons/cross.png) `cross.png` | `b_delete_file` | Delete file |
| ![](used_icons/arrow-curve-180-left.png) `arrow-curve-180-left.png` | `b_clean` | Restore (git restore) |
| ![](used_icons/disk--minus.png) `disk--minus.png` | `b_reset` | Reset (git reset) |

---

## 2. Dynamic Icon Switching

### 2.1 Repository Tree Item Status

**Function:** `Folder.update_icon()`
**Priority:** status_checked → error → pull → push → commit → clean

```
┌────────────────────────────────────────────────────────────────────┐
│ Condition                         │ Icon                          │
├───────────────────────────────────┼───────────────────────────────┤
│ status_checked = false            │ arrow-circle-315.png          │
│ status_error = true               │ exclamation-red.png           │
│ is_repo && need_pull              │ drive-download.png            │
│ is_repo && need_push              │ drive-upload.png              │
│ is_repo && need_commit            │ disk--plus.png                │
│ is_repo && clean                  │ document.png                  │
│ !is_repo && parent == null        │ drive.png                     │
│ !is_repo && parent != null        │ folder-horizontal.png         │
└───────────────────────────────────┴───────────────────────────────┘
```

| Icon | Condition | Meaning |
|------|-----------|---------|
| ![](used_icons/arrow-circle-315.png) `arrow-circle-315.png` | `status_checked = false` | Status not yet checked (loading) |
| ![](used_icons/exclamation-red.png) `exclamation-red.png` | `status_error = true` | Error accessing repository |
| ![](used_icons/drive-download.png) `drive-download.png` | `need_pull = true` | Remote has new commits |
| ![](used_icons/drive-upload.png) `drive-upload.png` | `need_push = true` | Local has unpushed commits |
| ![](used_icons/disk--plus.png) `disk--plus.png` | `need_commit = true` | Has uncommitted changes |
| ![](used_icons/document.png) `document.png` | Clean | Repository up to date |
| ![](used_icons/drive.png) `drive.png` | Root folder | Configured path root |
| ![](used_icons/folder-horizontal.png) `folder-horizontal.png` | Folder | Non-repo folder in tree |

---

### 2.2 Spinner Animation

**Function:** `update_spin()`
**Widget:** `update_tree` button
**Trigger:** Status update in progress
**Interval:** 200ms per frame

```
┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐
│ spin-1   │ -> │ spin-2   │ -> │ spin-3   │ -> │ spin-4   │ -> (repeat)
└──────────┘    └──────────┘    └──────────┘    └──────────┘
```

| Icon | Frame |
|------|-------|
| ![](used_icons/spin-1.png) `spin-1.png` | Frame 1 (also idle state) |
| ![](used_icons/spin-2.png) `spin-2.png` | Frame 2 |
| ![](used_icons/spin-3.png) `spin-3.png` | Frame 3 |
| ![](used_icons/spin-4.png) `spin-4.png` | Frame 4 |

---

### 2.3 Extend Button Toggle

**Function:** `on_b_extend()`
**Widget:** `b_extend` button
**Trigger:** Button click

```
┌─────────────────────────────────────────────────────────────────┐
│ State              │ Icon                        │ Next Action  │
├────────────────────┼─────────────────────────────┼──────────────┤
│ Collapsed (normal) │ navigation-000-button-white │ → Expand     │
│ Extended           │ navigation-180-button-white │ → Collapse   │
└────────────────────┴─────────────────────────────┴──────────────┘
```

| Icon | State | Purpose |
|------|-------|---------|
| ![](used_icons/navigation-000-button-white.png) `navigation-000-button-white.png` | Collapsed | Arrow pointing right (expand) |
| ![](used_icons/navigation-180-button-white.png) `navigation-180-button-white.png` | Extended | Arrow pointing left (collapse) |

---

## 3. Icon Summary

**Total:** 20 icons

| Category | Count | Type |
|----------|-------|------|
| Button icons | 7 | Static |
| Navigation icons | 2 | Dynamic (toggle) |
| Spinner icons | 4 | Dynamic (animation) |
| Status icons | 5 | Dynamic (state) |
| Tree icons | 2 | Dynamic (state) |

---

## 4. File List

```
used_icons/
├── arrow-circle-315.png        # Update/refresh button, loading state
├── arrow-curve-180-left.png    # Restore button
├── arrow-skip-090.png          # Push button
├── arrow-skip-270.png          # Pull button
├── cross-button.png            # Delete branch button
├── cross.png                   # Delete file button
├── disk--minus.png             # Reset button
├── disk--plus.png              # Needs commit status
├── document.png                # Clean repo status
├── drive-download.png          # Needs pull status
├── drive-upload.png            # Needs push status
├── drive.png                   # Root folder icon
├── exclamation-red.png         # Error status
├── folder-horizontal.png       # Folder icon
├── navigation-000-button-white.png  # Extend (expand)
├── navigation-180-button-white.png  # Extend (collapse)
├── spin-1.png                  # Spinner frame 1
├── spin-2.png                  # Spinner frame 2
├── spin-3.png                  # Spinner frame 3
└── spin-4.png                  # Spinner frame 4
```
