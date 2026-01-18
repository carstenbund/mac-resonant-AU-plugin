# AUv3 Custom View Diagnostic Checklist

This checklist will help you diagnose why hosts are showing a generic parameter tree instead of your custom SwiftUI tabbed view.

## Critical Fix Applied

✅ **Info.plist NSExtensionPointIdentifier** - Changed from `com.apple.AudioUnit` to `com.apple.AudioUnit-v3`

This was a CRITICAL bug. Without the `-v3` suffix, hosts may not recognize this as a proper AUv3 extension and will fall back to the generic parameter view.

---

## Diagnostic Procedure

### Step 1: Verify the Fix is Compiled

After rebuilding, verify the extension bundle has the corrected Info.plist:

```bash
# Find the built extension
find ~/Library/Developer/Xcode/DerivedData -name "*.appex" -path "*ModalAttractors*"

# Or if built to a specific location
# Check the compiled Info.plist
plutil -p /path/to/ModalAttractorsExtension.appex/Contents/Info.plist | grep NSExtensionPointIdentifier
```

**Expected Output:**
```
"NSExtensionPointIdentifier" => "com.apple.AudioUnit-v3"
```

**If you see `com.apple.AudioUnit` (without -v3):** The build didn't pick up the change. Clean build folder and rebuild.

---

### Step 2: Check Console Logs (MOST IMPORTANT)

The debug version now has extensive logging with color-coded emojis to trace the initialization sequence.

**How to check:**

1. Open **Console.app** on your Mac
2. In the search bar, type: `ModalAttractors` or `AUv3 DEBUG`
3. Clear the log (optional)
4. Open your plugin in a host (Logic, GarageBand, Ableton, etc.)
5. Watch the console output in real-time

**Expected Log Sequence (What You SHOULD See):**

```
🔵 AUv3 DEBUG: ModalAttractorsAUViewController.init() - ViewController INSTANTIATED
🟡 AUv3 DEBUG: loadView - Creating root view programmatically (size: 520.0x400.0)
🟡 AUv3 DEBUG: loadView complete
🟠 AUv3 DEBUG: viewDidLoad - Setting up SwiftUI hosting controller
🟣 AUv3 DEBUG: setupHostingController - Building SwiftUI view hierarchy
🟣 AUv3 DEBUG: Hosting controller preferredContentSize: 520.0x400.0
🟣 AUv3 DEBUG: setupHostingController complete - custom view ready
🟠 AUv3 DEBUG: viewDidLoad complete - view hierarchy ready
🟢 AUv3 DEBUG: createAudioUnit called - type=aumi, subtype=Test, manufacturer=Bund
🔷 AUv3 DEBUG: AudioUnit init - type=aumi, subtype=Test, mfr=Bund
🔍 AUv3 DEBUG: Querying AudioComponent for hasCustomView property
🔍 AUv3 DEBUG: AudioComponent found, checking properties...
✅ AUv3 DEBUG: *** hasCustomView = TRUE *** - Hosts SHOULD request custom UI
🟢 AUv3 DEBUG: Audio unit created, type: ModalAttractorsExtensionAudioUnit
🟢 AUv3 DEBUG: createAudioUnit returning AU instance to host
🟢 AUv3 DEBUG: Setting audioUnit on main queue (async)
🔶 AUv3 DEBUG: bindAudioUnit - Attempting to bind parameter tree to UI
🔶 AUv3 DEBUG: Binding parameter tree with X parameters
🔶 AUv3 DEBUG: Parameter tree bound to UI successfully
🎨 AUv3 DEBUG: MainView appeared - Custom tabbed UI is RENDERING
```

**Diagnostic Results:**

| What You See | What It Means | Next Step |
|-------------|---------------|-----------|
| **Nothing at all** | Host cannot find your View Controller class | → Go to Step 3 (Class Discovery) |
| **🔵 init() only** | ViewController created but not loaded | → Go to Step 4 (View Loading) |
| **🔵🟡🟠🟣 but no 🟢** | View loaded but createAudioUnit never called | → Go to Step 5 (Factory) |
| **🔴 hasCustomView = FALSE** | **CRITICAL:** System doesn't know you have custom UI! | → Check Info.plist NSExtensionPointIdentifier |
| **✅ hasCustomView = TRUE but generic UI** | Component recognized but host ignoring it | → Go to Step 6 (Visual Check) |
| **All logs + 🎨 MainView appeared** | Everything worked! Custom UI should be visible | → Victory! |
| **🔴 ERROR logs** | Specific error occurred | → Read error message carefully |

---

### Step 2b: The hasCustomView Property Check (MOST CRITICAL)

This is THE definitive test. If `hasCustomView = FALSE`, the system doesn't know your extension has a custom UI, and hosts will ALWAYS show the generic parameter view.

**Look for these logs:**

```
🔍 AUv3 DEBUG: Querying AudioComponent for hasCustomView property
🔍 AUv3 DEBUG: AudioComponent found, checking properties...
```

**Then one of:**

**✅ GOOD:**
```
✅ AUv3 DEBUG: *** hasCustomView = TRUE *** - Hosts SHOULD request custom UI
```
Your component is properly configured! If you still see generic UI, it's a host-specific issue (go to Step 6).

**🔴 BAD:**
```
🔴 AUv3 ERROR: *** hasCustomView = FALSE *** - This explains generic parameter view!
🔴 AUv3 ERROR: Info.plist may be missing NSExtension configuration
```
This is THE problem! Your Info.plist is missing or has incorrect NSExtension configuration.

**Fixes if hasCustomView = FALSE:**

1. ✅ **Already fixed in this build:** Changed `NSExtensionPointIdentifier` from `com.apple.AudioUnit` to `com.apple.AudioUnit-v3`

2. **Verify the fix compiled:**
   ```bash
   # Find built extension
   find ~/Library/Developer/Xcode/DerivedData -name "ModalAttractorsExtension.appex" 2>/dev/null | head -1

   # Check Info.plist (replace path with above result)
   plutil -p /path/to/ModalAttractorsExtension.appex/Contents/Info.plist | grep -A1 NSExtensionPointIdentifier
   ```

   Should show: `"NSExtensionPointIdentifier" => "com.apple.AudioUnit-v3"`

3. **If still FALSE after rebuild:**
   - Clean Build Folder in Xcode (⇧⌘K)
   - Check that you're building the correct target
   - Verify Info.plist in source shows `-v3`
   - Reset plugin cache: `killall -9 AudioComponentRegistrar`

---

### Step 3: Class Discovery Test

If you see NO logs at all in Console.app:

**Problem:** The host cannot find the `ModalAttractorsAUViewController` class in the Objective-C runtime.

**Checks:**

1. **@objc attribute present:**
   ```swift
   @objc(ModalAttractorsAUViewController)
   public class ModalAttractorsAUViewController: AUViewController, AUAudioUnitFactory
   ```
   ✅ Already present in your code.

2. **Info.plist NSExtensionPrincipalClass matches:**
   ```xml
   <key>NSExtensionPrincipalClass</key>
   <string>$(PRODUCT_MODULE_NAME).ModalAttractorsAUViewController</string>
   ```
   ✅ Already correct in your code.

3. **Target Membership:**
   - In Xcode, select `ModalAttractorsAUViewController.swift`
   - Open File Inspector (right panel)
   - Verify it's checked for the **Extension** target (not just the App target)

4. **Clean and Rebuild:**
   ```bash
   # In Xcode: Product → Clean Build Folder (Shift+Cmd+K)
   # Then: Product → Build (Cmd+B)
   ```

---

### Step 4: View Loading Test

If you see 🔵 init() but nothing after:

**Problem:** The ViewController was instantiated but `loadView()` or `viewDidLoad()` never called.

**Possible Causes:**

1. **Host crashed during view setup** - Check Crash Reports:
   ```bash
   open ~/Library/Logs/DiagnosticReports/
   # Look for recent crashes related to your extension
   ```

2. **Sandbox permissions issue** - Check Console.app for sandbox violations:
   ```
   # Filter by "Sandbox" in Console.app
   ```

3. **App Groups mismatch** - Verify in Xcode:
   - Select Extension target → Signing & Capabilities
   - Check if "App Groups" capability exists
   - If yes, ensure the App target has the SAME app group

---

### Step 5: Factory Method Test

If you see 🔵🟡🟠🟣 but no 🟢:

**Problem:** The view was set up but `createAudioUnit(with:)` was never called.

**Possible Causes:**

1. **Host is using in-process mode instead of factory** - Some hosts call `requestViewController()` on the Audio Unit instead of instantiating via the factory.

2. **Check if requestViewController is being called:**

   In `ModalAttractorsExtensionAudioUnit.swift:217-234`, there's a `requestViewController` method. Add logging there:

   ```swift
   public override func requestViewController(
       completionHandler: @escaping (AUViewController?) -> Void
   ) {
       NSLog("⚡️ AUv3 DEBUG: requestViewController called on AudioUnit (in-process mode)")
       // ... rest of method
   }
   ```

3. **Wrong component description** - The host might be requesting a different AU type.

---

### Step 6: Visual Verification Test

If ALL logs appear but you still see the generic parameter list:

**Test 1: Look for the blue background**

The debug build sets a light blue background on the root view. If you see even a hint of blue, your custom view IS loading but might be:
- Hidden behind the generic view
- Sized to zero
- Not being prioritized by the host

**Test 2: Check preferredContentSize**

Look for this log:
```
🟣 AUv3 DEBUG: Hosting controller preferredContentSize: 520.0x400.0
```

If the size is `0.0x0.0`, the host might be rejecting the view.

**Test 3: Activity Monitor Check**

1. Open Activity Monitor
2. Search for: `ModalAttractorsExtension`
3. Does the process exist and stay running?
   - **Yes**: Extension is loaded successfully
   - **No / Crashes**: Extension is crashing silently - check Crash Reports

---

### Step 7: Host-Specific Issues

Some hosts have quirks:

**Logic Pro / GarageBand:**
- Usually respect custom views well
- Try: Close and reopen the plugin UI
- Try: Save session, quit, reopen

**Ableton Live:**
- Sometimes caches the generic view
- Try: Delete the plugin, re-scan plugins
- Try: Check "Show Plugin Window" vs "Show Editor"

**Reaper:**
- Has "Generic" vs "Native" UI modes
- Right-click plugin → UI → Try different options

**FL Studio:**
- macOS support varies
- May always show generic view for AUv3

---

### Step 8: auval Validation

Run Apple's Audio Unit validation tool:

```bash
auval -v aumi Test Bund
```

**What to look for:**

1. **Does it pass?**
   ```
   * * PASS
   ```

2. **Does it mention a custom view?**
   ```
   AU Validation Successful.
   ```

3. **Any warnings about the view?**

---

## Quick Reference: Log Color Codes

| Emoji | Stage | Meaning |
|-------|-------|---------|
| 🔵 | Init | ViewController class instantiated by host |
| 🟡 | Load View | Root NSView being created |
| 🟠 | View Did Load | SwiftUI hosting controller setup starting |
| 🟣 | Setup Hosting | SwiftUI view hierarchy being built |
| 🟢 | Create AU | Audio Unit factory method called |
| 🔷 | AU Init | Audio Unit DSP engine initialized |
| 🔍 | Check Property | Checking AudioComponent hasCustomView property |
| ✅ | Success | hasCustomView = TRUE (component properly configured!) |
| 🔶 | Bind | Parameter tree being connected to UI |
| 🎨 | UI Render | SwiftUI view appeared and rendering |
| 🔴 | Error | Something went wrong - read the message! |
| ⚠️ | Warning | Non-critical issue detected |
| ⚡️ | Request VC | In-process requestViewController called |

---

## Most Likely Causes (in order)

Based on the debug logs you see, here are the most common issues:

1. **NO LOGS** → Info.plist issue or class not found (FIXED by changing to `-v3`)
2. **Init but no loadView** → Sandbox crash or target membership issue
3. **View loaded but generic UI** → Host preference, size issue, or host bug
4. **All logs but light blue background visible** → View is there but behind generic UI (host issue)

---

## Next Steps After Testing

1. **Run the plugin in a host**
2. **Capture the console logs**
3. **Compare against the expected sequence above**
4. **Report findings:**
   - Which logs appeared?
   - Which logs were missing?
   - Any crash reports?
   - Which host/DAW are you testing in?

---

## Additional Debug Commands

### Find the built .appex bundle:
```bash
find ~/Library/Developer/Xcode/DerivedData -name "ModalAttractorsExtension.appex" 2>/dev/null
```

### Inspect the bundle:
```bash
# Replace with actual path from above
cd /path/to/ModalAttractorsExtension.appex/Contents
ls -la
plutil -p Info.plist
```

### Check if extension is registered:
```bash
pluginkit -m -v -i com.bund.media.ModalAttractorsExtension
```

### Reset plugin cache (if plugin not appearing):
```bash
killall -9 AudioComponentRegistrar
```

---

## Summary

The CRITICAL fix has been applied (`com.apple.AudioUnit-v3`), and extensive debug logging has been added. The next step is to:

1. **Rebuild** the project completely (Clean Build Folder first)
2. **Open the plugin** in your preferred host
3. **Watch Console.app** for the color-coded debug logs
4. **Compare** what you see against the expected sequence in Step 2
5. **Report back** with your findings

The logs will tell us exactly where the handshake is breaking!
