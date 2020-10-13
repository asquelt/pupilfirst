#!/bin/bash

set -e

# Remove a potentially pre-existing server.pid for Rails.
rm -f /myapp/tmp/pids/server.pid

# Set defaults
[ -z "$RAILS_ENV" ] && export RAILS_ENV=production
[ -z "$RACK_ENV" ] && export RACK_ENV=$RAILS_ENV
[ -z "$JAVASCRIPT_DRIVER" ] && export JAVASCRIPT_DRIVER=headless_chrome
[ -z "$SPEC_USER_TIME_ZONE" ] && export SPEC_USER_TIME_ZONE=Etc/UTC
[ -z "$DEFAULT_SENDER_EMAIL_ADDRESS" ] && export DEFAULT_SENDER_EMAIL_ADDRESS=admin@$ASSET_HOST
[ -z "$SSO_DOMAIN" ] && export SSO_DOMAIN=sso.$ASSET_HOST
[ -z "$CAPYBARA_MAX_WAIT_TIME" ] && export CAPYBARA_MAX_WAIT_TIME=30
[ -z "$SLOWPOKE_TIMEOUT" ] && export SLOWPOKE_TIMEOUT=300
[ -z "$WEBHOOK_READ_TIMEOUT" ] && export WEBHOOK_READ_TIMEOUT=30
[ -z "$GRAPH_API_RATE_LIMIT" ] && export GRAPH_API_RATE_LIMIT=30
[ -z "$RUBYOPT" ] && export RUBYOPT="-W:no-deprecated -W:no-experimental"
[ -z "$RAILS_SERVE_STATIC_FILES" ] && export RAILS_SERVE_STATIC_FILES=true

# This has been set in container metadata, so let's keep it hardcoded
export PORT=3000

# Container native logging
export RAILS_LOG_TO_STDOUT=true

# Prepare DB
err=0
[ -z "DB_HOST" ] && "DB_HOST is mandatory" && err=1
[ -z "DB_NAME" ] && "DB_NAME is mandatory" && err=1
[ -z "DB_USERNAME" ] && "DB_USERNAME is mandatory" && err=1
[ -z "DB_PASSWORD" ] && "DB_PASSWORD is mandatory" && err=1
[ $err -gt 0 ] && exit 1

bundle exec rails db:setup || :

# Bootstrap the App
err=0
[ -z "$INITIAL_ADMIN_PASSWORD" ] && echo "INITIAL_ADMIN_PASSWORD is mandatory" && err=1
[ -z "$ASSET_HOST" ] && echo "ASSET_HOST is mandatory" && err=1
[ $err -gt 0 ] && exit 1

bundle exec rails console -e $RAILS_ENV <<.
user = User.find_by(email: 'admin@example.com')
user.update!(password: '$INITIAL_ADMIN_PASSWORD', password_confirmation: '$INITIAL_ADMIN_PASSWORD')
School.first.domains.create!(fqdn: '$ASSET_HOST', primary: true)
.

# Run container
exec "$@"
