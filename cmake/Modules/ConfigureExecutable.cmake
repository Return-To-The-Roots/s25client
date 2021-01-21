# Configures the given input file and copies it to the output_dir/output_name with executable permissions
# output_dir has to be a path relative to CMAKE_CURRENT_BINARY_DIR
function(configure_executable input_file output_dir output_name)
    set(tmpDir "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}")
    file(MAKE_DIRECTORY "${tmpDir}")
    set(tmpFile "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${output_name}")
    configure_file("${input_file}" "${tmpFile}" @ONLY)
    file(COPY "${tmpFile}"
        DESTINATION "${output_dir}"
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
        GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
endfunction()
