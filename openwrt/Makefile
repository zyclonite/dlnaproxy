#
# Copyright (C) 2013 zyclonite networx
#

include $(TOPDIR)/rules.mk

PKG_BRANCH:=trunk
PKG_SOURCE_URL:=https://srv.wsn.at/svn/dlnaproxy/trunk
PKG_REV:=2

PKG_NAME:=dlnaproxy
PKG_VERSION:=$(PKG_REV)
PKG_RELEASE:=1

PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCE_SUBDIR).tar.gz
PKG_SOURCE_PROTO:=svn
PKG_SOURCE_VERSION:=$(PKG_REV)

include $(INCLUDE_DIR)/package.mk

define Package/dlnaproxy
  SECTION:=net
  CATEGORY:=Network
  TITLE:=DLNA Proxy daemon
  URL:=http://dlna.wsn.at/
  SUBMENU:=Firewall
  #DEPENDS:=+libopenssl +libpthread +kmod-tun
  MAINTAINER:=Lukas Prettenthaler <rdx@wsn.at>
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		INSTALL_PROG=":"
endef

define Package/dlnaproxy/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dlnaproxy $(1)/usr/bin/dlnaproxy
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) ./files/dlnaproxy.conf $(1)/etc/config/dlnaproxy
endef

define Package/dlnaproxy/conffiles
/etc/config/dlnaproxy
endef

$(eval $(call BuildPackage,dlnaproxy))
