#!/bin/bash

# abort on errors
set -e

BASEDIR=$(dirname "$0")

"$BASEDIR/../../../packages/scripts/misc/sync_lambda.sh" functions/create_user amplify/backend/function/createUser
