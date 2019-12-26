
PROJECT_ROOT:=$(PWD)
NGINX_PATH:=$(PROJECT_ROOT)/nginx-1.10.2
WORKSPACE:=$(PROJECT_ROOT)/workspace
NGINX_MODULES_PATH:=nginx_modules
NGINX_TAR:=nginx-1.10.2.tar.gz


nginx:
	cd $(NGINX_PATH); make

all: wget conf nginx install

wget:
	$(shell if ! test -d $(NGINX_PATH); then wget http://nginx.org/download/$(NGINX_TAR) && tar -zxf $(NGINX_TAR) && rm -rf $(NGINX_TAR); fi;)

conf:
	cd $(NGINX_PATH); ./configure \
		--prefix=$(WORKSPACE) \
		--with-http_ssl_module \
		--with-stream \
		--with-stream_ssl_module \
		--with-debug \
		--add-module=../$(NGINX_MODULES_PATH)

install:
	cd $(NGINX_PATH); make install
	cp -rf $(PROJECT_ROOT)/myconf/nginx.conf $(WORKSPACE)/conf/nginx.conf

clean:
	rm -rf $(NGINX_PATH)/objs
	rm -rf $(WORKSPACE)