LOGGING=/usr/lib64/logging
#NVMALLOC_HOME=/home/stewart/codes/nvmalloc
INCLUDE=$(NVMALLOC_HOME)
src_path=$(NVMALLOC_HOME)
LIB_PATH := $(NVMALLOC_HOME)
BENCH:= $(NVMALLOC_HOME)/compare_bench
CMU_MALLOC:=$(NVMALLOC_HOME)/compare_bench/cmu_nvram/nvmalloc
LDFLAGS=-ldl
DEBUGFLG=-O2 #-g

# See source code comments to avoid memory leaks when enabling MALLOC_MAG.
#CPPFLAGS := -DMALLOC_PRODUCTION -DMALLOC_MAG
CPPFLAGS := -fPIC -I$(INCLUDE) -I$(INCLUDE)/jemalloc -I/usr/include $(DEBUGFLG) #-I$(INCLUDE)/pmem_intel/linux-examples_flex/libpmem -g #-O3
CPPFLAGS:=  $(CPPFLAGS) -lssl -lcrypto -fPIC
CPPFLAGS := $(CPPFLAGS) -DMALLOC_PRODUCTION -fPIC 
CPPFLAGS := $(CPPFLAGS)  -Wno-pointer-arith -Wno-unused-but-set-variable
CPPFLAGS := $(CPPFLAGS)  -Wno-unused-function
CPPFLAGS := $(CPPFLAGS)  -Wno-unused-variable #-fpermissive
CPPFLAGS := $(CPPFLAGS) -cpp 
CPPFLAGS := $(CPPFLAGS) -D_USENVRAM
#CPPFLAGS := $(CPPFLAGS) -D_NVDEBUG
#CPPFLAGS := $(CPPFLAGS) -D_NOCHECKPOINT
#CPPFLAGS := $(CPPFLAGS) -D_USE_CHECKPOINT
#CPPFLAGS := $(CPPFLAGS) -D_ENABLE_RESTART
#CPPFLAGS := $(CPPFLAGS) -D_ENABLE_SWIZZLING
CPPFLAGS := $(CPPFLAGS) -D_NVRAM_OPTIMIZE

#cache related flags
#CPPFLAGS := $(CPPFLAGS) -D_USE_CACHEFLUSH
#CPPFLAGS:= $(CPPFLAGS) -D_LIBPMEMINTEL
#CPPFLAGS := $(CPPFLAGS) -D_USE_HOTPAGE

#allocator usage
#CPPFLAGS := $(CPPFLAGS) -D_USE_JEALOC_PERSISTONLY
#CPPFLAGS:= $(CPPFLAGS) -D_USE_CMU_NVMALLOC
#CPPFLAGS := $(CPPFLAGS) -D_USERANDOM_PROCID
#CPPFLAGS := $(CPPFLAGS) -D_USE_MALLOCLIB

#can be use together or induvidually
#When using transactions, the logging type
#needs to be specified
#CPPFLAGS := $(CPPFLAGS) -D_USE_SHADOWCOPY
#CPPFLAGS := $(CPPFLAGS) -D_USE_TRANSACTION
#CPPFLAGS := $(CPPFLAGS) -D_USE_UNDO_LOG
#CPPFLAGS := $(CPPFLAGS) -D_USE_REDO_LOG
#CPPFLAGS := $(CPPFLAGS) -D_DUMMY_TRANS
#STATS related flags
#CPPFLAGS := $(CPPFLAGS) -D_NVSTATS
#CPPFLAGS := $(CPPFLAGS) -D_FAULT_STATS 
#emulation related flags
CPPFLAGS := $(CPPFLAGS) -D_USE_FAKE_NVMAP
#CPPFLAGS := $(CPPFLAGS) -D_USE_BASIC_MMAP
#CPPFLAGS := $(CPPFLAGS) -D_USEPIN
#checkpoint related flags
#CPPFLAGS := $(CPPFLAGS) -D_ASYNC_RMT_CHKPT
#CPPFLAGS := $(CPPFLAGS) -D_ASYNC_LCL_CHK
#CPPFLAGS := $(CPPFLAGS) -D_RMT_PRECOPY
#CPPFLAGS := $(CPPFLAGS) -D_COMPARE_PAGES 
#CPPFLAGS:= $(CPPFLAGS) -D_ARMCI_CHECKPOINT
#CPPFLAGS:= $(CPPFLAGS) -D_VALIDATE_CHKSM


#Maintains a list of object names for a process
CPPFLAGS:= $(CPPFLAGS) -D_OBJNAMEMAP
CPPFLAGS:= $(CPPFLAGS) -D_USELOCKS
#CPPFLAGS:= $(CPPFLAGS) -D_ENABLE_INTEL_LOG
#Flags that needs to be cleaned later
#NVFLAGS:= -cpp -D_NOCHECKPOINT $(NVFLAGS)
#NVFLAGS:= -D_VALIDATE_CHKSM -cpp $(NVFLAGS)
#NVFLAGS:= -cpp $(NVFLAGS) 
#NVFLAGS:= -cpp -D_USESCR $(NVFLAGS) -lscrf
#NVFLAGS:= $(NVFLAGS) -D_GTC_STATS
#NVFLAGS:= $(NVFLAGS) -D_COMPARE_PAGES -lsnappy
#NVFLAGS:= -cpp $(NVFLAGS) -D_SYNTHETIC
#NVFLAGS:= -D_USENVRAM -cpp $(NVFLAGS)
#NVFLAGS:= $(NVFLAGS) -D_NVSTATS
#NVFLAGS:= $(NVFLAGS) -D_USE_FAKENO_NVMAP
#NVFLAGS:= $(NVFLAGS) -D_NOCHECKPOINT 
#NVFLAGS:= $(NVFLAGS) -D_ASYNC_RMT_CHKPT
#NVFLAGS:= $(NVFLAGS) -D_RMT_PRECOPY
#NVFLAGS:= $(NVFLAGS) -D_ASYNC_LCL_CHK
CXX=g++
CC=gcc

GNUFLAG :=  -std=gnu99 -fPIC -fopenmp 
CFLAGS := $(DEBUGFLG) -I$(INCLUDE) -Wall -pipe -fvisibility=hidden \
	  -funroll-loops  -Wno-implicit -Wno-uninitialized \
	  -Wno-unused-function -fPIC -fopenmp -lpmemlog #-larmci 

STDFLAGS :=-std=gnu++0x 
CPPFLAGS := $(CPPFLAGS) -I$(LOGGING)/include -I$(LOGGING)/include/include -I$(LOGGING)/include/port \
	    -I$(CMU_MALLOC)/include  -I$(BENCH) -I$(BENCH)/compare_bench/c-hashtable

LIBS= -lpthread -L$(LOGGING)/lib64  -lm -lssl \
       -Wl,-z,defs -lpthread -lm -lcrypto -lpthread -L/home/sudarsun/codes/nvmalloc/pmem_nvml/src/debug -lpmemlog \
       -L$(CMU_MALLOC)/lib  #-lrdpmc 
#	   -lpmem \
#		-lnvmalloc #-llogging

all:  SHARED_LIB NVMTEST
#BENCHMARK

JEMALLOC_OBJS= 	$(src_path)/jemalloc.o $(src_path)/arena.o $(src_path)/atomic.o \
		$(src_path)/base.o $(src_path)/ckh.o $(src_path)/ctl.o $(src_path)/extent.o \
        $(src_path)/hash.o $(src_path)/huge.o $(src_path)/mb.o \
	    $(src_path)/mutex.o $(src_path)/prof.o $(src_path)/quarantine.o \
	    $(src_path)/rtree.o $(src_path)/stats.o $(src_path)/tcache.o \
	    $(src_path)/util.o $(src_path)/tsd.o $(src_path)/chunk.o \
		$(src_path)/bitmap.o $(src_path)/chunk_mmap.o $(src_path)/chunk_dss.o \
		$(src_path)/np_malloc.o #$(src_path)/malloc_hook.o

RBTREE_OBJS= 	$(src_path)/rbtree.o

NVM_OBJS = $(src_path)/util_func.o $(src_path)/cache_flush.o \
		 $(src_path)/hash_maps.o  $(src_path)/LogMngr.o\
	  	 $(src_path)/checkpoint.o $(src_path)/nv_map.o \
		 $(src_path)/nv_transact.o $(src_path)/nv_stats.o\
		 $(src_path)/gtthread_spinlocks.o  \
		 $(src_path)/c_io.o  $(src_path)/nv_debug.o \
		 $(src_path)/pin_mapper.o
		 #$(src_path)/nv_rmtckpt.cc 
		 #$(src_path)/armci_checkpoint.o  \

BENCHMARK_OBJS = $(BENCH)/c-hashtable/hashtable.o $(BENCH)/c-hashtable/tester.o \
		 $(BENCH)/c-hashtable/hashtable_itr.o $(BENCH)/malloc_bench/nvmalloc_bench.o \
		 $(BENCH)/benchmark.o

$(src_path)/c_io.o: $(src_path)/c_io.cc 
	$(CXX) -c $(src_path)/c_io.cc -o $(src_path)/c_io.o $(LIBS) $(CPPFLAGS)

$(src_path)/nv_map.o: $(src_path)/nv_map.cc 
	$(CXX) -c $(src_path)/nv_map.cc -o $(src_path)/nv_map.o $(LIBS) $(CPPFLAGS) $(STDFLAGS)

$(src_path)/hash_maps.o: $(src_path)/hash_maps.cc 
	$(CXX) -c $(src_path)/hash_maps.cc -o $(src_path)/hash_maps.o $(LIBS) $(CPPFLAGS) $(STDFLAGS)

$(src_path)/rbtree.o: $(src_path)/rbtree.cc
	$(CXX) -c $(src_path)/rbtree.cc -o $(src_path)/rbtree.o $(LIBS) $(CFLAGS)

$(src_path)/LogMngr.o: $(src_path)/LogMngr.cc
	$(CXX) -c $(src_path)/LogMngr.cc -o $(src_path)/LogMngr.o $(LIBS) $(CPPFLAGS) $(STDFLAGS)

$(src_path)/nv_transact.o: $(src_path)/nv_transact.cc
	$(CXX) -c $(src_path)/nv_transact.cc -o $(src_path)/nv_transact.o $(LIBS) $(CPPFLAGS) $(STDFLAGS)

$(src_path)/nv_stats.o: $(src_path)/nv_stats.cc
	$(CXX) -c $(src_path)/nv_stats.cc -o $(src_path)/nv_stats.o $(LIBS) $(CPPFLAGS) $(STDFLAGS)	

$(src_path)/cache_flush.o: $(src_path)/cache_flush.cc
	$(CXX) -c $(src_path)/cache_flush.cc -o $(src_path)/cache_flush.o $(LIBS) $(CPPFLAGS) $(STDFLAGS)	

$(src_path)/nv_debug.o: $(src_path)/nv_debug.cc
	$(CXX) -c $(src_path)/nv_debug.cc -o $(src_path)/nv_debug.o $(LIBS) $(CPPFLAGS) $(STDFLAGS)	

$(src_path)/pin_mapper.o: $(src_path)/pin_mapper.cc
	$(CXX) -c $(src_path)/pin_mapper.cc -o $(src_path)/pin_mapper.o $(LIBS) $(CPPFLAGS) $(STDFLAGS)	

				
OBJLIST= $(RBTREE_OBJS) $(NVM_OBJS)  $(JEMALLOC_OBJS)
SHARED_LIB: $(RBTREE_OBJS) $(JEMALLOC_OBJS) $(NVM_OBJS)
	#$(CC) -c $(RBTREE_OBJS) -I$(INCLUDE) $(CFLAGS) 
	#$(CC) -c $(JEMALLOC_OBJS) -I$(INCLUDE) $(CFLAGS) $(NVFLAGS)
	#$(CXX) -c $(NVM_OBJS) -I$(INCLUDE) $(CPPFLAGS) $(NVFLAGS)  $(LDFLAGS)
	#ar crf  libnvmchkpt.a $(OBJLIST) $(NVFLAGS)  
	#ar rv  libnvmchkpt.a $(OBJLIST) $(NVFLAGS)
	$(CXX) -shared -fPIC -o libnvmchkpt.so $(OBJLIST) $(NVFLAGS) $(LIBS) $(LDFLAGS)
	$(CXX) -g varname_commit_test.cc -o varname_commit_test $(OBJLIST) -I$(INCLUDE) $(CPPFLAGS) $(NVFLAGS)  $(LIBS)
	#$(CXX) -g varname_commit_test.cc -o varname_commit_test util_func.o -I$(INCLUDE) $(CPPFLAGS) $(LIBS)

BENCHMARK: $(JEMALLOC_OBJS) $(NVM_OBJS) $(BENCHMARK_OBJS)
	$(CXX) -shared -fPIC -o libnvmchkpt.so $(OBJLIST) -I$(INCLUDE) $(CPPFLAGS) $(NVFLAGS)  $(LIBS)  $(LDFLAGS)
	$(CXX)  $(BENCHMARK_OBJS) -o benchmark $(OBJLIST) -I$(INCLUDE) $(CPPFLAGS) $(NVFLAGS)  $(LIBS)

NVMTEST:
	cd test && make clean && make && cd ..

clean:
	rm -f *.o *.so.0 *.so *.so* nv_read_test
	rm -f nvmalloc_bench
	rm -f test_dirtypgcnt test_dirtypgcpy
	rm -f ../*.o
	rm -f $(BENCHMARK_OBJS) "benchmark" 
	rm -rf libnvmchkpt.*
	cd test && make clean && cd ..

install:
	mkdir -p /usr/lib64/nvmalloc
	mkdir -p /usr/lib64/nvmalloc/lib
	mkdir -p /usr/lib64/nvmalloc/include
	cp libnvmchkpt.so libnvmchkpt.so.1
	sudo cp libnvmchkpt.so /usr/lib64/nvmalloc/lib/
	sudo cp libnvmchkpt.so.* /usr/lib64/nvmalloc/lib/
	sudo cp libnvmchkpt.so.* /usr/lib/
	sudo cp -r *.h /usr/lib64/nvmalloc/include/
	sudo cp -r *.h /usr/include/
	sudo cp -r *.h /usr/local/include/
	sudo cp libnvmchkpt.so.* /usr/local/lib/
	sudo cp libnvmchkpt.so /usr/local/lib/
	sudo cp libnvmchkpt.so.* /usr/lib/
	sudo cp libnvmchkpt.so /usr/lib/
	#sudo cp pmem_intel/linux-examples_flex/libpmem/libpmem.so  /usr/lib/
	#sudo cp pmem_intel/linux-examples_flex/libpmem/libpmem.so  /usr/lib64/
uninstall:
	rm -rf libnvmchkpt.*
	rm -rf /usr/lib64/nvmalloc/lib/libnvmchkpt.so*
	sudo rm -rf /usr/local/lib/libnvmchkpt.so*
	sudo rm -rf /usr/lib/libnvmchkpt.so*
#
# Copyright (c) 2014-2015, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#     * Neither the name of Intel Corporation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# Makefile -- top-level Makefile for NVM Library
#
# Use "make" to build the library.
#
# Use "make test" to build unit tests.
#
# Use "make check" to run unit tests.
#
# Use "make clean" to delete all intermediate files (*.o, etc).
#
# Use "make clobber" to delete everything re-buildable (binaries, etc.).
#
# Use "make cstyle" to run cstyle on all C source files
#
# Use "make rpm" to build rpm packages
#
# Use "make dpkg" to build dpkg packages
#
# Use "make source DESTDIR=path_to_dir" to copy source files
# from HEAD to 'path_to_dir/nvml' directory.
#
# As root, use "make install" to install the library in the usual
# locations (/usr/lib, /usr/include, and /usr/share/man).
# You can provide custom directory prefix for installation using
# DESTDIR variable e.g.: "make install DESTDIR=/opt"

export SRCVERSION = $(shell git describe 2>/dev/null || cat .version)

RPM_BUILDDIR=rpmbuild
DPKG_BUILDDIR=dpkgbuild
rpm : override DESTDIR=$(CURDIR)/$(RPM_BUILDDIR)
dpkg: override DESTDIR=$(CURDIR)/$(DPKG_BUILDDIR)

all:
	$(MAKE) -C src $@
	$(MAKE) -C doc $@

clean:
	$(MAKE) -C src $@
	$(MAKE) -C doc $@
	$(RM) -r $(RPM_BUILDDIR) $(DPKG_BUILDDIR)

clobber:
	$(MAKE) -C src $@
	$(MAKE) -C doc $@
	$(RM) -r $(RPM_BUILDDIR) $(DPKG_BUILDDIR) rpm dpkg

test check: all
	$(MAKE) -C src $@

cstyle:
	$(MAKE) -C src $@
	@echo Checking files for whitespace issues...
	@utils/check_whitespace
	@echo Done.

source:
	$(if $(shell git rev-parse 2>&1), $(error Not a git repository))
	$(if $(shell git status --porcelain), $(error Working directory is dirty))
	$(if $(DESTDIR), , $(error Please provide DESTDIR variable))
	mkdir -p $(DESTDIR)/nvml
	echo -n $(SRCVERSION) > $(DESTDIR)/nvml/.version
	git archive HEAD | tar -x -C $(DESTDIR)/nvml

pkg-clean:
	$(RM) -r $(DESTDIR)

rpm dpkg: pkg-clean source
	utils/build-$@.sh $(SRCVERSION) $(DESTDIR)/nvml $(DESTDIR) $(CURDIR)/$@\
			$(CURDIR)/src/test/testconfig.sh

install uninstall:
	$(MAKE) -C src $@
	$(MAKE) -C doc $@

.PHONY: all clean clobber test check cstyle install uninstall\
	source rpm dpkg pkg-clean $(SUBDIRS)
