FILE(REMOVE_RECURSE
  "CMakeFiles/dev_build"
  "nvmemul.ko"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/dev_build.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
