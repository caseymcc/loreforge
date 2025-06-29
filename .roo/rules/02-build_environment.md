### **Build Environment:** 
All `LoreForge` build, execution, debugging, and testing occurs within a Docker container located at the project root, `Dockerfile`.

  * ### Managing the Docker Container
    The runDocker.sh script, located in the project root, manages the Docker container lifecycle.

    * `runDocker.sh [command]`: Starts (or creates and starts) the container. If already running, it attaches. If `[command]` are provided, they are executed within the running container via docker exec.
    * `runDocker.sh --stop`: Stops and removes the container.
    * `runDocker.sh --restart`: Restarts an existing container and attaches.
    * `runDocker.sh --rebuild`: Rebuilds the Docker image and then starts/restarts the container. Only use --rebuild if Dockerfile or its dependencies change.
  * ### Building the Application
    Execute build.sh inside the running Docker container from the project root.
    * `build.sh`: Builds the application.
    * `build.sh --rebuild`: Cleans and then builds the application.
    * `build.sh --rebuild-cmake`: Cleans CMake build directory, re-runs CMake, and then builds.
  * ### Running the LoreForge Server
    The runServer.sh script, located in the project root, manages the LoreForge gRPC server. `runServer.sh [options] [command]`
    * `runServer.sh` or `runServer.sh start`: Starts the server.
    * `runServer.sh stop`: Stops the server.
    * `runServer.sh --os <os>`: Sets the OS (default: linux).
    * `runServer.sh --arch <arch>`: Sets the architecture (default: x64).
    * `runServer.sh --build <type>`: Sets the build type (default: debug).
  * The application binary is located at `build/${OS}_${ARCH}_${BUILD_TYPE}` within the container.
  