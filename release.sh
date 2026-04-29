#!/bin/bash

read -p  "Want you to push on github ? [y/n] " ANSWER

if [[ $ANSWER == "y" ]]; then
    read -p "Your commit message : " COMMITMSG
    git add -A
    git commit -m "$COMMITMSG"
    git push
    echo "Project pushed on github"
fi

read -p "Want you to publish a release ? [y/n] " RELEASEANSWER

if [[ $RELEASEANSWER == "y" ]]; then
    read -p "Release version: " VERSION
    read -p "Release notes: " NOTES
    
    # Spinner function
    spinner() {
        local pid=$!
        local spinstr='|/-\'
        local i=0
        while kill -0 $pid 2>/dev/null; do
            i=$(((i+1)%4))
            printf "\r%s [%c] " "$1" "${spinstr:$i:1}"
            sleep 0.1
        done
        wait $pid
        printf "\r%s [DONE]\n" "$1"
    }
    
    echo "Building release..."
    
    # Linux build
    echo -n "Cleaning... "
    make clean >/dev/null 2>&1 & spinner "Cleaning"
    
    echo -n "Linux build... "
    make >/dev/null 2>&1 & spinner "Linux build"
    
    if [[ ! -f "main" ]]; then
        echo "❌ ERROR: Linux build failed (no main)"
        exit 1
    fi
    
    mkdir -p linux && cp "main" "linux/" && rm main
    
    # Windows build
    echo -n "Windows build... "
    make clean >/dev/null 2>&1 & spinner "Cleaning"
    make -f "makefile.win.linux" >/dev/null 2>&1 & spinner "Windows build"
    
    if [[ ! -f "main.exe" ]]; then
        echo "❌ ERROR: Windows build failed (no main.exe)"
        exit 1
    fi
    
    mkdir -p windows && cp "main.exe" "windows/" && rm main.exe
    
    
    # Copy assets (créé dossier si absent)
    mkdir -p windows/assets linux/assets
    cp -r dll/* windows/
    if [[ -d "assets" ]]; then
        cp -r assets/. windows/assets/ 2>/dev/null || true
        cp -r assets/. linux/assets/ 2>/dev/null || true
    else
        echo "⚠️  No assets folder found"
    fi
    
    # ZIPs
    echo -n "Windows ZIP... "
    zip -r "v$VERSION-windows.zip" "windows" >/dev/null 2>&1 & spinner "Windows ZIP"
    
    echo -n "Linux ZIP... "
    zip -r "v$VERSION-linux.zip" "linux" >/dev/null 2>&1 & spinner "Linux ZIP"
    
    # Confirmation
    echo "Release v$VERSION ready!"
    ls -lh v$VERSION-*.zip
    read -p "Publish to GitHub? [y/N]: " CONFIRM
    if [[ $CONFIRM =~ ^[Yy]$ ]]; then
        gh release create "v$VERSION" "./v$VERSION-linux.zip" "./v$VERSION-windows.zip" \
            --title "Version $VERSION" --notes "$NOTES"
        echo "✅ Published!"
        # Nettoyage
        rm -rf windows linux v$VERSION-*.zip
    else
        rm -rf windows linux v$VERSION-*.zip
        echo "❌ Cancelled."
    fi
fi
