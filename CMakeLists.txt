CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(TorchlightModder C CXX)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -pedantic -O2")

SET(LIBADM_SOURCES
	src/adm.cpp
	src/adm_file_loader.cpp
	src/adm_file_writer.cpp
	src/config.cpp
	src/dat_file_adm_loader.cpp
	src/filename_utils.cpp
	src/mapped_file.cpp
	src/massfile.cpp
	src/modcompiler.cpp
	src/moddirectory.cpp
	src/unicode.cpp
	src/unicode_line_reader.cpp
)

SET(LIBADM_HEADERS
	src/adm.h
	src/adm_file_loader.h
	src/adm_file_writer.h
	src/charactercreate.h
	src/config.h
	src/dat_file_adm_loader.h
	src/dir_iterator.h
	src/filename_utils.h
	src/mapped_file.h
	src/massfile.h
	src/masterresourceunits.h
	src/modcompiler.h
	src/moddirectory.h
	src/unicode.h
	src/unicode_line_reader.h
)

SET(DAT2ADM_SOURCES
	src/dat2adm.cpp
)

SET(ADM2DAT_SOURCES
	src/adm2dat.cpp
)

SET(MODDER_SOURCES
	src/main.cpp
)

SET(CLASSCREATE_SOURCES
	src/classcreate.cpp
)

ADD_LIBRARY(adm STATIC ${LIBADM_SOURCES} ${LIBADM_HEADERS})
TARGET_LINK_LIBRARIES(adm)

ADD_EXECUTABLE(dat2adm ${DAT2ADM_SOURCES})
TARGET_LINK_LIBRARIES(dat2adm adm)

ADD_EXECUTABLE(adm2dat ${ADM2DAT_SOURCES})
TARGET_LINK_LIBRARIES(adm2dat adm)

ADD_EXECUTABLE(tlmodder ${MODDER_SOURCES})
TARGET_LINK_LIBRARIES(tlmodder adm)

#ADD_EXECUTABLE(classcreate ${CLASSCREATE_SOURCES})
#TARGET_LINK_LIBRARIES(classcreate adm)
