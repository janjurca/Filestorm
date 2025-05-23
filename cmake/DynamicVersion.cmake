# Helper to get dynamic version Format is made compatible with python's setuptools_scm
# (https://github.com/pypa/setuptools_scm#git-archives)

function(dynamic_version)
  # Configure project to use dynamic versioning
  #
  # Named arguments:: PROJECT_PREFIX (string): Prefix to be used for namespacing targets, typically
  # ${PROJECT_NAME} OUTPUT_VERSION (string) [PROJECT_VERSION]: Variable where to save the calculated
  # version OUTPUT_DESCRIBE (string) [GIT_DESCRIBE]: Variable where to save the pure git_describe
  # OUTPUT_COMMIT (string) [GIT_COMMIT]: Variable where to save the git commit PROJECT_SOURCE (path)
  # [${CMAKE_CURRENT_SOURCE_DIR}]: Location of the project source. (either extracted git archive or
  # git clone) GIT_ARCHIVAL_FILE (path) [${PROJECT_SOURCE}/.git_archival.txt]: Location of
  # .git_archival.txt FALLBACK_VERSION (string): Fallback version FALLBACK_HASH (string): Fallback
  # git hash. If not defined target GitHash will not be created if project is not a git repo
  # TMP_FOLDER (path) [${CMAKE_CURRENT_BINARY_DIR}/tmp]: Temporary path to store temporary files
  # OUTPUT_FOLDER (path) [${CMAKE_CURRENT_BINARY_DIR}]: Path where to store generated files
  #
  # Options:: ALLOW_FAILS: Do not return with FATAL_ERROR. Developer is responsible for setting
  # appropriate version if fails
  #
  # Targets:: ${PROJECT_PREFIX}Version: Target that recalculates the dynamic version each time
  # ${PROJECT_PREFIX}GitHash:
  #
  # Generated files:: (Note: files are regenerated only when they change)
  # ${OUTPUT_FOLDER}/.DynamicVersion.json: All computed data of DynamicVersion
  # ${OUTPUT_FOLDER}/.version: Extracted version ${OUTPUT_FOLDER}/.git_describe: Computed git
  # describe ${OUTPUT_FOLDER}/.git_commit: Current commit

  set(ARGS_Options "")
  set(ARGS_OneValue "")
  set(ARGS_MultiValue "")
  list(APPEND ARGS_Options ALLOW_FAILS)
  list(
    APPEND
    ARGS_OneValue
    PROJECT_PREFIX
    OUTPUT_VERSION
    OUTPUT_DESCRIBE
    OUTPUT_COMMIT
    PROJECT_SOURCE
    GIT_ARCHIVAL_FILE
    FALLBACK_VERSION
    FALLBACK_HASH
    TMP_FOLDER
    OUTPUT_FOLDER
  )

  cmake_parse_arguments(ARGS "${ARGS_Options}" "${ARGS_OneValue}" "${ARGS_MultiValue}" ${ARGN})

  set(DynamicVersion_ARGS "")

  # Set default values
  if(NOT DEFINED ARGS_OUTPUT_VERSION)
    set(ARGS_OUTPUT_VERSION PROJECT_VERSION)
  endif()
  if(NOT DEFINED ARGS_OUTPUT_DESCRIBE)
    set(ARGS_OUTPUT_DESCRIBE GIT_DESCRIBE)
  endif()
  if(NOT DEFINED ARGS_OUTPUT_COMMIT)
    set(ARGS_OUTPUT_COMMIT GIT_COMMIT)
  endif()
  if(NOT DEFINED ARGS_PROJECT_SOURCE)
    set(ARGS_PROJECT_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
  if(NOT DEFINED ARGS_GIT_ARCHIVAL_FILE)
    set(ARGS_GIT_ARCHIVAL_FILE ${ARGS_PROJECT_SOURCE}/.git_archival.txt)
  endif()
  if(DEFINED ARGS_FALLBACK_VERSION OR ARGS_ALLOW_FAILS)
    # If we have a fallback version or it is specified it is ok if this fails, don't make messages
    # FATAL_ERROR
    set(error_message_type AUTHOR_WARNING)
  else()
    # Otherwise it should
    set(error_message_type FATAL_ERROR)
  endif()
  if(NOT ARGS_PROJECT_PREFIX)
    message(
      AUTHOR_WARNING
        "DynamicVersion: No PROJECT_PREFIX was given. Please provide one to avoid target name clashes"
    )
  elseif(NOT ARGS_PROJECT_PREFIX MATCHES ".*_$")
    # Append an underscore _ to the prefix if not provided
    message(
      AUTHOR_WARNING
        "DynamicVersion: PROJECT_PREFIX did not contain an underscore, please add it for clarity"
    )
    set(ARGS_PROJECT_PREFIX ${ARGS_PROJECT_PREFIX}_)
  endif()
  if(NOT DEFINED ARGS_TMP_FOLDER)
    set(ARGS_TMP_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/tmp)
  endif()
  if(NOT DEFINED ARGS_OUTPUT_FOLDER)
    set(ARGS_OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR})
  endif()
  if(ARGS_OUTPUT_FOLDER EQUAL ARGS_TMP_FOLDER)
    message(
      FATAL_ERROR
        "DynamicVersion misconfigured: Cannot have both OUTPUT_FOLDER and TMP_FOLDER point to the same path"
    )
  endif()

  list(
    APPEND
    DynamicVersion_ARGS
    PROJECT_SOURCE
    ${ARGS_PROJECT_SOURCE}
    GIT_ARCHIVAL_FILE
    ${ARGS_GIT_ARCHIVAL_FILE}
    TMP_FOLDER
    ${ARGS_TMP_FOLDER}
  )
  if(DEFINED ARGS_FALLBACK_VERSION)
    list(APPEND DynamicVersion_ARGS FALLBACK_VERSION ${ARGS_FALLBACK_VERSION})
  endif()
  if(DEFINED ARGS_FALLBACK_HASH)
    list(APPEND DynamicVersion_ARGS FALLBACK_HASH ${ARGS_FALLBACK_HASH})
  endif()
  if(ARGS_ALLOW_FAILS)
    list(APPEND DynamicVersion_ARGS ALLOW_FAILS)
  endif()
  # Normalize DynamicVersion_ARGS to be passed as string
  list(JOIN DynamicVersion_ARGS "\\;" DynamicVersion_ARGS)

  # Check if .version file exists and use it if so
  set(SKIP_AUTOGENERATION False)
  if(EXISTS CMAKE_SOURCE_DIR/.version)
    file(READ CMAKE_SOURCE_DIR/.version custom_version)
    message(WARNING "DynamicVersion: Using version from .version file: ${custom_version}")
    # set fallback version in ARGS_FALLBACK_VERSION variable
    set(ARGS_FALLBACK_VERSION ${custom_version})
    set(ARGS_FALLBACK_HASH "custom")
    set(SKIP_AUTOGENERATION True)
  endif()

  if(SKIP_AUTOGENERATION)
    message(WARNING "DynamicVersion: Skipping autogeneration of version")
    # Write the version file
    file(WRITE ${ARGS_TMP_FOLDER}/.version ${ARGS_FALLBACK_VERSION})
    file(WRITE ${ARGS_TMP_FOLDER}/.git_commit ${ARGS_FALLBACK_HASH})
    # Create the JSON content using the variables
    set(JSON_CONTENT "{\n")
    set(JSON_CONTENT "${JSON_CONTENT}  \"allow-fails\" : false,\n")
    set(JSON_CONTENT "${JSON_CONTENT}  \"commit\" : \"${ARGS_FALLBACK_HASH}\",\n")
    set(JSON_CONTENT
        "${JSON_CONTENT}  \"describe\" : \"v${ARGS_FALLBACK_VERSION}-${ARGS_FALLBACK_HASH}\",\n"
    )
    set(JSON_CONTENT "${JSON_CONTENT}  \"failed\" : false,\n")
    set(JSON_CONTENT "${JSON_CONTENT}  \"version\" : \"${ARGS_FALLBACK_VERSION}\"\n}")
    set(JSON_CONTENT "${JSON_CONTENT}")
    # Write the JSON content to the file
    file(WRITE ${ARGS_TMP_FOLDER}/.DynamicVersion.json ${JSON_CONTENT})
  else()
    # Execute get_dynamic_version once to know the current configuration
    execute_process(
      COMMAND
        ${CMAKE_COMMAND} -DDynamicVersion_RUN:BOOL=True
        # Note: DynamicVersion_ARGS cannot be escaped with ""
        -DDynamicVersion_ARGS:STRING=${DynamicVersion_ARGS} -P ${CMAKE_CURRENT_FUNCTION_LIST_FILE}
        COMMAND_ERROR_IS_FATAL ANY
    )
  endif()
  # Copy all configured files
  foreach(file IN ITEMS .DynamicVersion.json .version .git_describe .git_commit)
    if(EXISTS ${file})
      file(COPY_FILE ${ARGS_TMP_FOLDER}/${file} ${ARGS_OUTPUT_FOLDER}/${file})
    endif()
  endforeach()

  # Check configuration state
  file(READ ${ARGS_TMP_FOLDER}/.DynamicVersion.json data)
  string(JSON failed GET ${data} failed)
  string(
    JSON
    ${ARGS_OUTPUT_VERSION}
    ERROR_VARIABLE
    _
    GET
    ${data}
    version
  )
  string(
    JSON
    ${ARGS_OUTPUT_DESCRIBE}
    ERROR_VARIABLE
    _
    GET
    ${data}
    describe
  )
  string(
    JSON
    ${ARGS_OUTPUT_COMMIT}
    ERROR_VARIABLE
    _
    GET
    ${data}
    commit
  )
  message(DEBUG "DynamicVersion: Computed data:\n" "  data = ${data}")
  message(DEBUG "DynamicVersion: Computed version:\n" "  version = ${${ARGS_OUTPUT_VERSION}}")
  message(DEBUG "DynamicVersion: Computed describe:\n" "  describe = ${${ARGS_OUTPUT_DESCRIBE}}")
  message(DEBUG "DynamicVersion: Computed commit:\n" "  commit = ${${ARGS_OUTPUT_COMMIT}}")
  message(DEBUG "DynamicVersion: Computed failed:\n" "  failed = ${failed}")
  # Configure targets
  if(failed)
    # If configuration failed, create dummy targets
    add_custom_target(${ARGS_PROJECT_PREFIX}Version COMMAND ${CMAKE_COMMAND} -E true)
    add_custom_target(${ARGS_PROJECT_PREFIX}GitHash COMMAND ${CMAKE_COMMAND} -E true)
  else()
    # Otherwise create the targets outputting to the appropriate files
    add_custom_target(
      ${ARGS_PROJECT_PREFIX}DynamicVersion ALL
      BYPRODUCTS ${ARGS_TMP_FOLDER}/.DynamicVersion.json ${ARGS_TMP_FOLDER}/.git_describe
                 ${ARGS_TMP_FOLDER}/.version ${ARGS_TMP_FOLDER}/.git_commit
                 ${ARGS_TMP_FOLDER}/.n_commit
      COMMAND
        ${CMAKE_COMMAND} -DDynamicVersion_RUN:BOOL=True
        # Note: For some reason DynamicVersion_ARGS needs "" here, but it doesn't in execute_process
        -DDynamicVersion_ARGS:STRING="${DynamicVersion_ARGS}" -P ${CMAKE_CURRENT_FUNCTION_LIST_FILE}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ARGS_TMP_FOLDER}/.DynamicVersion.json
              ${ARGS_OUTPUT_FOLDER}/.DynamicVersion.json
    )
    add_custom_target(
      ${ARGS_PROJECT_PREFIX}Version ALL
      DEPENDS ${ARGS_PROJECT_PREFIX}DynamicVersion
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ARGS_TMP_FOLDER}/.git_describe
              ${ARGS_OUTPUT_FOLDER}/.git_describe
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ARGS_TMP_FOLDER}/.version
              ${ARGS_OUTPUT_FOLDER}/.version
    )
    add_custom_target(
      ${ARGS_PROJECT_PREFIX}GitHash
      DEPENDS ${ARGS_PROJECT_PREFIX}DynamicVersion
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ARGS_TMP_FOLDER}/.git_commit
              ${ARGS_OUTPUT_FOLDER}/.git_commit
    )
  endif()

  # This ensures that the project is reconfigured (at least at second run) whenever the version
  # changes
  set_property(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS ${ARGS_OUTPUT_FOLDER}/.version
  )

  message(VERBOSE "DynamicVersion: Calculated version = ${${ARGS_OUTPUT_VERSION}}")

  if(CMAKE_VERSION VERSION_LESS 3.25)
    # TODO: Remove when cmake 3.25 is commonly distributed
    set(${ARGS_OUTPUT_DESCRIBE}
        ${${ARGS_OUTPUT_DESCRIBE}}
        PARENT_SCOPE
    )
    set(${ARGS_OUTPUT_VERSION}
        ${${ARGS_OUTPUT_VERSION}}
        PARENT_SCOPE
    )
    set(${ARGS_OUTPUT_COMMIT}
        ${${ARGS_OUTPUT_COMMIT}}
        PARENT_SCOPE
    )
  endif()
  return(PROPAGATE ${ARGS_OUTPUT_DESCRIBE} ${ARGS_OUTPUT_VERSION} ${ARGS_OUTPUT_COMMIT})
endfunction()

function(get_dynamic_version)
  # Compute the dynamic version
  #
  # Named arguments:: PROJECT_SOURCE (path): Location of the project source. (either extracted git
  # archive or git clone) GIT_ARCHIVAL_FILE (path): Location of .git_archival.txt FALLBACK_VERSION
  # (string): Fallback version FALLBACK_HASH (string): Fallback git hash. If not defined target
  # GitHash will not be created if project is not a git repo TMP_FOLDER (path): Temporary path to
  # store temporary files
  message(STATUS "DynamicVersion: Entered get_dynamic_version()")
  set(ARGS_Options "")
  set(ARGS_OneValue "")
  set(ARGS_MultiValue "")
  list(
    APPEND
    ARGS_OneValue
    PROJECT_SOURCE
    GIT_ARCHIVAL_FILE
    FALLBACK_VERSION
    FALLBACK_HASH
    TMP_FOLDER
  )
  list(APPEND ARGS_Options ALLOW_FAILS)

  cmake_parse_arguments(ARGS "${ARGS_Options}" "${ARGS_OneValue}" "${ARGS_MultiValue}" ${ARGN})

  if(DEFINED ARGS_FALLBACK_VERSION OR ARGS_ALLOW_FAILS)
    # If we have a fallback version or it is specified it is ok if this fails, don't make messages
    # FATAL_ERROR
    set(error_message_type AUTHOR_WARNING)
  else()
    # Otherwise it should fail
    set(error_message_type FATAL_ERROR)
  endif()

  set(data "{}")
  # Default set
  string(JSON data SET ${data} failed true)
  if(ARGS_ALLOW_FAILS)
    string(JSON data SET ${data} allow-fails true)
  else()
    string(JSON data SET ${data} allow-fails false)
  endif()

  # Set fallback values
  if(DEFINED ARGS_FALLBACK_VERSION)
    message(STATUS "DynamicVersion: Using fallback version: ${ARGS_FALLBACK_VERSION}")
    string(JSON data SET ${data} version ${ARGS_FALLBACK_VERSION})
    file(WRITE ${ARGS_TMP_FOLDER}/.DynamicVersion.json ${data})
    file(WRITE ${ARGS_TMP_FOLDER}/.version ${ARGS_FALLBACK_VERSION})
  endif()
  if(DEFINED ARGS_FALLBACK_HASH)
    message(STATUS "DynamicVersion: Using fallback hash: ${ARGS_FALLBACK_HASH}")
    # TODO We should probably dont ignore this string(JSON data SET ${data} commit
    # ${ARGS_FALLBACK_HASH})
    file(WRITE ${ARGS_TMP_FOLDER}/.DynamicVersion.json ${data})
    file(WRITE ${ARGS_TMP_FOLDER}/.git_commit ${ARGS_FALLBACK_HASH})
  endif()

  if(DEFINED SKIP_AUTOGENERATION)
    # If SKIP_AUTOGENERATION is defined, we are done
    message(WARNING "DynamicVersion: Skipping autogeneration of version")
    return()
  endif()

  if(NOT EXISTS ${ARGS_GIT_ARCHIVAL_FILE})
    # If git_archival.txt is missing, project is ill-formed
    message(${error_message_type} "DynamicVersion: Missing file .git_archival.txt\n"
            "  .git_archival.txt: ${ARGS_GIT_ARCHIVAL_FILE}"
    )
    return()
  endif()

  # Get version dynamically from git_archival.txt
  message(STATUS "DynamicVersion: Trying to get version from .git_archival.txt")
  file(STRINGS ${ARGS_GIT_ARCHIVAL_FILE} describe-name REGEX "^describe-name:.*")
  if(NOT describe-name)
    # If git_archival.txt does not contain the field "describe-name:", it is ill-formed
    message(${error_message_type}
            "DynamicVersion: Missing string \"describe-name\" in .git_archival.txt\n"
            "  .git_archival.txt: ${ARGS_GIT_ARCHIVAL_FILE}"
    )
    return()
  endif()

  message(STATUS "DynamicVersion: Found describe-name-format: ${describe-name}")
  # Try to get the version tag of the form `vX.Y.Z` or `X.Y.Z` (with arbitrary suffix)
  if(describe-name MATCHES "^describe-name:[ ]?([v]?([0-9\\.]+)-([0-9]*).*)")
    message(STATUS "DynamicVersion: Found appropriate tag in .git_archival.txt file")
    # First matched group is the full `git describe` of the latest tag Second matched group is only
    # the version, i.e. `X.Y.Z`

    string(JSON data SET ${data} describe \"${CMAKE_MATCH_1}\")
    file(WRITE ${ARGS_TMP_FOLDER}/.git_describe ${CMAKE_MATCH_1})
    string(JSON data SET ${data} version \"${CMAKE_MATCH_2}.${CMAKE_MATCH_3}\")
    file(WRITE ${ARGS_TMP_FOLDER}/.version ${CMAKE_MATCH_2}.${CMAKE_MATCH_3})
    # Third matched group is the number of commits since the latest tag
    string(JSON data SET ${data} n_commit \"${CMAKE_MATCH_3}\")
    file(WRITE ${ARGS_TMP_FOLDER}/.n_commit ${CMAKE_MATCH_3})
    # Get commit hash
    file(STRINGS ${ARGS_GIT_ARCHIVAL_FILE} node REGEX "^node:[ ]?(.*)")
    string(JSON data SET ${data} commit \"${CMAKE_MATCH_1}\")
    file(WRITE ${ARGS_TMP_FOLDER}/.git_commit ${CMAKE_MATCH_1})
    message(DEBUG "DynamicVersion: Found appropriate tag in .git_archival.txt file")
  else()
    message(
      STATUS
        "DynamicVersion: No appropriate tag found in .git_archival.txt file, trying to get it from git"
    )
    # If not it has to be computed from the git archive
    find_package(Git REQUIRED)
    # Test if project is a git repository
    execute_process(
      COMMAND ${GIT_EXECUTABLE} status
      WORKING_DIRECTORY ${ARGS_PROJECT_SOURCE}
      RESULT_VARIABLE git_status_result
      OUTPUT_QUIET
    )
    if(NOT git_status_result EQUAL 0)
      message(${error_message_type}
              "DynamicVersion: Project source is neither a git repository nor a git archive:\n"
              "  Source: ${ARGS_PROJECT_SOURCE}"
      )
      return()
    endif()
    # Get most recent commit hash
    message(STATUS "DynamicVersion: Getting most recent commit hash")
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
      WORKING_DIRECTORY ${ARGS_PROJECT_SOURCE}
      OUTPUT_VARIABLE git-hash
      OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY
    )

    message(STATUS "DynamicVersion: Getting most recent tag")
    execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --tags --match=v?[0-9.]* --dirty
      WORKING_DIRECTORY ${ARGS_PROJECT_SOURCE}
      OUTPUT_VARIABLE describe-name
      OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY
    )
    message(STATUS "DynamicVersion: describe-name: ${describe-name}")
    # Match any part containing digits and periods (strips out rc and so on)
    if(NOT describe-name MATCHES "^([v]?([0-9\\.]+)(-([0-9]*))?.*)")
      message(${error_message_type} "DynamicVersion: Version tag is ill-formatted\n"
              "  Describe-name: ${describe-name}"
      )
      return()
    endif()
    message(
      STATUS "DynamicVersion: Found appropriate tag from git. Saving it to ${ARGS_TMP_FOLDER}"
    )
    message(
      STATUS
        "DynamicVersion: CMAKE_MATCH_1: ${CMAKE_MATCH_1} CMAKE_MATCH_2: ${CMAKE_MATCH_2} CMAKE_MATCH_3: ${CMAKE_MATCH_3}  CMAKE_MATCH_4: ${CMAKE_MATCH_4}"
    )
    string(JSON data SET ${data} describe \"${CMAKE_MATCH_1}\")
    file(WRITE ${ARGS_TMP_FOLDER}/.git_describe ${CMAKE_MATCH_1})

    if(NOT CMAKE_MATCH_4)
      message(STATUS "DynamicVersion: No commits since last tag")
      string(JSON data SET ${data} version \"${CMAKE_MATCH_2}\")
      file(WRITE ${ARGS_TMP_FOLDER}/.version ${CMAKE_MATCH_2})
    else()
      message(STATUS "DynamicVersion: ${CMAKE_MATCH_3} commits since last tag")
      string(JSON data SET ${data} version \"${CMAKE_MATCH_2}.${CMAKE_MATCH_4}\")
      file(WRITE ${ARGS_TMP_FOLDER}/.version ${CMAKE_MATCH_2}.${CMAKE_MATCH_4})
      string(JSON data SET ${data} n_commit \"${CMAKE_MATCH_3}\")
    endif()
    file(WRITE ${ARGS_TMP_FOLDER}/.n_commit ${CMAKE_MATCH_3})
    string(JSON data SET ${data} commit \"${git-hash}\")
    file(WRITE ${ARGS_TMP_FOLDER}/.git_commit ${git-hash})
    message(DEBUG "DynamicVersion: Found appropriate tag from git")
  endif()

  # Mark success and output results
  string(JSON data SET ${data} failed false)
  message(DEBUG "DynamicVersion: Computed data:\n" "  data = ${data}")
  file(WRITE ${ARGS_TMP_FOLDER}/.DynamicVersion.json ${data})
endfunction()

# Logic to run get_dynamic_version() by running this script
if(DynamicVersion_RUN)
  if(NOT DEFINED DynamicVersion_ARGS)
    message(FATAL_ERROR "DynamicVersion: DynamicVersion_ARGS not defined")
  endif()
  get_dynamic_version(${DynamicVersion_ARGS})
endif()
