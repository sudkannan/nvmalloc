# CMake generated Testfile for 
# Source directory: /home/sudarsun/NVM_EMUL/test
# Build directory: /home/sudarsun/NVM_EMUL/test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
ADD_TEST(interpose "/home/sudarsun/NVM_EMUL/test/test_interpose")
SET_TESTS_PROPERTIES(interpose PROPERTIES  ENVIRONMENT "LD_PRELOAD=/home/sudarsun/NVM_EMUL/src/emul/libnvmemul.so;ENUM_INI=emul.ini")
