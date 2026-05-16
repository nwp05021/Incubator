# CMakeLists.txt 전체 내용 교체
cmake_minimum_required(VERSION 3.16)

# 2. ESP-IDF 코어 파이프라인 로드 및 프로젝트 매핑
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Incubator C CXX ASM)
