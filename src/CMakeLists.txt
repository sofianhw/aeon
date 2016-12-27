find_package(PythonLibs)
find_package(PythonInterp)
 
if (PYTHONLIBS_FOUND)
    message("python found at '${PYTHON_INCLUDE_DIR}'")
    message("python interp found at '${PYTHON_EXECUTABLE}'")

    execute_process(COMMAND "${PYTHON_EXECUTABLE}" "-c"
        "import numpy as n; print(n.get_include());"
        RESULT_VARIABLE NUMPY_RESULT
        OUTPUT_VARIABLE NUMPY_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NUMPY_RESULT MATCHES 0)
        set(NUMPY_FOUND true)
        message("numpy found at '${NUMPY_INCLUDE_DIR}'")
    else(NUMPY_RESULT MATCHES 0)
    endif(NUMPY_RESULT MATCHES 0)

    include_directories(${PYTHON_INCLUDE_DIR})
    include_directories(${NUMPY_INCLUDE_DIR})
else()
    message("python not found")
endif (PYTHONLIBS_FOUND)

set (SRC
    api.cpp
    avi.cpp
    base64.cpp
    batch_iterator.cpp
    block.cpp
    block_loader_file.cpp
    block_loader_nds.cpp
    block_manager.cpp
    box.cpp
    bstream.cpp
    buffer_batch.cpp
    cap_mjpeg_decoder.cpp
    cpio.cpp
    crc.cpp
    etl_audio.cpp
    etl_boundingbox.cpp
    etl_char_map.cpp
    etl_depthmap.cpp
    etl_image.cpp
    etl_label_map.cpp
    etl_localization.cpp
    etl_multicrop.cpp
    etl_pixel_mask.cpp
    etl_video.cpp
    file_util.cpp
    image.cpp
    interface.cpp
    loader.cpp
    log.cpp
    manifest_file.cpp
    manifest_nds.cpp
    noise_clips.cpp
    provider_audio_classifier.cpp
    provider_audio_only.cpp
    provider_audio_transcriber.cpp
    provider_factory.cpp
    provider_image_boundingbox.cpp
    provider_image_classifier.cpp
    provider_image_localization.cpp
    provider_image_only.cpp
    provider_image_pixelmask.cpp
    provider_image_stereo.cpp
    provider_video_classifier.cpp
    provider_video_only.cpp
    specgram.cpp
    tiff.cpp
    util.cpp
    wav_data.cpp
    web_app.cpp
    web_server.cpp
)

add_library(aeon ${SRC})
