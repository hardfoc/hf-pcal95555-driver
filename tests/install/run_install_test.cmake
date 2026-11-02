# Install the driver from the build tree into a scratch prefix, then configure, build and run a
# consumer that only sees that prefix. Arguments: -DBUILD_DIR= -DSOURCE_DIR= -DWORK_DIR=
file(REMOVE_RECURSE "${WORK_DIR}")
execute_process(COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${WORK_DIR}/prefix"
                RESULT_VARIABLE rc)
if(rc)
  message(FATAL_ERROR "install failed (${rc})")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}/tests/install" -B "${WORK_DIR}/build"
                        "-DCMAKE_PREFIX_PATH=${WORK_DIR}/prefix" "-DCMAKE_CXX_COMPILER=${CXX}"
                RESULT_VARIABLE rc)
if(rc)
  message(FATAL_ERROR "consumer configure failed (${rc})")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${WORK_DIR}/build" RESULT_VARIABLE rc)
if(rc)
  message(FATAL_ERROR "consumer build failed (${rc})")
endif()
execute_process(COMMAND "${WORK_DIR}/build/consumer" RESULT_VARIABLE rc)
if(rc)
  message(FATAL_ERROR "consumer run failed (${rc})")
endif()
