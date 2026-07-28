cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "drivers" "components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(weather_station)