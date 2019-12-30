
PROJECT_ROOT:=$(PWD)
NGINX_PATH:=$(PROJECT_ROOT)/nginx-1.10.2
WORKSPACE:=$(PROJECT_ROOT)/workspace
NGINX_MODULES_PATH:=nginx_modules
NGINX_TAR:=nginx-1.10.2.tar.gz
APT:=none
PLAT=$(shell cat /etc/issue | head -n 1 | awk -F" " '{print $$1}' | tr A-Z a-z)

ifeq ($(PLAT),centos)
  APT:=yum install -y
  DEPENDS:=zlib zlib-devel openssl openssl-devel pcre pcre-devel gcc cgdb
endif

ifeq ($(PLAT),ubuntu)
  APT:=apt install -y
  DEPENDS:=openssl libssl-dev libpcre3 libpcre3-dev zlib1g-dev gcc cgdb
endif

ifeq ($(APT),none)
.PHONY: fail
endif


nginx:
	cd $(NGINX_PATH); make

all: wget conf nginx install

fail:
	@echo "unknown platform $(PLAT)"

init: env all

wget:
	$(shell if ! test -d $(NGINX_PATH); then wget http://nginx.org/download/$(NGINX_TAR) && tar -zxf $(NGINX_TAR) && rm -rf $(NGINX_TAR); fi;)

env:
	sudo $(APT) $(DEPENDS)

conf:
	sed "/Werror/d" -i $(NGINX_PATH)/auto/cc/gcc
	cd $(NGINX_PATH); ./configure \
		--prefix=$(WORKSPACE) \
		--with-http_ssl_module \
		--with-stream \
		--with-stream_ssl_module \
		--with-debug \
		--add-module=../$(NGINX_MODULES_PATH)/hello \
		--add-module=../$(NGINX_MODULES_PATH)/echo \


install: nginx stop
	cd $(NGINX_PATH); make install
	cp -rf $(PROJECT_ROOT)/myconf/nginx.conf $(WORKSPACE)/conf/nginx.conf

clean: stop
	rm -rf $(NGINX_PATH)/objs
	rm -rf $(WORKSPACE)

run: stop
	@sed 's/daemon off/daemon on/g' -i $(WORKSPACE)/conf/nginx.conf
	@sed 's/master_process off/master_process on/g' -i $(WORKSPACE)/conf/nginx.conf
	cd $(WORKSPACE)/sbin; sudo ./nginx

stop:
	$(shell if test -d $(WORKSPACE)/sbin; then cd $(WORKSPACE)/sbin && sudo ./nginx -s stop; fi)

reopen:
	cd $(WORKSPACE)/sbin; sudo ./nginx -s reopen
	

ps:
	@ps -ef | grep nginx

gdb: stop
	@sed 's/daemon on/daemon off/g' -i $(WORKSPACE)/conf/nginx.conf
	@sed 's/master_process on/master_process off/g' -i $(WORKSPACE)/conf/nginx.conf
	cd $(WORKSPACE)/sbin; sudo cgdb ./nginx

p: stop
	@sed 's/daemon on/daemon off/g' -i $(WORKSPACE)/conf/nginx.conf
	@sed 's/master_process on/master_process off/g' -i $(WORKSPACE)/conf/nginx.conf
	cd $(WORKSPACE)/sbin; sudo ./nginx



hello:
	curl http://127.0.0.1/helloworld

echo:
	curl http://127.0.0.1/echo
	@echo "# test ver:"
	@curl http://127.0.0.1/echo/ver
	@echo "# test ngx_types:"
	curl http://127.0.0.1/echo/type
	