#!/bin/bash

IMAGE_NAME="loreforge-dev"
CONTAINER_NAME="loreforge-dev-container"

REBUILD=false
STOP=false
RESTART=false
COMMAND_ARGS=()

while [[ "$#" -gt 0 ]]; do
    case $1 in
        --rebuild)
            REBUILD=true
            shift
            ;;
        --stop)
            STOP=true
            shift
            ;;
        --restart)
            RESTART=true
            shift
            ;;
        *)
            COMMAND_ARGS=("$@")
            break
            ;;
    esac
done

if [ "$STOP" = true ] ; then
    if [ "$(docker ps -a -q -f name=$CONTAINER_NAME)" ]; then
        echo "Stopping and removing container..."
        docker stop $CONTAINER_NAME
        docker rm $CONTAINER_NAME
    else
        echo "Container not running."
    fi
    exit 0
fi

if [ "$REBUILD" = true ] ; then
    if [ "$(docker ps -a -q -f name=$CONTAINER_NAME)" ]; then
        echo "Stopping and removing existing container before rebuild..."
        docker stop $CONTAINER_NAME
        docker rm $CONTAINER_NAME
    fi
    echo "Rebuilding Docker image..."
    docker build -t $IMAGE_NAME .
fi

if [ "$RESTART" = true ] ; then
    if [ "$(docker ps -a -q -f name=$CONTAINER_NAME)" ]; then
        echo "Restarting container..."
        docker restart $CONTAINER_NAME
    else
        echo "Container not running. Use --rebuild to create it."
        exit 1
    fi
fi

if [ ! "$(docker ps -q -f name=$CONTAINER_NAME)" ]; then
    if [ ! "$(docker ps -aq -f status=exited -f name=$CONTAINER_NAME)" ]; then
        echo "Starting new container..."
        docker run -d -it --name $CONTAINER_NAME -v "$(pwd)":/app -e VCPKG_OVERLAY_TRIPLETS=/app/triplets $IMAGE_NAME
    else
        echo "Restarting existing container..."
        docker start $CONTAINER_NAME
    fi
fi

if [ ${#COMMAND_ARGS[@]} -gt 0 ]; then
    docker exec -i "$CONTAINER_NAME" /bin/bash -c "${COMMAND_ARGS[*]}"
else
    echo "Attaching to container..."
    docker exec -it "$CONTAINER_NAME" /bin/bash
fi