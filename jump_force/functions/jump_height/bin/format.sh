#!/bin/bash

# abort on errors
set -e

BASEDIR=$(dirname "$0")

"$BASEDIR/../../../../packages/scripts/poetry/format.sh" functions/python_lambda/application_analyze
