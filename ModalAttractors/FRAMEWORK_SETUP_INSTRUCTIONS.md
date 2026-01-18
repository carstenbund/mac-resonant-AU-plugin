# AUv3 Custom View Fix: Framework Setup Instructions

## Overview

This document explains the changes made to fix the AUv3 custom panel view not showing in the ModalAttractorsExtension. The solution implements Apple's recommended split architecture pattern, separating the UI extension from the audio unit implementation via a framework.

## What Changed

### Architecture Before (Not Working)
```
ModalAttractors (Main App)
└── ModalAttractorsExtension
    ├── Extension Point: com.apple.AudioUnit
    ├── hasCustomView: true
    ├── View Controller (SwiftUI)
    ├── Audio Unit Implementation
    └── All in one bundle
```

### Architecture After (Working - matches Apple's demo)
```
ModalAttractors (Main App)
├── ModalAttractorsExtension (UI Extension)
│   ├── Extension Point: com.apple.AudioUnit-UI
│   ├── Minimal code (imports framework)
│   └── References framework via AudioComponentBundle
│
└── ModalAttractorsFramework (Framework)
    ├── View Controller (SwiftUI + NSHostingController)
    ├── Audio Unit Implementation
    ├── All UI Views and Components
    └── Factory Extension (AUAudioUnitFactory)
```

## Key Differences Found

1. **Extension Point Identifier**
   - Demo uses: `com.apple.AudioUnit-UI` (UI-only extension)
   - Your code used: `com.apple.AudioUnit` (audio unit extension with hasCustomView flag)
   - The UI-specific extension point ensures better host compatibility

2. **AudioComponentBundle**
   - Demo has: Points to framework bundle containing audio unit
   - Your code: Missing (everything in one bundle)
   - This tells the host where to find the actual audio unit

3. **Separation of Concerns**
   - Demo: View controller in framework, minimal extension
   - Your code: Everything combined in extension
   - Separation allows better code reuse and clearer architecture

## Files Created/Modified

### New Files Created

1. **ModalAttractorsFramework/** (entire directory)
   - `Info.plist` - Framework configuration
   - `ModalAttractorsAUViewControllerExtension.swift` - Factory implementation
   - `UI/` - All UI files (ViewController, Views, Utilities)
   - `Common/` - Audio Unit implementation and parameters

2. **ModalAttractorsExtension/ModalAttractorsExtension.swift**
   - Minimal placeholder file that imports framework

### Modified Files

1. **ModalAttractorsExtension/Info.plist**
   - Changed `NSExtensionPointIdentifier` to `com.apple.AudioUnit-UI`
   - Added `AudioComponentBundle` = `bund.media.ModalAttractorsFramework`
   - Changed `NSExtensionPrincipalClass` to `ModalAttractorsFramework.ModalAttractorsAUViewController`
   - Removed `hasCustomView` key (not needed with AudioUnit-UI extension point)

2. **ModalAttractorsFramework/UI/ModalAttractorsAUViewController.swift**
   - Removed `AUAudioUnitFactory` conformance (moved to extension file)
   - Changed properties from `private` to `internal` for extension access
   - Kept SwiftUI hosting and view lifecycle code

## Required Xcode Configuration Steps

### Step 1: Create the Framework Target

1. Open `ModalAttractors.xcodeproj` in Xcode
2. Click on the project in the navigator (top-level ModalAttractors)
3. Click the `+` button at the bottom of the targets list
4. Select **macOS** → **Framework**
5. Configure the framework:
   - Product Name: `ModalAttractorsFramework`
   - Organization Identifier: `bund.media`
   - Bundle Identifier: `bund.media.ModalAttractorsFramework` (should match AudioComponentBundle in extension Info.plist)
   - Language: Swift
   - Click Finish

### Step 2: Add Files to Framework Target

1. In the Project Navigator, select all files in `ModalAttractorsFramework/` directory:
   - `Info.plist`
   - `ModalAttractorsAUViewControllerExtension.swift`
   - All files in `UI/` subdirectory
   - All files in `Common/` subdirectory

2. For each file, open the File Inspector (⌥⌘1) and:
   - Check the box next to `ModalAttractorsFramework` in Target Membership
   - Ensure `ModalAttractorsExtension` is **unchecked** for these files

### Step 3: Configure Framework Build Settings

1. Select the `ModalAttractorsFramework` target
2. Go to **Build Settings**
3. Verify these settings:
   - **Product Name**: `ModalAttractorsFramework`
   - **Product Bundle Identifier**: `bund.media.ModalAttractorsFramework`
   - **Defines Module**: `Yes`
   - **Code Signing**: Same as your other targets
   - **Deployment Target**: Match your minimum macOS version

### Step 4: Link Framework to Extension

1. Select the `ModalAttractorsExtension` target
2. Go to **General** tab
3. In **Frameworks and Libraries** section:
   - Click the `+` button
   - Select `ModalAttractorsFramework.framework`
   - Set to **"Embed & Sign"** or **"Do Not Embed"** (extension will reference the framework from the app bundle)

### Step 5: Link Framework to Main App

1. Select the `ModalAttractors` target (main app)
2. Go to **General** tab
3. In **Frameworks and Libraries** section:
   - Click the `+` button
   - Select `ModalAttractorsFramework.framework`
   - Set to **"Embed & Sign"**

### Step 6: Update Extension Target Membership

1. For files in `ModalAttractorsExtension/` directory:
   - Keep only `ModalAttractorsExtension.swift` and `Info.plist` in the extension target
   - Remove all UI and Audio Unit files from extension target membership
   - These files should now ONLY be in the framework target

2. Specifically check:
   - `UI/ModalAttractorsAUViewController.swift` → Framework only
   - `Common/Audio Unit/ModalAttractorsExtensionAudioUnit.swift` → Framework only
   - `Common/AudioUnitFactory/AudioUnitFactory.swift` → **Can be deleted** (replaced by ViewControllerExtension)

### Step 7: Configure Embed Frameworks Build Phase

1. Select `ModalAttractors` target (main app)
2. Go to **Build Phases** tab
3. Expand **Embed Frameworks** (or create it via Editor → Add Build Phase)
4. Ensure `ModalAttractorsFramework.framework` is listed
5. Ensure **Code Sign On Copy** is checked

### Step 8: Clean and Build

1. Clean the build folder: **Product** → **Clean Build Folder** (⇧⌘K)
2. Build the framework target first: Select `ModalAttractorsFramework` scheme and build (⌘B)
3. Build the extension: Select `ModalAttractorsExtension` scheme and build (⌘B)
4. Build the main app: Select `ModalAttractors` scheme and build (⌘B)

## Bundle Structure After Build

When built correctly, your app bundle should look like:
```
ModalAttractors.app/
├── Contents/
│   ├── Frameworks/
│   │   └── ModalAttractorsFramework.framework/
│   │       ├── ModalAttractorsFramework (binary)
│   │       └── Resources/
│   │           └── Info.plist
│   └── PlugIns/
│       └── ModalAttractorsExtension.appex/
│           ├── Contents/
│           │   └── Info.plist (com.apple.AudioUnit-UI)
│           └── (References framework via AudioComponentBundle)
```

## How It Works

1. **Host loads the extension**: When a DAW loads your AUv3, it looks for extensions with `com.apple.AudioUnit-UI` extension point

2. **Extension references framework**: The extension's Info.plist specifies:
   - `AudioComponentBundle`: Where to find the audio unit
   - `NSExtensionPrincipalClass`: The view controller class to instantiate

3. **Framework provides both UI and Audio Unit**: The framework contains:
   - `ModalAttractorsAUViewController` - The view controller
   - `ModalAttractorsExtensionAudioUnit` - The audio unit
   - `AUAudioUnitFactory` extension - Factory to create the audio unit

4. **Host instantiates view controller**: The host calls the factory method on the view controller, which creates the audio unit and configures the SwiftUI view

## Troubleshooting

### Build Error: "No such module 'ModalAttractorsFramework'"
- Ensure framework target is built before extension
- Check framework is added to extension's linked frameworks
- Verify "Defines Module" is set to "Yes" in framework build settings

### Runtime Error: "Could not find principal class"
- Verify bundle identifier in framework Info.plist matches AudioComponentBundle value
- Check that framework is embedded in main app bundle
- Ensure view controller class is marked `public`

### Custom View Still Not Showing
- Check that extension Info.plist has correct extension point: `com.apple.AudioUnit-UI`
- Verify AudioComponentBundle matches framework bundle identifier exactly
- Test in different hosts (Logic Pro, GarageBand, etc.)
- Add more NSLog statements to track when viewDidLoad is called

### Code Signing Issues
- Ensure all targets (app, extension, framework) use the same signing certificate
- Check that "Code Sign On Copy" is enabled for embedded framework
- Verify framework is signed: `codesign -vv ModalAttractors.app/Contents/Frameworks/ModalAttractorsFramework.framework`

## SwiftUI vs XIB

This implementation uses **SwiftUI** instead of Interface Builder (XIB) files like the demo. This is perfectly valid and works the same way:

- Demo: `NSViewController` + `.xib` file + AppKit views
- Your code: `AUViewController` + `NSHostingController` + SwiftUI views

The framework pattern works for both approaches. The key is the split architecture, not the UI technology used.

## Next Steps After Configuration

1. Build and run the main app
2. Load the AUv3 in your DAW (Logic Pro, GarageBand, etc.)
3. Verify the custom SwiftUI view appears
4. Check that parameters update correctly
5. If issues persist, check Console.app for debug logs from NSLog statements

## References

- Apple's AUv3FilterDemo: `/demo-project-filter/` in this repository
- Apple Documentation: [Creating Custom Audio Units](https://developer.apple.com/documentation/audiotoolbox/creating_custom_audio_effects)
- WWDC Sessions on Audio Units and App Extensions
