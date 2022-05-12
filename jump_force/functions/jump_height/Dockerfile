# docker container for lambda functions with python
FROM python:3.10

LABEL mantainer="flex"

ARG OPENCV_VERSION="4.5.5"

ENV OPENCV_VERSION="$OPENCV_VERSION"

# poppler-utils for merging pdf

RUN apt-get -qq update \
  && apt-get -qq install -y --no-install-recommends \
  libgl1 \
  poppler-utils \
  libsm6 \
  libxext6 \
  tesseract-ocr

# headless opencv goes faster

# tesseract config:
# https://github.com/ricktorzynski/ocr-tesseract-docker/blob/master/Dockerfile
# https://github.com/janza/docker-python3-opencv

RUN  pip install numpy opencv-contrib-python-headless

# poetry config
# see https://stackoverflow.com/a/54763270

ENV PYTHONFAULTHANDLER=1 \
  PYTHONUNBUFFERED=1 \
  PYTHONHASHSEED=random \
  PIP_NO_CACHE_DIR=off \
  PIP_DISABLE_PIP_VERSION_CHECK=on \
  PIP_DEFAULT_TIMEOUT=100 \
  POETRY_VERSION=1.1.13

ARG function_dir="/app"
ENV function_dir="$function_dir"

# install poetry
RUN pip install "poetry==$POETRY_VERSION"

# create app directory
RUN mkdir -p "$function_dir"
WORKDIR "$function_dir"

# copy files for project
COPY dist .

# Project initialization:
RUN poetry config virtualenvs.create false \
  && poetry install --no-dev --no-interaction --no-ansi

WORKDIR "$function_dir/src"

# run application
ENTRYPOINT python -m awslambdaric main.lambda_handler
