#!/bin/bash
# Install git hooks for macOS/Linux
cp ./post-merge ../../.git/hooks/post-merge
chmod +x ../../.git/hooks/post-merge
echo "Hook installed!"
