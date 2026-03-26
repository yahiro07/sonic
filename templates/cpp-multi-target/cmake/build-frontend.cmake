execute_process(
  COMMAND sh -c "npm install && npm run build"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/frontend"
  RESULT_VARIABLE NODE_BUILD_RESULT)
if(NOT NODE_BUILD_RESULT EQUAL 0)
  message(
    FATAL_ERROR
      "frontend project build failed with exit code ${NODE_BUILD_RESULT}")
endif()
