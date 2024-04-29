project(baresip_gRPC)

# list(APPEND MODULES_DETECTED ${PROJECT_NAME})
# set(MODULES_DETECTED ${MODULES_DETECTED} PARENT_SCOPE)

# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
# option(protobuf_MODULE_COMPATIBLE TRUE)

find_package(Protobuf)
find_package(gRPC)

if (NOT Protobuf_FOUND OR NOT gRPC_FOUND) 
    return()
endif()

list(APPEND MODULES_DETECTED ${PROJECT_NAME})
set(MODULES_DETECTED ${MODULES_DETECTED} PARENT_SCOPE)

find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
if(CMAKE_CROSSCOMPILING)
    find_program(_PROTOBUF_PROTOC protoc)
else()
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)
if(CMAKE_CROSSCOMPILING)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

# Proto file
get_filename_component(baresip_controller_proto "./protos/baresip_controller.proto" ABSOLUTE)
get_filename_component(baresipController_proto_path "${baresip_controller_proto}" PATH)

# Generated sources
set(baresipController_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/baresip_controller.pb.cc")
set(baresipController_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/baresip_controller.pb.h")
set(baresip_controller_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/baresip_controller.grpc.pb.cc")
set(baresip_controller_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/baresip_controller.grpc.pb.h")
add_custom_command(
      OUTPUT "${baresipController_proto_srcs}" "${baresipController_proto_hdrs}" "${baresip_controller_grpc_srcs}" "${baresip_controller_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${baresipController_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${baresip_controller_proto}"
      DEPENDS "${baresip_controller_proto}")

# protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS baresip_controller.proto)


set(SRCS ${baresip_controller_grpc_srcs} BaresipController.cpp)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

if(STATIC)
    add_library(${PROJECT_NAME} OBJECT ${SRCS})
else()
    add_library(${PROJECT_NAME} MODULE ${SRCS})
endif()

# target_include_directories(${PROJECT_NAME} PRIVATE ${WEBRTC_AEC_INCLUDE_DIRS})
# target_link_libraries(${PROJECT_NAME} PRIVATE ${WEBRTC_AEC_LIBRARIES})
# target_compile_options(${PROJECT_NAME} PRIVATE
#   -isystem $(WEBRTC_AEC_INCLUDE_DIRS)
#   -fPIC
#   -Wno-shadow
#   -Wno-pedantic
#   -Wno-missing-field-initializers
#   -Wno-unused-parameter
# )
# target_link_options(${PROJECT_NAME} PRIVATE -lstdc++)