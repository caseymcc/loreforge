**Build Environment:** The entire build environment for `LoreForge` is managed within a Docker container.

  * The Docker image definition is located in the `Dockerfile`.
  * A helper script, `runDocker.sh`, is provided to manage the Docker container lifecycle (build, start, stop, restart, attach).
  * Before running `runDocker.sh`, you should check if the docker `loreforge-dev-container` is already running.
  * All build-related commands (e.g., CMake configuration, compilation) should be executed *inside* the running Docker container.
  * All execution, debugging, and testing of the application should be executed *inside* the running Docker container.
  * The `build.sh` script automates CMake configuration and Ninja compilation steps, and **should also be run inside the Docker container.**
  * `runDocker.sh`/`build.sh` should be called in separete calls as they can not run together.
  * **To start/manage the Docker build environment, execute `runDocker.sh [options]` from the project root.** Common commands include:
      * `runDocker.sh`: Starts the container if it exists, or creates it if not, and then attaches.
      * `runDocker.sh`: Starts the container if it exists, or creates it if not, and then attaches.
      * `runDocker.sh --stop`: Stops and removes the container.
      * `runDocker.sh --restart`: Restarts an existing container and attaches.
      * `runDocker.sh --rebuild`: Builds the Docker image and starts/restarts the container.
  * **Rebuilds of the Docker image (`docker build`) should only occur when changes are made to the `Dockerfile` or its direct dependencies.** Otherwise, simply starting or restarting the container is sufficient.
  * **To build execute `build.sh [options]` from inside the running docker in the project root.** Common commands include:
      * `build.sh`: will build the application
      * `build.sh --rebuild`: cleans the build and then builds the application
      * `build.sh --rebuild-cmake`: clean cmake build directory and re-runs cmake and build
  