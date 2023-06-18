if(IDF_VERSION_MAJOR GREATER_EQUAL 5)
    idf_component_register(SRCS "tm1637.c"
    INCLUDE_DIRS "include"
    REQUIRES driver)
elseif(IDF_VERSION_MAJOR EQUAL 3 OR IDF_VERSION_MAJOR EQUAL 4)
    idf_component_register(SRCS "tm1637.c"
        INCLUDE_DIRS "include")
else()
    set(COMPONENT_SRCDIRS .)
    set(COMPONENT_REQUIRES log)
    register_component()
endif()
