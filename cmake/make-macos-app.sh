#!/bin/sh
# make-macos-app.sh <build dir> <out dir> [version]: wraps <build dir>/evnova64 and its FFmpeg dylibs into
# "<out dir>/EV Nova Plus.app" (ad-hoc signed). Runs on macOS. The game's data stays beside the .app
# (PlatExeFolder looks there).
set -e
build=$1
out=$2
version=${3:-0.0.0}
app="$out/EV Nova Plus.app"

rm -rf "$app"
mkdir -p "$app/Contents/MacOS" "$app/Contents/Frameworks" "$app/Contents/Resources"
cp "$build/evnova64" "$app/Contents/MacOS/"
cp "$build"/*.dylib "$app/Contents/Frameworks/"
# the dylibs are @rpath libraries; the program finds them in Frameworks
install_name_tool -add_rpath @executable_path/../Frameworks "$app/Contents/MacOS/evnova64"

cat > "$app/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>           <string>evnova64</string>
    <key>CFBundleIdentifier</key>           <string>io.github.xellzul.evplus</string>
    <key>CFBundleName</key>                 <string>EV Nova Plus</string>
    <key>CFBundleDisplayName</key>          <string>EV Nova Plus</string>
    <key>CFBundlePackageType</key>          <string>APPL</string>
    <key>CFBundleShortVersionString</key>   <string>$version</string>
    <key>CFBundleVersion</key>              <string>$version</string>
    <key>LSMinimumSystemVersion</key>       <string>11.0</string>
    <key>NSHighResolutionCapable</key>      <true/>
    <key>LSApplicationCategoryType</key>    <string>public.app-category.games</string>
</dict>
</plist>
EOF

# Apple silicon runs only signed code; install_name_tool broke the linker's signature
codesign --force --sign - "$app/Contents/Frameworks/"*.dylib
codesign --force --sign - "$app"
