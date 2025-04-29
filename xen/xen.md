## Xen 启动步骤

```shell
sudo xen-create-image --hostname=debian-vm1 \
  --memory=1024mb --vcpus=1 \
  --size=8Gb \
  --dhcp \
  --dist=bullseye \
  --mirror=http://deb.debian.org/debian/ \
  --dir=/xen-guests \
  --genpass=1
```

```shell
sudo xl create /etc/xen/debian-vm1.cfg
```