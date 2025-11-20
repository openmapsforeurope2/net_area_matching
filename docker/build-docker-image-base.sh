#!/bin/sh
DOCKER_NAME=net_area_matching_base
DOCKER_TAG="latest"

docker build --no-cache -t $DOCKER_NAME:$DOCKER_TAG -f Dockerfile.base ./..