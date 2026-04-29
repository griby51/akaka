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
    read -p "The version of the release : " VERSION
    read -p "The notes of the release : " NOTES
    make clean
    make
    cp "main" "linux"
    rm main
    make clean
    make -f "makefile.win.linux"
    cp "main.exe" "windows"
    rm main.exe
    cp -r "assets/*" "windows/assets" "linux/assets"
    zip -r "v$VERSION-windows.zip" "windows"
    zip -r "v$VERSION-linux.zip" "linux"
    gh release create "v$VERSION" "./v$VERSION-linux.zip" "./v$VERSION-windows.zip" --title "Version $VERSION" --notes "$NOTES"
    echo "Release created"
fi
