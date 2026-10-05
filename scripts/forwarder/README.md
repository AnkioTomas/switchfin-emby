# 构建 NSP forwarder （桌面图标）

bash
```shell
make -C scripts/forwarder Switchfin.nacp

hacbrewpack -k prod.keys --titleid 010FF000FFFFE000 --titlename Switchfin-emby --noromfs --nologo
```

# Thanks to

https://github.com/The-4n/hacBrewPack
https://github.com/switchbrew/nx-hbloader
