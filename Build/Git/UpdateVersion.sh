#!/bin/bash
# Update version information from git for macOS/Linux

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

FILE="../../Partitura/AgentinoVoce.arp/VersionInfo.acc.xtrsvn"

# Attempt to fetch and unshallow if needed (suppress errors)
git fetch --prune --unshallow 2>/dev/null

# Get revision count from origin/master or HEAD
REV=$(git rev-list --count origin/master 2>/dev/null)
if [ -z "$REV" ]; then
    REV=$(git rev-list --count HEAD 2>/dev/null)
fi

if [ -z "$REV" ]; then
    echo "Failed to compute revision count."
    exit 1
fi

# Check if working tree is dirty
if git diff-index --quiet HEAD --; then
    DIRTY=0
else
    DIRTY=1
fi

echo "Git revision: $REV, dirty: $DIRTY"
echo "Processing file: $FILE"

# Generate output filename by removing .xtrsvn extension
OUT="${FILE%.xtrsvn}"

# Process the file and replace placeholders
# Using double quotes and proper escaping for $
sed -e "s/\\\$WCREV\\\$/$REV/g" -e "s/\\\$WCMODS?1:0\\\$/$DIRTY/g" "$FILE" > "$OUT"

echo "Wrote $OUT with WCREV=$REV and WCMODS=$DIRTY"
