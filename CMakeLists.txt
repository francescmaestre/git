file(GLOB SRC_FILES
  include/*.h
  src/*.cpp
)

add_library(git STATIC
    ${SRC_FILES}
)

target_link_libraries(git
    PUBLIC
    Qt::Core
    Qt::Widgets
    QLogger
)

target_include_directories(git PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
