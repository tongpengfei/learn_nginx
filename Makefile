
PROJECT_ROOT:=$(PWD)
NGINX_PATH:=$(PROJECT_ROOT)/nginx-1.10.2
WORKSPACE:=$(PROJECT_ROOT)/workspace
NGINX_MODULES_PATH:=nginx_modules
NGINX_TAR:=nginx-1.10.2.tar.gz


nginx:
	cd $(NGINX_PATH); make

all: wget conf nginx install

init: env all

wget:
	$(shell if ! test -d $(NGINX_PATH); then wget http://nginx.org/download/$(NGINX_TAR) && tar -zxf $(NGINX_TAR) && rm -rf $(NGINX_TAR); fi;)

env:
	sudo apt install -y openssl libssl-dev
	sudo apt install -y libpcre3 libpcre3-dev
	sudo apt install -y zlib1g-dev
	sudo apt install -y cgdb

conf:
	cd $(NGINX_PATH); ./configure \
		--prefix=$(WORKSPACE) \
		--with-http_ssl_module \
		--with-stream \
		--with-stream_ssl_module \
		--with-debug \
		--add-module=../$(NGINX_MODULES_PATH)

install: nginx
	$(shell if test -d $(WORKSPACE); then cd $(WORKSPACE)/sbin; && sudo ./nginx -s stop)
	cd $(NGINX_PATH); make install
	cp -rf $(PROJECT_ROOT)/myconf/nginx.conf $(WORKSPACE)/conf/nginx.conf

clean:
	rm -rf $(NGINX_PATH)/objs
	rm -rf $(WORKSPACE)

run:
	cd $(WORKSPACE)/sbin; sudo ./nginx

stop:
	cd $(WORKSPACE)/sbin; sudo ./nginx -s stop

reopen:
	cd $(WORKSPACE)/sbin; sudo ./nginx -s reopen
	

ps:
	@ps -ef | grep nginx

gdb:
	cd $(WORKSPACE)/sbin; sudo cgdb ./nginx

hello:
	curl http://127.0.0.1/http_test
	