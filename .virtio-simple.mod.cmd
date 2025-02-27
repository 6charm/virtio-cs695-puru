savedcmd_/home/vm/virtio-simple/virtio-simple.mod := printf '%s\n'   virtio-simple.o | awk '!x[$$0]++ { print("/home/vm/virtio-simple/"$$0) }' > /home/vm/virtio-simple/virtio-simple.mod
