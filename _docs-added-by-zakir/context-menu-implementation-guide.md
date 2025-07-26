# LibreOffice Writer Context Menu Implementation Guide

This document provides a comprehensive guide for adding new items to the right-click context menu in LibreOffice Writer, based on the successful implementation of the "Smart Rewrite with AI" feature.

## The Critical Fix That Made It Work

### The Problem
After implementing all the necessary components (UNO command, SDI registration, C++ handlers, UI configuration), the "Smart Rewrite with AI" menu item still wouldn't appear in the context menu.

### The Root Cause
**LibreOffice's SDI slot generation system wasn't processing our new command registration.** Even though our command was correctly defined in `_textsh.sdi`, the slot generation process had already run and cached the results, so our new command wasn't included in the generated slot tables.

### The Critical Fix
**Force regeneration of SDI slots** by removing the cached generated files:

```bash
rm -rf workdir/SdiTarget/sw/sdi/
make sw
```

This forced LibreOffice to regenerate the slot mapping tables, which finally included our `SID_SMART_REWRITE` command.

### Verification
After the fix, we could verify success by checking the generated slots file:

```bash
grep -A5 -B5 "SID_SMART_REWRITE" workdir/SdiTarget/sw/sdi/swslots.hxx
```

This showed our command properly registered in multiple shells (SwTextShell, SwWebTextShell, etc.).

---

## Complete Step-by-Step Guide: Adding Context Menu Items

### Overview
Adding a context menu item to LibreOffice Writer requires coordination between multiple systems:

1. **SID Definition** - Define the internal command ID
2. **UNO Command** - Define the external command interface  
3. **SDI Registration** - Register command with appropriate shells
4. **C++ Handlers** - Implement the command logic
5. **UI Integration** - Add to context menu XML
6. **Slot Generation** - Ensure slots are properly generated

### Step 1: Define the SID (Slot ID)

**File**: `core/include/svx/svxids.hrc` (for shared commands) or `core/sw/inc/cmdid.h` (for Writer-specific commands)

For shared commands like our Smart Rewrite:

```cpp
#define SID_YOUR_COMMAND                               ( SID_SVX_START + 466 )
```

**Rules**:
- Use SVX range (`SID_SVX_START + offset`) for commands that might be shared across applications
- Use Writer range (`FN_` prefix) for Writer-specific commands only
- Choose an unused offset number

### Step 2: Define the UNO Command

**File**: `core/officecfg/registry/data/org/openoffice/Office/UI/GenericCommands.xcu`

```xml
<node oor:name=".uno:YourCommand" oor:op="replace">
  <prop oor:name="Label" oor:type="xs:string">
    <value xml:lang="en-US">Your Command Label</value>
  </prop>
  <prop oor:name="ContextLabel" oor:type="xs:string">
    <value xml:lang="en-US">Your Command Label</value>
  </prop>
  <prop oor:name="TooltipLabel" oor:type="xs:string">
    <value xml:lang="en-US">Description of your command</value>
  </prop>
  <prop oor:name="Properties" oor:type="xs:int">
    <value>1</value>
  </prop>
</node>
```

**Critical**: The UNO command name (`.uno:YourCommand`) must match exactly with what you'll use in the context menu XML.

### Step 3: Register in SDI Files

#### A. Main SDI Definition (for shared commands)
**File**: `core/svx/sdi/svx.sdi`

```cpp
SfxVoidItem YourCommand SID_YOUR_COMMAND
()
[
        AutoUpdate = FALSE,
        FastCall = FALSE,
        ReadOnlyDoc = TRUE,
        Toggle = FALSE,
        Container = FALSE,
        RecordAbsolute = FALSE,
        RecordPerSet;

        AccelConfig = TRUE,
        MenuConfig = TRUE,
        ToolBoxConfig = TRUE,
        GroupId = SfxGroupId::Edit;
]
```

#### B. Shell-Specific SDI Registration
**File**: `core/sw/sdi/_textsh.sdi`

```cpp
SID_YOUR_COMMAND
[
    ExecMethod = Execute ;
    StateMethod = GetState;
    DisableFlags="SfxDisableFlags::SwOnProtectedCursor";
]
```

**Important**: Add this within the appropriate interface block in the SDI file.

### Step 4: Implement C++ Handlers

**File**: `core/sw/source/uibase/shells/textsh1.cxx`

#### A. Add include for your SID header
```cpp
#include <svx/svxids.hrc>  // for SVX SIDs
// or
#include <cmdid.h>         // for Writer FN_ commands
```

#### B. Add Execute handler
```cpp
case SID_YOUR_COMMAND:
{
    // Get selected text or current context
    OUString sSelectedText;
    rWrtSh.GetSelectedText(sSelectedText);
    
    if (!sSelectedText.isEmpty())
    {
        // Your command logic here
        // TODO: Implement your feature
    }
    
    rReq.Done();
    break;
}
```

#### C. Add GetState handler
```cpp
case SID_YOUR_COMMAND:
{
    // Enable only when appropriate (e.g., when text is selected)
    if (!rSh.HasSelection())
    {
        rSet.DisableItem(nWhich);
    }
    // If conditions are met, leave it enabled
}
break;
```

### Step 5: Add to Context Menu

**File**: `core/sw/uiconfig/swriter/popupmenu/text.xml`

```xml
<menu:menuitem menu:id=".uno:YourCommand"/>
```

Add this line in the appropriate location within the context menu structure.

### Step 6: Force Slot Regeneration

This is the **critical step** that was missing in our initial implementation:

```bash
cd core
rm -rf workdir/SdiTarget/sw/sdi/
make sw
```

**Why this is necessary**:
- LibreOffice caches generated slot mapping files
- New SDI registrations won't take effect until slots are regenerated
- Simply running `make` again won't regenerate cached slots

### Step 7: Verification Steps

#### A. Check Generated Slots
```bash
grep -A5 -B5 "SID_YOUR_COMMAND" workdir/SdiTarget/sw/sdi/swslots.hxx
```

You should see entries like:
```cpp
// Slot Nr. XX : [YOUR_SID_VALUE]
SFX_NEW_SLOT_ARG( SwTextShell,SID_YOUR_COMMAND,SfxGroupId::Edit,
    ...
    SFX_STUB_PTR(SwTextShell,Execute),SFX_STUB_PTR(SwTextShell,GetState),
    ...
```

#### B. Add Debug Logging (Optional)
```cpp
case SID_YOUR_COMMAND:
{
    std::cerr << "*** DEBUG: YourCommand Execute called!" << std::endl;
    // ... your logic
}
```

#### C. Test in LibreOffice
1. Launch LibreOffice Writer
2. Type and select some text
3. Right-click to open context menu
4. Verify your menu item appears
5. Click it and verify it executes

---

## Common Pitfalls and Solutions

### Issue 1: Menu Item Doesn't Appear
**Cause**: SDI slots not regenerated  
**Solution**: Force slot regeneration (Step 6 above)

### Issue 2: Command Exists but Does Nothing
**Cause**: Missing or incorrect C++ handlers  
**Solution**: Verify Execute/GetState cases in textsh1.cxx

### Issue 3: Command is Always Disabled
**Cause**: GetState handler always disabling the command  
**Solution**: Check the conditions in your GetState handler

### Issue 4: Wrong SID Range
**Cause**: Using FN_ for shared commands or SID_ for Writer-specific  
**Solution**: Choose appropriate range (SVX for shared, Writer for specific)

### Issue 5: UNO Command Mismatch
**Cause**: UNO command name in XML doesn't match GenericCommands.xcu  
**Solution**: Ensure exact name matching including case sensitivity

---

## Pattern Comparison: Insert Hyperlink vs Smart Rewrite

### Insert Hyperlink (Working Reference)
- **SID**: `SID_INSERT_HYPERLINK = SID_SVX_START + 458`  
- **UNO**: `.uno:InsertHyperlink`
- **Handler**: `SwTextShell::Execute`/`GetState` in `textsh1.cxx`
- **SDI**: Registered in both `svx.sdi` and `_textsh.sdi`

### Smart Rewrite (Our Implementation)
- **SID**: `SID_SMART_REWRITE = SID_SVX_START + 466`
- **UNO**: `.uno:SmartRewrite`  
- **Handler**: `SwTextShell::Execute`/`GetState` in `textsh1.cxx`
- **SDI**: Registered in both `svx.sdi` and `_textsh.sdi`

Both follow the exact same pattern, which is why our implementation works correctly.

---

## Files Modified for Smart Rewrite Implementation

1. `core/include/svx/svxids.hrc` - Added SID definition
2. `core/officecfg/registry/data/org/openoffice/Office/UI/GenericCommands.xcu` - Added UNO command
3. `core/svx/sdi/svx.sdi` - Added main SDI definition  
4. `core/sw/sdi/_textsh.sdi` - Added shell registration
5. `core/sw/source/uibase/shells/textsh1.cxx` - Added C++ handlers
6. `core/sw/uiconfig/swriter/popupmenu/text.xml` - Added menu item

**Total**: 6 files modified + 1 critical build step (slot regeneration)

---

## Key Insights

1. **LibreOffice's slot system** is the bridge between UNO commands and C++ implementation
2. **SDI files** define this mapping but require regeneration when changed
3. **Shared commands** (SVX range) can be used across applications
4. **Context menu visibility** depends on proper slot generation, not just registration
5. **The build system caches** slot mappings and won't regenerate them automatically

This guide should enable successful implementation of new context menu items in LibreOffice Writer without the trial-and-error process we experienced. 