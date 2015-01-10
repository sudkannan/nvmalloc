import os

os.system('rm -rf out')
n = 64 
count = 1
while count <= 60: 
	print n 
	os.system('echo "#define ELEMSZ %s" > memlat.h' %n)
	os.system('gcc memlat.c -L. -o memlat -lrdpmc -lnuma')
	os.system('./memlat 3242354 1000000 0 0 >>out') 
	count += 1
	n = 64*count 

print 'done'

