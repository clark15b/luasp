deb.conf.name='debian'
deb.conf.codename='etch'
deb.conf.architecture='i386'
deb.conf.liblua='liblua5.1-0'
deb.conf.libcurl='libcurl3'
deb.conf.libmysql='libmysqlclient15off'
deb.conf.libuuid='uuid-dev'
deb.conf.apache='apache2'
--deb.conf.apachempm='apache2-mpm-prefork'
deb.conf.apachempm=deb.conf.apache
deb.conf.apache_modules='/usr/lib/apache2/modules'
deb.conf.apache_mods_available='/etc/apache2/mods-available'
deb.conf.apache_mods_enabled='/etc/apache2/mods-enabled'
deb.conf.apachectl='apache2ctl'