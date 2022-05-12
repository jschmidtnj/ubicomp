#!/bin/bash

# abort on errors
set -e

BASEDIR=$(dirname "$0")

"$BASEDIR/../../../../packages/scripts/poetry/lint.sh" functions/python_lambda/application_analyze
