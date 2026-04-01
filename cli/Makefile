build_udp_log_server:
	cd ../log-server && npm run build

pack: build_udp_log_server
	npm run build
	rm -rf __pack
	mkdir __pack
	cp -r bin __pack/
	cp README.md __pack/
	cp package.json __pack/
	cp package-lock.json __pack/

	rm -rf ../templates/vst-simple/build
	rm -rf ../templates/cpp-multi-target/build
	rm -rf ../templates/_common/frontend/node_modules
	cp -r ../templates __pack/
	cp ../log-server/dist/index.js __pack/log-server.js

publish_dry_run: pack
	cd __pack && npm publish --dry-run

publish: pack
	cd __pack && npm publish

clean:
	rm -rf __pack
