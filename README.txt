This document is based on Ubuntu/Mint Linux.

1. Install LAMP stack
$ sudo apt-get install tasksel
$ sudo tasksel install lamp-server

2. Install Piwik
- May need to add "extension=mysqli.so" in /etc/php5/apache2/php.ini.
- May need to grant privilege to piwik user in mysql.
mysql> CREATE USER 'piwik'@'localhost' IDENTIFIED BY 'piwik';
mysql> GRANT ALL PRIVILEGES ON *.* TO 'piwik'@'localhost';

3. Install GSL
./configure
make
sudo make install

4. Install necessary libraries
sudo apt-get install libglib2.0-dev
sudo apt-get install libmysql++-dev

