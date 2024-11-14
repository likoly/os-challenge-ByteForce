#!/bin/sh


PATHTOCOMMON=~/os/master/
# Get the first argument passed to the script, or default to 5002 if no argument is given
PORT=${1:-5000}

# Run the server with the given or default port
$(./get-bin-path.sh)/server $PORT
