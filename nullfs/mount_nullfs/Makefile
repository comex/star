include ../../env.mk
mount_nullfs: mount_nullfs.c
	$(GCC) -o mount_nullfs mount_nullfs.c
	ldid -S mount_nullfs
clean:	
	rm -f mount_nullfs

