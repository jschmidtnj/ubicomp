#!/bin/bash

# abort on errors
set -e

BASEDIR=$(dirname "$0")

"$BASEDIR/../../../packages/scripts/misc/check_changes.sh" functions/create_user
