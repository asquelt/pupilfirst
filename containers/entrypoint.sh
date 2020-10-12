#!/bin/bash

set -e

# Remove a potentially pre-existing server.pid for Rails.
rm -f /myapp/tmp/pids/server.pid

# Prepare DB
bundle exec rails db:setup

# Run container
exec "$@"
