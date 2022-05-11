cmake_minimum_required(VERSION 3.12)

# Set number of recognized words
# 2 -> "yes","no"
# 8 -> "yes","no","up","down","left","right","on","off"
# 10 -> "yes","no","up","down","left","right","on","off","stop","go"
set(WORDCOUNT 8)

# Project
set(PROJECT_NAME tflm_hotword_recognition)
set(PROJECT_BINARY ${PROJECT_NAME})
project(${PROJECT_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)


#### Tensorflow Lite Mirco library
set(TFLM_LIBRARY tflm_lib)
set(TFLM_LIB_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/tflm)

file(GLOB_RECURSE TFLM_SOURCE_FILES ${TFLM_LIB_DIR}/tensorflow/lite/*.c ${TFLM_LIB_DIR}/tensorflow/lite/*.cpp ${TFLM_LIB_DIR}/third_party/*.c ${TFLM_LIB_DIR}/third_party/*.cpp)
file(GLOB_RECURSE TFLM_HEADER_FILES ${TFLM_LIB_DIR}/tensorflow/lite/*.h ${TFLM_LIB_DIR}/third_party/*.h)

add_library(${TFLM_LIBRARY} INTERFACE)

target_include_directories(${TFLM_LIBRARY}
  INTERFACE
  ${TFLM_LIB_DIR}
  ${TFLM_LIB_DIR}/third_party
  ${TFLM_LIB_DIR}/third_party/flatbuffers/include
  ${TFLM_LIB_DIR}/third_party/gemmlowp
  ${TFLM_LIB_DIR}/third_party/kissfft/
  ${TFLM_LIB_DIR}/third_party/kissfft/tools
  ${TFLM_LIB_DIR}/third_party/ruy
  ${TFLM_LIB_DIR}/third_party/cmsis
  ${TFLM_LIB_DIR}/third_party/cmsis/CMSIS/NN/Include
  ${TFLM_LIB_DIR}/third_party/cmsis/CMSIS/DSP/Include
  ${TFLM_LIB_DIR}/third_party/cmsis/CMSIS/Core/Include
)

target_compile_definitions(
  ${TFLM_LIBRARY}
  INTERFACE
  COMPILE_DEFINITIONS TF_LITE_DISABLE_X86_NEON=1
  # COMPILE_DEFINITIONS TF_LITE_STATIC_MEMORY=1 # microcontroller
  COMPILE_DEFINITIONS CMSIS_NN=1
  # COMPILE_DEFINITIONS ARDUINOSTL_M_H=1 # for mbed core
)

target_sources(${TFLM_LIBRARY}
  INTERFACE
  ${TFLM_SOURCE_FILES}
  ${TFLM_HEADER_FILES}
)


#### Project sources
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
file(GLOB_RECURSE PROJECT_SOURCE_FILES ${SRC_DIR}/*.cpp)
file(GLOB_RECURSE PROJECT_HEADER_FILES ${SRC_DIR}/*.h)

# Add used model data file
list(FILTER PROJECT_SOURCE_FILES EXCLUDE REGEX "${SRC_DIR}/micro_speech_model_data.*\.cpp")
if(WORDCOUNT EQUAL 2)
  list(APPEND PROJECT_SOURCE_FILES ${SRC_DIR}/micro_speech_model_data_yesno.cpp)
elseif(WORDCOUNT EQUAL 8)
  list(APPEND PROJECT_SOURCE_FILES ${SRC_DIR}/micro_speech_model_data_8hotwords.cpp)
elseif(WORDCOUNT EQUAL 10)
  list(APPEND PROJECT_SOURCE_FILES ${SRC_DIR}/micro_speech_model_data_10hotwords.cpp)
else()
  message(FATAL_ERROR "WORDCOUNT must be 2, 8 or 10" )
endif()
add_compile_definitions(WORDCOUNT=${WORDCOUNT})

add_executable(${PROJECT_BINARY}
    ${PROJECT_SOURCE_FILES}
    ${PROJECT_HEADER_FILES}
)

target_include_directories(${PROJECT_BINARY} PRIVATE ${SRC_DIR})

target_link_libraries(${PROJECT_BINARY} PRIVATE ${TFLM_LIBRARY})
