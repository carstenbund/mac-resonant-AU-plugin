#!/bin/bash

echo "==========================================="
echo "Force Audio Component Re-registration"
echo "==========================================="
echo ""

echo "This script will:"
echo "1. Increment the plugin version number"
echo "2. Kill audio system daemons to clear cache"
echo "3. Force pluginkit to re-scan your extension"
echo ""

# Step 1: Increment version in Info.plist
echo "Step 1: Incrementing version number..."
PLIST_PATH="ModalAttractors/ModalAttractorsExtension/Info.plist"

if [ -f "$PLIST_PATH" ]; then
    # Read current version
    CURRENT_VERSION=$(/usr/libexec/PlistBuddy -c "Print :NSExtension:NSExtensionAttributes:AudioComponents:0:version" "$PLIST_PATH" 2>/dev/null)

    if [ -n "$CURRENT_VERSION" ]; then
        NEW_VERSION=$((CURRENT_VERSION + 1))
        echo "  Current version: $CURRENT_VERSION"
        echo "  New version: $NEW_VERSION"

        # Update version
        /usr/libexec/PlistBuddy -c "Set :NSExtension:NSExtensionAttributes:AudioComponents:0:version $NEW_VERSION" "$PLIST_PATH"

        # Also update xcode-project version
        XCODE_PLIST="xcode-project/ModalAttractorsExtension/Info.plist"
        if [ -f "$XCODE_PLIST" ]; then
            /usr/libexec/PlistBuddy -c "Set :NSExtension:NSExtensionAttributes:AudioComponents:0:version $NEW_VERSION" "$XCODE_PLIST"
            echo "  ✓ Version updated in both Info.plist files"
        fi
    else
        echo "  ⚠ Could not read current version"
    fi
else
    echo "  ⚠ Info.plist not found at $PLIST_PATH"
fi

echo ""
echo "Step 2: Clearing Audio Component cache..."

# Kill audio component registrar
echo "  - Killing AudioComponentRegistrar..."
killall -9 AudioComponentRegistrar 2>/dev/null && echo "    ✓ Killed" || echo "    (not running)"

# Kill other audio daemons
echo "  - Killing coreaudiod..."
sudo killall -9 coreaudiod 2>/dev/null && echo "    ✓ Killed" || echo "    (not running)"

echo "  - Killing audiomxd..."
killall -9 audiomxd 2>/dev/null && echo "    ✓ Killed" || echo "    (not running)"

echo ""
echo "Step 3: Resetting pluginkit database..."
pluginkit -m -v 2>/dev/null | grep -i modal || echo "  Plugin not yet registered"

echo ""
echo "==========================================="
echo "Next Steps:"
echo "==========================================="
echo "1. Clean Build Folder in Xcode (⇧⌘K)"
echo "2. Rebuild your project"
echo "3. Run your app to register the extension"
echo "4. Test in your DAW and check Console.app"
echo ""
echo "Look for: \"HasCustomView\": 1"
echo "NOT:      \"HasCustomView\": 0"
echo ""
echo "If still 0, you may need to reboot macOS."
echo "==========================================="
