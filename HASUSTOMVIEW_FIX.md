# HasCustomView = 0 Fix Guide

## The Problem

Your console logs show:
```
"HasCustomView": 0
```

This is why hosts show the generic parameter tree instead of your custom SwiftUI view. When `HasCustomView` is `0` (false), macOS tells hosts "this plugin has no custom UI," so they skip calling `loadView()` and `viewDidLoad()` on your ViewController.

**Evidence from your logs:**
- ✅ ViewController.init() is called (🔵)
- ✅ createAudioUnit() is called (🟢)
- ✅ Parameters are bound (🔶)
- ❌ loadView() is NEVER called (no 🟡)
- ❌ viewDidLoad() is NEVER called (no 🟠)
- ❌ MainView never appears (no 🎨)

The host **deliberately skips** loading your custom view because macOS reported `HasCustomView: 0`.

---

## Why This Happens

The `HasCustomView` property is:
1. **Calculated by macOS** when your extension is first registered
2. **Cached in the Audio Component database**
3. **Only recalculated when:**
   - Version number changes
   - Bundle identifier changes
   - Audio component cache is cleared
   - System is rebooted

Your Info.plist is **correctly configured** - this is purely a **caching issue**.

---

## The Solution

### Option 1: Increment Version Number (Recommended)

This forces macOS to treat your plugin as a "new" component and recalculate `HasCustomView`.

**Run the script:**
```bash
chmod +x FORCE_REREGISTER.sh
./FORCE_REREGISTER.sh
```

Or **manually** in your Info.plist files:

Change:
```xml
<key>version</key>
<integer>67072</integer>
```

To:
```xml
<key>version</key>
<integer>67073</integer>
```

Update **both** Info.plist files:
- `ModalAttractors/ModalAttractorsExtension/Info.plist`
- `xcode-project/ModalAttractorsExtension/Info.plist`

Then:
1. **Clean Build Folder** in Xcode (⇧⌘K)
2. **Rebuild**
3. **Run your app** to register the extension
4. **Test in your DAW**
5. **Check Console.app** - you should now see `"HasCustomView": 1`

---

### Option 2: Clear Audio Component Cache

```bash
# Kill audio component registrar
killall -9 AudioComponentRegistrar

# Kill core audio daemon (requires sudo)
sudo killall -9 coreaudiod

# Clear pluginkit cache
pluginkit -r ~/Library/Developer/Xcode/DerivedData/*/Build/Products/*/ModalAttractors.app/Contents/PlugIns/ModalAttractorsExtension.appex
```

Then rebuild and test.

---

### Option 3: Reboot (Most Reliable)

If the above don't work:

1. Increment version number
2. Clean build folder
3. Rebuild
4. **Reboot macOS**
5. Test again

Rebooting clears all audio system caches and forces complete re-registration.

---

## How to Verify the Fix

After applying the fix, rebuild and load your plugin. In Console.app, you should see:

### BEFORE (Broken):
```
🔵 ModalAttractorsAUViewController.init() - ViewController INSTANTIATED
🟢 createAudioUnit called
🔍 Checking AudioComponent custom view capability...
"HasCustomView": 0  ← PROBLEM!
🔶 bindAudioUnit - Parameters bound
(no loadView, no viewDidLoad, no MainView appeared)
```

### AFTER (Fixed):
```
🔵 ModalAttractorsAUViewController.init() - ViewController INSTANTIATED
🟡 loadView - Creating root view  ← NOW APPEARS!
🟠 viewDidLoad - Setting up SwiftUI hosting controller  ← NOW APPEARS!
🟣 setupHostingController - Building SwiftUI view hierarchy  ← NOW APPEARS!
🟢 createAudioUnit called
🔍 Checking AudioComponent custom view capability...
"HasCustomView": 1  ← FIXED!
🔶 bindAudioUnit - Parameters bound
🎨 MainView appeared - Custom tabbed UI is RENDERING  ← NOW APPEARS!
```

You should also see your **custom tabbed SwiftUI UI** instead of the generic parameter list!

---

## If Still Broken After Fix

If `HasCustomView` is still `0` after all the above:

1. **Check compiled .appex bundle:**
   ```bash
   # Find your built extension
   find ~/Library/Developer/Xcode/DerivedData -name "ModalAttractorsExtension.appex" | head -1

   # Check the Info.plist inside it
   plutil -p /path/to/ModalAttractorsExtension.appex/Contents/Info.plist
   ```

   Verify:
   - NSExtensionPointIdentifier is `com.apple.AudioUnit`
   - NSExtensionPrincipalClass is set
   - Version number increased

2. **Check the ViewController is included in target:**
   - Open Xcode
   - Select `ModalAttractorsAUViewController.swift`
   - Open File Inspector (right panel)
   - Verify "Target Membership" includes the Extension target

3. **Check for build errors:**
   - Clean Build Folder (⇧⌘K)
   - Rebuild
   - Check build log for warnings about the ViewController

4. **Nuclear option - Change Bundle ID:**
   If nothing else works, change your bundle identifier slightly (add `.v2` to the end), increment version, and rebuild. This forces macOS to treat it as a completely new plugin.

---

## Summary

**Root Cause:** macOS cached `HasCustomView: 0` for your component

**Fix:** Increment version number to force re-registration

**Expected Result:** `HasCustomView: 1` and your custom SwiftUI tabbed view appears

**If still broken:** Reboot macOS after version increment

Good luck! 🎉
