# $Id:$

set(RelativeDir "./json/")
set(RelativeSourceGroup "src\\json")
set(SubDirs)

set(DirFiles
    json.h
    json-forwards.h

    _SourceFiles.cmake
)
set(DirFiles_SourceGroup "${RelativeSourceGroup}")

set(LocalSourceGroupFiles)
foreach(File ${DirFiles})
    list(APPEND LocalSourceGroupFiles "${RelativeDir}/${File}")
    list(APPEND ProjectSources "${RelativeDir}/${File}")
endforeach()
source_group(${DirFiles_SourceGroup} FILES ${LocalSourceGroupFiles})

set(SubDirFiles "")
foreach(Dir ${SubDirs})
    list(APPEND SubDirFiles "${RelativeDir}/${Dir}/_SourceFiles.cmake")
endforeach()

foreach(SubDirFile ${SubDirFiles})
    include(${SubDirFile})
endforeach()

