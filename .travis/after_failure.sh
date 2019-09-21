#!/bin/bash

set -e

# Travis Webhook
wget https://raw.githubusercontent.com/DiscordHooks/travis-ci-discord-webhook/master/send.sh
chmod +x send.sh
./send.sh failure $WEBHOOK_URL
