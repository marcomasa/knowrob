## Installing SWI Prolog

KnowRob requires SWI Prolog's latest stable version 8.2.4 or higher. 
The latest version shipped with Ubuntu 20.04 or older is 7.6.4,
so you might need to manually update the library.

```bash
# Install from apt
sudo apt install swi-prolog

# Check the version
swipl --version

# If it's under 8.2.4, add the ppa of the latest stable version and update
sudo apt-add-repository -y ppa:swi-prolog/stable
sudo apt update

# upgrade
sudo apt install swi-prolog
```

## Installing MongoDB

KnowRob requires version 4.4 or higher.
If your version is lower you will need to update.

Newer versions are **not** compatible with old DBs and won't allow the mongodb service to start.
To avoid issues later, a complete re-installation of the newer version is performed, 
which requires wiping, dumping or restoring all existing DBs.
As this will delete all previous deps, settings and DBs, 
you should store them **before** starting the update.
Instructions partly taken from [here](https://www.digitalocean.com/community/tutorials/how-to-install-mongodb-on-ubuntu-20-04).

```bash
# Install mongodb
sudo apt install libmongoc-dev libmongoc-1.0-0

# Check the mongodb version
mongod --version

# If below 4.4, an update is needed.
# ! Store your DBs before proceeding !


# Stop the service
sudo systemctl stop mongod.service
# Remove all DBs
sudo rm -r /var/log/mongodb
sudo rm -r /var/lib/mongodb
# Uninstall all mongo packages
# Be aware that this also removes unrelated packages starting with 'mongo*'
sudo apt purge mongo*

## Install version 4.4
# Add the source, should return 'OK'
curl -fsSL https://www.mongodb.org/static/pgp/server-4.4.asc | sudo apt-key add -

echo "deb [ arch=amd64,arm64 ] https://repo.mongodb.org/apt/ubuntu focal/mongodb-org/4.4 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-4.4.list

# Update references and install mongodb
sudo apt update
sudo apt install mongodb-org

# Troubleshoot: If dpkg errors occurr, the deps still refer to old versions. Force the new version
# Replace the <version> with your own new version. To this day it is '4.4.25'. 
sudo dpkg -i --force-overwrite /var/cache/apt/archives/mongodb-org-tools_4.4.<version>_amd64.deb

# Try to run the mongodb service
sudo systemctl start mongod.service
# Check if the service is running properly
sudo systemctl status mongod.service
# Refer to the mongodb error code explaination for further insight: 
# https://github.com/mongodb/mongo/blob/master/src/mongo/util/exit_code.h
# Status 62 identifies old DBs in /var/log and /var/lib, so delete them.
# To instead keep them, you'll need to downgrade mongo, dump DBs, upgrade mongo, recreate DBs.
# When it fails to open /var/log/mongodb/mongod.log the permissons for that file are incorrect.
# Either set owner and group of these two paths to mongodb, or reinstall mongodb.
```



## Installing Raptor2, spdlog and fmt

(Tested using a WSL2 and Windows 11)

```
sudo apt update
sudo apt install libspdlog-dev raptor2-utils libraptor2-dev libfmt-dev
sudo apt install librdf0-dev
```

## Installing ROS Noetic

Follow [these instructions](http://wiki.ros.org/noetic/Installation/Ubuntu) to install ROS Noetic on your machine.
Only required if you want to use catkin and ROS tools.

