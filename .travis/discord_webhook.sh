#!/bin/bash

set -e

# Travis Webhook
wget https://raw.githubusercontent.com/DiscordHooks/travis-ci-discord-webhook/master/send.sh
chmod +x send.sh
./send.sh ${1} ${WEBHOOK_URL}
