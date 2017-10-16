all:
	@echo "Please take a look at README.md"

install-osx: plugin
	mkdir -p ~/Library/Application\ Support/LLDB/PlugIns/
	cp -rf ./out/Release/llnode.dylib \
		~/Library/Application\ Support/LLDB/PlugIns/

uninstall-osx:
	rm ~/Library/Application\ Support/LLDB/PlugIns/llnode.dylib

install-linux: plugin
	mkdir -p /usr/lib/lldb/plugins
	cp -rf ./out/Release/lib.target/llnode.so /usr/lib/lldb/plugins

uninstall-linux:
	rm /usr/lib/lldb/plugins/llnode.so

format:
	clang-format -i src/*

_travis:
	./gyp_llnode -Dlldb_dir=/usr/lib/llvm-3.6/ -f make
	make -C out/
	TEST_LLDB_BINARY=`which lldb-3.6` npm test

clean:
	rm -rf out
	rm -rf build
	rm -rf options.gypi
	rm -rf lldb
	rm -rf addon.node llnode.so

addon:
	node scripts/configure.js
	node-gyp rebuild

plugin:
	node ./scripts/configure.js
	./gyp_llnode
	$(MAKE) -C out/

.PHONY: all addon clean plugin format
