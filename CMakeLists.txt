cmake_minimum_required(VERSION 3.5)

project(bcg729_test)


# -DCMAKE_EXPORT_COMPILE_COMMANDS=1
# output compile_json.txt
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)



# mmp libs
set(MPP_PATH /media/luoyang/Workspace/Hi3531DV100_SDK_V2.0.4.0/mpp)



aux_source_directory(${PROJECT_SOURCE_DIR}/src APP_SRCS)

set(POMP_SRCS
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libpomp/src/pomp_buffer.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libpomp/src/pomp_conn.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libpomp/src/pomp_ctx.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libpomp/src/pomp_evt.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libpomp/src/pomp_loop.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libpomp/src/pomp_log.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libpomp/src/pomp_timer.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libpomp/src/pomp_prot.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libpomp/src/pomp_msg.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libpomp/src/pomp_decoder.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libpomp/src/pomp_encoder.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/ulog/libulog/ulog_write_android.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/ulog/libulog/ulog_write.c
    # /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/ulog/libulog/ulog_write_raw.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libh264/src/h264.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libh264/src/h264_bac.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libh264/src/h264_ctx.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libh264/src/h264_reader.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libh264/src/h264_slice_data.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libh264/src/h264_types.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libh264/src/h264_writer.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libh264/src/h264_fmo.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libh264/src/h264_cabac.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libh264/src/h264_bitstream.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libh264/src/h264_cabac_ctx_tables.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libh264/src/h264_macroblock.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libh265/src/h265_bitstream.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libh265/src/h265_ctx.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libh265/src/h265_reader.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/librtp/src/rtcp_pkt.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/librtp/src/rtp_jitter.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/librtp/src/rtp_pkt.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-streaming/src/vstrm_dbg.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-streaming/src/vstrm_frame.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-streaming/src/vstrm_rtp_h264_rx.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-streaming/src/vstrm_rtp_h264_tx.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-streaming/src/vstrm_rtp_h265_tx.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-streaming/src/vstrm_sender.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-streaming/src/vstrm_session_metadata.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-streaming/src/vstrm_video_stats.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-streaming/src/vstrm_receiver.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-streaming/src/vstrm_h264_sei_streaming.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-streaming/src/vstrm_clock_delta.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-metadata/src/vmeta_frame.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-metadata/src/vmeta_utils.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-metadata/src/vmeta_session.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-metadata/src/vmeta_frame_v1.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-metadata/src/vmeta_frame_v2.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-metadata/src/vmeta_frame_v3.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-metadata/src/vmeta_csv.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libvideo-metadata/src/vmeta_json.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libfutils/src/fdutils.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libfutils/src/random.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libfutils/src/timetools.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/libfutils/src/systimetools.c
    /home/luoyang/workspace.lnk/projects_learn/Parrot-Developers/learn-test/video_stream/vstrm_test_socket.c
)

set(SOURCES
    /media/luoyang/Workspace/hisi/libs/ht_hicomm/src/ht_audio.c
    /media/luoyang/Workspace/hisi/libs/ht_hicomm/src/ht_sys.c
    /media/luoyang/Workspace/hisi/libs/ht_hicomm/src/audio_aac_adp.c
    /media/luoyang/Workspace/hisi/libs/ht_hicomm/src/utils_log.c    
    )

# Include header files
include_directories(
    ${PROJECT_SOURCE_DIR}/inc
    ${MPP_PATH}/include
    ${MPP_PATH}/extdrv/tlv320aic31
    /media/luoyang/Workspace/hisi/libs/ht_hicomm/inc
    /home/luoyang/Dev/cross/hi3531d_v600/libbcg729/include
    /home/luoyang/Dev/cross/hi3531d_v600/nanomsg/include
    )

# link libs path
link_directories(
    ${MPP_PATH}/lib
    /home/luoyang/Dev/cross/hi3531d_v600/libbcg729/lib
    /home/luoyang/Dev/cross/hi3531d_v600/nanomsg/lib
    )



############################################################
# Create a library
############################################################

#Generate the static library from the library sources
# add_library(${PROJECT_NAME} STATIC
#     ${SOURCES} ${CSRCS}
#     )

# target_include_directories(${PROJECT_NAME}
#     PUBLIC
#         ${PROJECT_SOURCE_DIR}
#         )


############################################################
# Create an executable
############################################################

# Add an executable with the above sources
add_executable(${PROJECT_NAME}
    ${APP_SRCS}
    ${SOURCES}
    )

# link the new hello_library target with the hello_binary target
target_link_libraries( ${PROJECT_NAME}
    PRIVATE
    dl
    pthread
    rt
    m
    mpi
    hdmi
    VoiceEngine
    jpeg
    upvqe
    dnvqe
    aacenc
    aacdec
    libbcg729.a
    nanomsg
    )
