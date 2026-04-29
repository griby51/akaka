#!/bin/bash

read -p  "Want you to push on github ? [y/n] " ANSWER

if [[ $ANSWER == "y" ]]; then
    read -p "Your commit message : " COMMITMSG
    git add -A
    git commit -m "$COMMITMSG"
    git push
    echo "You answered yes"
fi
