if(IDF_VERSION_MAJOR AND IDF_VERSION_MAJOR GREATER 3)
    idf_component_register(SRCS "tm1637.c"
        INCLUDE_DIRS "include")
else()
    set(COMPONENT_SRCDIRS .)
    set(COMPONENT_REQUIRES log)
    register_component()
endif()
