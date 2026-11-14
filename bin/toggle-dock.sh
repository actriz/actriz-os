#!/bin/bash
if pgrep -x "dock" > /dev/null; then
    pkill -x "dock"
else
    /opt/actriz/bin/dock &
fi
